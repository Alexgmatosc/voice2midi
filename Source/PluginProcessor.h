#pragma once

#include <JuceHeader.h>
#include "DSP/CircularBuffer.h"
#include "DSP/EnvelopeFollower.h"
#include "DSP/YinPitchDetector.h"
#include "MIDI/MidiEventGenerator.h"
#include "UI/AudioVisualFifo.h"
#include "DSP/MedianFilter.h"
#include "DSP/NoiseRejecter.h"
#include "DSP/SpectralAnalyzer.h"
#include "DSP/OnsetDetector.h"
#include "DSP/DrumClassifier.h"

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
    int getCurrentPlayingNote() const { return currentPlayingNote.load(); }
    float getCurrentCentroid() const { return currentCentroid.load(); }
    v2m::AudioVisualFifo& getVisualFifo() { return visualFifo; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Parameter caches (std::atomic<float>*)
    std::atomic<float>* inputGainParameter = nullptr;
    std::atomic<float>* gateThresholdParameter = nullptr;
    std::atomic<float>* pitchBendRangeParameter = nullptr;
    std::atomic<float>* minFreqParameter = nullptr;
    std::atomic<float>* maxFreqParameter = nullptr;
    std::atomic<float>* scaleRootParameter = nullptr;
    std::atomic<float>* scaleTypeParameter = nullptr;
    std::atomic<float>* pitchBendGlideParameter = nullptr;
    std::atomic<float>* trackingModeParameter = nullptr;
    std::atomic<float>* expressionCCParameter = nullptr;

    v2m::CircularBuffer circularBuffer;
    v2m::EnvelopeFollower envelopeFollower;
    v2m::YinPitchDetector pitchDetector;
    v2m::MidiEventGenerator midiGenerator;
    v2m::MedianFilter medianFilter;
    v2m::NoiseRejecter noiseRejecter;
    v2m::SpectralAnalyzer spectralAnalyzer;
    v2m::OnsetDetector onsetDetector;
    v2m::DrumClassifier drumClassifier;

    struct ActiveDrumNote
    {
        int noteNumber = -1;
        int samplesRemaining = 0;
    };
    ActiveDrumNote activeDrumNotes[8] = {};

    std::vector<float> yinWindowBuffer;
    std::vector<float> fftWindowBuffer;

    std::atomic<float> currentLevelDb { -100.0f };
    std::atomic<float> currentPitchHz { -1.0f };
    std::atomic<int> currentPlayingNote { -1 };
    std::atomic<float> currentCentroid { 0.0f };

    v2m::AudioVisualFifo visualFifo;
    int downsampleCounter = 0;
    float maxPeakThisWindow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceToMidiProcessor)
};
