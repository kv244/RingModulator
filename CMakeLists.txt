cmake_minimum_required(VERSION 3.22)

project(RingMod VERSION 1.0.0)

# Allow overriding JUCE location via -DJUCE_PATH=...
if(NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "C:/Users/julia/Downloads/juce-8.0.13-windows/JUCE")
endif()
add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)

juce_add_plugin(RingMod
    COMPANY_NAME "AudioDSP"
    PLUGIN_MANUFACTURER_CODE AudD
    PLUGIN_CODE RngM
    FORMATS VST3 Standalone
    PRODUCT_NAME "RingMod"
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.svg"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.svg"
)

target_sources(RingMod PRIVATE
    source/PluginProcessor.h
    source/PluginProcessor.cpp
    source/PluginEditor.h
    source/PluginEditor.cpp
)

target_compile_features(RingMod PRIVATE cxx_std_17)

target_compile_definitions(RingMod PUBLIC
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

target_link_libraries(RingMod PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

juce_generate_juce_header(RingMod)
