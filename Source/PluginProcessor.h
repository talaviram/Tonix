#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <span>

class TonixProcessor final : public juce::AudioProcessor
{
public:
    TonixProcessor();
    ~TonixProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void reset() override;

    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undoManager;

private:
    struct Saturator
    {
        double process (double sample);
        void reset();
        int type;
        double x2, x4, x6, x8;
    };

    struct Channel
    {
        enum class Brightness
        {
            Opal = 0,
            Gold,
            Sapphire
        };

        enum class Type
        {
            Luminiscent = 0,
            Iridescent,
            Radiant,
            Luster,
            DarkEssence
        };

        void setProcessing (double amount);

        void setMode (Type, Brightness);
        void reset();
        double process (double sample);

        double processing;
        double curProcessing;

        // memory
        double x1, x2, x3, x4, x5, y;

        // coeffs
        double hpf_k, lpf_k;
        double a3, f1, p20, p24;
        int g0;
        double autoGain_a1, autoGain_a2;
        double autoGain;
        bool useAutoGain;

        Saturator saturator;
        Type type;
        Brightness brightness;

        float autoGainComp;

        double s, prev_x;
        double srScale { 1.0 };
    };

    float inputGain, outputGain;
    std::vector<Channel> m_processors;

    struct Params
    {
        std::atomic<float>*inputTrim, *process, *outputTrim, *autoGain, *bypass;
        juce::Value brightness, type;
    } m_params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TonixProcessor)
};
