#include "CalibrationManager.h"

namespace v2m
{

void CalibrationManager::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    // 2 seconds for noise, 3 seconds for vocals
    samplesForNoise = static_cast<int>(currentSampleRate * 2.0);
    samplesForVocals = static_cast<int>(currentSampleRate * 3.0);
}

void CalibrationManager::startCalibration()
{
    elapsedSamples = 0;
    
    noiseRmsSum = 0.0f;
    noiseBlockCount = 0;
    
    vocalRmsSum = 0.0f;
    vocalBlockCount = 0;
    
    tempMinHz = 20000.0f;
    tempMaxHz = 0.0f;
    
    currentState.store(State::MeasuringNoise);
}

void CalibrationManager::processBlock(const float* buffer, int numSamples, float detectedHz, float currentDb)
{
    State state = currentState.load();
    if (state == State::Inactive || state == State::Completed)
        return;

    // Calculate RMS of current block to help with thresholding
    float sumSquared = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sumSquared += buffer[i] * buffer[i];
    }
    float rms = std::sqrt(sumSquared / static_cast<float>(numSamples));
    float rmsDb = juce::Decibels::gainToDecibels(rms, -100.0f);

    if (state == State::MeasuringNoise)
    {
        noiseRmsSum += rmsDb;
        noiseBlockCount++;
        elapsedSamples += numSamples;

        if (elapsedSamples >= samplesForNoise)
        {
            // Finish noise measurement
            float avgNoiseDb = noiseBlockCount > 0 ? (noiseRmsSum / static_cast<float>(noiseBlockCount)) : -100.0f;
            noiseFloorDb.store(avgNoiseDb);
            
            elapsedSamples = 0;
            currentState.store(State::MeasuringVocalRange);
        }
    }
    else if (state == State::MeasuringVocalRange)
    {
        // Only count vocal blocks that are significantly above the noise floor
        float nFloor = noiseFloorDb.load();
        if (rmsDb > nFloor + 10.0f)
        {
            vocalRmsSum += rmsDb;
            vocalBlockCount++;

            if (detectedHz > 20.0f && detectedHz < 2000.0f) // Sanity bounds
            {
                if (detectedHz < tempMinHz) tempMinHz = detectedHz;
                if (detectedHz > tempMaxHz) tempMaxHz = detectedHz;
            }
        }
        
        elapsedSamples += numSamples;

        if (elapsedSamples >= samplesForVocals)
        {
            // Finish vocal measurement
            float avgVocalDb = vocalBlockCount > 0 ? (vocalRmsSum / static_cast<float>(vocalBlockCount)) : nFloor + 20.0f;
            
            // Set gate threshold halfway between noise floor and average vocal level
            float safeGateDb = nFloor + 12.0f; 
            if (vocalBlockCount > 0)
            {
                safeGateDb = nFloor + ((avgVocalDb - nFloor) * 0.5f);
            }
            gateThresholdDb.store(juce::jlimit(-100.0f, 0.0f, safeGateDb));

            // Set freq bounds with a 10% safety margin, clamping to safe limits
            float finalMin = tempMinHz < 20000.0f ? tempMinHz * 0.9f : 80.0f;
            float finalMax = tempMaxHz > 0.0f ? tempMaxHz * 1.1f : 1000.0f;
            
            minFreqHz.store(juce::jlimit(20.0f, 400.0f, finalMin)); // Cap minimum
            maxFreqHz.store(juce::jlimit(400.0f, 2000.0f, finalMax)); // Cap maximum

            currentState.store(State::Completed);
        }
    }
}

} // namespace v2m
