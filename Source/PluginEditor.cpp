#include "PluginEditor.h"
#include "BinaryData.h"
#include "PluginProcessor.h"
#include "GitHash.hpp"

using namespace juce;

void TonixKnobStyle::drawRotarySlider (Graphics& g,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       float sliderPosProportional,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       Slider& slider)
{
    juce::ignoreUnused (sliderPosProportional, rotaryStartAngle, rotaryEndAngle);
    // Knob origin https://www.g200kg.com/en/webknobman/gallery.php?m=p&p=1540
    Image myStrip = ImageCache::getFromMemory (BinaryData::KNB_metal_pink_L_png, BinaryData::KNB_metal_pink_L_pngSize);
    const double fractRotation = (slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()); // normalized
    const int nFrames = myStrip.getHeight() / myStrip.getWidth();
    const int frameIdx = (int) ceil (fractRotation * ((double) nFrames - 1.0));

    const float radius = jmin (width / 2.0f, height / 2.0f);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius - 1.0f;
    const float ry = centreY - radius - 1.0f;
    g.drawImage (myStrip,
                 (int) rx,
                 (int) ry,
                 2 * (int) radius,
                 2 * (int) radius, //Dest
                 0,
                 frameIdx * myStrip.getWidth(),
                 myStrip.getWidth(),
                 myStrip.getWidth()); //Source
}

TonixEditor::TonixEditor (TonixProcessor& p)
    : AudioProcessorEditor (&p), m_attachments (p.apvts, m_sliders, m_bypassButton, m_autoGainButton), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    m_pluginName.setFont (FontOptions().withHeight (20.0f).withStyle ("bold"));
    m_pluginName.setText ("TONIX", juce::dontSendNotification);
    m_pluginDesc.setFont (FontOptions().withHeight (15.0f));
    m_pluginDesc.setText ("Tape Emulation", juce::dontSendNotification);
    m_buildDetails.setJustificationType (Justification::centredTop);
    m_buildDetails.setFont (Font (FontOptions().withHeight (10.0f)));
    m_buildDetails.setText (JucePlugin_VersionString + juce::String (" ") + juce::String(GitHash::shortSha1), juce::dontSendNotification);
    addAndMakeVisible (m_pluginName);
    addAndMakeVisible (m_pluginDesc);
    addAndMakeVisible (m_buildDetails);

    const juce::Colour pinky (243, 0, 243);

    m_autoGainButton.setLookAndFeel (&m_textButtonStyle);
    m_autoGainButton.setToggleState (processorRef.apvts.getParameterAsValue ("autoGain").getValue(), dontSendNotification);
    m_autoGainButton.setClickingTogglesState (true);
    m_autoGainButton.setColour (TextButton::ColourIds::textColourOnId, pinky);

    m_bypassNotifier = std::make_unique<GenericListener> ([this]
                                                          {
        const bool bypass = processorRef.apvts.getRawParameterValue("bypass")->load() > 0.5f;
        for (auto* s : {&m_sliders.type, &m_sliders.brightness, &m_sliders.inputTrim, &m_sliders.outputTrim, &m_sliders.processPercentage})
        {
            s->setAlpha (bypass ? 0.5f : 1.0f);
        } });
    processorRef.apvts.addParameterListener ("bypass", m_bypassNotifier.get());

    m_bypassButton.setClickingTogglesState (true);
    m_bypassButton.setToggleState (processorRef.apvts.getParameterAsValue ("bypass").getValue(), sendNotificationAsync);
    m_bypassButton.setLookAndFeel (&m_textButtonStyle);
    m_bypassButton.setColour (TextButton::ColourIds::textColourOnId, pinky);
    m_undoButton.setLookAndFeel (&m_textButtonStyle);
    m_redoButton.setLookAndFeel (&m_textButtonStyle);

    m_sliders.inputTrim.setLookAndFeel (&m_knobStyle);
    m_sliders.processPercentage.setLookAndFeel (&m_knobStyle);
    m_sliders.outputTrim.setLookAndFeel (&m_knobStyle);
    m_sliders.brightness.setLookAndFeel (&m_knobStyle);
    m_sliders.type.setLookAndFeel (&m_knobStyle);

    m_labels.inputTrim.setText ("INPUT TRIM", juce::dontSendNotification);
    m_labels.processPercentage.setText ("PROCESS", juce::dontSendNotification);
    m_labels.outputTrim.setText ("OUTPUT TRIM", juce::dontSendNotification);
    m_labels.brightness.setText ("BRIGHTNESS", juce::dontSendNotification);
    m_labels.type.setText ("TYPE", juce::dontSendNotification);
    for (auto* l : { &m_labels.inputTrim, &m_labels.processPercentage, &m_labels.outputTrim, &m_labels.brightness, &m_labels.type })
    {
        l->setJustificationType (Justification::centred);
        l->setColour (Label::ColourIds::textColourId, juce::Colours::black);
        addAndMakeVisible (l);
    }
    addAndMakeVisible (m_sliders.inputTrim);
    addAndMakeVisible (m_sliders.processPercentage);
    addAndMakeVisible (m_sliders.outputTrim);
    addAndMakeVisible (m_sliders.brightness);
    addAndMakeVisible (m_sliders.type);

    m_bypassButton.setButtonText ("BYPASS");
    addAndMakeVisible (m_bypassButton);

    m_autoGainButton.setButtonText ("AUTO GAIN");
    addAndMakeVisible (m_autoGainButton);

    m_undoButton.setButtonText ("UNDO");
    m_undoButton.onClick = [&undoManager = p.undoManager]
    {
        undoManager.undo();
    };
    m_redoButton.setButtonText ("REDO");
    m_redoButton.onClick = [&undoManager = p.undoManager]
    {
        undoManager.redo();
    };
    addAndMakeVisible (m_undoButton);
    addAndMakeVisible (m_redoButton);
    m_undoButton.setEnabled (processorRef.undoManager.canUndo());
    m_redoButton.setEnabled (processorRef.undoManager.canRedo());

    m_undoNotifier = std::make_unique<GenericListener> ([this]
                                                        {
        m_undoButton.setEnabled (processorRef.undoManager.canUndo());
        m_redoButton.setEnabled (processorRef.undoManager.canRedo()); });
    processorRef.undoManager.addChangeListener (m_undoNotifier.get());

    setSize (600, 200);
}

