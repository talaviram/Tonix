#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

constexpr auto kParamVersion = 1;

TonixProcessor::TonixProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                          ),
      apvts (*this, &undoManager, "parameters", { std::make_unique<AudioParameterFloat> (ParameterID { "inputTrim", kParamVersion }, "Input Trim", NormalisableRange<float> (-10.0f, 10.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")), std::make_unique<AudioParameterFloat> (ParameterID { "process", kParamVersion }, "Process", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("%")), std::make_unique<AudioParameterFloat> (ParameterID { "outputTrim", kParamVersion }, "Output Trim Trim", NormalisableRange<float> (-6.0f, 6.0f, 0.01f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")), std::make_unique<AudioParameterChoice> (ParameterID { "brightness", kParamVersion }, "Brightness", StringArray { "Opal", "Gold", "Sapphire" }, 1), std::make_unique<AudioParameterChoice> (ParameterID { "type", kParamVersion }, "Type", StringArray { "Luminiscent", "Iridescent", "Radiant", "Luster", "Dark Essence" }, 1), std::make_unique<AudioParameterBool> (ParameterID { "bypass", kParamVersion }, "Bypass", false), std::make_unique<AudioParameterBool> (ParameterID { "autoGain", kParamVersion }, "Auto-Gain", true) })
{
    reset();
    m_params.inputTrim = apvts.getRawParameterValue ("inputTrim");
    m_params.process = apvts.getRawParameterValue ("process");
    m_params.outputTrim = apvts.getRawParameterValue ("outputTrim");
    m_params.brightness = apvts.getParameterAsValue ("brightness");
    m_params.autoGain = apvts.getRawParameterValue ("autoGain");
    m_params.type = apvts.getParameterAsValue ("type");
    m_params.bypass = apvts.getRawParameterValue ("bypass");
}

TonixProcessor::~TonixProcessor()
{
}

const juce::String TonixProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TonixProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TonixProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool TonixProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double TonixProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TonixProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int TonixProcessor::getCurrentProgram()
{
    return 0;
}

void TonixProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String TonixProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void TonixProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void TonixProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    jassert (getTotalNumInputChannels() == getTotalNumOutputChannels());
    const auto maxChannels = static_cast<size_t> (std::max (getTotalNumInputChannels(), getTotalNumOutputChannels()));
    m_processors.clear();
    m_processors = std::vector<Channel> (maxChannels);

    for (auto& p : m_processors)
    {
        p.reset();
        // original has fixed scaling depending on sample rate: {1.0, 0.5, 0.25}
        p.srScale = 1.0 / floor (sampleRate / 44100.0);
    }
}

void TonixProcessor::reset()
{
    for (auto& ch : m_processors)
        ch.reset();
}

void TonixProcessor::Channel::reset()
{
    processing = 0.0;
    curProcessing = 0.0;
    y = 0.0;
    s = 0.0;
    prev_x = 0.0;
    autoGainComp = 1.0f;

    saturator.reset();
}

double TonixProcessor::Channel::process (double x)
{
    curProcessing = processing * a3;
    x1 = hpf_k * x + (x - prev_x);
    x2 = x1 * f1 + x1;
    x3 = (! g0) ? x : x2;
    x4 = (type == Type::Luster) ? saturator.process (x2 * curProcessing) : saturator.process (x2);
    x5 = saturator.process (x4 * curProcessing * p20 + x3);

    prev_x = x;

    s += (x5 - s) * lpf_k;

    y = curProcessing * (s - x * p24);

    if (type == Type::Luster)
        y *= 0.5;

    y += x;
    if (useAutoGain)
        y *= autoGain;

    return y;
}

void TonixProcessor::Saturator::reset()
{
    x2 = x4 = x6 = x8 = 0.0;
}

