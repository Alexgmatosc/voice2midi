#pragma once
#include <JuceHeader.h>

namespace v2m
{

class TimbreMeter : public juce::Component
{
public:
    TimbreMeter() = default;

    // Sets the normalized spectral centroid value (0.0 to 1.0)
    void setCentroidValue(float normalizedCentroid)
    {
        currentValue = juce::jlimit(0.0f, 1.0f, normalizedCentroid);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background box
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Fill area rising from the bottom
        float fillHeight = bounds.getHeight() * currentValue;
        auto fillArea = bounds.removeFromBottom(fillHeight);

        // Glow color: cyan (closed vowel, low centroid) to orange (open vowel, high centroid)
        juce::Colour meterColor = juce::Colours::cyan.interpolatedWith(juce::Colours::orange, currentValue);
        g.setColour(meterColor);
        g.fillRoundedRectangle(fillArea, 4.0f);

        // Vertical label
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawText("VOWEL", getLocalBounds().removeFromBottom(15), juce::Justification::centred);
    }

private:
    float currentValue = 0.0f;
};

} // namespace v2m
