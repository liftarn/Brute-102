#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace brute102
{
    namespace Colors
    {
        inline const juce::Colour background    { 0xff090a0c };
        inline const juce::Colour chassis       { 0xff14161a };
        inline const juce::Colour recess        { 0xff0c0d10 };
        inline const juce::Colour border        { 0xff272a30 };
        inline const juce::Colour borderStrong  { 0xff3a404c };
        inline const juce::Colour textPrimary   { 0xffe4e6ea };
        inline const juce::Colour textMuted     { 0xff71717a };
        inline const juce::Colour amber         { 0xfff59e0b };
        inline const juce::Colour knobCap       { 0xff1e2128 };
        inline const juce::Colour knobInner     { 0xff14171c };
    }

    class BruteLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        BruteLookAndFeel()
        {
            setColour(juce::ComboBox::backgroundColourId, Colors::recess);
            setColour(juce::ComboBox::outlineColourId, Colors::border);
            setColour(juce::ComboBox::textColourId, Colors::textPrimary);
            setColour(juce::ComboBox::arrowColourId, Colors::textMuted);

            setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff121418));
            setColour(juce::PopupMenu::textColourId, Colors::textPrimary);
            setColour(juce::PopupMenu::highlightedBackgroundColourId, Colors::amber.withAlpha(0.18f));
            setColour(juce::PopupMenu::highlightedTextColourId, Colors::amber);

            setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xfff4f4f5));
            setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff18181b));
            setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xffa1a1aa));
            setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, Colors::amber);
            setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x28ffffff));
            setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour(0xff181a1f));
            setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, juce::Colour(0xffa1a1aa));
        }

        void drawRotarySlider(juce::Graphics& g,
                              int x, int y, int width, int height,
                              float sliderPosProportional,
                              float rotaryStartAngle,
                              float rotaryEndAngle,
                              juce::Slider& slider) override
        {
            const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                       static_cast<float>(width), static_cast<float>(height));
            const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
            const auto centre = bounds.getCentre();
            const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // Background track arc
            juce::Path bgArc;
            bgArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff272b33));
            g.strokePath(bgArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Active amber sweep arc
            if (sliderPosProportional > 0.004f)
            {
                juce::Path activeArc;
                activeArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, toAngle, true);
                g.setColour(Colors::amber);
                g.strokePath(activeArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Machined knob cap
            const float capRadius = radius * 0.74f;
            g.setColour(Colors::knobCap);
            g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

            g.setColour(slider.isMouseOverOrDragging() ? Colors::amber : Colors::borderStrong);
            g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.25f);

            const float innerRadius = capRadius - 2.5f;
            g.setColour(Colors::knobInner);
            g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

            // Pointer line
            juce::Path pointer;
            const float pInner = capRadius * 0.32f;
            const float pOuter = capRadius - 2.0f;
            pointer.startNewSubPath(0.0f, -pInner);
            pointer.lineTo(0.0f, -pOuter);
            pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
            g.setColour(juce::Colour(0xfff3f4f6));
            g.strokePath(pointer, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        void drawButtonBackground(juce::Graphics& g,
                                  juce::Button& button,
                                  const juce::Colour&,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override
        {
            const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
            const bool toggled = button.getToggleState();

            juce::Colour fill = Colors::recess;
            if (toggled)
                fill = Colors::amber;
            else if (shouldDrawButtonAsDown)
                fill = juce::Colour(0xff272b33);
            else if (shouldDrawButtonAsHighlighted)
                fill = juce::Colour(0xff1d2026);

            g.setColour(fill);
            g.fillRoundedRectangle(bounds, 2.0f);

            g.setColour(toggled ? Colors::amber.brighter(0.2f) : Colors::border);
            g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        }

        void drawButtonText(juce::Graphics& g,
                            juce::TextButton& button,
                            bool, bool) override
        {
            const bool toggled = button.getToggleState();
            g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
            g.setColour(toggled ? juce::Colour(0xff090a0c) : Colors::textMuted);
            g.drawFittedText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
        }
    };
}
