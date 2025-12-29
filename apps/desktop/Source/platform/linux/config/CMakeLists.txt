# Platform Linux config sources
file(GLOB PLATFORM_LINUX_CONFIG_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_CONFIG_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_CONFIG_SOURCES})
endif()
