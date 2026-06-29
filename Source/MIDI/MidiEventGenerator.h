#pragma once
#include <JuceHeader.h>
#include "../DSP/ScaleQuantizer.h"

namespace v2m
{

class MidiEventGenerator
{
public:
    enum class State
    {
        Silence,
        Attack,
        Sustain,
        Release
    };

    MidiEventGenerator() = default;

    void prepare(double sampleRate);

    // Processes the state machine and injects MIDI events into the buffer.
    void processBlock(float currentDb, float gateThreshold, bool bypassGate,
                      float detectedFreqHz, float velocityLinear, int pitchBendRangeSemi,
                      int scaleRoot, int scaleType, float glideMs,
                      float normalizedCentroid, int targetCC,
                      int intellibendMode, float stickinessCents,
                      int numSamples, juce::MidiBuffer& midiMessages);

    void reset();

    int getCurrentlyPlayingNote() const { return currentlyPlayingNote; }

private:
    double currentSampleRate = 44100.0;
    State currentState = State::Silence;
    
    int samplesSinceAttack = 0;
    int debounceSamples = 0; // Calculated in prepare (e.g. 10ms)

    int currentlyPlayingNote = -1;
    float stableFreqHz = 0.0f;
    float smoothedPitchBend = 8192.0f; // 14-bit center value
    int lastSentCCValue = -1;

    ScaleQuantizer scaleQuantizer;
    
    float hzToMidi(float hz) const;
};

} // namespace v2m
