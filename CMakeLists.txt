cmake_minimum_required(VERSION 3.22)

project(RingMod VERSION 1.1.1)

# ---------------------------------------------------------------------------
# JUCE dependency
#
# Use a local JUCE tree if -DJUCE_PATH=<path> is provided on the CMake command
# line and the path actually exists on disk.  This is the fastest option for
# local development because it skips the network fetch.
#
# Otherwise fall back to FetchContent, which clones JUCE 8.0.13 from GitHub.
# FetchContent is used by CI pipelines and first-time cloners who do not have
# JUCE checked out locally.
# ---------------------------------------------------------------------------
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}")
    message(STATUS "RingMod: using local JUCE at ${JUCE_PATH}")
    add_subdirectory("${JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
else()
    message(STATUS "RingMod: fetching JUCE 8.0.13 via FetchContent")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13
        GIT_SHALLOW    TRUE   # shallow clone — only tip of tag, no full history
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Optional: auto-install the VST3 to the system plug-in folder after each build.
#
# Enable with:  cmake -DCOPY_PLUGIN=ON ...
# Requires write access to C:/Program Files/Common Files/VST3 on Windows.
# Run cmake --build as administrator if you see permission errors.
# ---------------------------------------------------------------------------
option(COPY_PLUGIN "Copy VST3 to system VST3 folder after each build" OFF)

# ---------------------------------------------------------------------------
# Plug-in target
#
# Builds both a VST3 bundle and a standalone executable.
# PLUGIN_MANUFACTURER_CODE and PLUGIN_CODE must be unique four-character codes
# registered with Steinberg for commercial distribution.
# ---------------------------------------------------------------------------
juce_add_plugin(RingMod
    COMPANY_NAME "AudioDSP"
    PLUGIN_MANUFACTURER_CODE AudD               # Unique 4-char manufacturer code
    PLUGIN_CODE RngM                            # Unique 4-char plug-in code
    FORMATS VST3 Standalone                     # Output formats to generate
    PRODUCT_NAME "RingMod"
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN}      # Controlled by the COPY_PLUGIN option above
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.svg"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.svg"
)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
target_sources(RingMod PRIVATE
    source/PluginProcessor.h
    source/PluginProcessor.cpp
    source/PluginEditor.h
    source/PluginEditor.cpp
)

# Require C++17 for structured bindings, std::array, etc.
target_compile_features(RingMod PRIVATE cxx_std_17)

# ---------------------------------------------------------------------------
# Compile definitions
#
# JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
#   Suppresses a JUCE warning that fires when the numeric VST3 parameter IDs
#   do not match the APVTS string IDs; this is expected when using
#   AudioParameterChoice and string-keyed APVTS parameters together.
# ---------------------------------------------------------------------------
target_compile_definitions(RingMod PUBLIC
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# ---------------------------------------------------------------------------
# Link libraries
#
# juce_audio_utils   — AudioProcessorEditor base class, APVTS, etc.
# juce_dsp           — DryWetMixer, AudioBlock, FloatVectorOperations
# juce_gui_extra     — Additional GUI helpers
# juce_recommended_*_flags — Compiler warning/optimisation presets from JUCE
# ---------------------------------------------------------------------------
target_link_libraries(RingMod PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Generates the JuceHeader.h amalgamation that source files include.
juce_generate_juce_header(RingMod)
