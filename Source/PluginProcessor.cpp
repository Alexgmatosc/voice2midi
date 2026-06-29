#include "PluginProcessor.h"
#include "PluginEditor.h"

VoiceToMidiProcessor::VoiceToMidiProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Retrieve pointers to the parameters
    inputGainParameter = apvts.getRawParameterValue("input_gain");
    gateThresholdParameter = apvts.getRawParameterValue("gate_threshold");
    pitchBendRangeParameter = apvts.getRawParameterValue("pitch_bend_range");
    minFreqParameter = apvts.getRawParameterValue("min_freq");
    maxFreqParameter = apvts.getRawParameterValue("max_freq");
    scaleRootParameter = apvts.getRawParameterValue("scale_root");
    scaleTypeParameter = apvts.getRawParameterValue("scale_type");
    pitchBendGlideParameter = apvts.getRawParameterValue("pitch_bend_glide");
    trackingModeParameter = apvts.getRawParameterValue("tracking_mode");
    expressionCCParameter = apvts.getRawParameterValue("expression_cc");
    intellibendModeParameter = apvts.getRawParameterValue("intellibend_mode");
    intellibendStickinessParameter = apvts.getRawParameterValue("intellibend_stickiness");
}

VoiceToMidiProcessor::~VoiceToMidiProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoiceToMidiProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("input_gain", 1), "Input Gain", -24.0f, 24.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("gate_threshold", 1), "Gate Threshold", -60.0f, 0.0f, -40.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("pitch_bend_range", 1), "Pitch Bend Range",
        juce::StringArray{"1", "2", "12", "24"}, 1)); // Default index 1 is "2" semitones

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("min_freq", 1), "Min Frequency", 40.0f, 200.0f, 65.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("max_freq", 1), "Max Frequency", 300.0f, 2000.0f, 1000.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("scale_root", 1), "Scale Root",
        juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("scale_type", 1), "Scale Type",
        juce::StringArray{"Chromatic", "Major", "Minor", "Pentatonic"}, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pitch_bend_glide", 1), "Pitch Bend Glide (ms)", 0.0f, 200.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("tracking_mode", 1), "Tracking Mode",
        juce::StringArray{"Melody", "Beatbox", "Hybrid"}, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("expression_cc", 1), "Expression Target CC",
        juce::StringArray{"1 - ModWheel", "74 - Brightness/Timbre", "11 - Expression"}, 1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("intellibend_mode", 1), "Intellibend Mode",
        juce::StringArray{"Sticky", "Raw/True"}, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("intellibend_stickiness", 1), "Intellibend Stickiness (cents)", 0.0f, 100.0f, 25.0f));

    return layout;
}

const juce::String VoiceToMidiProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VoiceToMidiProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VoiceToMidiProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VoiceToMidiProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VoiceToMidiProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VoiceToMidiProcessor::getNumPrograms()
{
    return 1;
}

int VoiceToMidiProcessor::getCurrentProgram()
{
    return 0;
}

void VoiceToMidiProcessor::setCurrentProgram (int index)
{
}

const juce::String VoiceToMidiProcessor::getProgramName (int index)
{
    return {};
}

void VoiceToMidiProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void VoiceToMidiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Pre-allocate buffers and DSP objects here.
    // We allocate 8192 samples for the circular buffer to safely hold low-frequency periods at high sample rates.
    circularBuffer.prepare(8192);
    
    // Attack 5ms, Release 50ms for the envelope follower
    envelopeFollower.prepare(sampleRate, 5.0, 50.0);
    
    // Prepare YIN with a max window size (2048 is common for low frequencies down to ~65Hz at 44.1kHz)
    pitchDetector.prepare(sampleRate, 2048);
    
    // Prepare MIDI generator
    midiGenerator.prepare(sampleRate);

    // Prepare Median Filter (3-tap window)
    medianFilter.prepare(3);

    // Prepare Spectral Analyzer
    spectralAnalyzer.prepare();

    // Prepare Beatbox Engine components
    onsetDetector.prepare(sampleRate);
    drumClassifier.prepare(sampleRate);
    for (auto& drum : activeDrumNotes)
    {
        drum.noteNumber = -1;
        drum.samplesRemaining = 0;
    }

    // Pre-allocate thread-safe window buffers
    yinWindowBuffer.resize(2048, 0.0f);
    fftWindowBuffer.resize(1024, 0.0f);
    
    // Prepare Calibration Manager
    calibrationManager.prepare(sampleRate);
}

