#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

namespace v2m
{

class SpectralAnalyzer
{
public:
    SpectralAnalyzer()
        : fft (10), // order 10 = size 1024
          window (1024, juce::dsp::WindowingFunction<float>::hann)
    {
    }

    // Pre-allocates local buffers
    void prepare()
    {
        std::fill(std::begin(fftData), std::end(fftData), 0.0f);
    }

    // Calculates the spectral centroid of a 1024-sample block in Hz.
    // Lock-free, allocation-free, and real-time safe.
    float calculateCentroid(const float* timeDomainSamples, double sampleRate)
    {
        // 1. Copy to the FFT buffer and zero-pad the second half (required for forward transform)
        std::memcpy(fftData, timeDomainSamples, 1024 * sizeof(float));
        std::memset(fftData + 1024, 0, 1024 * sizeof(float));

        // 2. Apply Hann window to input samples
        window.multiplyWithWindowingTable(fftData, 1024);

        // 3. Perform forward FFT (in-place, magnitude spectrum in the first half)
        fft.performFrequencyOnlyForwardTransform(fftData);

        // 4. Calculate Spectral Centroid: Sum(f * magnitude) / Sum(magnitude)
        // For a 1024 FFT size, we check up to the Nyquist bin (512)
        double sumNumerator = 0.0;
        double sumDenominator = 0.0;
        double binWidth = sampleRate / 1024.0;

        for (int bin = 0; bin <= 512; ++bin)
        {
            double magnitude = static_cast<double>(fftData[bin]);
            double frequency = bin * binWidth;

            sumNumerator += frequency * magnitude;
            sumDenominator += magnitude;
        }

        if (sumDenominator > 1e-5)
        {
            return static_cast<float>(sumNumerator / sumDenominator);
        }

        return 0.0f;
    }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    
    // Size is 2 * FFT_Size (2048) to hold complex numbers in-place
    float fftData[2048] = { 0.0f };
};

} // namespace v2m
