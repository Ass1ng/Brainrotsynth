cmake_minimum_required(VERSION 3.22)
project(BrainrotSynth VERSION 1.0.0)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 7.0.12 GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

file(GLOB SAMPLE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Samples/*.wav)
juce_add_binary_data(BrainrotData SOURCES ${SAMPLE_FILES})

juce_add_plugin(BrainrotSynth
  COMPANY_NAME "Brainrot Labs"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  PLUGIN_MANUFACTURER_CODE Brnr
  PLUGIN_CODE Brsy
  FORMATS VST3
  PRODUCT_NAME "Brainrot Synth")

target_sources(BrainrotSynth PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_definitions(BrainrotSynth PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BrainrotSynth
  PRIVATE BrainrotData juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
