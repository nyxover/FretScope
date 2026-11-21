cmake_minimum_required(VERSION 3.22)
project(FretScope VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Par défaut : plugin de type "instrument" (silencieux) -> accepté par tous les DAW.
# -DFRETSCOPE_MIDI_FX=ON : vrai effet MIDI (le MIDI traverse le plugin), pour les DAW qui le supportent.
option(FRETSCOPE_MIDI_FX "Build as a MIDI effect instead of an instrument" OFF)
if(FRETSCOPE_MIDI_FX)
    set(FS_SYNTH FALSE)
    set(FS_FX TRUE)
    set(FS_CAT "Fx")
else()
    set(FS_SYNTH TRUE)
    set(FS_FX FALSE)
    set(FS_CAT "Instrument")
endif()

set(FS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND FS_FORMATS AU)
endif()

juce_add_plugin(FretScope
    COMPANY_NAME "FretScope"
    PLUGIN_MANUFACTURER_CODE Fscp
    PLUGIN_CODE Fsc1
    FORMATS ${FS_FORMATS}
    PRODUCT_NAME "FretScope"
    IS_SYNTH ${FS_SYNTH}
    IS_MIDI_EFFECT ${FS_FX}
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT ${FS_FX}
    VST3_CATEGORIES ${FS_CAT}
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(FretScope PRIVATE
    Source/Theory.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(FretScope PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FretScope
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
