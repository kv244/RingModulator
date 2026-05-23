cmake_minimum_required(VERSION 3.22)

project(RingMod VERSION 1.1.0)

# Use a local JUCE tree if -DJUCE_PATH=... is provided and the path exists.
# Otherwise fall back to FetchContent (used by CI and first-time cloners).
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}")
    message(STATUS "RingMod: using local JUCE at ${JUCE_PATH}")
    add_subdirectory("${JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
else()
    message(STATUS "RingMod: fetching JUCE 8.0.13 via FetchContent")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Set -DCOPY_PLUGIN=ON to auto-install VST3 to the system folder after each build.
# Requires write access to C:/Program Files/Common Files/VST3 (run cmake --build as admin).
option(COPY_PLUGIN "Copy VST3 to system VST3 folder after each build" OFF)

juce_add_plugin(RingMod
    COMPANY_NAME "AudioDSP"
    PLUGIN_MANUFACTURER_CODE AudD
    PLUGIN_CODE RngM
    FORMATS VST3 Standalone
    PRODUCT_NAME "RingMod"
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN}
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
