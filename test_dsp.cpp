#include <iostream>
#include <vector>
#include <cmath>

#include "Source/DSP/CircularBuffer.h"
#include "Source/DSP/EnvelopeFollower.h"
#include "Source/DSP/YinPitchDetector.h"
#include "Source/DSP/MedianFilter.h"
#include "Source/DSP/NoiseRejecter.h"
#include "Source/DSP/ScaleQuantizer.h"

int main() {
    std::cout << "--- Testing CircularBuffer ---\n";
    v2m::CircularBuffer cb;
    cb.prepare(10);
    for (int i = 1; i <= 15; ++i) cb.push((float)i);
    float out[5] = {0};
    cb.readRecent(out, 5);
    bool bufferOk = true;
    for (int i = 0; i < 5; ++i) if (out[i] != (11 + i)) bufferOk = false;
    std::cout << "CircularBuffer logic: " << (bufferOk ? "PASSED" : "FAILED") << "\n";

    std::cout << "\n--- Testing EnvelopeFollower ---\n";
    v2m::EnvelopeFollower env;
    env.prepare(44100.0, 5.0, 50.0); 
    for (int i = 0; i < 1000; ++i) env.processSample(0.0f);
    for (int i = 0; i < 220; ++i) env.processSample(1.0f);
    std::cout << "Envelope logic: " << (env.getCurrentEnvelope() > 0.88f ? "PASSED" : "FAILED") << "\n";

    std::cout << "\n--- Testing YinPitchDetector ---\n";
    v2m::YinPitchDetector yin;
    double sampleRate = 44100.0;
    yin.prepare(sampleRate, 2048);
    
    // Generate a 440 Hz (A4) sine wave
    float testFreq = 440.0f;
    std::vector<float> sineWave(2048);
    for (int i = 0; i < 2048; ++i) {
        sineWave[i] = std::sin(2.0 * M_PI * testFreq * i / sampleRate);
    }
    
    // Test detection
    float detected = yin.process(sineWave.data(), 2048, 100.0f, 1000.0f);
    
    std::cout << "Injected Frequency: " << testFreq << " Hz\n";
    std::cout << "Detected Frequency: " << detected << " Hz\n";
    
    if (std::abs(detected - testFreq) < 1.0f) {
        std::cout << "YIN logic: PASSED (Accuracy < 1Hz)\n";
    } else {
        std::cout << "YIN logic: FAILED\n";
    }

    // Generate a lower frequency: 110 Hz (A2)
    float lowFreq = 110.0f;
    for (int i = 0; i < 2048; ++i) {
        sineWave[i] = std::sin(2.0 * M_PI * lowFreq * i / sampleRate);
    }
    float lowDetected = yin.process(sineWave.data(), 2048, 80.0f, 300.0f);
    std::cout << "\nInjected Low Frequency: " << lowFreq << " Hz\n";
    std::cout << "Detected Low Frequency: " << lowDetected << " Hz\n";

    std::cout << "\n--- Testing MedianFilter ---\n";
    v2m::MedianFilter filter;
    filter.prepare(3);
    
    // Inject sequence with a single outlier: 440, 440, 880, 440, 440
    std::vector<float> inputs = { 440.0f, 440.0f, 880.0f, 440.0f, 440.0f };
    std::vector<float> outputs;
    for (float in : inputs) {
        outputs.push_back(filter.filter(in));
    }
    
    std::cout << "Inputs:  440, 440, 880, 440, 440\n";
    std::cout << "Outputs: ";
    for (float out : outputs) std::cout << out << " ";
    std::cout << "\n";
    
    // Outlier 880 should be filtered out when it arrives (since history is 440, 440, 880 -> sorted: 440, 440, 880 -> median: 440)
    bool filterOk = (outputs[2] == 440.0f);
    std::cout << "MedianFilter logic: " << (filterOk ? "PASSED" : "FAILED") << "\n";

    std::cout << "\n--- Testing NoiseRejecter ---\n";
    v2m::NoiseRejecter rejecter;
    
    // Test ZCR of pure sine wave (vowel sound)
    float sineZcr = rejecter.calculateZCR(sineWave.data(), 2048);
    std::cout << "Sine wave (110Hz) ZCR: " << sineZcr << " (Expected: < 0.05)\n";
    
    // Generate white noise (random values between -1.0 and 1.0)
    std::vector<float> noise(512);
    for (int i = 0; i < 512; ++i) {
        noise[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
    }
    
    float noiseZcr = rejecter.calculateZCR(noise.data(), 512);
    std::cout << "White noise ZCR: " << noiseZcr << " (Expected: ~0.5)\n";
    
    bool rejecterOk = (sineZcr < 0.05f) && rejecter.isUnvoiced(noise.data(), 512, 0.15f);
    std::cout << "NoiseRejecter logic: " << (rejecterOk ? "PASSED" : "FAILED") << "\n";

    std::cout << "\n--- Testing ScaleQuantizer ---\n";
    v2m::ScaleQuantizer quantizer;
    
    // Scale Root: 0 (C), Scale Type: 1 (Major: C, D, E, F, G, A, B)
    int rootC = 0;
    int majorScale = 1;
    
    // Test 1: Note 60 (C4) -> should stay 60
    int q1 = quantizer.quantize(60, rootC, majorScale);
    std::cout << "Quantize C4 (60) in C Major: " << q1 << " (Expected: 60)\n";
    
    // Test 2: Note 61 (C#4) -> should snap to 60 (C4) or 62 (D4)
    int q2 = quantizer.quantize(61, rootC, majorScale);
    std::cout << "Quantize C#4 (61) in C Major: " << q2 << " (Expected: 60 or 62)\n";
    
    // Test 3: Note 66 (F#4) -> should snap to 65 (F4) or 67 (G4)
    int q3 = quantizer.quantize(66, rootC, majorScale);
    std::cout << "Quantize F#4 (66) in C Major: " << q3 << " (Expected: 65 or 67)\n";

    // Test 4: Note 64 (E4) -> should stay 64
    int q4 = quantizer.quantize(64, rootC, majorScale);
    std::cout << "Quantize E4 (64) in C Major: " << q4 << " (Expected: 64)\n";

    bool quantizerOk = (q1 == 60) && (q2 == 60 || q2 == 62) && (q3 == 65 || q3 == 67) && (q4 == 64);
    std::cout << "ScaleQuantizer logic: " << (quantizerOk ? "PASSED" : "FAILED") << "\n";

    return 0;
}
