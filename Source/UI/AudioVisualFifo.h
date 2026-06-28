#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

namespace v2m
{

class AudioVisualFifo
{
public:
    AudioVisualFifo() : fifo(4096)
    {
        buffer.resize(4096, 0.0f);
    }
    
    // Writes audio samples into the FIFO (lock-free)
    void push(const float* data, int numSamples)
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(numSamples, start1, size1, start2, size2);
        
        if (size1 > 0) {
            std::copy(data, data + size1, buffer.begin() + start1);
        }
        if (size2 > 0) {
            std::copy(data + size1, data + size1 + size2, buffer.begin() + start2);
        }
        
        fifo.finishedWrite(size1 + size2);
    }
    
    // Reads audio samples from the FIFO (lock-free)
    // Returns the actual number of samples read.
    int read(float* dest, int numSamplesRequested)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(numSamplesRequested, start1, size1, start2, size2);
        
        int totalRead = size1 + size2;
        if (size1 > 0) {
            std::copy(buffer.begin() + start1, buffer.begin() + start1 + size1, dest);
        }
        if (size2 > 0) {
            std::copy(buffer.begin() + start2, buffer.begin() + start2 + size2, dest + size1);
        }
        
        fifo.finishedRead(totalRead);
        return totalRead;
    }
    
    int getNumAvailable() const
    {
        return fifo.getNumReady();
    }
    
private:
    juce::AbstractFifo fifo;
    std::vector<float> buffer;
};

} // namespace v2m
