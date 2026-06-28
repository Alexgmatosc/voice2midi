#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace v2m
{

class ScaleQuantizer
{
public:
    ScaleQuantizer() = default;

    // Snaps a MIDI note to the nearest note in the selected scale relative to the root semitone (0-11).
    // rootSemitone: 0 (C) to 11 (B).
    // scaleTypeIndex: 0 (Chromatic), 1 (Major), 2 (Minor), 3 (Pentatonic).
    int quantize(int midiNote, int rootSemitone, int scaleTypeIndex) const
    {
        if (scaleTypeIndex == 0) return midiNote; // Chromatic is no-op

        const auto& scaleDegrees = getScaleDegrees(scaleTypeIndex);
        if (scaleDegrees.empty()) return midiNote;

        // Calculate semitone offset relative to the root in the octave [0, 11]
        int noteInOctave = (midiNote - rootSemitone) % 12;
        if (noteInOctave < 0) noteInOctave += 12;

        int bestDiff = 999;
        int closestDegree = 0;
        int octaveShift = 0;

        // Evaluate degrees in the current, previous, and next octave
        // to handle wraparounds (e.g. B snapping up to C in next octave)
        for (int shift : {-12, 0, 12})
        {
            for (int degree : scaleDegrees)
            {
                int degreeShifted = degree + shift;
                int diff = std::abs(degreeShifted - noteInOctave);
                
                if (diff < bestDiff)
                {
                    bestDiff = diff;
                    closestDegree = degree;
                    octaveShift = shift;
                }
            }
        }

        int quantizedOffset = closestDegree + octaveShift;
        return midiNote + (quantizedOffset - noteInOctave);
    }

private:
    std::vector<int> getScaleDegrees(int scaleTypeIndex) const
    {
        switch (scaleTypeIndex)
        {
            case 1: return { 0, 2, 4, 5, 7, 9, 11 };       // Major (Ionian)
            case 2: return { 0, 2, 3, 5, 7, 8, 10 };       // Minor (Aeolian)
            case 3: return { 0, 2, 4, 7, 9 };             // Major Pentatonic
            default: return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }; // Fallback
        }
    }
};

} // namespace v2m
