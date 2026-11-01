#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

static float rd (const juce::AudioBuffer<float>& b, double p)
{
    const int n = b.getNumSamples();
    if (n < 2 || p < 0.0 || p >= (double) (n - 1)) return 0.0f;
    const int i = (int) p; const float f = (float) (p - i);
    const float* d = b.getReadPointer (0);
    return d[i] + f * (d[i + 1] - d[i]);
}

static juce::AudioBuffer<float> decode (juce::AudioFormatManager& fm, const char* data, int size)
{
    juce::AudioBuffer<float> b;
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false)));
    if (r != nullptr)
    {
        b.setSize (1, (int) r->lengthInSamples);
        r->read (&b, 0, (int) r->lengthInSamples, 0, true, false);
    }
    return b;
}

juce::AudioProcessorValueTreeState::ParameterLayout BrainrotProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "char", 1 }, "Personagem",
        StringArray { "Tung Tung Sahur", "Tralalero Tralala", "Bombardiro Crocodilo", "Ballerina Cappuccina",
                      "Lirili Larila", "Chimpanzini Bananini", "Brr Brr Patapim", "La Vaca Saturno Saturnita" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "morph", 1 }, "Goofy <-> Dark", NormalisableRange<float> (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pitch", 1 }, "Pitch (semitons)", NormalisableRange<float> (-24.f, 24.f, 0.01f), 0.f));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "reverse", 1 }, "Reverse", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "crush", 1 }, "Bitcrush (bits)", NormalisableRange<float> (3.f, 16.f, 0.1f), 16.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "down", 1 }, "Downsample", NormalisableRange<float> (1.f, 32.f, 0.1f), 1.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Distorcao", NormalisableRange<float> (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Filtro (Hz)", NormalisableRange<float> (200.f, 20000.f, 1.f, 0.3f), 20000.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reverb", 1 }, "Reverb", NormalisableRange<float> (0.f, 1.f, 0.001f), 0.2f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release (s)", NormalisableRange<float> (0.02f, 5.f, 0.001f, 0.5f), 3.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Volume (dB)", NormalisableRange<float> (-24.f, 6.f, 0.1f), 0.f));
    return { p.begin(), p.end() };
}

BrainrotProcessor::BrainrotProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    loadSamples();
}

void BrainrotProcessor::loadSamples()
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    struct P { const char* g; int gs; const char* d; int ds; };
    const P t[kChars] = {
        { BinaryData::tung_goofy_wav,       BinaryData::tung_goofy_wavSize,       BinaryData::tung_dark_wav,       BinaryData::tung_dark_wavSize },
        { BinaryData::tralalero_goofy_wav,  BinaryData::tralalero_goofy_wavSize,  BinaryData::tralalero_dark_wav,  BinaryData::tralalero_dark_wavSize },
        { BinaryData::bombardiro_goofy_wav, BinaryData::bombardiro_goofy_wavSize, BinaryData::bombardiro_dark_wav, BinaryData::bombardiro_dark_wavSize },
        { BinaryData::ballerina_goofy_wav,  BinaryData::ballerina_goofy_wavSize,  BinaryData::ballerina_dark_wav,  BinaryData::ballerina_dark_wavSize },
        { BinaryData::lirili_goofy_wav,     BinaryData::lirili_goofy_wavSize,     BinaryData::lirili_dark_wav,     BinaryData::lirili_dark_wavSize },
        { BinaryData::chimpanzini_goofy_wav,BinaryData::chimpanzini_goofy_wavSize,BinaryData::chimpanzini_dark_wav,BinaryData::chimpanzini_dark_wavSize },
        { BinaryData::brr_goofy_wav,        BinaryData::brr_goofy_wavSize,        BinaryData::brr_dark_wav,        BinaryData::brr_dark_wavSize },
        { BinaryData::vaca_goofy_wav,       BinaryData::vaca_goofy_wavSize,       BinaryData::vaca_dark_wav,       BinaryData::vaca_dark_wavSize } };
    for (int i = 0; i < kChars; ++i) { goofy[i] = decode (fm, t[i].g, t[i].gs); dark[i] = decode (fm, t[i].d, t[i].ds); }
}

void BrainrotProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate; reverb.setSampleRate (sampleRate); reverb.reset();
    for (auto& v : voices) v.active = false;
    hold = holdCount = z1 = z2 = 0;
}

void BrainrotProcessor::noteOn (int note, float vel, bool reverse, int ch)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { v = &voices[nextVoice]; nextVoice = (nextVoice + 1) % kVoices; }
    const double len = (double) std::max (goofy[ch].getNumSamples(), dark[ch].getNumSamples());
    v->active = true; v->released = false; v->note = note; v->vel = vel; v->env = 0.f;
    v->dir = reverse ? -1.0 : 1.0; v->pos = reverse ? len - 2.0 : 0.0;
}

void BrainrotProcessor::renderVoices (float* out, int a, int b, int ch, float morph, float semis, float rel)
{
    const auto& G = goofy[ch]; const auto& D = dark[ch];
    const double maxLen = (double) std::max (G.getNumSamples(), D.getNumSamples());
    const float atk = 1.f / (0.003f * (float) sr), rl = 1.f / (std::max (0.005f, rel) * (float) sr);
    for (auto& v : voices)
    {
        if (! v.active) continue;
        const double inc = std::pow (2.0, ((double) (v.note - 60) + (double) semis) / 12.0) * 44100.0 / sr * v.dir;
        for (int i = a; i < b; ++i)
        {
            const float s = rd (G, v.pos) * (1.f - morph) + rd (D, v.pos) * morph;
            if (v.released) { v.env -= rl; if (v.env <= 0.f) { v.active = false; break; } }
            else if (v.env < 1.f) v.env = std::min (1.f, v.env + atk);
            out[i] += s * v.env * v.vel * 0.8f;
            v.pos += inc;
            if (v.pos < 0.0 || v.pos >= maxLen) { v.active = false; break; }
        }
    }
}

void BrainrotProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear();
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const int ch = juce::jlimit (0, kChars - 1, (int) std::lround (get ("char")));
    const float morph = get ("morph"), semis = get ("pitch"), rel = get ("release");
    const bool rev = get ("reverse") > 0.5f;
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int sp = juce::jlimit (0, n, meta.samplePosition);
        if (sp > pos) { renderVoices (L, pos, sp, ch, morph, semis, rel); pos = sp; }
        if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), rev, ch);
        else if (m.isNoteOff()) { for (auto& v : voices) if (v.active && v.note == m.getNoteNumber()) v.released = true; }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& v : voices) v.active = false; }
    }
    if (pos < n) renderVoices (L, pos, n, ch, morph, semis, rel);

    const float bits = get ("crush"), ds = get ("down"), drive = get ("drive"), cutoff = get ("cutoff"), revAmt = get ("reverb");
    const float gain = juce::Decibels::decibelsToGain (get ("gain"));
    const float q = std::pow (2.f, bits - 1.f);
    const float a = 1.f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);
    const float g = 1.f + drive * 15.f;
    for (int i = 0; i < n; ++i)
    {
        float x = L[i];
        if (ds > 1.01f) { if (holdCount <= 0.f) { hold = x; holdCount += ds; } holdCount -= 1.f; x = hold; }
        if (bits < 15.9f) x = std::round (x * q) / q;
        if (drive > 0.01f) x = std::tanh (x * g) / (1.f + drive * 0.5f);
        if (cutoff < 19000.f) { z1 += a * (x - z1); z2 += a * (z1 - z2); x = z2; }
        L[i] = x * gain;
    }
    juce::FloatVectorOperations::copy (R, L, n);

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.8f; rp.damping = 0.4f; rp.width = 1.f; rp.freezeMode = 0.f;
    rp.wetLevel = revAmt * 0.45f; rp.dryLevel = 1.f - revAmt * 0.3f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, n);

    float pk = 0.f;
    for (int i = 0; i < n; ++i)
    {
        L[i] = juce::jlimit (-1.f, 1.f, L[i]); R[i] = juce::jlimit (-1.f, 1.f, R[i]);
        pk = std::max (pk, std::abs (L[i]));
    }
    outLevel.store (std::min (1.f, pk * 1.4f));
}

void BrainrotProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void BrainrotProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* BrainrotProcessor::createEditor() { return new BrainrotEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BrainrotProcessor(); }
