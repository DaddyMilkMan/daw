# Platform Linux system sources
file(GLOB PLATFORM_LINUX_SYSTEM_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_SYSTEM_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_SYSTEM_SOURCES})
endif()
