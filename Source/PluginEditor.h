#pragma once

#include "PluginProcessor.h"

class TonixKnobStyle : public juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class TonixTextButtonStyle : public juce::LookAndFeel_V4
{
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
};

class GenericListener : public juce::AudioProcessorValueTreeState::Listener,
                        public juce::ChangeListener
{
public:
    GenericListener (std::function<void()> func) : onChange (func) {}
    ~GenericListener() override = default;
    std::function<void()> onChange;
    void parameterChanged (const juce::String&, float) override
    {
        onChange();
    }
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        onChange();
    }
};

class TonixEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TonixEditor (TonixProcessor&);
    ~TonixEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::TextButton m_bypassButton, m_autoGainButton, m_undoButton, m_redoButton;
    juce::Label m_pluginName, m_pluginDesc;

    struct SliderLabels
    {
        juce::Label inputTrim, processPercentage, outputTrim, brightness, type;
    } m_labels;
    struct Sliders
    {
        juce::Slider inputTrim, processPercentage, outputTrim;
        juce::Slider brightness, type;
    } m_sliders;
    struct Attachments
    {
        Attachments (juce::AudioProcessorValueTreeState&, Sliders&, juce::Button& bypass, juce::Button& autoGain);
        juce::AudioProcessorValueTreeState::ButtonAttachment bypass, autoGain;
        juce::AudioProcessorValueTreeState::SliderAttachment inputTrim, processPercentage, outputTrim;
        juce::AudioProcessorValueTreeState::SliderAttachment brightness, type;
    } m_attachments;
    TonixTextButtonStyle m_textButtonStyle;
    TonixKnobStyle m_knobStyle;
    TonixProcessor& processorRef;
    std::unique_ptr<GenericListener> m_bypassNotifier, m_undoNotifier;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TonixEditor)
};
