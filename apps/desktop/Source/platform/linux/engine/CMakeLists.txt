# Platform Linux engine sources
file(GLOB PLATFORM_LINUX_ENGINE_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_ENGINE_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_ENGINE_SOURCES})
endif()
