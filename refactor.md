🔴 SYSTEM DIRECTIVE FOR ANTIGRAVITY: CORE DSP REFACTORING & SAFETY

To: Antigravity (AI Coding Agent)
Context: We have successfully built the MVP for voice2midi. However, before we implement Phase 8 (Spectral Analyzer/Dubler UX), we MUST refactor the current codebase to eliminate severe real-time audio thread violations and CPU bottlenecks.

Please execute the following tasks sequentially. Provide the updated code for each file.

TASK 1: Strict Audio Thread Safety (CRITICAL)

Target Files: Source/PluginProcessor.h & Source/PluginProcessor.cpp

Eliminate Dynamic Allocation:

Issue: There are dynamic memory allocations (e.g., std::vector, push_back, or implicit allocations) happening inside the processBlock for the YIN window buffer.

Action: Move all temporary buffers to private member variables in PluginProcessor.h. Resize them EXCLUSIVELY inside prepareToPlay() using the estimated maximum buffer size. In processBlock, only overwrite the pre-allocated memory.

Optimize APVTS Parameter Lookups:

Issue: Parameters are being fetched via string lookup (e.g., apvts.getRawParameterValue("gate_threshold")) inside the processBlock, which locks the thread.

Action: In the constructor or prepareToPlay, cache the atomic pointers (std::atomic<float>*) for all active parameters. Inside processBlock, read their values purely using the .load() method.

TASK 2: YIN Algorithm CPU Optimization

Target File: Source/DSP/YinPitchDetector.cpp & Source/DSP/YinPitchDetector.h

Issue: The difference() function currently iterates over a static, massive range of $\tau$ values, calculating correlations for frequencies outside the human vocal range, causing massive CPU spikes.

Action: Utilize the min_frequency and max_frequency user parameters. Inside the DSP loop, dynamically calculate minTau = sampleRate / max_frequency and maxTau = sampleRate / min_frequency. Restrict the inner for loops of the YIN algorithm to only calculate the difference and CMNDF functions within this strict [minTau, maxTau] bounds.

TASK 3: MIDI State Machine Hysteresis (Playability)

Target File: Source/MIDI/MidiEventGenerator.cpp

Issue: Natural vocal fluctuations around the exact gate_threshold cause "MIDI machine-gunning" (rapid switching between ATTACK and RELEASE states).

Action: Implement an asymmetrical Hysteresis for the Gate.

The threshold to enter the ATTACK state remains the user's gate_threshold.

The threshold to fall back to RELEASE / SILENCE must be exactly -3.0 dB lower than the attack threshold.

Ensure this hysteresis logic evaluates securely inside the state machine.

Execution: Acknowledge these instructions and provide the refactored code for PluginProcessor.h/cpp to complete Task 1.