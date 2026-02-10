# =============================================================================
# ZENITH ENGINE CORE MODULE
# =============================================================================

include_guard(GLOBAL)

# Engine core sources - modular architecture
set(ZENITH_ENGINE_CORE_SOURCES
    modules/zenith_core/engine/core/EngineCore.cpp
    modules/zenith_core/engine/core/AudioDeviceManager.cpp
    modules/zenith_core/engine/core/TrackManager.cpp
    modules/zenith_core/engine/core/TransportController.cpp
    modules/zenith_core/engine/core/AudioRenderer.cpp
)

# Interface definitions
set(ZENITH_ENGINE_CORE_HEADERS
    modules/zenith_core/engine/core/IAudioEngine.h
    modules/zenith_core/engine/core/IAudioDeviceManager.h
    modules/zenith_core/engine/core/ITrackManager.h
    modules/zenith_core/engine/core/ITransportController.h
    modules/zenith_core/engine/core/IAudioRenderer.h
    modules/zenith_core/engine/core/EngineCore.h
)

# Core headers
set(ZENITH_ENGINE_CORE_PUBLIC_HEADERS
    modules/zenith_core/engine/core/AudioDeviceManager.h
    modules/zenith_core/engine/core/TrackManager.h
    modules/zenith_core/engine/core/TransportController.h
    modules/zenith_core/engine/core/AudioRenderer.h
)

# Create engine core library
add_library(zenith_engine_core STATIC
    ${ZENITH_ENGINE_CORE_SOURCES}
    ${ZENITH_ENGINE_CORE_PUBLIC_HEADERS}
)

# Target properties
target_include_directories(zenith_engine_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/engine/core>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/modules/zenith_core/engine
)

target_compile_features(zenith_engine_core
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_engine_core
    PUBLIC
        zenith_core
        zenith_audio_utils
        zenith_dsp
    PRIVATE
        zenith_ai_client
        juce_audio_basics
        juce_audio_devices
        juce_audio_formats
        juce_audio_processors
        juce_core
        juce_events
        juce_data_structures
)

# Installation
install(
    TARGETS zenith_engine_core
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    FILES ${ZENITH_ENGINE_CORE_PUBLIC_HEADERS}
    DESTINATION include/zenith/engine/core
)
