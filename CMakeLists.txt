cmake_minimum_required(VERSION 3.20)

project(voice2midi VERSION 1.0.0)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9 # Use version 8.0.9
)
FetchContent_MakeAvailable(juce)

# Add JUCE Plugin
juce_add_plugin(voice2midi
    COMPANY_NAME "OpenSource"
    PLUGIN_MANUFACTURER_CODE "Open"
    PLUGIN_CODE "V2MI"
    FORMATS Standalone VST3 AU
    PRODUCT_NAME "voice2midi"
    NEEDS_MIDI_OUTPUT TRUE
    PLIST_TO_MERGE
    "<plist>
      <dict>
        <key>NSMicrophoneUsageDescription</key>
        <string>voice2midi necesita acceso al microfono para poder convertir tu voz en notas MIDI.</string>
      </dict>
    </plist>"
)
juce_generate_juce_header(voice2midi)

# C++20 Standard
target_compile_features(voice2midi PUBLIC cxx_std_20)

target_compile_definitions(voice2midi PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Source files
target_sources(voice2midi PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/YinPitchDetector.cpp
    Source/MIDI/MidiEventGenerator.cpp
    Source/UI/WaveformVisualizer.cpp
    Source/UI/HUDDisplay.cpp
)

# Include directories
target_include_directories(voice2midi PRIVATE Source)

# Link to JUCE libraries
target_link_libraries(voice2midi PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_events
    juce::juce_core
)

# Set IDE folders
set_target_properties(voice2midi PROPERTIES FOLDER "Plugins")
