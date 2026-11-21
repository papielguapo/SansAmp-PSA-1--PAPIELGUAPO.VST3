cmake_minimum_required(VERSION 3.22)

project(SansAmpPSA1 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reuse local JUCE 8 source cache if available for fast offline builds
if(EXISTS "C:/Users/anton/PitchCanvas/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("C:/Users/anton/PitchCanvas/build/_deps/juce-src"
                     "${CMAKE_CURRENT_BINARY_DIR}/JUCE"
                     EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(SansAmpPSA1
    COMPANY_NAME                "papielguapo"
    PLUGIN_MANUFACTURER_CODE    Papi
    PLUGIN_CODE                 PSA1
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "SansAmp PSA-1"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx" "Distortion"
)

file(GLOB KNOB_PNGS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/SansAmp*.png")

juce_add_binary_data(SansAmpPSA1Assets
    SOURCES
        Assets/back.png
        Assets/g_psa1.bin
        Assets/g_psa2.bin
        ${KNOB_PNGS}
)

target_sources(SansAmpPSA1
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SansAmpDSP.h
        Source/SansAmpFIR.h
        Source/SansAmpPresets.h
)

target_include_directories(SansAmpPSA1
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(SansAmpPSA1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SansAmpPSA1
    PRIVATE
        SansAmpPSA1Assets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_graphics
        juce::juce_events
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
