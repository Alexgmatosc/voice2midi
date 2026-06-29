#include <JuceHeader.h>
#include "MIDI/MidiEventGenerator.h"

class MidiEventGeneratorTests : public juce::UnitTest
{
public:
    MidiEventGeneratorTests() : juce::UnitTest ("MidiEventGeneratorTests", "MIDI") {}

    void runTest() override
    {
        beginTest ("Case 1: Debounce & Attack");
        {
            v2m::MidiEventGenerator generator;
            double sampleRate = 44100.0;
            generator.prepare(sampleRate);

            juce::MidiBuffer midiMessages;
            
            // 1. Initial silence: no events should trigger
            generator.processBlock(-100.0f, -30.0f, false, 0.0f, 0.0f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            expect (midiMessages.isEmpty());
            expect (generator.getCurrentlyPlayingNote() == -1);

            // 2. Sudden crossing of threshold (e.g., -10 dB, threshold is -30 dB)
            // Block 1 (128 samples / ~2.9ms) -> Debounce is 10ms (~441 samples), so should NOT trigger yet
            generator.processBlock(-10.0f, -30.0f, false, 440.0f, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            expect (midiMessages.isEmpty());
            expect (generator.getCurrentlyPlayingNote() == -1);

            // Block 2 (256 samples / ~5.8ms)
            generator.processBlock(-10.0f, -30.0f, false, 440.0f, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            expect (midiMessages.isEmpty());

            // Block 3 (384 samples / ~8.7ms)
            generator.processBlock(-10.0f, -30.0f, false, 440.0f, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            expect (midiMessages.isEmpty());

            // Block 4 (512 samples / ~11.6ms) -> Debounce period exceeded, should trigger Note On!
            generator.processBlock(-10.0f, -30.0f, false, 440.0f, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            expect (!midiMessages.isEmpty());
            expect (generator.getCurrentlyPlayingNote() == 69); // 440 Hz is MIDI 69
        }

        beginTest ("Case 2: Pitch Bend Legato vs Glide");
        {
            v2m::MidiEventGenerator generator;
            double sampleRate = 44100.0;
            generator.prepare(sampleRate);

            juce::MidiBuffer midiMessages;
            
            // 1. Establish initial Note On at 440 Hz (MIDI 69) by passing debounce
            for (int i = 0; i < 5; ++i)
            {
                midiMessages.clear();
                generator.processBlock(-10.0f, -30.0f, false, 440.0f, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            }
            expect (generator.getCurrentlyPlayingNote() == 69);

            // 2. Microtonal shift: +40 cents (approx 450.3 Hz)
            // Expect Pitch Bend output, but the note number must remain 69
            midiMessages.clear();
            float microtonalFreq = 450.3f;
            generator.processBlock(-10.0f, -30.0f, false, microtonalFreq, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            
            expect (generator.getCurrentlyPlayingNote() == 69);
            
            // Check if there is a Pitch Bend event in the buffer
            bool hasPitchBend = false;
            for (const auto metadata : midiMessages)
            {
                if (metadata.getMessage().isPitchWheel())
                    hasPitchBend = true;
            }
            expect (hasPitchBend);

            // 3. Legato transition: +3 semitones jump (approx 523.25 Hz -> MIDI 72)
            // Expect Note Off for 69, Note On for 72
            midiMessages.clear();
            float legatoFreq = 523.25f;
            generator.processBlock(-10.0f, -30.0f, false, legatoFreq, 0.5f, 2, 0, 0, 0.0f, 0.0f, 1, 0, 25.0f, 128, midiMessages);
            
            expect (generator.getCurrentlyPlayingNote() == 72);
            
            bool hasNoteOff = false;
            bool hasNoteOn = false;
            for (const auto metadata : midiMessages)
            {
                auto msg = metadata.getMessage();
                if (msg.isNoteOff() && msg.getNoteNumber() == 69)
                    hasNoteOff = true;
                if (msg.isNoteOn() && msg.getNoteNumber() == 72)
                    hasNoteOn = true;
            }
            expect (hasNoteOff);
            expect (hasNoteOn);
        }
    }
};

static MidiEventGeneratorTests midiEventGeneratorTests;
