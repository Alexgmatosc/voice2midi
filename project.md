SYSTEM CONTEXT & TECHNICAL SPECIFICATION (V3 - EXTENDED & POST-MVP)

Project: Low-Latency Voice-to-MIDI Converter (Standalone & Plugin)
Role of the AI: You are a Senior C++ Audio DSP Architect specialized in the JUCE framework. Your goal is to write highly optimized, real-time safe, and mathematically rigorous audio processing code.

1. PROJECT OBJECTIVE & SCOPE

Develop a C++ application/plugin using JUCE 8 that captures a monophonic microphone input (human voice), detects the fundamental frequency (pitch) and amplitude envelope with extremely low latency ($< 10\text{ ms}$), and outputs the corresponding MIDI Note On/Off, Velocity, and Pitch Bend events in real-time to control external synthesizers.

2. TECH STACK & BUILD SYSTEM

Language: C++20 standard.

Framework: JUCE 8.

Build System: CMake (juce_add_plugin with FORMATS Standalone VST3 AU).

Core Algorithm: YIN Algorithm (or pYIN) for pitch detection.

Parameter Management: juce::AudioProcessorValueTreeState (APVTS) for all UI-to-Processor parameters.

3. STRICT REAL-TIME AUDIO THREAD CONSTRAINTS (CRITICAL)

Code generated for the processBlock function and any sub-calls executed inside the audio loop MUST strictly adhere to the following rules. Failure to do so invalidates the code:

NO Dynamic Allocation: No new, malloc(), std::vector::push_back, std::string manipulation, or dynamic resizing inside processBlock. All DSP buffers, circular buffers, and arrays MUST be pre-allocated in prepareToPlay().

NO Blocking Locks: No std::mutex, std::lock_guard, or OS-level locks. Use std::atomic for primitive parameter reading. For complex data passing to the UI (e.g., drawing waveforms, pitch state), use lock-free queues like juce::AbstractFifo.

NO System Calls: No printf, std::cout, file I/O, or network calls inside processBlock.

Parameter Reading: Read parameters via std::atomic<float>* cached pointers provided by APVTS, never via string lookups during the audio callback.

4. ARCHITECTURE & COMPONENT OVERVIEW

4.1. Core DSP Classes

VoiceToMidiProcessor: Inherits from juce::AudioProcessor. Manages the APVTS, the audio I/O, and the MIDI buffer.

PitchDetector (YIN):

Maintains a pre-allocated internal Circular Buffer (Ring Buffer).

Because YIN requires a minimum window size (e.g., 1024 or 2048 samples) to detect low frequencies (like 65Hz), but the host might send blocks of 64 or 128 samples, this class MUST accumulate samples and process overlapping windows (e.g., 50% or 75% overlap) to maintain high temporal resolution (low latency) while providing enough data to the YIN math.

EnvelopeFollower: Calculates the RMS or Peak amplitude of the incoming signal to determine if the voice is active (Gate) and maps this to MIDI Velocity.

MedianFilter: A running sliding filter to reject transient frequency outliers and sudden octave-doubling jumps before sending pitch to the state machine.

NoiseRejecter: Computes Zero-Crossing Rate (ZCR) or Spectral Flatness to bypass the pitch detector instantly when unvoiced sibilants ("S", "T", "F") are present.

4.2. MIDI State Machine (Crucial Logic)

To prevent "MIDI machine-gunning" (triggering multiple notes due to natural vocal vibrato or noise), implement a strict State Machine inside MidiEventGenerator:

State: SILENCE. If EnvelopeFollower amplitude $<$ Gate Threshold or the NoiseRejecter flags unvoiced noise.

State: ATTACK. Amplitude crosses Gate Threshold. Wait for PitchDetector (plus MedianFilter) to return a stable fundamental frequency for at least DebounceTime (e.g., 5ms-10ms). Generate MIDI Note On.

State: SUSTAIN. While amplitude remains above threshold:

If the detected pitch fluctuates within $\pm 1$ semitone, DO NOT trigger a new note. Calculate the cent deviation and output MIDI Pitch Bend messages to bend the currently playing note.

If the detected pitch jumps significantly ($> 1$ semitone) and stays stable, generate MIDI Note Off for the old note, and MIDI Note On for the new note (Legato transition).

State: RELEASE. Amplitude drops below Gate Threshold. Send MIDI Note Off. Clear pitch bend.

4.3. UI & Visual Components (Custom Editor)

WaveformVisualizer: A lock-free component reading downsampled audio data from a FIFO queue to paint the rolling input waveform.

