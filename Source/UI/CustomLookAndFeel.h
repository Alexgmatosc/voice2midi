#pragma once
#include <JuceHeader.h>

namespace v2m
{

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        // Set basic colors
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00E5FF)); // Cyan
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00E5FF));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1A1C20).brighter(0.1f));
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1A1C20).brighter(0.05f));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1A1C20));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff00E5FF).withAlpha(0.3f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1A1C20).brighter(0.2f));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        
        // Font
        setDefaultSansSerifTypefaceName("Helvetica Neue"); // Or standard sans
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::FontOptions(14.0f);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::FontOptions(14.0f);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Track Background
        g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
        g.fillEllipse(rx, ry, rw, rw);

        // Glowing Arc (Fill)
        juce::Path filledArc;
        filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.7f);
        
        // Add a slight glow effect
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId).withAlpha(0.2f));
        g.fillPath(filledArc); // Blur would be better, but alpha is okay
        
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.fillPath(filledArc);

        // Knob inner circle
        auto innerRadius = radius * 0.7f;
        juce::Path innerCircle;
        innerCircle.addEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        
        juce::ColourGradient grad(juce::Colour(0xff2A2D34), centreX, centreY - innerRadius,
                                  juce::Colour(0xff15161A), centreX, centreY + innerRadius, false);
        g.setGradientFill(grad);
        g.fillPath(innerCircle);

        // Pointer
        juce::Path pointer;
        auto pointerLength = innerRadius * 0.8f;
        auto pointerThickness = 3.0f;
        pointer.addRectangle(-pointerThickness * 0.5f, -innerRadius, pointerThickness, pointerLength);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillPath(pointer);
    }
    
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 8.0f;
        
        juce::Colour baseColor = backgroundColour;
        if (shouldDrawButtonAsDown)        baseColor = baseColor.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted) baseColor = baseColor.brighter(0.1f);
        
        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, cornerSize);
        
        // Draw the cyan border if it's the "Calibrate" button (we can identify it by text or just do it for all)
        if (button.getButtonText().containsIgnoreCase("Calibrate"))
        {
            g.setColour(juce::Colour(0xff00E5FF).withAlpha(0.6f));
            g.drawRoundedRectangle(bounds, cornerSize, 2.0f);
            
            // Subtle inner glow
            g.setColour(juce::Colour(0xff00E5FF).withAlpha(0.1f));
            g.fillRoundedRectangle(bounds.reduced(2.0f), cornerSize - 1.0f);
        }
    }
    
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        auto cornerSize = 4.0f;
        juce::Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

        g.setColour (juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f), cornerSize, 1.0f);

        juce::Path path;
        auto arrowX = buttonX + buttonW * 0.5f;
        auto arrowY = buttonY + buttonH * 0.5f;
        auto arrowW = 8.0f;

        path.addTriangle(arrowX - arrowW * 0.5f, arrowY - 2.0f,
                         arrowX + arrowW * 0.5f, arrowY - 2.0f,
                         arrowX, arrowY + 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillPath(path);
    }
};

} // namespace v2m
