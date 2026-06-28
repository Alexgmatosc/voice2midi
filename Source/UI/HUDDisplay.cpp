#include "HUDDisplay.h"
#include <cmath>

namespace v2m
{

HUDDisplay::HUDDisplay()
{
}

void HUDDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Transparent glass panel background
    g.fillAll(juce::Colours::black.withAlpha(0.2f));

    // Divide bounds into left (Note HUD) and right (Tuning Needle)
    auto noteArea = bounds.removeFromLeft(bounds.getWidth() * 0.4f);
    auto needleArea = bounds.reduced(10.0f);

    // 1. Draw Note HUD
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillRoundedRectangle(noteArea.reduced(5.0f), 8.0f);
    
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRoundedRectangle(noteArea.reduced(5.0f), 8.0f, 1.5f);

    // Draw active note text
    juce::String noteName = getNoteName(currentMidiNote);
    bool isSilent = (currentMidiNote < 0);
    
    g.setColour(isSilent ? juce::Colours::grey.withAlpha(0.6f) : juce::Colours::cyan);
    g.setFont(juce::Font(juce::FontOptions().withHeight(36.0f).withStyle("Bold")));
    g.drawText(noteName, noteArea, juce::Justification::centred, true);

    // 2. Draw Tuning Needle Panel
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillRoundedRectangle(needleArea, 8.0f);
    g.setColour(juce::Colours::grey.withAlpha(0.2f));
    g.drawRoundedRectangle(needleArea, 8.0f, 1.0f);

    // Draw scale ticks (-50, 0, +50 cents)
    float centerX = needleArea.getX() + needleArea.getWidth() / 2.0f;
    float needleY = needleArea.getY() + needleArea.getHeight() * 0.72f;
    float scaleWidth = needleArea.getWidth() * 0.8f;
    float startX = centerX - scaleWidth / 2.0f;
    
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(needleY), startX, startX + scaleWidth);
    
    // Draw tick marks
    for (int i = -50; i <= 50; i += 25)
    {
        float ratio = (i + 50.0f) / 100.0f;
        float x = startX + scaleWidth * ratio;
        float tickLen = (i == 0) ? 6.0f : 3.0f;
        g.drawVerticalLine(static_cast<int>(x), needleY - tickLen, needleY + 1.0f);
        
        // Labels
        if (i == -50 || i == 0 || i == 50)
        {
            g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
            juce::String labelStr = (i == 0) ? "0" : juce::String::formatted("%+d", i);
            g.drawText(labelStr, static_cast<int>(x) - 15, static_cast<int>(needleY) + 4, 30, 12, juce::Justification::centred);
        }
    }

    // Draw the tuning needle
    if (!isSilent)
    {
        float devRatio = (centsDeviation + 50.0f) / 100.0f;
        devRatio = juce::jlimit(0.0f, 1.0f, devRatio);
        float needleX = startX + scaleWidth * devRatio;
        
        // Needle glows green if very close to pitch, orange if flat/sharp
        bool inTune = (std::abs(centsDeviation) < 5.0f);
        g.setColour(inTune ? juce::Colours::lightgreen : juce::Colours::orange);
        
        // Draw physical needle (shortened to prevent overlap)
        juce::Path needlePath;
        needlePath.startNewSubPath(needleX, needleY - 10.0f);
        needlePath.lineTo(needleX - 4.0f, needleY + 1.0f);
        needlePath.lineTo(needleX + 4.0f, needleY + 1.0f);
        needlePath.closeSubPath();
        g.fillPath(needlePath);
        
        // Draw the text showing cent deviation (Shifted up to y + 2)
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
        juce::String centStr = juce::String::formatted("Tuning: %+.1f ct", centsDeviation);
        g.drawText(centStr, needleArea.getX(), needleArea.getY() + 2, needleArea.getWidth(), 15, juce::Justification::centred);
    }
    else
    {
        // Silent needle (rest in center, faded and shortened)
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawVerticalLine(static_cast<int>(centerX), needleY - 6.0f, needleY);
        
        // Draw label (Shifted up to y + 2)
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
        g.drawText("Tuning", needleArea.getX(), needleArea.getY() + 2, needleArea.getWidth(), 15, juce::Justification::centred);
    }
}

void HUDDisplay::resized()
{
}

void HUDDisplay::updateState(int midiNote, float pitchHz)
{
    currentMidiNote = midiNote;
    currentPitchHz = pitchHz;
    calculateCentsDeviation();
    repaint();
}

juce::String HUDDisplay::getNoteName(int midiNote) const
{
    if (midiNote < 0 || midiNote > 127) return "---";
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}

void HUDDisplay::calculateCentsDeviation()
{
    if (currentMidiNote < 0 || currentPitchHz <= 0.0f)
    {
        centsDeviation = 0.0f;
        return;
    }
    
    // exactMidi = 69.0 + 12.0 * log2(freq / 440.0)
    float exactMidi = 69.0f + 12.0f * std::log2(currentPitchHz / 440.0f);
    centsDeviation = (exactMidi - static_cast<float>(currentMidiNote)) * 100.0f;
    centsDeviation = juce::jlimit(-50.0f, 50.0f, centsDeviation);
}

} // namespace v2m
