#include <JuceHeader.h>
#include "DSP/YinPitchDetector.h"

class YinPitchDetectorTests : public juce::UnitTest
{
public:
    YinPitchDetectorTests() : juce::UnitTest ("YinPitchDetectorTests", "DSP") {}

    void runTest() override
    {
        beginTest ("Case 1: Perfect Sine Wave (440 Hz)");
        {
            v2m::YinPitchDetector detector;
            double sampleRate = 48000.0;
            detector.prepare(sampleRate, 2048);

            // Generate a pure sine wave at exactly 440.0 Hz
            float testFreq = 440.0f;
            std::vector<float> buffer(2048);
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                buffer[i] = std::sin(2.0 * M_PI * testFreq * static_cast<double>(i) / sampleRate);
            }

            float detected = detector.process(buffer.data(), 2048, 100.0f, 1000.0f);
            
            // Expect within 0.5 Hz error
            expectWithinAbsoluteError (detected, testFreq, 0.5f);
        }

        beginTest ("Case 2: Silence & Noise");
        {
            v2m::YinPitchDetector detector;
            double sampleRate = 48000.0;
            detector.prepare(sampleRate, 2048);

            // Silence (all zeros)
            std::vector<float> silence(2048, 0.0f);
            float detectedSilence = detector.process(silence.data(), 2048, 100.0f, 1000.0f);
            expect (detectedSilence < 0.0f);

            // Low-level white noise (to simulate background microphone hiss)
            std::vector<float> noise(2048);
            for (size_t i = 0; i < noise.size(); ++i)
            {
                noise[i] = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * 0.0001f;
            }
            float detectedNoise = detector.process(noise.data(), 2048, 100.0f, 1000.0f);
            expect (detectedNoise < 0.0f);
        }
    }
};

static YinPitchDetectorTests yinPitchDetectorTests;