double TonixProcessor::Saturator::process (double x)
{
    double y = 0.0;
    // polynomial approximation instead of table lookup
    switch (type)
    {
        case 0:
        {
            // hard clip
            x = std::max (-1.0, std::min (x, 1.0));
            x2 = x * x;
            x4 = x2 * x2;
            x6 = x4 * x2;
            x8 = x4 * x4;

            y = x * 2.827568855 + x2 * 0.0003903798913 + x2 * x * -4.17220229 + x4 * -0.0001107320401 + x4 * x * 0.523459874 + x6 * 0.0002768079893 + x6 * x * -0.423546883 + x8 * -0.001448632 + x8 * x * 3.224580615 + x8 * x2 * 0.002728704 + x8 * x2 * x * -5.495344862 + x8 * x4 * -0.002846356 + x8 * x4 * x * 5.449768693 + x8 * x6 * 0.001310366 + x8 * x6 * x * -2.414078731;
        }
        break;
        case 1:
        {
            // hard clip
            x = std::max (-0.991184403, std::min (x, 0.990821248));
            x2 = x * x;
            x4 = x2 * x2;
            x6 = x4 * x2;
            x8 = x4 * x4;

            y = x * 1.501040337 + x2 * -0.0002757478168 + x2 * x * -0.301802438 + x4 * 0.003273802 + x4 * x * 1.786333688 + x6 * -0.046104732 + x6 * x * -24.582679252 + x8 * 0.110553367 + x8 * x * 41.112226106 + x8 * x2 * -0.092987632 + x8 * x2 * x * -16.724196818 + x8 * x4 * 0.01857341 + x8 * x4 * x * -9.331919223 + x8 * x6 * 0.006696015 + x8 * x6 * x * 6.543207186;
        }
        break;
        case 2:
        {
            // hard clip
            x = std::max (-0.991022224, std::min (x, 0.990984424));
            x2 = x * x;
            x4 = x2 * x2;
            x6 = x4 * x2;
            x8 = x4 * x4;

            y = x * 2.063930806 + x2 * 0.0002008141989 + x2 * x * -0.414990906 + x4 * -0.003741183 + x4 * x * 2.456380956 + x6 * 0.03108163 + x6 * x * -33.802027499 + x8 * -0.092816819 + x8 * x * 56.531406839 + x8 * x2 * 0.134928028 + x8 * x2 * x * -22.998647073 + x8 * x4 * -0.098216457 + x8 * x4 * x * -12.829323005 + x8 * x6 * 0.028676158 + x8 * x6 * x * 8.996306767;
        }
        break;
        default:
            jassertfalse;
    }
    return y;
}

void TonixProcessor::releaseResources()
{
}

bool TonixProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() != 1 || layouts.outputBuses.size() != 1)
        return false;
    if (layouts.getNumChannels (true, 0) != layouts.getNumChannels (false, 0))
        return false;
    // only allow symmetrical I/Os
    return true;
}

void TonixProcessor::Channel::setProcessing (const double amount)
{
    processing = amount;
    // simple auto-gain compensation
    autoGain = 1.0 + processing * autoGain_a1 + processing * processing * autoGain_a2;
}

