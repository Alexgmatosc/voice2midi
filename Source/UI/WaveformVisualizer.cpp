#include "WaveformVisualizer.h"

namespace v2m
{

WaveformVisualizer::WaveformVisualizer(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    waveformHistory.assign(historySize, 0.0f);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Dark background with glassmorphism style gradient
    g.fillAll(juce::Colours::black.withAlpha(0.3f));
    
    // Draw grid lines
    g.setColour(juce::Colours::grey.withAlpha(0.15f));
    int numGridLines = 6;
    for (int i = 1; i < numGridLines; ++i)
    {
        float y = bounds.getHeight() * (static_cast<float>(i) / static_cast<float>(numGridLines));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
    }

    // Draw scrolling Waveform (Symmetric)
    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    juce::Path path;
    
    float centerY = bounds.getHeight() / 2.0f;
    float stepX = bounds.getWidth() / static_cast<float>(historySize - 1);
    
    for (size_t i = 0; i < waveformHistory.size(); ++i)
    {
        float x = static_cast<float>(i) * stepX;
        // Apply scaling. 0.0 to 1.0 mapped to centerY height
        float magnitude = juce::jlimit(0.0f, 1.0f, std::abs(waveformHistory[i]));
        float yOffset = magnitude * centerY * 0.9f; // Leave 10% headroom
        
        if (i == 0)
        {
            path.startNewSubPath(x, centerY - yOffset);
        }
        else
        {
            path.lineTo(x, centerY - yOffset);
        }
    }
    
    // Draw the bottom half of the symmetric path in reverse order
    for (int i = static_cast<int>(waveformHistory.size()) - 1; i >= 0; --i)
    {
        float x = static_cast<float>(i) * stepX;
        float magnitude = juce::jlimit(0.0f, 1.0f, std::abs(waveformHistory[i]));
        float yOffset = magnitude * centerY * 0.9f;
        path.lineTo(x, centerY + yOffset);
    }
    
    path.closeSubPath();
    g.fillPath(path);

    // Draw the center line
    g.setColour(juce::Colours::cyan.withAlpha(0.2f));
    g.drawHorizontalLine(static_cast<int>(centerY), 0.0f, bounds.getWidth());

    // Draw the gate threshold line
    float gateY = getGateY();
    
    if (isDraggingGate || isMouseOverGate)
        g.setColour(juce::Colours::orange);
    else
        g.setColour(juce::Colours::orange.withAlpha(0.6f));
        
    g.drawHorizontalLine(static_cast<int>(gateY), 0.0f, bounds.getWidth());
    
    // Add text label next to the gate threshold line
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawText(juce::String::formatted("Gate: %.1f dB", getGateDb()),
               10, static_cast<int>(gateY) - 15, 120, 15,
               juce::Justification::left);
}

void WaveformVisualizer::resized()
{
}

void WaveformVisualizer::pushSamples(const float* samples, int numSamples)
{
    if (numSamples <= 0) return;

    // Shift history left
    if (numSamples < historySize)
    {
        std::copy(waveformHistory.begin() + numSamples, waveformHistory.end(), waveformHistory.begin());
        std::copy(samples, samples + numSamples, waveformHistory.end() - numSamples);
    }
    else
    {
        // If we got more samples than history size, just copy the latest ones
        std::copy(samples + numSamples - historySize, samples + numSamples, waveformHistory.begin());
    }
    
    repaint();
}

float WaveformVisualizer::getGateY() const
{
    float db = getGateDb();
    float gain = juce::Decibels::decibelsToGain(db);
    
    float height = getHeight();
    float centerY = height / 2.0f;
    
    // Map linear gain (0.0 to 1.0) to (centerY to 0) since top is 0
    // gateY = centerY - (gain * centerY * 0.9f)
    return centerY - (gain * centerY * 0.9f);
}

void WaveformVisualizer::mouseDown(const juce::MouseEvent& event)
{
    float gateY = getGateY();
    // Allow a 10 pixel grab range around the gate line
    if (std::abs(event.position.y - gateY) < 10.0f)
    {
        isDraggingGate = true;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        repaint();
    }
}

void WaveformVisualizer::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingGate)
    {
        float y = juce::jlimit(0.0f, static_cast<float>(getHeight()), event.position.y);
        float centerY = getHeight() / 2.0f;
        
        // Calculate the linear gain from Y position
        float gain = 0.0f;
        if (y < centerY)
        {
            gain = (centerY - y) / (centerY * 0.9f);
        }
        gain = juce::jlimit(0.0f, 1.0f, gain);
        
        // Convert back to decibels
        float db = juce::Decibels::gainToDecibels(gain, -60.0f);
        updateGateParameter(db);
    }
}

void WaveformVisualizer::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDraggingGate = false;
    setMouseCursor(isMouseOverGate ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void WaveformVisualizer::mouseMove(const juce::MouseEvent& event)
{
    float gateY = getGateY();
    bool hover = (std::abs(event.position.y - gateY) < 10.0f);
    
    if (hover != isMouseOverGate)
    {
        isMouseOverGate = hover;
        setMouseCursor(isMouseOverGate ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void WaveformVisualizer::updateGateParameter(float newDb)
{
    if (auto* parameter = apvts.getParameter("gate_threshold"))
    {
        float normalized = parameter->convertTo0to1(newDb);
        parameter->setValueNotifyingHost(normalized);
    }
}

} // namespace v2m
