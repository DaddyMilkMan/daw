# =============================================================================
# ZENITH AUDIO FORMATS MODULE
# =============================================================================

# Audio formats sources
set(ZENITH_AUDIO_FORMATS_SOURCES
    modules/zenith_core/engine/AudioExporter.cpp
    modules/zenith_core/engine/AudioExporter.h
    modules/zenith_core/engine/AudioFilePool.cpp
    modules/zenith_core/engine/AudioFilePool.h
)

# Create audio formats library
add_library(zenith_audio_formats STATIC
    ${ZENITH_AUDIO_FORMATS_SOURCES}
)

# Target properties
target_include_directories(zenith_audio_formats
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/engine>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_audio_formats
    PUBLIC
        cxx_std_17
)

target_link_libraries(zenith_audio_formats
    PUBLIC
        zenith_audio_utils
        juce_audio_formats
        juce_core
    PRIVATE
        zenith_dsp
)

# Installation
install(
    TARGETS zenith_audio_formats
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)