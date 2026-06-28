#pragma once
#include <cmath>

namespace v2m
{

class NoiseRejecter
{
public:
    NoiseRejecter() = default;

    // Computes the Zero-Crossing Rate (ZCR) of an audio buffer block.
    // ZCR = count of crossings divided by 2 * (N - 1). Range: 0.0 to 1.0.
    float calculateZCR(const float* data, int numSamples) const
    {
        if (numSamples <= 1) return 0.0f;

        int crossings = 0;
        // Determine initial sign
        float prevSign = (data[0] >= 0.0f) ? 1.0f : -1.0f;

        for (int i = 1; i < numSamples; ++i)
        {
            float currentSign = (data[i] >= 0.0f) ? 1.0f : -1.0f;
            if (currentSign != prevSign)
            {
                crossings++;
                prevSign = currentSign;
            }
        }

        // Normalize ZCR
        return static_cast<float>(crossings) / (2.0f * (numSamples - 1));
    }

    // Evaluates if the current block is unvoiced noise (e.g. sibilants or mouth click)
    // based on ZCR threshold.
    bool isUnvoiced(const float* data, int numSamples, float threshold = 0.15f) const
    {
        if (numSamples < 16) return false;
        
        float zcr = calculateZCR(data, numSamples);
        return zcr > threshold;
    }
};

} // namespace v2m
