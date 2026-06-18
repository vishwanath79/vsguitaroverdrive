cmake_minimum_required(VERSION 3.22)
project(VSGuitarOverdrive VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(VSGuitarOverdrive
    VERSION 0.1.0
    COMPANY_NAME "VSPlugins"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE VsgX
    PLUGIN_CODE VsgO
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "VS Guitar Overdrive")

juce_generate_juce_header(VSGuitarOverdrive)

target_sources(VSGuitarOverdrive
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_compile_definitions(VSGuitarOverdrive
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VSGuitarOverdrive
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
