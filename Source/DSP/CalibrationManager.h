#pragma once
#include <JuceHeader.h>
#include <atomic>

namespace v2m
{

class CalibrationManager
{
public:
    enum class State {
        Inactive,
        MeasuringNoise,
        MeasuringVocalRange,
        Completed
    };

    CalibrationManager() = default;

    void prepare(double sampleRate);
    void startCalibration();
    
    // Call this inside the audio thread
    void processBlock(const float* buffer, int numSamples, float detectedHz, float currentDb);

    // Thread-safe accessors for UI / Logic
    State getCurrentState() const { return currentState.load(); }
    float getNoiseFloorDb() const { return noiseFloorDb.load(); }
    float getGateThresholdDb() const { return gateThresholdDb.load(); }
    float getMinFreqHz() const { return minFreqHz.load(); }
    float getMaxFreqHz() const { return maxFreqHz.load(); }

    void markAsInactive() { currentState.store(State::Inactive); }

private:
    double currentSampleRate = 44100.0;
    
    std::atomic<State> currentState { State::Inactive };
    
    int elapsedSamples = 0;
    int samplesForNoise = 0;
    int samplesForVocals = 0;

    // Measurement accumulators
    float noiseRmsSum = 0.0f;
    int noiseBlockCount = 0;
    
    float vocalRmsSum = 0.0f;
    int vocalBlockCount = 0;
    
    float tempMinHz = 20000.0f;
    float tempMaxHz = 0.0f;

    // Final results
    std::atomic<float> noiseFloorDb { -100.0f };
    std::atomic<float> gateThresholdDb { -60.0f };
    std::atomic<float> minFreqHz { 80.0f };
    std::atomic<float> maxFreqHz { 1000.0f };
};

} // namespace v2m
