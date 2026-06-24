cmake_minimum_required(VERSION 3.21)

project(RNBOSynthJuce VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#==============================================================================
# 1. JUCE (fetched automatically). Pin to a known-good tag.
#==============================================================================
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

#==============================================================================
# 2. RNBO C++ export.
#    Generate it from patch/SimpleSynth.rnbopat into this "export" folder,
#    with "Export RNBO C++ Library" enabled (see README.md).
#==============================================================================
set(RNBO_EXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/export")

# The exported patcher source. RNBO names it after the patch
# (e.g. SimpleSynth.rnbopat.cpp), so we glob the export root.
file(GLOB RNBO_PATCHER_SOURCE "${RNBO_EXPORT_DIR}/*.cpp")

# The RNBO C++ runtime, amalgamated into a single translation unit.
set(RNBO_LIBRARY_SOURCE "${RNBO_EXPORT_DIR}/rnbo/RNBO.cpp")

if (NOT RNBO_PATCHER_SOURCE OR NOT EXISTS "${RNBO_LIBRARY_SOURCE}")
    message(FATAL_ERROR
        "RNBO export incomplete in: ${RNBO_EXPORT_DIR}\n"
        "Expected an exported patcher .cpp at the export root and "
        "${RNBO_LIBRARY_SOURCE}.\n"
        "Open patch/SimpleSynth.rnbopat in RNBO, export the C++ source code "
        "into the 'export' folder (with 'Export RNBO C++ Library' enabled), "
        "then re-run CMake. See README.md.")
endif()

#==============================================================================
# 3. The standalone GUI application.
#==============================================================================
juce_add_gui_app(RNBOSynthJuce
    PRODUCT_NAME "RNBO Synth")

target_sources(RNBOSynthJuce PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    ${RNBO_PATCHER_SOURCE}
    ${RNBO_LIBRARY_SOURCE})

target_include_directories(RNBOSynthJuce PRIVATE
    ${RNBO_EXPORT_DIR}
    ${RNBO_EXPORT_DIR}/rnbo
    ${RNBO_EXPORT_DIR}/rnbo/common
    ${RNBO_EXPORT_DIR}/rnbo/src)

target_compile_definitions(RNBOSynthJuce PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:RNBOSynthJuce,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RNBOSynthJuce,JUCE_VERSION>")

target_link_libraries(RNBOSynthJuce PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# RNBO's runtime may use threads.
find_package(Threads REQUIRED)
target_link_libraries(RNBOSynthJuce PRIVATE Threads::Threads)
