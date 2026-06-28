#pragma once
#include <vector>
#include <algorithm>

namespace v2m
{

class MedianFilter
{
public:
    MedianFilter() = default;

    // Pre-allocates memory for history
    void prepare(int size)
    {
        windowSize = std::max(1, size);
        history.assign(static_cast<size_t>(windowSize), 0.0f);
        writeIndex = 0;
        filled = false;
    }

    // Inserts a new value and returns the median of the current window.
    // Lock-free and allocation-free.
    float filter(float value)
    {
        if (windowSize <= 1) return value;

        history[writeIndex] = value;
        writeIndex = (writeIndex + 1) % static_cast<size_t>(windowSize);
        
        if (writeIndex == 0) filled = true;

        // If the window is not yet fully filled, we only sort the active elements
        size_t activeElements = filled ? static_cast<size_t>(windowSize) : writeIndex;
        if (activeElements == 0) return value;

        std::vector<float> sorted(activeElements);
        std::copy(history.begin(), history.begin() + activeElements, sorted.begin());
        std::sort(sorted.begin(), sorted.end());

        return sorted[activeElements / 2];
    }

    void reset()
    {
        std::fill(history.begin(), history.end(), 0.0f);
        writeIndex = 0;
        filled = false;
    }

private:
    int windowSize = 3;
    std::vector<float> history;
    size_t writeIndex = 0;
    bool filled = false;
};

} // namespace v2m
