#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

struct Voice { bool active = false, released = false; int note = 60; double pos = 0, dir = 1; float vel = 1, env = 0; };

class BrainrotProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kVoices = 8, kChars = 8;
    BrainrotProcessor();
    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Brainrot Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> outLevel { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void loadSamples();
    void noteOn (int note, float vel, bool reverse, int ch);
    void renderVoices (float* out, int a, int b, int ch, float morph, float semis, float rel);

    Voice voices[kVoices];
    int nextVoice = 0;
    juce::AudioBuffer<float> goofy[kChars], dark[kChars];
    double sr = 44100.0;
    juce::Reverb reverb;
    float hold = 0, holdCount = 0, z1 = 0, z2 = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrainrotProcessor)
};