GateLineOverlay: A horizontal, semi-transparent line representing the current gate_threshold parameter mapped to the visual limits of the waveform, supporting interactive dragging to update the parameter.

HUDDisplay: Highly visible, large typography component showing the currently played note (e.g., "C3", "F#4") when active, or "---" when silent.

TuningNeedle: Cent-deviation gauge displaying fractional tuning offset ($\pm 50$ cents) in sync with pitch bend.

5. USER PARAMETERS (APVTS DEFINITION)

The APVTS layout must register and maintain the following parameter nodes:

input_gain: Float (-24.0 dB to +24.0 dB).

gate_threshold: Float (-60.0 dB to 0.0 dB). Below this, silence is assumed.

pitch_bend_range: Choice (1, 2, 12, 24 semitones). Scale coefficient for Pitch Bend.

min_frequency: Float (40 Hz to 200 Hz). Limits the maximum YIN search window ($\tau$).

max_frequency: Float (300 Hz to 2000 Hz). Limits the minimum YIN search window.

scale_root: Choice (C, C#, D, ... B). Semitone offsets (0-11) for Scale Quantization. [NEW]

scale_type: Choice (Chromatic, Major, Minor, Pentatonic). Quantizer modes. [NEW]

pitch_bend_glide: Float (0.0 ms to 200.0 ms). Temporal smoothing of pitch-wheel events. [NEW]

6. IMPLEMENTATION ROADMAP FOR AI

Phase 1: Project Setup & Parameters ✅

[x] Generate the CMakeLists.txt using FetchContent for JUCE 8.

[x] Implement the AudioProcessor skeleton and the AudioProcessorValueTreeState setup with parameters.

[x] Setup basic generic GUI using juce::GenericAudioProcessorEditor for validation.

Phase 2: Audio Capture & Envelope Following ✅

[x] Implement the internal lock-free CircularBuffer logic in processBlock.

[x] Implement the EnvelopeFollower (Peak/RMS) to detect voice presence against the gate threshold.

Phase 3: DSP Engine (YIN Pitch Detection) ✅

[x] Implement YIN steps (Difference function, CMNDF, Absolute threshold, Parabolic interpolation).

[x] Ensure the algorithm operates safely on overlapping windows with dynamic $\tau$ limits.

Phase 4: MIDI State Machine & Output ✅

[x] Implement the MidiEventGenerator observing the strict State Machine (Silence, Attack, Sustain, Release) and debounce logic.

[x] Map cents deviations to precise 14-bit Pitch Bend output and output Legato transitions.

Phase 5: Custom GUI & Visual Feedback ✅

[x] Audio-to-UI Lock-free FIFO: Implement a downsampler inside processBlock to send small packages of downsampled audio samples to the Editor via juce::AbstractFifo without locking.

[x] Waveform & Gate Component: Design a custom Component that paints the real-time scrolling waveform with an overlay of the gate_threshold as a draggable line.

[x] Active Note Display & Cent Needle: Build a prominent visual HUD displaying the current note string and a cent deviation tuning needle linked directly to the detected Pitch Bend state.

Phase 6: Pitch Refinement & Noise Filtering ✅

[x] Median Filter Implementation: Integrate a 3-tap or 5-tap running MedianFilter before sending pitch values to the state machine to reject momentary octave jumps.

[x] Zero-Crossing Rate (ZCR) Discriminator: Calculate ZCR inside processBlock. If the threshold flags unvoiced white noise (high ZCR indicative of sibilants like "S"), immediately force the MIDI state machine to bypass YIN.

Phase 7: Compositional & Expressive Control [CURRENT]

[ ] Scale Quantizer logic: Integrate scale snapping using the scale_root and scale_type parameters to restrict Note On outputs to specified musical grids.

[ ] Pitch Bend Smoother: Implement a time-coefficient-based glide (pitch_bend_glide) to smooth out continuous pitch wheel messages, preventing jittery synthesizer transitions.

7. MATHEMATICAL FORMULAS REFERENCE

Hz to MIDI Note:


$$exactMidi = 69.0f + 12.0f \times \log_2\left(\frac{frequency}{440.0f}\right)$$

Nearest MIDI Note:


$$noteNumber = \text{round}(exactMidi)$$

Pitch Bend Calculation:


$$cents = (exactMidi - noteNumber) \times 100.0f$$


Map $cents$ based on the user's pitch_bend_range parameter to the 14-bit MIDI range ($0$ to $16383$, center $8192$):


$$PitchBendValue = 8192 + \left(\frac{cents}{100.0f \times pitch\_bend\_range}\right) \times 8192$$