#include <JuceHeader.h>
#include "DSP/CircularBuffer.h"

class CircularBufferTests : public juce::UnitTest
{
public:
    CircularBufferTests() : juce::UnitTest ("CircularBufferTests", "DSP") {}

    void runTest() override
    {
        beginTest ("Case 1: Erratic Block Sizes & Wraparound");
        {
            v2m::CircularBuffer cb;
            // Prepare buffer with a capacity of 2048
            cb.prepare (2048);

            // Push exactly 5000 samples to guarantee multiple ring wraparounds
            // We use highly erratic block sizes to simulate various DAW buffers (like 13, 64, 5, 128, etc.)
            int blockSizes[] = { 13, 64, 5, 128, 33, 7, 256, 1 };
            int blockSizeCount = 8;
            int blockIdx = 0;
            
            float valueCounter = 0.0f;
            int totalPushed = 0;
            
            while (totalPushed < 5000)
            {
                int size = blockSizes[blockIdx % blockSizeCount];
                blockIdx++;
                
                for (int j = 0; j < size; ++j)
                {
                    cb.push (valueCounter);
                    valueCounter += 1.0f;
                }
                
                totalPushed += size;
            }

            // Read the most recent 128 samples
            float readOut[128] = { 0.0f };
            cb.readRecent (readOut, 128);

            // The last sample pushed was valueCounter - 1.0f.
            // The readRecent output returns values chronologically (oldest to newest).
            // So index 0 of readOut should be valueCounter - 128.0f.
            bool correctOrder = true;
            for (int i = 0; i < 128; ++i)
            {
                float expected = (valueCounter - 128.0f) + static_cast<float>(i);
                if (std::abs (readOut[i] - expected) > 1e-4f)
                {
                    correctOrder = false;
                    break;
                }
            }

            expect (correctOrder, "CircularBuffer read output values do not match expected chronological sequence");
        }
    }
};

static CircularBufferTests circularBufferTests;
