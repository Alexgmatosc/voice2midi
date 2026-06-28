#pragma once

#include <JuceHeader.h>
#include "DSP/CircularBuffer.h"
#include "DSP/EnvelopeFollower.h"
#include "DSP/YinPitchDetector.h"
#include "MIDI/MidiEventGenerator.h"

class VoiceToMidiProcessor : public juce::AudioProcessor
{
public:
    VoiceToMidiProcessor();
    ~VoiceToMidiProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    float getCurrentLevelDb() const { return currentLevelDb.load(); }
    float getCurrentPitchHz() const { return currentPitchHz.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Parameter caches (std::atomic<float>*)
    std::atomic<float>* inputGainParameter = nullptr;
    std::atomic<float>* gateThresholdParameter = nullptr;
    std::atomic<float>* pitchBendRangeParameter = nullptr;
    std::atomic<float>* minFreqParameter = nullptr;
    std::atomic<float>* maxFreqParameter = nullptr;

    v2m::CircularBuffer circularBuffer;
    v2m::EnvelopeFollower envelopeFollower;
    v2m::YinPitchDetector pitchDetector;
    v2m::MidiEventGenerator midiGenerator;

    std::atomic<float> currentLevelDb { -100.0f };
    std::atomic<float> currentPitchHz { -1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceToMidiProcessor)
};