TonixEditor::~TonixEditor()
{
    for (auto* ch : getChildren())
        ch->setLookAndFeel (nullptr);
    processorRef.undoManager.removeChangeListener (m_undoNotifier.get());
    processorRef.apvts.removeParameterListener ("bypass", m_bypassNotifier.get());
}

void TonixEditor::paint (juce::Graphics& g)
{
    const auto gf = ColourGradient::vertical (Colours::darkgrey, 0, Colours::grey, static_cast<float> (getHeight()));
    g.setGradientFill (gf);
    g.fillAll();
}

void TonixEditor::resized()
{
    constexpr auto pad = 10;
    auto bounds = getLocalBounds().reduced (20, 10);
    {
        auto topArea = bounds.removeFromTop (40);
        m_pluginName.setBounds (topArea.removeFromLeft (60));
        m_buildDetails.setBounds (m_pluginName.getBounds().translated (0, +m_pluginName.getHeight()));
        m_pluginDesc.setBounds (topArea.removeFromLeft (100));
        m_redoButton.setBounds (topArea.removeFromRight (40).reduced (0, pad));
        m_undoButton.setBounds (topArea.removeFromRight (40).reduced (0, pad));
    }
    {
        auto labelsArea = bounds.removeFromTop (40);
        FlexBox fb;
        constexpr auto defaultSize = 100;
        fb.alignItems = FlexBox::AlignItems::center;
        fb.justifyContent = FlexBox::JustifyContent::flexEnd;
        fb.items = {
            FlexItem (defaultSize, defaultSize, m_labels.inputTrim),
            FlexItem (defaultSize, defaultSize, m_labels.processPercentage),
            FlexItem (defaultSize, defaultSize, m_labels.outputTrim),
            FlexItem (defaultSize, defaultSize, m_labels.brightness),
            FlexItem (defaultSize, defaultSize, m_labels.type)
        };
        fb.performLayout (labelsArea.toFloat());
    }
    {
        FlexBox fb;
        constexpr auto defaultSize = 100;
        fb.alignItems = FlexBox::AlignItems::center;
        fb.justifyContent = FlexBox::JustifyContent::flexEnd;
        fb.items = {
            FlexItem (defaultSize, defaultSize, m_sliders.inputTrim),
            FlexItem (defaultSize, defaultSize, m_sliders.processPercentage),
            FlexItem (defaultSize, defaultSize, m_sliders.outputTrim),
            FlexItem (defaultSize, defaultSize, m_sliders.brightness),
            FlexItem (defaultSize, defaultSize, m_sliders.type)
        };
        fb.performLayout (bounds.toFloat());
    }
    {
        auto leftSide = getLocalBounds().removeFromLeft (80);
        leftSide.removeFromTop (80);
        m_bypassButton.setBounds (leftSide.removeFromTop (40).reduced (0, pad));
        m_autoGainButton.setBounds (leftSide.removeFromTop (40).reduced (0, pad));
    }
}

TonixEditor::Attachments::Attachments (AudioProcessorValueTreeState& apvts, Sliders& sliders, juce::Button& bypassButton, juce::Button& autoGainButton) : bypass (apvts, "bypass", bypassButton),
                                                                                                                                                          autoGain (apvts, "autoGain", autoGainButton),
                                                                                                                                                          inputTrim (apvts, "inputTrim", sliders.inputTrim),
                                                                                                                                                          processPercentage (apvts, "process", sliders.processPercentage),
                                                                                                                                                          outputTrim (apvts, "outputTrim", sliders.outputTrim),
                                                                                                                                                          brightness (apvts, "brightness", sliders.brightness),
                                                                                                                                                          type (apvts, "type", sliders.type)
{
    for (auto* s : { &sliders.inputTrim, &sliders.processPercentage, &sliders.outputTrim, &sliders.brightness, &sliders.type })
    {
        s->setColour (Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::black);
        s->setColour (Slider::ColourIds::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.1f));
        s->setSliderStyle (Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, false, 80, 20);
    }
}
