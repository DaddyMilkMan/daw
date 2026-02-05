# =============================================================================
# ZENITH DSP MODULE
# =============================================================================

# DSP sources
set(ZENITH_DSP_SOURCES
    modules/zenith_core/dsp/AudioFifo.h
    modules/zenith_core/dsp/ClassicAutoTune.cpp
    modules/zenith_core/dsp/ClassicAutoTune.h
    modules/zenith_core/dsp/DSPStemSeparator.cpp
    modules/zenith_core/dsp/DSPStemSeparator.h
    modules/zenith_core/dsp/DSPVoiceChanger.cpp
    modules/zenith_core/dsp/DSPVoiceChanger.h
    modules/zenith_core/dsp/Dither.h
    modules/zenith_core/dsp/EnvelopeFollower.h
    modules/zenith_core/dsp/GlobalLFO.h
    modules/zenith_core/dsp/HarmonyGenerator.h
    modules/zenith_core/dsp/MasterLimiter.h
    modules/zenith_core/dsp/MidiPitchController.cpp
    modules/zenith_core/dsp/MidiPitchController.h
    modules/zenith_core/dsp/ONNXStemSeparator.cpp
    modules/zenith_core/dsp/ONNXStemSeparator.h
    modules/zenith_core/dsp/Oversampler.h
    modules/zenith_core/dsp/PitchCorrector.cpp
    modules/zenith_core/dsp/PitchCorrector.h
    modules/zenith_core/dsp/PitchDetector.cpp
    modules/zenith_core/dsp/PitchDetector.h
    modules/zenith_core/dsp/PlatformModelUtils.cpp
    modules/zenith_core/dsp/PlatformModelUtils.h
    modules/zenith_core/dsp/PlatformONNXUtils.h
    modules/zenith_core/dsp/ProPitchShifter.cpp
    modules/zenith_core/dsp/ProPitchShifter.h
    modules/zenith_core/dsp/SIMDHelpers.h
    modules/zenith_core/dsp/ScaleAutoDetector.h
    modules/zenith_core/dsp/SpectralProcessor.cpp
    modules/zenith_core/dsp/SpectralProcessor.h
    modules/zenith_core/dsp/StereoAudioFifo.h
    modules/zenith_core/dsp/ThroatModel.cpp
    modules/zenith_core/dsp/ThroatModel.h
    modules/zenith_core/dsp/TimeStretcher.cpp
    modules/zenith_core/dsp/TimeStretcher.h
)

# Create DSP library
add_library(zenith_dsp STATIC
    ${ZENITH_DSP_SOURCES}
)

# Target properties
target_include_directories(zenith_dsp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/dsp>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_dsp
    PUBLIC
        cxx_std_17
)

target_link_libraries(zenith_dsp
    PUBLIC
        juce_dsp
        juce_audio_basics
    PRIVATE
        zenith_audio_utils
        juce_core
)

# Installation
install(
    TARGETS zenith_dsp
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)