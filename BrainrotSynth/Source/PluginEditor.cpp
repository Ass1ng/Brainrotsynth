#include "PluginEditor.h"
#include <cmath>

static const char* kPadNames[8] = { "Tung Tung Sahur", "Tralalero Tralala", "Bombardiro Crocodilo", "Ballerina Cappuccina",
                                    "Lirili Larila", "Chimpanzini Bananini", "Brr Brr Patapim", "La Vaca Saturno Saturnita" };

// ---------------------------------------------------------------- look and feel
void BrainrotLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startA + pos * (endA - startA);
    const float arcR = r - 3.f;
    const juce::PathStrokeType thin (5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    const juce::PathStrokeType fat (11.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path bg; bg.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, endA, true);
    g.setColour (juce::Colour (0x22ffffff)); g.strokePath (bg, thin);

    juce::Path val; val.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, ang, true);
    g.setColour (accent.withAlpha (0.25f)); g.strokePath (val, fat);
    g.setColour (accent); g.strokePath (val, thin);

    const float br = r * 0.62f;
    g.setColour (juce::Colour (0xff120a1f)); g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (accent.withAlpha (0.5f));   g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.5f);

    juce::Path p; p.addRoundedRectangle (-1.8f, -br * 0.92f, 3.6f, br * 0.5f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void BrainrotLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                                            juce::Slider::SliderStyle, juce::Slider&)
{
    auto track = juce::Rectangle<float> ((float) x + 8.f, (float) y + (float) h * 0.5f - 8.f, (float) w - 16.f, 16.f);
    juce::ColourGradient grad (juce::Colour (0xffff5ac8), track.getX(), 0.f, juce::Colour (0xff3a0000), track.getRight(), 0.f, false);
    grad.addColour (0.5, juce::Colour (0xff7a3cff));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (track, 8.f);
    g.setColour (juce::Colour (0x66ffffff));
    g.drawRoundedRectangle (track, 8.f, 1.f);

    const float cy = track.getCentreY();
    g.setColour (accent.withAlpha (0.3f));      g.fillEllipse (sliderPos - 20.f, cy - 20.f, 40.f, 40.f);
    g.setColour (juce::Colours::white);         g.fillEllipse (sliderPos - 13.f, cy - 13.f, 26.f, 26.f);
    g.setColour (accent);                       g.fillEllipse (sliderPos - 8.f, cy - 8.f, 16.f, 16.f);
}

void BrainrotLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent : juce::Colour (0x22ffffff));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? juce::Colours::black : juce::Colours::white.withAlpha (highlighted ? 1.f : 0.7f));
    g.setFont (juce::Font (13.f, juce::Font::bold));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

// ---------------------------------------------------------------- editor
BrainrotEditor::BrainrotEditor (BrainrotProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (760, 540);

    const char* ids[8] = { "pitch", "crush", "down", "drive", "cutoff", "reverb", "release", "gain" };
    const char* nm[8]  = { "PITCH", "CRUSH", "LO-FI", "DRIVE", "FILTRO", "REVERB", "RELEASE", "VOLUME" };
    for (int i = 0; i < 8; ++i)
    {
        auto& k = knob[i];
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
        k.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k);

        names[i].setText (nm[i], juce::dontSendNotification);
        names[i].setJustificationType (juce::Justification::centred);
        names[i].setFont (juce::Font (12.f, juce::Font::bold));
        names[i].setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
        addAndMakeVisible (names[i]);

        att[i] = std::make_unique<SliderAtt> (proc.apvts, ids[i], k);
    }

    morph.setSliderStyle (juce::Slider::LinearHorizontal);
    morph.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (morph);
    morphAtt = std::make_unique<SliderAtt> (proc.apvts, "morph", morph);

    reverse.setButtonText ("REVERSE");
    addAndMakeVisible (reverse);
    reverseAtt = std::make_unique<ButtonAtt> (proc.apvts, "reverse", reverse);

    startTimerHz (30);
}

BrainrotEditor::~BrainrotEditor() { stopTimer(); setLookAndFeel (nullptr); }

juce::Rectangle<int> BrainrotEditor::padRect (int i) const
{
    return { 320 + (i % 4) * 108, 84 + (i / 4) * 90, 96, 80 };
}

void BrainrotEditor::resized()
{
    morph.setBounds (24, 288, 712, 44);
    reverse.setBounds (612, 24, 120, 30);
    for (int i = 0; i < 8; ++i)
    {
        names[i].setBounds (20 + i * 90, 352, 90, 18);
        knob[i].setBounds (20 + i * 90, 370, 90, 136);
    }
}

void BrainrotEditor::timerCallback()
{
    selected = juce::jlimit (0, 7, (int) std::lround (proc.apvts.getRawParameterValue ("char")->load()));
    morphVal = proc.apvts.getRawParameterValue ("morph")->load();
    level = std::max (proc.outLevel.load(), level * 0.86f);
    phase += 0.14f + level * 0.3f;
    laf.accent = charColour (selected);
    repaint();
}

void BrainrotEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 8; ++i)
    {
        if (padRect (i).contains (e.getPosition()))
        {
            if (auto* p = proc.apvts.getParameter ("char"))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
                p->endChangeGesture();
            }
            break;
        }
    }
}

