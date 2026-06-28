#pragma once
#include <cmath>
#include <algorithm>

namespace v2m
{

class OnsetDetector
{
public:
    OnsetDetector() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        baselineEnergy = 0.001f;
        lockoutCounter = 0;
    }

    // Processes a sub-block of samples and returns true if an onset is detected.
    // Lock-free, allocation-free, and real-time safe.
    bool process(const float* data, int numSamples, float sensitivityThreshold = 2.2f)
    {
        if (numSamples <= 0) return false;

        // 1. Calculate RMS energy of the current sub-block
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            sumSquares += data[i] * data[i];
        }
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples));

        // Decrement lockout counter
        if (lockoutCounter > 0)
        {
            lockoutCounter -= numSamples;
        }

        bool triggered = false;

        // 2. Trigger check: if energy exceeds threshold * baseline and lockout is over
        // We also require a minimum absolute energy of 0.008f to avoid triggering on background hiss.
        if (lockoutCounter <= 0 && rms > 0.008f && rms > sensitivityThreshold * baselineEnergy)
        {
            triggered = true;
            // Lockout double-triggers for 35ms
            lockoutCounter = static_cast<int>(currentSampleRate * 0.035);
        }

        // 3. Update baseline energy using a slow-moving low-pass filter
        // We track slower when energy rises to prevent transients from polluting the noise floor baseline.
        float alpha = (rms > baselineEnergy) ? 0.995f : 0.93f;
        baselineEnergy = alpha * baselineEnergy + (1.0f - alpha) * rms;

        return triggered;
    }

    void reset()
    {
        baselineEnergy = 0.001f;
        lockoutCounter = 0;
    }

private:
    double currentSampleRate = 44100.0;
    float baselineEnergy = 0.001f;
    int lockoutCounter = 0;
};

} // namespace v2m
