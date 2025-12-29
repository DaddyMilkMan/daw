# Platform-specific implementations
# Detect OS and include appropriate platform sources

if(UNIX AND NOT APPLE)
    # Linux
    add_subdirectory(linux/config)
    add_subdirectory(linux/dsp)
    add_subdirectory(linux/engine)
    add_subdirectory(linux/fonts)
    add_subdirectory(linux/network)
    add_subdirectory(linux/system)
    add_subdirectory(linux/ui)
    add_subdirectory(linux/window)
elseif(APPLE)
    # macOS
    add_subdirectory(mac/config)
    add_subdirectory(mac/dsp)
    add_subdirectory(mac/engine)
    add_subdirectory(mac/fonts)
    add_subdirectory(mac/network)
    add_subdirectory(mac/system)
    add_subdirectory(mac/ui)
    add_subdirectory(mac/window)
elseif(WIN32)
    # Windows
    add_subdirectory(windows/config)
    add_subdirectory(windows/dsp)
    add_subdirectory(windows/engine)
    add_subdirectory(windows/fonts)
    add_subdirectory(windows/network)
    add_subdirectory(windows/system)
    add_subdirectory(windows/ui)
    add_subdirectory(windows/window)
endif()
