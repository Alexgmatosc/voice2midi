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
    bool isGateOpen = (currentDb > gateThreshold);

    // 1. Zero-Crossing Rate Noise rejection (bypass YIN and close gate if unvoiced sibilant noise)
    bool isUnvoicedNoise = noiseRejecter.isUnvoiced(inData, buffer.getNumSamples(), 0.15f);
    if (isUnvoicedNoise)
    {
        isGateOpen = false;
    }

    float detectedPitchHz = -1.0f;
    if (isGateOpen)
    {
        // Read the most recent 2048 samples from the circular buffer
        float window[2048];
        circularBuffer.readRecent(window, 2048);
        
        float minFreq = *minFreqParameter;
        float maxFreq = *maxFreqParameter;
        
        float rawPitch = pitchDetector.process(window, 2048, minFreq, maxFreq);
        
        // 2. Median filter only valid pitch estimates to reject transient glitches/octave jumps
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

    // Convert APVTS choice parameter to actual semitones (0=1, 1=2, 2=12, 3=24)
    int bendChoice = static_cast<int>(*pitchBendRangeParameter);
    int bendRangeSemi = (bendChoice == 0) ? 1 : (bendChoice == 1) ? 2 : (bendChoice == 2) ? 12 : 24;

    int scaleRoot = static_cast<int>(*scaleRootParameter);
    int scaleType = static_cast<int>(*scaleTypeParameter);
    float glideMs = *pitchBendGlideParameter;

    // We pass the linear amplitude to map to MIDI velocity
    float linearVel = envelopeFollower.getCurrentEnvelope();
    
    midiGenerator.processBlock(isGateOpen, detectedPitchHz, linearVel, bendRangeSemi,
                               scaleRoot, scaleType, glideMs, buffer.getNumSamples(), midiMessages);

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
