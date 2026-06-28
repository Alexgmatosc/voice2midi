#include "MidiEventGenerator.h"
#include <cmath>

namespace v2m
{

void MidiEventGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    // 10ms debounce time to avoid machine-gunning notes
    debounceSamples = static_cast<int>(currentSampleRate * 0.010);
    reset();
}

void MidiEventGenerator::reset()
{
    currentState = State::Silence;
    samplesSinceAttack = 0;
    currentlyPlayingNote = -1;
    stableFreqHz = 0.0f;
}

float MidiEventGenerator::hzToMidi(float hz) const
{
    if (hz <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(hz / 440.0f);
}

void MidiEventGenerator::processBlock(bool isGateOpen, float detectedFreqHz, float velocityLinear, int pitchBendRangeSemi, int numSamples, juce::MidiBuffer& midiMessages)
{
    // State transitions based on Gate
    if (!isGateOpen)
    {
        if (currentState == State::Attack || currentState == State::Sustain)
        {
            currentState = State::Release;
        }
    }
    else
    {
        if (currentState == State::Silence || currentState == State::Release)
        {
            currentState = State::Attack;
            samplesSinceAttack = 0;
            stableFreqHz = detectedFreqHz;
        }
    }
    
    // We add all events at the beginning of the block (sample offset 0) for simplicity.
    // In a hyper-accurate system, we could interpolate the exact sample of attack.
    int sampleOffset = 0;
    
    switch (currentState)
    {
        case State::Silence:
            break;
            
        case State::Attack:
            samplesSinceAttack += numSamples;
            
            // Only process if we have a valid pitch
            if (detectedFreqHz > 0.0f)
            {
                // If pitch drifts significantly during attack, reset the debounce timer
                if (stableFreqHz <= 0.0f || std::abs(hzToMidi(detectedFreqHz) - hzToMidi(stableFreqHz)) > 1.0f)
                {
                    stableFreqHz = detectedFreqHz;
                    samplesSinceAttack = 0;
                }
                
                if (samplesSinceAttack >= debounceSamples)
                {
                    currentState = State::Sustain;
                    currentlyPlayingNote = juce::roundToInt(hzToMidi(stableFreqHz));
                    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocityLinear * 127.0f)));
                    
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, currentlyPlayingNote, velocity), sampleOffset);
                }
            }
            break;
            
        case State::Sustain:
            if (detectedFreqHz > 0.0f)
            {
                float exactMidi = hzToMidi(detectedFreqHz);
                float diffSemis = exactMidi - static_cast<float>(currentlyPlayingNote);
                
                // If pitch drifts more than 1 semitone, trigger Legato Note On
                if (std::abs(diffSemis) > 1.0f)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentlyPlayingNote), sampleOffset);
                    
                    currentlyPlayingNote = juce::roundToInt(exactMidi);
                    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocityLinear * 127.0f)));
                    
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, currentlyPlayingNote, velocity), sampleOffset);
                    midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, 8192), sampleOffset); // Reset bend
                }
                else
                {
                    // Pitch Bend calculation (14-bit MIDI: 0 to 16383, center 8192)
                    float bendNormalized = diffSemis / static_cast<float>(pitchBendRangeSemi);
                    bendNormalized = juce::jlimit(-1.0f, 1.0f, bendNormalized);
                    int bendValue = 8192 + static_cast<int>(bendNormalized * 8191.0f);
                    midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, bendValue), sampleOffset);
                }
            }
            break;
            
        case State::Release:
            if (currentlyPlayingNote != -1)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentlyPlayingNote), sampleOffset);
                currentlyPlayingNote = -1;
                midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, 8192), sampleOffset); // Reset bend
            }
            currentState = State::Silence;
            break;
    }
}

} // namespace v2m
