#include "YinPitchDetector.h"
#include <cmath>
#include <algorithm>

namespace v2m
{

void YinPitchDetector::prepare(double sampleRate, int maxWindowSize)
{
    currentSampleRate = sampleRate;
    // The maximum possible lag (tau) is half the window size
    yinBuffer.assign(maxWindowSize / 2, 0.0f);
}

float YinPitchDetector::process(const float* buffer, int bufferSize, float minFrequency, float maxFrequency)
{
    if (bufferSize <= 2 || minFrequency <= 0.0f || maxFrequency <= 0.0f) return -1.0f;

    // Convert frequency bounds to tau (lag) limits in samples
    int minTau = static_cast<int>(currentSampleRate / maxFrequency);
    int maxTau = static_cast<int>(currentSampleRate / minFrequency);

    // Constrain tau bounds to the buffer size limits
    int maxSearchTau = std::min(maxTau, bufferSize / 2);
    minTau = std::max(1, minTau); // tau=0 is not useful for CMNDF
    
    if (minTau >= maxSearchTau) return -1.0f;

    // Restrict YIN calculation to the vocal search range (plus 1 element boundaries for parabolic interpolation)
    int searchMinTau = std::max(1, minTau - 1);
    int searchMaxTau = std::min(static_cast<int>(yinBuffer.size()) - 1, maxSearchTau + 1);

    difference(buffer, bufferSize, searchMinTau, searchMaxTau);
    cumulativeMeanNormalizedDifference(searchMinTau, searchMaxTau);
    
    // Step 3: Absolute Threshold
    int tauEstimate = absoluteThreshold(minTau, maxSearchTau);
    
    if (tauEstimate != -1)
    {
        // Step 4: Parabolic Interpolation for better resolution
        float fractionalTau = parabolicInterpolation(tauEstimate);
        return static_cast<float>(currentSampleRate / fractionalTau);
    }
    
    return -1.0f;
}

void YinPitchDetector::difference(const float* buffer, int bufferSize, int minTau, int maxTau)
{
    int windowSize = bufferSize / 2;
    
    // Reset yinBuffer elements to 0
    std::fill(yinBuffer.begin(), yinBuffer.end(), 0.0f);

    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        float delta = 0.0f;
        for (int i = 0; i < windowSize; ++i)
        {
            float diff = buffer[i] - buffer[i + tau];
            delta += diff * diff;
        }
        yinBuffer[tau] = delta;
    }
}

void YinPitchDetector::cumulativeMeanNormalizedDifference(int minTau, int maxTau)
{
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau < minTau; ++tau)
    {
        yinBuffer[tau] = 1.0f;
    }
    
    float runningSum = 0.0f;
    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        runningSum += yinBuffer[tau];
        int numElements = tau - minTau + 1;
        if (runningSum > 0.0f)
            yinBuffer[tau] = yinBuffer[tau] * numElements / runningSum;
        else
            yinBuffer[tau] = 1.0f; // Handle exact zeros
    }
}

int YinPitchDetector::absoluteThreshold(int minTau, int maxTau)
{
    int tau = minTau;
    while (tau <= maxTau)
    {
        if (yinBuffer[tau] < threshold)
        {
            // Found a value below threshold. Let's find the local minimum.
            while (tau + 1 <= maxTau && yinBuffer[tau + 1] < yinBuffer[tau])
            {
                tau++;
            }
            return tau;
        }
        tau++;
    }
    
    // If no pitch found below threshold, we could fallback to the absolute minimum,
    // but returning -1 is safer for Voice to MIDI to avoid wild guesses.
    return -1;
}

float YinPitchDetector::parabolicInterpolation(int tauEstimate)
{
    // Cannot interpolate the boundaries
    if (tauEstimate <= 1 || tauEstimate >= static_cast<int>(yinBuffer.size()) - 1)
        return static_cast<float>(tauEstimate);
        
    float s0 = yinBuffer[tauEstimate - 1];
    float s1 = yinBuffer[tauEstimate];
    float s2 = yinBuffer[tauEstimate + 1];

    // Standard parabolic interpolation formula
    float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
    
    // Protect against division by zero or wild peaks
    if (std::isnan(adjustment) || std::abs(adjustment) > 1.0f)
        return static_cast<float>(tauEstimate);
        
    return tauEstimate + adjustment;
}

} // namespace v2m