void VoiceToMidiProcessor::releaseResources()
{
    // Free any non-audio-thread resources if needed
}

bool VoiceToMidiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Make sure we have a mono input
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono())
        return false;

    // Output can be stereo to pass the signal through for monitoring
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void VoiceToMidiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Retrieve parameter values
    float inputGain = *inputGainParameter;
    float gateThreshold = *gateThresholdParameter;
    
    // Apply Input Gain (convert dB to linear)
    float linearGain = juce::Decibels::decibelsToGain(inputGain);
    buffer.applyGain(0, buffer.getNumSamples(), linearGain);

    // Read only from the mono input (channel 0)
    const float* inData = buffer.getReadPointer(0);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        circularBuffer.push(inData[i]);
        envelopeFollower.processSample(inData[i]);

        // Peak downsampler for GUI scrolling waveform visualizer
        float absVal = std::abs(inData[i]);
        if (absVal > maxPeakThisWindow)
            maxPeakThisWindow = absVal;

        downsampleCounter++;
        if (downsampleCounter >= 64)
        {
            visualFifo.push(&maxPeakThisWindow, 1);
            maxPeakThisWindow = 0.0f;
            downsampleCounter = 0;
        }
    }

    float currentDb = envelopeFollower.getCurrentEnvelopeDb();

    int trackingMode = static_cast<int>(*trackingModeParameter);
    bool runMelody = (trackingMode == 0 || trackingMode == 2);
    bool runBeatbox = (trackingMode == 1 || trackingMode == 2);

    // 1. Beatbox Drum Engine (Sub-block slicing for sub-2ms response)
    if (runBeatbox)
    {
        int numSamples = buffer.getNumSamples();
        for (int offset = 0; offset < numSamples; offset += 64)
        {
            int subBlockSize = std::min(64, numSamples - offset);
            
            if (onsetDetector.process(inData + offset, subBlockSize))
            {
                int drumNote = drumClassifier.classify(inData + offset, subBlockSize);
                
                // Channel 10 for MIDI Drums (General MIDI standard)
                midiMessages.addEvent(juce::MidiMessage::noteOn(10, drumNote, static_cast<juce::uint8>(100)), offset);
                
                // Schedule Note Off exactly 50ms later
                int releaseSamples = static_cast<int>(getSampleRate() * 0.05);
                for (auto& drum : activeDrumNotes)
                {
                    if (drum.noteNumber == -1)
                    {
                        drum.noteNumber = drumNote;
                        drum.samplesRemaining = releaseSamples;
                        break;
                    }
                }
            }
        }
    }

    // 2. Active Drum Note Off scheduler (countdown)
    for (auto& drum : activeDrumNotes)
    {
        if (drum.noteNumber != -1)
        {
            drum.samplesRemaining -= buffer.getNumSamples();
            if (drum.samplesRemaining <= 0)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(10, drum.noteNumber), 0);
                drum.noteNumber = -1;
                drum.samplesRemaining = 0;
            }
        }
    }

    // 3. Sibilant Rejection for Melody tracking (bypassed in beatbox-only mode)
    bool wasGateOpen = (midiGenerator.getCurrentlyPlayingNote() != -1);
    bool isGateOpen = wasGateOpen ? (currentDb > (gateThreshold - 3.0f)) : (currentDb > gateThreshold);
    
    bool isUnvoicedNoise = false;
    if (runMelody && isGateOpen)
    {
        isUnvoicedNoise = noiseRejecter.isUnvoiced(inData, buffer.getNumSamples(), 0.15f);
    }
    
    bool gateOpenForMelody = isGateOpen && runMelody && !isUnvoicedNoise;

    // 4. Melody Pitch Tracker (YIN)
    float detectedPitchHz = -1.0f;
    if (gateOpenForMelody)
    {
        // Read the most recent 2048 samples from the circular buffer
        circularBuffer.readRecent(yinWindowBuffer.data(), 2048);
        
        float minFreq = *minFreqParameter;
        float maxFreq = *maxFreqParameter;
        
        float rawPitch = pitchDetector.process(yinWindowBuffer.data(), 2048, minFreq, maxFreq);
        
        // Median filter only valid pitch estimates to reject transient glitches/octave jumps
        if (rawPitch > 0.0f)
        {
            detectedPitchHz = medianFilter.filter(rawPitch);
        }
    }
    else
    {
        // Clear filter history during silence so new notes start with a fresh window
        medianFilter.reset();
    }

    // Process calibration logic (will do nothing if inactive)
    calibrationManager.processBlock(inData, buffer.getNumSamples(), detectedPitchHz, currentDb);

    // 5. Timbre CC modulation (Vowels)
    float normCentroid = 0.0f;
    if (gateOpenForMelody)
    {
        // Read the most recent 1024 samples for spectral analysis
        circularBuffer.readRecent(fftWindowBuffer.data(), 1024);
        float centroidHz = spectralAnalyzer.calculateCentroid(fftWindowBuffer.data(), getSampleRate());
        
        // Normalize the centroid: 300Hz (dark, Closed Vowel) to 2000Hz (bright, Open Vowel)
        normCentroid = (centroidHz - 300.0f) / (2000.0f - 300.0f);
        normCentroid = juce::jlimit(0.0f, 1.0f, normCentroid);
    }
    currentCentroid.store(normCentroid);

    // Convert APVTS choice parameter to actual semitones (0=1, 1=2, 2=12, 3=24)
    int bendChoice = static_cast<int>(*pitchBendRangeParameter);
    int bendRangeSemi = (bendChoice == 0) ? 1 : (bendChoice == 1) ? 2 : (bendChoice == 2) ? 12 : 24;

    int scaleRoot = static_cast<int>(*scaleRootParameter);
    int scaleType = static_cast<int>(*scaleTypeParameter);
    float glideMs = *pitchBendGlideParameter;

    int expressionCCIndex = static_cast<int>(*expressionCCParameter);
    // choice 0 is CC 1 (ModWheel), 1 is CC 74 (Brightness), 2 is CC 11 (Expression)
    int targetCC = (expressionCCIndex == 0) ? 1 : (expressionCCIndex == 1) ? 74 : 11;

    int intellibendMode = static_cast<int>(*intellibendModeParameter);
    float stickinessCents = *intellibendStickinessParameter;

    // We pass the linear amplitude to map to MIDI velocity
    float linearVel = envelopeFollower.getCurrentEnvelope();
    
    bool bypassGate = !runMelody || isUnvoicedNoise;
    midiGenerator.processBlock(currentDb, gateThreshold, bypassGate, detectedPitchHz, linearVel, bendRangeSemi,
                               scaleRoot, scaleType, glideMs,
                               normCentroid, targetCC,
                               intellibendMode, stickinessCents,
                               buffer.getNumSamples(), midiMessages);

    currentLevelDb.store(currentDb);
    currentPitchHz.store(detectedPitchHz);
    currentPlayingNote.store(midiGenerator.getCurrentlyPlayingNote());

    // Mute the audio output (this is a MIDI-only generator plugin)
    buffer.clear();
}

bool VoiceToMidiProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VoiceToMidiProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void VoiceToMidiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xmlState = apvts.copyState().createXml())
        copyXmlToBinary(*xmlState, destData);
}

void VoiceToMidiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoiceToMidiProcessor();
}
