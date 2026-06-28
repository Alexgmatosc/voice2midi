#pragma once
#include <JuceHeader.h>

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
    // Should be called once per audio block to minimize MIDI jitter and CPU load.
    void processBlock(bool isGateOpen, float detectedFreqHz, float velocityLinear, int pitchBendRangeSemi, int numSamples, juce::MidiBuffer& midiMessages);

    void reset();

private:
    double currentSampleRate = 44100.0;
    State currentState = State::Silence;
    
    int samplesSinceAttack = 0;
    int debounceSamples = 0; // Calculated in prepare (e.g. 10ms)

    int currentlyPlayingNote = -1;
    float stableFreqHz = 0.0f;
    
    float hzToMidi(float hz) const;
};

} // namespace v2m
