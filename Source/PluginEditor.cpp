#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
// Exact coordinates from Contents/Resources/SansAmpGUI.xml:
// Pre-Amp(27,30), Buzz(93,30), Punch(160,30), Crunch(227,30), Drive(294,29), Low(360,30), High(426,30), Level(493,30)
constexpr int kKnobX[8] = { 27, 93, 160, 227, 294, 360, 426, 493 };
constexpr int kKnobY[8] = { 30, 30,  30,  30,  29,  30,  30,  30 };
constexpr int kKnobW    = 54;
constexpr int kKnobH    = 64;
}

SansAmpPSA1AudioProcessorEditor::SansAmpPSA1AudioProcessorEditor (SansAmpPSA1AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&flLookAndFeel);
    loadAssets();

    for (int i = 0; i < 8; ++i)
    {
        knobs[i] = std::make_unique<FilmstripKnob> (knobFrames, SansAmpPSA1AudioProcessor::kParamNames[i]);
        knobs[i]->setDoubleClickReturnValue (true, 50.0);
        knobs[i]->onHintChanged = [this] (const juce::String& hint)
        {
            currentHintText = hint;
            repaint (0, 0, getWidth(), kHeaderHeight);
        };
        addAndMakeVisible (*knobs[i]);

        knobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processorRef.getAPVTS(), SansAmpPSA1AudioProcessor::kParamIDs[i], *knobs[i]);
    }

    populatePresetMenu();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1 && id <= kNumFactoryPresets)
            processorRef.setCurrentProgram (id - 1);
    };
    presetBox.onWheelStep = [this] (int delta) { stepPreset (delta); };
    addAndMakeVisible (presetBox);

    prevPresetBtn.onClick = [this] { stepPreset (-1); };
    addAndMakeVisible (prevPresetBtn);

    nextPresetBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (nextPresetBtn);

    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.getAPVTS(), "bypass", bypassButton);

    setSize (kRackWidth, kHeaderHeight + kRackHeight);
}

SansAmpPSA1AudioProcessorEditor::~SansAmpPSA1AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SansAmpPSA1AudioProcessorEditor::stepPreset (int delta)
{
    const int cur = processorRef.getCurrentProgram();
    const int next = (cur + delta + kNumFactoryPresets) % kNumFactoryPresets;
    processorRef.setCurrentProgram (next);
    presetBox.setSelectedId (next + 1, juce::dontSendNotification);
}

void SansAmpPSA1AudioProcessorEditor::loadAssets()
{
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::back_png, BinaryData::back_pngSize);

    for (int i = 0; i <= 100; ++i)
    {
        const juce::String resName = juce::String::formatted ("SansAmp%04d_png", i);
        int dataSize = 0;
        if (const char* data = BinaryData::getNamedResource (resName.toRawUTF8(), dataSize))
            knobFrames[static_cast<size_t> (i)] = juce::ImageCache::getFromMemory (data, dataSize);
    }
}

void SansAmpPSA1AudioProcessorEditor::populatePresetMenu()
{
    auto* rootMenu = presetBox.getRootMenu();
    rootMenu->clear();

    std::map<juce::String, juce::PopupMenu> subMenus;

    for (int i = 0; i < kNumFactoryPresets; ++i)
    {
        const auto& preset = kFactoryPresets[i];
        const juce::String cat (preset.category);
        if (cat == "Factory" || cat == "General")
            rootMenu->addItem (i + 1, preset.name);
        else
            subMenus[cat].addItem (i + 1, preset.name);
    }

    for (auto& kv : subMenus)
        rootMenu->addSubMenu (kv.first, kv.second);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
}

void SansAmpPSA1AudioProcessorEditor::paint (juce::Graphics& g)
{
    // FL Studio 1:1 dark slate header bar matching reference screenshot
    g.fillAll (juce::Colour (0xff21252b));

    const juce::Rectangle<float> headerPanel (4.0f, 3.0f, static_cast<float> (getWidth()) - 8.0f, static_cast<float> (kHeaderHeight) - 6.0f);
    g.setColour (juce::Colour (0xff2b3038));
    g.fillRoundedRectangle (headerPanel, 2.0f);
    g.setColour (juce::Colour (0xff181b20));
    g.drawRoundedRectangle (headerPanel, 2.0f, 1.0f);

    // Left title / live parameter hint in crisp FL Studio font
    g.setColour (juce::Colour (0xffd5dbe2));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    const juce::String leftLabel = currentHintText.isNotEmpty() ? currentHintText : juce::String ("SansAmp PSA-1");
    g.drawText (leftLabel, 14, 0, 145, kHeaderHeight, juce::Justification::centredLeft, false);

    // Label next to preset selector ("Preset") in FL Studio style
    g.setColour (juce::Colour (0xffd5dbe2));
    g.drawText ("Preset", 412, 0, 52, kHeaderHeight, juce::Justification::centredLeft, false);

    if (backgroundImage.isValid())
        g.drawImageAt (backgroundImage, 0, kHeaderHeight);
}

void SansAmpPSA1AudioProcessorEditor::resized()
{
    const int cy = (kHeaderHeight - 22) / 2;
    prevPresetBtn.setBounds (160, cy, 20, 22);
    presetBox.setBounds     (183, cy, 198, 22);
    nextPresetBtn.setBounds (384, cy, 20, 22);
    bypassButton.setBounds  (486, cy, 68, 22);

    for (int i = 0; i < 8; ++i)
        knobs[i]->setBounds (kKnobX[i], kHeaderHeight + kKnobY[i], kKnobW, kKnobH);
}
