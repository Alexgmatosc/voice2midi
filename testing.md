🔵 SYSTEM DIRECTIVE FOR ANTIGRAVITY: C++ UNIT TESTING IMPLEMENTATION

To: Antigravity (AI Coding Agent)
Context: We need to establish a rigorous Automated Unit Testing pipeline for the voice2midi project to guarantee mathematical precision and algorithmic stability before adding new features. We will use JUCE's native juce::UnitTest framework. No refactoring of the core logic right now; focus strictly on writing tests for the existing components.

Please execute the following tasks sequentially. Provide the updated code for each step.

TASK 1: Test Environment & CMake Setup

Target Files: CMakeLists.txt & New File Source/Tests/TestRunner.cpp

Action: Configure the CMake project to support an executable test runner alongside the plugin targets.

Implementation:

Update CMakeLists.txt to create a new executable target (e.g., voice2midi_tests) that links against the core DSP/MIDI classes and JUCE modules.

Create TestRunner.cpp containing a main() function that instantiates a juce::UnitTestRunner, runs all registered tests, and returns 0 if all pass, or 1 if any fail.

TASK 2: DSP Math Tests (YIN Accuracy)

Target File: New File Source/Tests/YinPitchDetectorTests.cpp

Action: Create a test class inheriting from juce::UnitTest named YinPitchDetectorTests.

Test Case 1 (Perfect Sine Wave):

Programmatically generate a std::vector<float> containing a pure sine wave at exactly $440.0\text{ Hz}$ (assuming a $48000\text{ Hz}$ sample rate).

Process this buffer through the YinPitchDetector.

Expectation: expectWithinAbsoluteError(detectedPitch, 440.0f, 0.5f) must pass.

Test Case 2 (Silence/Noise): Feed an array of zeros or low-level white noise. Ensure the detector correctly identifies it as unvoiced/silent.

TASK 3: MIDI State Machine & Hysteresis Tests

Target File: New File Source/Tests/MidiEventGeneratorTests.cpp

Action: Create a test class inheriting from juce::UnitTest named MidiEventGeneratorTests.

Test Case 1 (Debounce & Attack):

Simulate a sudden crossing of the gate threshold.

Expectation: Verify that the MidiEventGenerator correctly waits for the debounce period before emitting a Note On event.

Test Case 2 (Pitch Bend Legato vs Glide):

Simulate a stable pitch, followed by a microtonal shift (+40 cents). Expect a Pitch Bend message.

Simulate a jump of +3 semitones. Expect a Note Off for the old note and a Note On for the new note (Legato).

TASK 4: Memory Safety Tests (Circular Buffer)

Target File: New File Source/Tests/CircularBufferTests.cpp

Action: Create a test class inheriting from juce::UnitTest.

Test Case 1 (Erratic Block Sizes):

Initialize the buffer with a window size of 2048.

Simulate a DAW sending erratic block sizes in a loop (e.g., push 13 samples, then 64, then 5, then 128).

Expectation: Verify that the internal read/write pointers wrap around mathematically correctly and that reading a window returns the exact expected concatenated sequence of samples without out-of-bounds access.

Execution: Acknowledge this directive. Begin by providing the updated CMakeLists.txt and the TestRunner.cpp (Task 1) so we can compile the test suite.