cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# 1. PROJECT CONFIGURATION
# ==============================================================================
project(RumbleRoom VERSION 0.0.1 LANGUAGES CXX)

include(FetchContent)
include(CTest)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal Binary support" FORCE)
endif()

# ==============================================================================
# 2. JUCE FRAMEWORK LOCATION
# ==============================================================================
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Juce" CACHE PATH "Path to JUCE")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Run: git submodule update --init --recursive")
endif()
add_subdirectory("${JUCE_PATH}" JUCE)

set(BOUTIQUE_PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND BOUTIQUE_PLUGIN_FORMATS AU)
endif()

# ==============================================================================
# 3. JUCE PLUGIN DECLARATION
# ==============================================================================
juce_add_plugin(RumbleRoom
    COMPANY_NAME "bdEnergy"
    PLUGIN_MANUFACTURER "bdEnergy"
    PLUGIN_MANUFACTURER_CODE "bdEn"
    BUNDLE_ID "com.bdenergy.rumbleroom"
    FORMATS ${BOUTIQUE_PLUGIN_FORMATS}
    COPY_PLUGIN_AFTER_BUILD FALSE

  # Effect: IS_SYNTH FALSE | NEEDS_MIDI_INPUT FALSE (host playhead drives sync)
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE

    PLUGIN_CODE "RmRm"
    PLUGIN_NAME "RumbleRoom"
    PRODUCT_NAME "RumbleRoom"
    DESCRIPTION "RumbleRoom Boutique Delay"
)

if(APPLE)
    set_target_properties(RumbleRoom PROPERTIES
        JUCE_AU_MANUFACTURER_CODE "bdEn"
        JUCE_AU_PLUGIN_CODE "RmRm"
        JUCE_PLUGIN_CODE "RmRm"
        JUCE_MANUFACTURER_CODE "bdEn"
    )
endif()

set_target_properties(RumbleRoom PROPERTIES
    JUCE_PLUGIN_CODE "RmRm"
    JUCE_MANUFACTURER_CODE "bdEn"
    JUCE_AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(RumbleRoom)

# ==============================================================================
# 4. SOURCE FILE COMPILATION
# ==============================================================================
target_sources(RumbleRoom PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UI/BoutiqueLookAndFeel.h
    Source/UI/BoutiqueLookAndFeel.cpp
    Source/UI/BoutiqueKnobHelpers.h
    Source/UI/BoutiqueKnobHelpers.cpp
    Source/UI/BoutiqueLayoutConstants.h
    Source/UI/ChromeTextButton.h
    Source/UI/ChromeTextButton.cpp
    Source/UI/HydraPalette.h
    Source/UI/HydraPanelBackgroundLayout.h
    Source/UI/HydraPanelBackgroundLayout.cpp
    Source/UI/ProceduralDarkLookAndFeel.h
    Source/UI/ProceduralDarkLookAndFeel.cpp
)

target_include_directories(RumbleRoom PRIVATE Source Source/UI)

target_link_libraries(RumbleRoom PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
)

target_compile_definitions(RumbleRoom PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=1
)

target_compile_features(RumbleRoom PRIVATE cxx_std_20)

# ==============================================================================
# 5. TEST INFRASTRUCTURE (CATCH2)
# ==============================================================================
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(BoutiqueTests
    Tests/PluginTests.cpp
)

target_include_directories(BoutiqueTests PRIVATE
    Source
    "${CMAKE_BINARY_DIR}/RumbleRoom_artefacts/JuceLibraryCode"
    "${JUCE_PATH}/modules"
)

add_dependencies(BoutiqueTests RumbleRoom)

target_compile_definitions(BoutiqueTests PRIVATE
    $<TARGET_PROPERTY:RumbleRoom,COMPILE_DEFINITIONS>
)

target_link_libraries(BoutiqueTests PRIVATE
    Catch2::Catch2WithMain
    RumbleRoom
)

target_compile_features(BoutiqueTests PRIVATE cxx_std_20)

if(BUILD_TESTING)
    add_test(NAME BoutiqueTests COMMAND BoutiqueTests)
endif()
