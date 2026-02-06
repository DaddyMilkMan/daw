# ZenithUltraSynth CMake Configuration
# Superior Synth Engine Implementation

# Component dependencies
target_include_directories(ZenithUltraSynth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/physical_modeling
    ${CMAKE_CURRENT_SOURCE_DIR}/neural_synthesis
    ${CMAKE_CURRENT_SOURCE_DIR}/wavetable
    ${CMAKE_CURRENT_SOURCE_DIR}/workflow
)

# Source files
set(ZenithUltraSynth_SOURCES
    ZenithUltraSynth.cpp
    ZenithUltraSynthVoice.cpp
)

# Physical modeling engine
set(PhysicalModeling_SOURCES
    physical_modeling/StringModel.cpp
    physical_modeling/StringModelVoice.cpp
    physical_modeling/WindModel.cpp
    physical_modeling/WindModelVoice.cpp
    physical_modeling/PercussionModel.cpp
    physical_modeling/PercussionModelVoice.cpp
)

# Neural synthesis engine
set(NeuralSynthesis_SOURCES
    neural_synthesis/NeuralSynthEngine.cpp
    neural_synthesis/TimbreTransfer.cpp
    neural_synthesis/SoundDesignAssistant.cpp
)

# Advanced wavetable engine
set(AdvancedWavetable_SOURCES
    wavetable/AdvancedWavetableEngine.cpp
    wavetable/WavetableEditor.cpp
    wavetable/WavetableMorpher.cpp
    wavetable/WavetableLibrary.cpp
)

# Workflow innovation
set(WorkflowInnovation_SOURCES
    workflow/VisualSynthesisCanvas.cpp
    workflow/MacroControlSystem.cpp
    workflow/PresetMorpher.cpp
    workflow/RandomizationEngine.cpp
)

# All sources
set(ZenithUltraSynth_All_SOURCES
    ${ZenithUltraSynth_SOURCES}
    ${PhysicalModeling_SOURCES}
    ${NeuralSynthesis_SOURCES}
    ${AdvancedWavetable_SOURCES}
    ${WorkflowInnovation_SOURCES}
)

# Create library
add_library(ZenithUltraSynth STATIC
    ${ZenithUltraSynth_All_SOURCES}
)

# Link dependencies
target_link_libraries(ZenithUltraSynth
    PUBLIC
        ZenithCore
        ZenithDSP
        ZenithAudioUtils
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# Neural synthesis dependencies (conditional)
if(ONNX_RUNTIME_FOUND)
    target_link_libraries(ZenithUltraSynth PRIVATE
        onnxruntime
    )
    target_compile_definitions(ZenithUltraSynth PRIVATE ONNX_RUNTIME_AVAILABLE)
endif()

# FFTW dependency (optional)
if(FFTW_FOUND)
    target_link_libraries(ZenithUltraSynth PRIVATE
        fftw3f
    )
    target_compile_definitions(ZenithUltraSynth PRIVATE FFTW_AVAILABLE)
endif()

# Skia dependency (optional, for UI components)
if(SKIA_FOUND)
    target_link_libraries(ZenithUltraSynth PRIVATE
        skia
    )
    target_compile_definitions(ZenithUltraSynth PRIVATE SKIA_AVAILABLE)
endif()

# Compiler definitions
target_compile_definitions(ZenithUltraSynth PRIVATE
    ZENITH_ULTRASYNTH_VERSION=1.0.0
    ZENITH_ULTRASYNTH_MAX_VOICES=256
    ZENITH_ULTRASYNTH_MAX_UNISON=16
)

# Optimization flags
target_compile_options(ZenithUltraSynth PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0>
)

# Platform-specific settings
if(APPLE)
    target_compile_definitions(ZenithUltraSynth PRIVATE APPLE=1)
elseif(WIN32)
    target_compile_definitions(ZenithUltraSynth PRIVATE WINDOWS=1)
elseif(UNIX)
    target_compile_definitions(ZenithUltraSynth PRIVATE LINUX=1)
endif()

# Installation
install(TARGETS ZenithUltraSynth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ZenithUltraSynth.h
    ZenithUltraSynthVoice.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Zenith
)

install(DIRECTORY
    physical_modeling/
    neural_synthesis/
    wavetable/
    workflow/
    presets/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Zenith/ZenithUltraSynth
)

# Testing
option(BUILD_ULTRASYNTH_TESTS "Build ZenithUltraSynth tests" OFF)

if(BUILD_ULTRASYNTH_TESTS)
    enable_testing()

    # Add test executables
    add_subdirectory(tests)

    # Test data
    install(DIRECTORY
        test_data/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/ZenithUltraSynth/tests
    )
endif()

# Examples
option(BUILD_ULTRASYNTH_EXAMPLES "Build ZenithUltraSynth examples" OFF)

if(BUILD_ULTRASYNTH_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
option(BUILD_ULTRASYNTH_DOCS "Build ZenithUltraSynth documentation" OFF)

if(BUILD_ULTRASYNTH_DOCS)
    add_subdirectory(docs)
endif()

# CMake package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ZenithUltraSynthConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT ZenithUltraSynthTargets
    FILE ZenithUltraSynthTargets.cmake
    NAMESPACE Zenith::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZenithUltraSynth
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ZenithUltraSynthConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZenithUltraSynth
)