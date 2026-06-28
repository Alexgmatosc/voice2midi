#pragma once
#include <cmath>
#include <algorithm>

namespace v2m
{

class DrumClassifier
{
public:
    DrumClassifier() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        
        // Calculate filter coefficients
        // LPF Cutoff at 280Hz
        float fc_lp = 280.0f;
        alphaLP = std::exp(-2.0f * static_cast<float>(M_PI) * fc_lp / static_cast<float>(currentSampleRate));
        
        // HPF Cutoff at 3200Hz
        float fc_hp = 3200.0f;
        alphaHP = std::exp(-2.0f * static_cast<float>(M_PI) * fc_hp / static_cast<float>(currentSampleRate));
        
        reset();
    }

    void reset()
    {
        lastY_lp = 0.0f;
        lastY_hp = 0.0f;
        lastX_hp = 0.0f;
    }

    // Classifies the transient sound inside the sub-block.
    // Returns MIDI note numbers: 36 (Kick), 38 (Snare), 42 (Hi-Hat).
    // Lock-free and real-time safe.
    int classify(const float* data, int numSamples)
    {
        if (numSamples <= 1) return 38; // Default to snare if not enough samples

        float sumSquaresLP = 0.0f;
        float sumSquaresHP = 0.0f;
        int crossings = 0;

        float prevSign = (data[0] >= 0.0f) ? 1.0f : -1.0f;

        // Apply 1st-order IIR LPF and HPF to separate low/high frequency bands
        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];

            // 1. ZCR calculation
            float currentSign = (x >= 0.0f) ? 1.0f : -1.0f;
            if (currentSign != prevSign)
            {
                crossings++;
                prevSign = currentSign;
            }

            // 2. Low-Pass Filter
            float y_lp = alphaLP * lastY_lp + (1.0f - alphaLP) * x;
            sumSquaresLP += y_lp * y_lp;
            lastY_lp = y_lp;

            // 3. High-Pass Filter
            float y_hp = alphaHP * lastY_hp + alphaHP * (x - lastX_hp);
            sumSquaresHP += y_hp * y_hp;
            lastY_hp = y_hp;
            lastX_hp = x;
        }

        float zcr = static_cast<float>(crossings) / (2.0f * (numSamples - 1));
        float ratio = sumSquaresHP / (sumSquaresLP + 1e-5f);

        // Classification Rules:
        // - Kick (36): Low ZCR and low HPF/LPF energy ratio (bass "dum" / "booom")
        // - Hi-Hat (42): Very high ZCR and high HPF/LPF energy ratio (treble "tsh" / "tss")
        // - Snare (38): Intermediate characteristics (mid-frequency impact "pakh" / "kkh")
        if (zcr < 0.08f || ratio < 0.15f)
        {
            return 36; // Kick Drum
        }
        else if (zcr > 0.28f && ratio > 1.8f)
        {
            return 42; // Hi-Hat
        }
        
        return 38; // Snare Drum
    }

private:
    double currentSampleRate = 44100.0;
    
    float alphaLP = 0.95f;
    float alphaHP = 0.6f;
    
    // Filter history states
    float lastY_lp = 0.0f;
    float lastY_hp = 0.0f;
    float lastX_hp = 0.0f;
};

} // namespace v2m
