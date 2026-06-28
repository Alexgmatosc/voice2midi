#include <iostream>
#include <vector>
#include <cmath>

#include "Source/DSP/CircularBuffer.h"
#include "Source/DSP/EnvelopeFollower.h"
#include "Source/DSP/YinPitchDetector.h"

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

    return 0;
}
