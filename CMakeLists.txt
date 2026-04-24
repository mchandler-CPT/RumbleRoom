cmake_minimum_required(VERSION 3.22)
project(RUMBLE_ROOM VERSION 0.0.1)

# 1. Fetch JUCE from GitHub automatically
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG master # Pulls the latest JUCE 8
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(RumbleRoom
    COMPANY_NAME "bdEnergy"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "bdEn"
    PLUGIN_CODE "RmRm"
    JUCE_VST3_CAN_REPLACE_VST2 FALSE
    FORMATS Standalone VST3 AU)   # Standalone for testing

# 3. Source Files
target_sources(RumbleRoom PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

# 4. Link everything (FetchContent makes these 'juce::' targets available)
target_link_libraries(RumbleRoom PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics)

target_compile_definitions(RumbleRoom PRIVATE
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=1)

juce_generate_juce_header(RumbleRoom)