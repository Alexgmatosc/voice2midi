#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>

namespace v2m
{

class CircularBuffer
{
public:
    CircularBuffer() = default;

    // Must be called in prepareToPlay, allocating the memory BEFORE the audio thread starts
    void prepare(int size)
    {
        buffer.assign(size, 0.0f);
        writeIndex = 0;
    }

    // Lock-free and allocation-free method to push a new sample
    inline void push(float sample)
    {
        if (buffer.empty()) return;
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % buffer.size();
    }

    // Read the most recent 'numSamples' into a destination array.
    // The data will be contiguous and ordered from oldest to newest.
    void readRecent(float* dest, int numSamples) const
    {
        if (buffer.empty() || numSamples > (int)buffer.size()) return;
        
        int readIndex = writeIndex - numSamples;
        if (readIndex < 0) 
            readIndex += buffer.size();

        int firstPart = std::min(numSamples, (int)buffer.size() - readIndex);
        int secondPart = numSamples - firstPart;

        std::copy(buffer.begin() + readIndex, buffer.begin() + readIndex + firstPart, dest);
        if (secondPart > 0)
        {
            std::copy(buffer.begin(), buffer.begin() + secondPart, dest + firstPart);
        }
    }
    
    int getSize() const { return static_cast<int>(buffer.size()); }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
};

} // namespace v2m
