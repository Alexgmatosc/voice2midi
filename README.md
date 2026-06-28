# Voice2MIDI

Voice2MIDI is a real-time, low-latency, monophonic voice-to-MIDI converter. It captures mono audio from your microphone, analyzes pitch and amplitude in real time using the YIN pitch detection algorithm, and translates vocal input into standard MIDI messages (Note On, Note Off, Velocity, and Pitch Bend).

It compiles as a **Standalone Application**, **VST3 plugin**, and **AU plugin** (on macOS), enabling you to control software synthesizers or virtual instruments using your voice.

---

## Features

- **YIN Pitch Detection:** High-accuracy monophonic pitch tracker operating in real time.
- **Visual Feedback HUD:** Displays the current active note and a fine-tuning needle showing cents deviation (-50 to +50 cents).
- **Interactive Waveform Visualizer:** Shows incoming audio with a draggable gate threshold line.
- **Sibilant Rejection (ZCR):** Analyzes the Zero-Crossing Rate (ZCR) to automatically detect unvoiced consonants (like "S", "T", "SH") and bypass pitch detection, preventing accidental notes.
- **Glitch Filtering (Median Filter):** Multi-tap running median filter to eliminate octave-doubling errors and transient pitch spikes.
- **Scale Quantization:** Snaps detected notes to musical grids (Chromatic, Major, Minor, Pentatonic) in any key.
- **Pitch Bend Smoother (Glide):** Adjustable low-pass filter ($0.0$ to $200.0$ ms) to smooth out pitch-bend data for continuous glides without stepping.

---

## Parameter Settings Guide

The parameter panel at the bottom of the interface lets you tune the tracking behavior:

### 1. Input Gain (dB)
* **Range:** `-24.0 dB` to `+24.0 dB` (Default: `0.0 dB`)
* **Purpose:** Boosts or attenuates microphone input before pitch analysis. Aim for vocal peaks between `-15 dB` and `-5 dB` on the waveform visualizer.

### 2. Gate Threshold (dB)
* **Range:** `-60.0 dB` to `0.0 dB` (Default: `-40.0 dB`)
* **Purpose:** Separates background noise from vocal performance. No MIDI messages are sent if the signal amplitude falls below this threshold. You can also drag the horizontal line on the waveform to update this.

### 3. Pitch Bend Range (Semitones)
* **Options:** `1`, `2`, `12`, `24` semitones (Default: `2`)
* **Purpose:** Sets the maximum range of pitch bend messages. **Important:** Match this setting with your target synthesizer's pitch bend range to ensure accurate glides.

### 4. Min / Max Frequency (Hz)
* **Min Range:** `40.0 Hz` to `200.0 Hz` (Default: `65.0 Hz`)
* **Max Range:** `300.0 Hz` to `2000.0 Hz` (Default: `1000.0 Hz`)
* **Purpose:** Limits the search window of the YIN algorithm. Restricting this range saves CPU and prevents low-frequency rumble or high-frequency breath sounds from triggering fake notes.

### 5. Scale Root & Scale Type
* **Scale Root:** Choice from C to B.
* **Scale Type:** Chromatic (no snapping), Major, Minor, or Pentatonic.
* **Purpose:** Snaps detected notes to a musical grid. Cents deviation (for vibrato) is calculated relative to the scale-quantized note.

### 6. Pitch Bend Glide (ms)
* **Range:** `0.0 ms` to `200.0 ms` (Default: `0.0 ms`)
* **Purpose:** Smooths pitch bend transitions over time using a one-pole filter. Higher values create a sliding portamento effect and reject vocal jitter.

---

## Building and Installation

### Prerequisites
- CMake (version 3.15 or higher)
- A C++20 compatible compiler (Clang, GCC, or MSVC)
- macOS (for AU and standalone mic permission configuration) or Windows/Linux

### Compiling
To build all formats (Standalone, VST3, AU) in release mode, run the build script in the project root:

```bash
chmod +x build.sh
./build.sh
```

Compiled binaries will be written to:
`build/voice2midi_artefacts/Release/`

---

## DAW Setup & Routing (Mac/GarageBand)

DAWs like GarageBand do not support routing MIDI outputs from plugins natively. To route MIDI from the **Standalone** target:

1. **Enable macOS IAC Driver:**
   - Open **Audio MIDI Setup** on your Mac.
   - Go to **Window** -> **Show MIDI Studio**.
   - Double-click **IAC Driver**, check **"Device is online"**, and click apply.
2. **Configure Voice2MIDI Standalone:**
   - Open `voice2midi.app`.
   - Go to **Options** -> **Audio/MIDI Settings**.
   - Set **Input** to your microphone.
   - Set **Active MIDI Outputs** to **IAC Driver Bus 1**.
   - Keep the **Audio buffer size** as low as possible (e.g., `128` or `256` samples) to minimize latency.
3. **Configure DAW:**
   - Open GarageBand / Logic / Reaper.
   - Create a software instrument track.
   - The DAW will automatically listen to incoming MIDI from the IAC Driver bus, and you can play the virtual instrument with your voice!
