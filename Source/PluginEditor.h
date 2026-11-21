#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class FLBarLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    FLBarLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff24282f));
        setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff20242a));
        setColour (juce::ComboBox::textColourId,       juce::Colour (0xffd5dbe2));
        setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffd5dbe2));

        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff24282f));
        setColour (juce::PopupMenu::textColourId,                  juce::Colour (0xffd5dbe2));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3d4552));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f, static_cast<float> (height) - 1.0f);

        g.setColour (juce::Colour (0xff24282f));
        g.fillRoundedRectangle (bounds, 2.5f);

        g.setColour (juce::Colour (0xff20242a));
        g.drawRoundedRectangle (bounds, 2.5f, 1.0f);

        juce::Path arrow;
        const float ax = static_cast<float> (width) - 13.0f;
        const float ay = static_cast<float> (height) * 0.5f - 2.0f;
        arrow.addTriangle (ax, ay, ax + 6.0f, ay, ax + 3.0f, ay + 4.5f);
        g.setColour (box.isEnabled() ? juce::Colour (0xffd5dbe2) : juce::Colour (0xff9aa3ad));
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (12.0f));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (7, 1, box.getWidth() - 24, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool isHighlighted, bool isDown) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto bg = isDown        ? juce::Colour (0xff1d2026)
                      : isHighlighted ? juce::Colour (0xff2d323b)
                                      : juce::Colour (0xff24282f);
        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 2.5f);
        g.setColour (juce::Colour (0xff20242a));
        g.drawRoundedRectangle (bounds, 2.5f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool isHighlighted, bool) override
    {
        g.setColour (isHighlighted ? juce::Colours::white : juce::Colour (0xffd5dbe2));
        g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }
};

class FLLedBypassButton final : public juce::Button
{
public:
    explicit FLLedBypassButton (const juce::String& labelText)
        : juce::Button (labelText)
    {
        setClickingTogglesState (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool isOver, bool) override
    {
        const float h = static_cast<float> (getHeight());
        const float ledSize = 9.0f;
        const float ledX = 2.0f;
        const float ledY = (h - ledSize) * 0.5f;

        g.setColour (juce::Colour (0xff181b20));
        g.fillEllipse (ledX - 1.0f, ledY - 1.0f, ledSize + 2.0f, ledSize + 2.0f);

        // When NOT bypassed (getToggleState() == false), the green "Active" LED glows like FL Studio
        const bool active = ! getToggleState();
        if (active)
        {
            g.setColour (juce::Colour (0xff52ff7a));
            g.fillEllipse (ledX, ledY, ledSize, ledSize);
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillEllipse (ledX + 2.0f, ledY + 1.8f, 3.0f, 3.0f);
        }
        else
        {
            g.setColour (isOver ? juce::Colour (0xff454d58) : juce::Colour (0xff323841));
            g.fillEllipse (ledX, ledY, ledSize, ledSize);
        }

        g.setColour (isOver ? juce::Colours::white : juce::Colour (0xffd5dbe2));
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (getButtonText(),
                    static_cast<int> (ledX + ledSize + 6.0f), 0,
                    getWidth() - static_cast<int> (ledX + ledSize + 6.0f), getHeight(),
                    juce::Justification::centredLeft, false);
    }
};

class FLPresetComboBox final : public juce::ComboBox
{
public:
    std::function<void (int)> onWheelStep;

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (onWheelStep)
            onWheelStep (wheel.deltaY > 0.0f ? -1 : 1);
    }
};

class FilmstripKnob final : public juce::Slider
{
public:
    std::function<void (const juce::String&)> onHintChanged;

    FilmstripKnob (const std::array<juce::Image, 101>& framesIn, const juce::String& paramTitle)
        : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
          frames (framesIn),
          title (paramTitle)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setVelocityModeParameters (1.0, 1, 0.08, true, juce::ModifierKeys::ctrlModifier);
    }

    void paint (juce::Graphics& g) override
    {
        const double norm = valueToProportionOfLength (getValue());
        const int frameIdx = juce::jlimit (0, 100, static_cast<int> (std::lround (norm * 100.0)));
        if (frames[static_cast<size_t> (frameIdx)].isValid())
            g.drawImageAt (frames[static_cast<size_t> (frameIdx)], 0, 0);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseEnter (e);
        emitHint();
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseExit (e);
        if (onHintChanged)
            onHintChanged ({});
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isAltDown())
        {
            setValue (50.0, juce::sendNotificationSync);
            emitHint();
            return;
        }
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            m.addItem (1, "Reset (50%)");
            m.addSeparator();
            m.addItem (10, "Set to 0%");
            m.addItem (11, "Set to 25%");
            m.addItem (12, "Set to 50%");
            m.addItem (13, "Set to 75%");
            m.addItem (14, "Set to 100%");
            juce::Component::SafePointer<FilmstripKnob> safeThis (this);
            m.showMenuAsync (juce::PopupMenu::Options(), [safeThis] (int res)
            {
                if (safeThis == nullptr)
                    return;
                if (res == 1)
                    safeThis->setValue (50.0, juce::sendNotificationSync);
                else if (res >= 10 && res <= 14)
                    safeThis->setValue ((res - 10) * 25.0, juce::sendNotificationSync);
                safeThis->emitHint();
            });
            return;
        }
        juce::Slider::mouseDown (e);
        emitHint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
            return;
        juce::Slider::mouseDrag (e);
        emitHint();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const double step = 2.0 * (wheel.deltaY > 0.0f ? 1.0 : -1.0);
        setValue (juce::jlimit (getMinimum(), getMaximum(), getValue() + step), juce::sendNotificationSync);
        emitHint();
    }

private:
    void emitHint()
    {
        if (onHintChanged)
            onHintChanged (title + ": " + juce::String (getValue(), 1) + "%");
    }

    const std::array<juce::Image, 101>& frames;
    juce::String title;
};

class SansAmpPSA1AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SansAmpPSA1AudioProcessorEditor (SansAmpPSA1AudioProcessor&);
    ~SansAmpPSA1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void loadAssets();
    void populatePresetMenu();
    void stepPreset (int delta);

    SansAmpPSA1AudioProcessor& processorRef;
    FLBarLookAndFeel flLookAndFeel;

    juce::Image backgroundImage;
    std::array<juce::Image, 101> knobFrames;

    std::unique_ptr<FilmstripKnob> knobs[8];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachments[8];

    FLPresetComboBox presetBox;
    juce::TextButton prevPresetBtn { "<" };
    juce::TextButton nextPresetBtn { ">" };
    FLLedBypassButton bypassButton { "Active" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::String currentHintText;

    static constexpr int kHeaderHeight = 36;
    static constexpr int kRackWidth    = 567;
    static constexpr int kRackHeight   = 120;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SansAmpPSA1AudioProcessorEditor)
};
