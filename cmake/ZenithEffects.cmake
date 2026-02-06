# =============================================================================
# ZENITH EFFECTS MODULE
# =============================================================================

include_guard(GLOBAL)

# Effects sources
set(ZENITH_EFFECTS_SOURCES
    modules/zenith_core/effects/ConsoleEmulation.cpp
    modules/zenith_core/effects/ZenithAutoTune.cpp
    modules/zenith_core/effects/ZenithChannelStrip.cpp
    modules/zenith_core/effects/ZenithDeEsser.cpp
    modules/zenith_core/effects/ZenithTransientShaper.cpp
    modules/zenith_core/effects/ZenithVoiceChanger.cpp
)

# Create effects library
add_library(zenith_effects STATIC
    ${ZENITH_EFFECTS_SOURCES}
)

# Target properties
target_include_directories(zenith_effects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_core/effects>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_effects
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_effects
    PUBLIC
        zenith_dsp
        juce_audio_basics
        juce_dsp
)

# Installation
install(
    TARGETS zenith_effects
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
