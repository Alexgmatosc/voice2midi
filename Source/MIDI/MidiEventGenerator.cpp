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
    smoothedPitchBend = 8192.0f;
    lastSentCCValue = -1;
}

float MidiEventGenerator::hzToMidi(float hz) const
{
    if (hz <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(hz / 440.0f);
}

void MidiEventGenerator::processBlock(bool isGateOpen, float detectedFreqHz, float velocityLinear, int pitchBendRangeSemi,
                                      int scaleRoot, int scaleType, float glideMs,
                                      float normalizedCentroid, int targetCC,
                                      int numSamples, juce::MidiBuffer& midiMessages)
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
                    int rawNote = juce::roundToInt(hzToMidi(stableFreqHz));
                    // Snap the new note to the selected musical scale
                    currentlyPlayingNote = scaleQuantizer.quantize(rawNote, scaleRoot, scaleType);
                    
                    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocityLinear * 127.0f)));
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, currentlyPlayingNote, velocity), sampleOffset);
                    
                    smoothedPitchBend = 8192.0f; // Start at center
                }
            }
            break;
            
        case State::Sustain:
            // 1. Timbre CC modulation output based on Spectral Centroid
            {
                int targetCCValue = juce::jlimit(0, 127, juce::roundToInt(normalizedCentroid * 127.0f));
                if (targetCCValue != lastSentCCValue)
                {
                    midiMessages.addEvent(juce::MidiMessage::controllerEvent(1, targetCC, targetCCValue), sampleOffset);
                    lastSentCCValue = targetCCValue;
                }
            }

            // 2. Pitch Bend & Legato note triggering
            if (detectedFreqHz > 0.0f)
            {
                float exactMidi = hzToMidi(detectedFreqHz);
                int rawTargetNote = juce::roundToInt(exactMidi);
                // Get the quantized target note
                int targetNote = scaleQuantizer.quantize(rawTargetNote, scaleRoot, scaleType);
                
                // If the quantized note changes, trigger Legato Note On/Off
                if (targetNote != currentlyPlayingNote)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentlyPlayingNote), sampleOffset);
                    
                    currentlyPlayingNote = targetNote;
                    juce::uint8 velocity = static_cast<juce::uint8>(juce::jlimit(1, 127, static_cast<int>(velocityLinear * 127.0f)));
                    
                    midiMessages.addEvent(juce::MidiMessage::noteOn(1, currentlyPlayingNote, velocity), sampleOffset);
                    smoothedPitchBend = 8192.0f; // Reset bend
                    midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, static_cast<int>(smoothedPitchBend)), sampleOffset);
                }
                else
                {
                    // Pitch Bend calculation relative to the currently playing (quantized) note
                    float diffSemis = exactMidi - static_cast<float>(currentlyPlayingNote);
                    float bendNormalized = diffSemis / static_cast<float>(pitchBendRangeSemi);
                    bendNormalized = juce::jlimit(-1.0f, 1.0f, bendNormalized);
                    float targetPitchBend = 8192.0f + (bendNormalized * 8191.0f);
                    
                    // Temporal smoothing (glide LPF filter)
                    if (glideMs > 0.0f)
                    {
                        float blockTimeSec = static_cast<float>(numSamples) / static_cast<float>(currentSampleRate);
                        float glideSec = glideMs / 1000.0f;
                        float alpha = std::exp(-blockTimeSec / glideSec);
                        
                        smoothedPitchBend = alpha * smoothedPitchBend + (1.0f - alpha) * targetPitchBend;
                    }
                    else
                    {
                        smoothedPitchBend = targetPitchBend;
                    }
                    
                    int finalBendValue = juce::jlimit(0, 16383, static_cast<int>(std::round(smoothedPitchBend)));
                    midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, finalBendValue), sampleOffset);
                }
            }
            break;
            
        case State::Release:
            if (currentlyPlayingNote != -1)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentlyPlayingNote), sampleOffset);
                currentlyPlayingNote = -1;
                smoothedPitchBend = 8192.0f;
                lastSentCCValue = -1;
                midiMessages.addEvent(juce::MidiMessage::pitchWheel(1, 8192), sampleOffset); // Reset bend
            }
            currentState = State::Silence;
            break;
    }
}

} // namespace v2m
