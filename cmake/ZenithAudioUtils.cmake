# =============================================================================
# ZENITH AUDIO UTILS MODULE
# =============================================================================

# Audio utilities sources
set(ZENITH_AUDIO_UTILS_SOURCES
    modules/zenith_core/audio/HardwareAudioInterface.h
    modules/zenith_core/audio/RealTimeAudioBuffer.cpp
    modules/zenith_core/audio/RealTimeAudioBuffer.h
    modules/zenith_core/analysis/AdvancedAudioAnalyzer.cpp
    modules/zenith_core/analysis/AdvancedAudioAnalyzer.h
)

# Create audio utils library
add_library(zenith_audio_utils STATIC
    ${ZENITH_AUDIO_UTILS_SOURCES}
)

# Target properties
target_include_directories(zenith_audio_utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/audio>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/analysis>
)

target_compile_features(zenith_audio_utils
    PUBLIC
        cxx_std_17
)

target_link_libraries(zenith_audio_utils
    PUBLIC
        zenith_dsp
        juce_audio_basics
        juce_audio_devices
    PRIVATE
        juce_core
        juce_data_structures
)

# Installation
install(
    TARGETS zenith_audio_utils
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)