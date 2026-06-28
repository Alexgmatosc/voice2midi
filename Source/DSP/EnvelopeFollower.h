#pragma once

#include <cmath>

namespace v2m
{

class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    // Pre-calculate coefficients based on sample rate and attack/release times in milliseconds
    void prepare(double sampleRate, double attackTimeMs, double releaseTimeMs)
    {
        // Avoid division by zero
        if (attackTimeMs > 0.0)
            attackCoef = std::exp(-std::log(9.0) / (sampleRate * attackTimeMs * 0.001));
        else
            attackCoef = 0.0f;
            
        if (releaseTimeMs > 0.0)
            releaseCoef = std::exp(-std::log(9.0) / (sampleRate * releaseTimeMs * 0.001));
        else
            releaseCoef = 0.0f;
            
        currentEnvelope = 0.0f;
    }

    // Process a single sample and return the updated envelope value
    inline float processSample(float sample)
    {
        float absSample = std::abs(sample);
        float coef = (absSample > currentEnvelope) ? attackCoef : releaseCoef;
        
        // Single-pole IIR filter
        currentEnvelope = coef * currentEnvelope + (1.0f - coef) * absSample;
        return currentEnvelope;
    }
    
    // Process a block of samples and return the final envelope value
    float processBlock(const float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            processSample(buffer[i]);
        }
        return currentEnvelope;
    }

    inline float getCurrentEnvelope() const { return currentEnvelope; }
    
    // Returns the envelope value in decibels
    inline float getCurrentEnvelopeDb() const
    {
        if (currentEnvelope <= 0.00001f) return -100.0f;
        return 20.0f * std::log10(currentEnvelope);
    }

private:
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;
    float currentEnvelope = 0.0f;
};

} // namespace dsp
