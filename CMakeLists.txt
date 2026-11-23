cmake_minimum_required(VERSION 3.22)

project(Brute102 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Point JUCE_DIR to a local JUCE checkout or let FetchContent pull JUCE 8.0.13
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Brute102
    COMPANY_NAME                "BruteAudio"
    PLUGIN_MANUFACTURER_CODE    Brut
    PLUGIN_CODE                 B102
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Brute-102"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(Brute102
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

set_source_files_properties(
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    PROPERTIES
        COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-Wshadow>"
)

target_compile_definitions(Brute102
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Brute102
        PUBLIC
            -Wno-unknown-warning-option
            -Wno-nontrivial-memcall
    )
endif()

target_link_libraries(Brute102
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)
