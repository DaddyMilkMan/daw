# ============================================================================
# Audio Driver Detection and Configuration
# ============================================================================
# This module detects and configures platform-specific audio drivers:
# - Windows: ASIO, WASAPI, DirectSound
# - macOS: CoreAudio
# - Linux: ALSA, JACK, PulseAudio

include(CheckIncludeFile)
include(CheckLibraryExists)

# ============================================================================
# Platform Detection
# ============================================================================

if(WIN32)
    set(VEXEL_PLATFORM_WINDOWS TRUE)
    set(VEXEL_PLATFORM_NAME "Windows")
elseif(APPLE)
    set(VEXEL_PLATFORM_MACOS TRUE)
    set(VEXEL_PLATFORM_NAME "macOS")
elseif(UNIX)
    set(VEXEL_PLATFORM_LINUX TRUE)
    set(VEXEL_PLATFORM_NAME "Linux")
endif()

# ============================================================================
# Windows Audio Drivers
# ============================================================================

if(VEXEL_PLATFORM_WINDOWS)
    message(STATUS "Configuring Windows audio drivers...")

    # ASIO SDK (user must provide - Steinberg license restriction)
    set(ASIO_SDK_DIR "" CACHE PATH "Path to ASIO SDK directory")

    if(EXISTS "${ASIO_SDK_DIR}/common/asio.h")
        set(VEXEL_AUDIO_ASIO TRUE)
        message(STATUS "  ✓ ASIO support enabled")
        add_library(asio_sdk INTERFACE)
        target_include_directories(asio_sdk INTERFACE "${ASIO_SDK_DIR}/common")
        target_compile_definitions(asio_sdk INTERFACE VEXEL_AUDIO_ASIO=1)
    else()
        message(STATUS "  ✗ ASIO SDK not found (set ASIO_SDK_DIR)")
    endif()

    # WASAPI - built into Windows
    set(VEXEL_AUDIO_WASAPI TRUE)
    message(STATUS "  ✓ WASAPI support enabled")

    # DirectSound - legacy fallback
    set(VEXEL_AUDIO_DIRECTSOUND TRUE)
    message(STATUS "  ✓ DirectSound support enabled")

endif()

# ============================================================================
# macOS Audio Drivers
# ============================================================================

if(VEXEL_PLATFORM_MACOS)
    message(STATUS "Configuring macOS audio drivers...")

    # CoreAudio - native macOS audio
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)

    if(COREAUDIO_FRAMEWORK)
        set(VEXEL_AUDIO_COREAUDIO TRUE)
        message(STATUS "  ✓ CoreAudio support enabled")

        add_library(coreaudio_driver INTERFACE)
        target_link_libraries(coreaudio_driver INTERFACE
            ${COREAUDIO_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
            ${AUDIOUNIT_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
        )
        target_compile_definitions(coreaudio_driver INTERFACE VEXEL_AUDIO_COREAUDIO=1)
    endif()

endif()

# ============================================================================
# Linux Audio Drivers
# ============================================================================

if(VEXEL_PLATFORM_LINUX)
    message(STATUS "Configuring Linux audio drivers...")

    # ALSA - Advanced Linux Sound Architecture
    find_package(ALSA)
    if(ALSA_FOUND)
        set(VEXEL_AUDIO_ALSA TRUE)
        message(STATUS "  ✓ ALSA support enabled")
        add_library(alsa_driver INTERFACE)
        target_link_libraries(alsa_driver INTERFACE ALSA::ALSA)
        target_compile_definitions(alsa_driver INTERFACE VEXEL_AUDIO_ALSA=1)
    else()
        message(STATUS "  ✗ ALSA not found (install libasound2-dev)")
    endif()

    # JACK Audio Connection Kit
    find_library(JACK_LIBRARY jack)
    find_path(JACK_INCLUDE_DIR jack/jack.h)

    if(JACK_LIBRARY AND JACK_INCLUDE_DIR)
        set(VEXEL_AUDIO_JACK TRUE)
        message(STATUS "  ✓ JACK support enabled")
        add_library(jack_driver INTERFACE)
        target_include_directories(jack_driver INTERFACE ${JACK_INCLUDE_DIR})
        target_link_libraries(jack_driver INTERFACE ${JACK_LIBRARY})
        target_compile_definitions(jack_driver INTERFACE VEXEL_AUDIO_JACK=1)
    else()
        message(STATUS "  ✗ JACK not found (install libjack-dev)")
    endif()

    # PulseAudio
    find_library(PULSE_LIBRARY pulse)
    find_path(PULSE_INCLUDE_DIR pulse/pulseaudio.h)

    if(PULSE_LIBRARY AND PULSE_INCLUDE_DIR)
        set(VEXEL_AUDIO_PULSEAUDIO TRUE)
        message(STATUS "  ✓ PulseAudio support enabled")
        add_library(pulse_driver INTERFACE)
        target_include_directories(pulse_driver INTERFACE ${PULSE_INCLUDE_DIR})
        target_link_libraries(pulse_driver INTERFACE ${PULSE_LIBRARY})
        target_compile_definitions(pulse_driver INTERFACE VEXEL_AUDIO_PULSEAUDIO=1)
    else()
        message(STATUS "  ✗ PulseAudio not found (install libpulse-dev)")
    endif()

endif()

# ============================================================================
# Summary Function
# ============================================================================

function(vexel_print_audio_config)
    message(STATUS "")
    message(STATUS "═══════════════════════════════════════")
    message(STATUS "Audio Driver Configuration Summary")
    message(STATUS "═══════════════════════════════════════")
    message(STATUS "Platform: ${VEXEL_PLATFORM_NAME}")

    if(VEXEL_AUDIO_ASIO)
        message(STATUS "  • ASIO")
    endif()
    if(VEXEL_AUDIO_WASAPI)
        message(STATUS "  • WASAPI")
    endif()
    if(VEXEL_AUDIO_DIRECTSOUND)
        message(STATUS "  • DirectSound")
    endif()
    if(VEXEL_AUDIO_COREAUDIO)
        message(STATUS "  • CoreAudio")
    endif()
    if(VEXEL_AUDIO_ALSA)
        message(STATUS "  • ALSA")
    endif()
    if(VEXEL_AUDIO_JACK)
        message(STATUS "  • JACK")
    endif()
    if(VEXEL_AUDIO_PULSEAUDIO)
        message(STATUS "  • PulseAudio")
    endif()

    message(STATUS "═══════════════════════════════════════")
    message(STATUS "")
endfunction()
