#pragma once
#include "PluginProcessor.h"

inline juce::Colour charColour (int i)
{
    static const juce::uint32 c[8] = { 0xfff5a623, 0xff1ec8ff, 0xffff4d4d, 0xffff7ac8,
                                       0xffe8c547, 0xff8be04e, 0xff38e0a0, 0xffb069ff };
    return juce::Colour (c[juce::jlimit (0, 7, i)]);
}

class BrainrotLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour accent { 0xfff5a623 };
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class BrainrotEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BrainrotEditor (BrainrotProcessor&);
    ~BrainrotEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> padRect (int i) const;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BrainrotLookAndFeel laf;
    BrainrotProcessor& proc;
    juce::Slider morph;
    juce::Slider knob[8];
    juce::Label names[8];
    juce::ToggleButton reverse;
    std::unique_ptr<SliderAtt> att[8];
    std::unique_ptr<SliderAtt> morphAtt;
    std::unique_ptr<ButtonAtt> reverseAtt;

    int selected = 0;
    float level = 0.f, phase = 0.f, morphVal = 0.f;
};
