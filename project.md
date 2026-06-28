SYSTEM CONTEXT & TECHNICAL SPECIFICATION (V4 - "DUBLER" EXPERIENCE & POST-MVP)

Project: Pro Vocal MIDI Controller (Voice-to-MIDI & Beatbox-to-MIDI)
Role of the AI: You are a Senior C++ Audio DSP Architect specialized in the JUCE 8 framework. Your goal is to write highly optimized, real-time safe, and mathematically rigorous audio processing code to create a multi-dimensional vocal controller.

1. PROJECT OBJECTIVE & SCOPE

Transform the existing Voice-to-MIDI MVP into a premium real-time vocal instrument. The application/plugin must capture monophonic microphone input to detect pitch (Melody) with extremely low latency ($< 10\text{ ms}$), extract timbral characteristics (Vowels mapped to MIDI CC), and instantly detect transient unvoiced bursts (Beatbox mapped to Drum MIDI).

2. TECH STACK & BUILD SYSTEM

Language: C++20 standard.

Framework: JUCE 8.

Build System: CMake (juce_add_plugin with FORMATS Standalone VST3 AU).

Core Algorithms: YIN Algorithm (Pitch), Spectral Centroid via FFT (Timbre), Energy Derivative (Onset/Beatbox).

Parameter Management: juce::AudioProcessorValueTreeState (APVTS).

3. STRICT REAL-TIME AUDIO THREAD CONSTRAINTS (CRITICAL)

Code generated for the processBlock function and any sub-calls executed inside the audio loop MUST strictly adhere to the following rules. Failure to do so invalidates the code:

NO Dynamic Allocation: No new, malloc(), std::vector::push_back, std::string manipulation, or dynamic resizing inside processBlock. All DSP buffers, circular buffers, and arrays MUST be pre-allocated in prepareToPlay().

NO Blocking Locks: No std::mutex, std::lock_guard, or OS-level locks. Use std::atomic for primitive parameter reading. For complex data passing to the UI (e.g., drawing waveforms, pitch state), use lock-free queues like juce::AbstractFifo.

NO System Calls: No printf, std::cout, file I/O, or network calls inside processBlock.

Parameter Reading: Read parameters via std::atomic<float>* cached pointers provided by APVTS, never via string lookups during the audio callback.

FFT Efficiency: Any Fast Fourier Transform (FFT) must use pre-allocated windowing and operate on specific sizes (e.g., 512 or 1024) to avoid CPU spikes.

4. ARCHITECTURE & COMPONENT OVERVIEW

4.1. Core DSP Classes (Existing & Refined)

VoiceToMidiProcessor: Inherits from juce::AudioProcessor. Manages the APVTS, audio I/O, and MIDI buffer.

PitchDetector (YIN): Dynamic search window based on min/max parameters, processing overlapping windows for high temporal resolution.

MedianFilter & NoiseRejecter: Filters outliers and rejects sibilants (ZCR) to stabilize the pitch state machine.

4.2. New Core DSP Components (The "Dubler" Upgrade)

SpectralAnalyzer (Timbre Engine):

Integrates juce::dsp::FFT.

Extracts the Spectral Centroid (brightness/vowel shape) from the audio block to differentiate between closed vowels (e.g., "Ooo") and open vowels (e.g., "Aaa").

Maps this continuous value to a user-selectable MIDI CC (e.g., CC 74 or CC 1) to control synthesizer filters.

OnsetDetector & DrumClassifier (Beatbox Engine):

Onset Detection: Calculates the energy difference between consecutive micro-blocks (e.g., 64 samples). An instant spike triggers a drum event, bypassing the YIN latency.

Lightweight Classification: Uses Low/High band energy ratios and ZCR to output Kick (Note 36), Snare (Note 38), or Hi-Hat (Note 42) exclusively on MIDI Channel 10.

4.3. MIDI State Machine (Crucial Logic)

State: SILENCE. Amplitude $<$ Gate Threshold or unvoiced noise.

State: ATTACK. Amplitude crosses threshold. Debounce timer starts (5ms-10ms). Generates MIDI Note On (Ch 1).

State: SUSTAIN.

Pitch Bend: Outputs microtonal Pitch Bend if drift is $< 1$ semitone.

Legato: If drift is $> 1$ semitone, generates Note Off/Note On sequence.

$$NEW$$

 Timbre Control: Continuously outputs MIDI CC messages based on the Spectral Centroid fluctuations.