void BrainrotEditor::paint (juce::Graphics& g)
{
    const auto acc = charColour (selected);
    const auto bounds = getLocalBounds().toFloat();
    const float twoPi = juce::MathConstants<float>::twoPi;

    // fundo: roxo goofy -> preto avermelhado dark
    const auto top = juce::Colour (0xff1b0b33).interpolatedWith (juce::Colour (0xff240606), morphVal);
    const auto bot = juce::Colour (0xff07040f).interpolatedWith (juce::Colours::black, morphVal);
    g.setGradientFill (juce::ColourGradient (top, 0.f, 0.f, bot, 0.f, bounds.getBottom(), false));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (acc.withAlpha (0.22f), bounds.getCentreX(), 0.f,
                                             acc.withAlpha (0.f), bounds.getCentreX(), 280.f, true));
    g.fillRect (bounds);

    // titulo
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (32.f, juce::Font::bold));
    g.drawText ("BRAINROT SYNTH", 28, 12, 420, 40, juce::Justification::centredLeft);
    g.setColour (acc);
    g.setFont (juce::Font (13.f, juce::Font::bold));
    g.drawText ("TUNG TUNG TUNG EDITION  //  GOOFY <-> DARK", 30, 50, 480, 18, juce::Justification::centredLeft);

    // paineis
    g.setColour (juce::Colour (0x40000000));
    g.fillRoundedRectangle (20.f, 84.f, 280.f, 170.f, 14.f);
    g.fillRoundedRectangle (14.f, 262.f, 732.f, 76.f, 16.f);
    g.fillRoundedRectangle (14.f, 344.f, 732.f, 172.f, 16.f);

    // visualizador + carinha
    const juce::Point<float> c (160.f, 169.f);
    const float baseR = 30.f + level * 8.f;
    for (int k = 0; k < 40; ++k)
    {
        const float a = twoPi * (float) k / 40.f;
        const float wob = 0.5f + 0.5f * std::sin (phase * 1.7f + (float) k * 0.9f);
        const float hh = 3.f + (4.f + level * 26.f) * wob * (0.4f + 0.6f * std::abs (std::sin ((float) k * 1.3f + phase * 0.5f)));
        const float dx = std::sin (a), dy = -std::cos (a);
        g.setColour (acc.withAlpha (0.35f + 0.65f * wob));
        g.drawLine (c.x + dx * (baseR + 8.f), c.y + dy * (baseR + 8.f), c.x + dx * (baseR + 8.f + hh), c.y + dy * (baseR + 8.f + hh), 3.f);
    }
    g.setGradientFill (juce::ColourGradient (acc.brighter (0.3f), c.x - 10.f, c.y - 12.f,
                                             acc.darker (1.2f).interpolatedWith (juce::Colours::black, morphVal), c.x + baseR, c.y + baseR, true));
    g.fillEllipse (c.x - baseR, c.y - baseR, baseR * 2.f, baseR * 2.f);

    const float eyeR = 9.f - 3.f * morphVal;
    const auto white = juce::Colours::white.interpolatedWith (juce::Colours::black, morphVal);
    const auto pupil = juce::Colours::black.interpolatedWith (juce::Colour (0xffff2020), morphVal);
    const float look = std::sin (phase * 0.7f) * 3.f * (1.f - morphVal);
    for (int s = -1; s <= 1; s += 2)
    {
        const float cx = c.x + (float) s * 14.f, cy = c.y - 8.f;
        g.setColour (white); g.fillEllipse (cx - eyeR, cy - eyeR, eyeR * 2.f, eyeR * 2.f);
        g.setColour (pupil); g.fillEllipse (cx + look - 3.5f, cy - 3.5f, 7.f, 7.f);
        g.setColour (juce::Colours::black);
        g.drawLine (cx + (float) s * 9.f, cy - eyeR - 5.f, cx - (float) s * 9.f, cy - eyeR - 5.f + 6.f * morphVal, 2.5f);
    }
    const float mh = 3.f + level * 16.f, mw = 22.f - 6.f * morphVal;
    g.setColour (juce::Colour (0xff1a0010));
    g.fillEllipse (c.x - mw * 0.5f, c.y + 16.f - mh * 0.5f, mw, mh);

    // pads dos personagens
    for (int i = 0; i < 8; ++i)
    {
        const auto r = padRect (i).toFloat();
        const auto col = charColour (i);
        const bool sel = (i == selected);
        if (sel) { g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (3.f), 12.f); }
        g.setColour (sel ? col.withAlpha (0.92f) : juce::Colour (0xff150b26));
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (col.withAlpha (sel ? 1.f : 0.5f));
        g.drawRoundedRectangle (r, 10.f, 1.5f);
        g.setColour (sel ? juce::Colours::black.withAlpha (0.6f) : col);
        g.setFont (juce::Font (11.f, juce::Font::bold));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), r.reduced (8.f, 5.f), juce::Justification::topLeft);
        g.setColour (sel ? juce::Colours::black : juce::Colours::white);
        g.setFont (juce::Font (12.5f, juce::Font::bold));
        g.drawFittedText (kPadNames[i], r.reduced (6.f, 4.f).toNearestInt(), juce::Justification::centred, 3);
    }

    // rotulos do morph
    g.setFont (juce::Font (13.f, juce::Font::bold));
    g.setColour (juce::Colour (0xffff5ac8)); g.drawText ("GOOFY", 30, 266, 100, 18, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffff3030)); g.drawText ("DARK", 630, 266, 100, 18, juce::Justification::centredRight);
    g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawText ("MISTURA", 330, 266, 100, 18, juce::Justification::centred);
}
