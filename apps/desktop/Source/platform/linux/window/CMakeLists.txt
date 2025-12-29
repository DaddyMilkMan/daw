# Platform Linux window sources
file(GLOB PLATFORM_LINUX_WINDOW_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_WINDOW_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_WINDOW_SOURCES})
endif()
