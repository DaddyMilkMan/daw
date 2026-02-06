# =============================================================================
# ZENITH AUDIO UTILS MODULE
# =============================================================================

include_guard(GLOBAL)

# Audio utilities sources
set(ZENITH_AUDIO_UTILS_SOURCES
    modules/zenith_core/audio/HardwareAudioInterface.h
    modules/zenith_core/audio/RealTimeAudioBuffer.cpp
    modules/zenith_core/utils/AudioAnalysisUtils.cpp
    modules/zenith_core/utils/PlatformLogUtils.cpp
    modules/zenith_core/utils/PlatformSystemUtils.cpp
    modules/zenith_core/utils/PowerManagement.cpp
    modules/zenith_core/utils/SampleGenerator.cpp
    modules/zenith_core/utils/StemSeparationJob.cpp
)

# Linux-only system deps (GTK/WebKit). Other platforms do not use pkg-config.
set(ZENITH_AUDIO_UTILS_PLATFORM_LIBS "")
if(ZENITH_PLATFORM_LINUX)
    # Disabled GTK/WebKit as they cause build issues on this system
    # find_package(PkgConfig REQUIRED)
    # pkg_check_modules(GTK QUIET IMPORTED_TARGET gtk+-x11-3.0)
    # pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
endif()

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
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/apps/desktop/Source>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        ${GTK_INCLUDE_DIRS}
)

target_compile_features(zenith_audio_utils
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_audio_utils
    PUBLIC
        zenith_dsp
        juce_audio_basics
        juce_audio_devices
        juce_audio_processors
        juce_audio_utils
        juce_gui_basics
        juce_graphics
        ${ZENITH_AUDIO_UTILS_PLATFORM_LIBS}
    PRIVATE
        juce_core
        juce_data_structures
        juce_events
)

# Installation
install(
    TARGETS zenith_audio_utils
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
