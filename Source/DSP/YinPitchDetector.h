#pragma once

#include <vector>

namespace v2m
{

class YinPitchDetector
{
public:
    YinPitchDetector() = default;

    // Must be called in prepareToPlay. Pre-allocates memory.
    void prepare(double sampleRate, int maxWindowSize);

    // Processes a contiguous block of audio samples and returns the fundamental frequency in Hz.
    // If no pitch is confidently detected, returns -1.0f.
    // The buffer size should be at least (sampleRate / minFrequency) * 2.
    float process(const float* buffer, int bufferSize, float minFrequency, float maxFrequency);

private:
    double currentSampleRate = 44100.0;
    float threshold = 0.15f;
    
    // Pre-allocated buffer to hold the Cumulative Mean Normalized Difference Function
    std::vector<float> yinBuffer;
    
    // Internal steps
    void difference(const float* buffer, int bufferSize, int minTau, int maxTau);
    void cumulativeMeanNormalizedDifference(int minTau, int maxTau);
    int absoluteThreshold(int minTau, int maxTau);
    float parabolicInterpolation(int tauEstimate);
};

} // namespace v2m