void TonixProcessor::Channel::setMode (Type t, Brightness b)
{
    brightness = b;
    type = t;
    switch (type)
    {
        case Type::Luminiscent:
        {
            switch (brightness)
            {
                case Brightness::Opal:
                    hpf_k = 0.625;
                    lpf_k = 0.1875;
                    break;
                case Brightness::Gold:
                    hpf_k = 0.4375;
                    lpf_k = 0.3125;
                    break;
                case Brightness::Sapphire:
                    hpf_k = 0.1875;
                    lpf_k = 0.375;
                    break;
            }
            a3 = 0.25;
            f1 = 0.75;
            p20 = 0.3125;
            p24 = 0.0625;
            g0 = 1;
            saturator.type = 0;
            autoGain_a1 = -0.416;
            autoGain_a2 = 0.092;
        }
        break;
        case Type::Iridescent:
        {
            switch (brightness)
            {
                case Brightness::Opal:
                    hpf_k = 0.625;
                    lpf_k = 0.1875;
                    break;
                case Brightness::Gold:
                    hpf_k = 0.375;
                    lpf_k = 0.3125;
                    break;
                case Brightness::Sapphire:
                    hpf_k = 0.3125;
                    lpf_k = 0.5;
                    break;
            }
            a3 = 0.25;
            f1 = 0.875;
            p20 = 0.3125;
            p24 = 0.0625;
            g0 = 1;
            saturator.type = 0;
            autoGain_a1 = -0.393;
            autoGain_a2 = 0.082;
        }
        break;
        case Type::Radiant:
        {
            switch (brightness)
            {
                case Brightness::Opal:
                    hpf_k = 0.75;
                    lpf_k = 0.125;
                    break;
                case Brightness::Gold:
                    hpf_k = 0.45629901;
                    lpf_k = 0.375;
                    break;
                case Brightness::Sapphire:
                    hpf_k = 0.375;
                    lpf_k = 0.5;
                    break;
            }
            a3 = 0.375;
            f1 = 0.75;
            p20 = 0.1875;
            p24 = 0.0125;
            g0 = 0;
            saturator.type = 1;
            autoGain_a1 = -0.441;
            autoGain_a2 = 0.103;
        }
        case Type::Luster:
        {
            switch (brightness)
            {
                case Brightness::Opal:
                    hpf_k = 0.75;
                    lpf_k = 0.125;
                    break;
                case Brightness::Gold:
                    hpf_k = 0.45629901;
                    lpf_k = 0.375;
                    break;
                case Brightness::Sapphire:
                    hpf_k = 0.375;
                    lpf_k = 0.5625;
                    break;
            }
            a3 = 1.0;
            f1 = 0.6875;
            p20 = 0.27343899;
            p24 = 0.1171875;
            g0 = 0;
            saturator.type = 2;
            autoGain_a1 = -0.712;
            autoGain_a2 = 0.172;
        }
        case Type::DarkEssence:
        {
            switch (brightness)
            {
                case Brightness::Opal:
                    hpf_k = 0.75;
                    lpf_k = 0.125;
                    break;
                case Brightness::Gold:
                    hpf_k = 0.45629901;
                    lpf_k = 0.375;
                    break;
                case Brightness::Sapphire:
                    hpf_k = 0.375;
                    lpf_k = 0.5625;
                    break;
            }
            a3 = 0.375;
            f1 = 0.75;
            p20 = 0.5625;
            p24 = 0.0125;
            g0 = 0;
            saturator.type = 2;
            autoGain_a1 = -0.636;
            autoGain_a2 = 0.17;
        }
    }
    // sample-rate scale
    hpf_k *= srScale;
    lpf_k *= srScale;
}

void TonixProcessor::processBlock (AudioBuffer<float>& buffer,
                                   MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // bypass
    if (m_params.bypass->load() > 0.5)
        return;

    inputGain = Decibels::decibelsToGain (m_params.inputTrim->load());
    outputGain = Decibels::decibelsToGain (m_params.outputTrim->load());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto& processor = m_processors[(size_t) channel];
        processor.setMode ((Channel::Type) static_cast<int> (m_params.type.getValue()), (Channel::Brightness) static_cast<int> (m_params.brightness.getValue()));
        processor.setProcessing (m_params.process->load() / 100.0);
        processor.useAutoGain = (m_params.process->load() > 0.5f);

        auto chData = buffer.getWritePointer (channel);
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
        {
            chData[i] = static_cast<float> (m_processors[(size_t) channel].process (chData[i] * inputGain) * outputGain);
        }
    }
}

juce::AudioProcessorParameter* TonixProcessor::getBypassParameter() const
{
    return apvts.getParameter ("bypass");
}

bool TonixProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TonixProcessor::createEditor()
{
    return new TonixEditor (*this);
}

void TonixProcessor::getStateInformation (MemoryBlock& destData)
{
    // store
    MemoryOutputStream out (destData, false);
    apvts.state.writeToStream (out);
}

void TonixProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    jassert (sizeInBytes >= 0);
    // restore
    apvts.state = ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TonixProcessor();
}