State: RELEASE. Amplitude drops. Send MIDI Note Off. Clear pitch bend.

4.4. UI & Visual Components (Custom Editor)

WaveformVisualizer & GateLineOverlay: Lock-free scrolling waveform with an interactive, draggable threshold line.

HUDDisplay & TuningNeedle: Prominent active note display with a cent-deviation gauge.

$$NEW$$

 TimbreMeter: Vertical bar showing the current Spectral Centroid value (Vowel mapping).

5. USER PARAMETERS (APVTS DEFINITION)

The APVTS layout must register and maintain the following parameter nodes:

input_gain & gate_threshold: Amplitude and sensitivity.

pitch_bend_range & pitch_bend_glide: Pitch wheel scaling and smoothing.

min_frequency & max_frequency: YIN optimization limits.

scale_root & scale_type: Musical quantization grids.

$$NEW$$

 tracking_mode: Choice (Melody, Beatbox, Hybrid). Determines active engines.

$$NEW$$

 expression_cc: Choice (1 - ModWheel, 74 - Timbre/Brightness, 11 - Expression). Target for vowel mapping.

6. IMPLEMENTATION ROADMAP FOR AI

Phase 1-7: Core MVP & Refinement ✅

$$x$$

 Project Setup, juce_add_plugin integration, and real-time safe constraints established.

$$x$$

 Audio Capture, CircularBuffer, and EnvelopeFollower.

$$x$$

 YinPitchDetector math and optimization.

$$x$$

 MidiEventGenerator (State machine, debouncing, pitch bend math).

$$x$$

 Custom GUI (FIFO Waveform, Gate Line, Note HUD).

$$x$$

 Noise rejection, Median Filtering, and Scale Quantization.

Phase 8: Expressive Control (Vowels to MIDI CC) ✅

$$x$$

 Integrate juce::dsp::FFT into a new SpectralAnalyzer class.

$$x$$

 Implement strictly lock-free math to compute the Spectral Centroid in real-time.

$$x$$

 Update MidiEventGenerator to output MIDI CC messages in the SUSTAIN state based on centroid fluctuations, mapped to the expression_cc parameter.

$$x$$

 Add a TimbreMeter component to the UI for visual feedback of the vowel shape.

Phase 9: Percussion Engine (Beatbox to Drums) ✅

$$x$$

 Implement an OnsetDetector that operates in parallel to the Pitch Detector, reacting in $< 2\text{ ms}$ using energy derivatives.

$$x$$

 Implement a DrumClassifier to quickly categorize the transient (Kick, Snare, Hi-Hat) using basic ZCR and frequency band energy.

$$x$$

 Update VoiceToMidiProcessor to route drum events strictly to MIDI Channel 10 when tracking_mode allows it.

7. MATHEMATICAL FORMULAS REFERENCE

Hz to MIDI Note & Pitch Bend:

$$exactMidi = 69.0f + 12.0f \times \log_2\left(\frac{frequency}{440.0f}\right)$$

$$noteNumber = \text{round}(exactMidi)$$

$$cents = (exactMidi - noteNumber) \times 100.0f$$

$$PitchBend = 8192 + \left(\frac{cents}{100.0f \times pitch\_bend\_range}\right) \times 8192$$

(Note on MIDI standard constraints: The value 8192 represents the absolute dead-center of the 14-bit MIDI pitch bend protocol, which strictly operates within a bounded range of 0 to 16383. The pitch_bend_range APVTS variable acts as a vital scaling factor and must mirror the external synthesizer's configured bend range (e.g., $\pm 2$ or $\pm 12$ semitones) to ensure accurate intonation. For instance, a $+50$ cent vocal drift with a 2-semitone range yields a MIDI value of $10240$. In the C++ implementation, this final calculated value must always be clamped via juce::jlimit(0, 16383, PitchBend) before being dispatched to the juce::MidiBuffer to prevent out-of-bounds MIDI errors during extreme vocal glissandos.)

Spectral Centroid (Timbre/Vowel):

$$Centroid = \frac{\sum_{n=0}^{N-1} f(n) \cdot x(n)}{\sum_{n=0}^{N-1} x(n)}$$

(Where $f(n)$ is the center frequency of the bin and $x(n)$ is the magnitude).

Energy Derivative (Onset Detection):

$$E_{current} = \frac{1}{N} \sum_{n=0}^{N-1} x^2(n)$$

$$E_{diff} = E_{current} - E_{previous}$$

(Trigger beatbox event if $E_{diff} > Threshold$).