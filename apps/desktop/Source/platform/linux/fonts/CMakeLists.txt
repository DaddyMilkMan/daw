# Platform Linux fonts sources
file(GLOB PLATFORM_LINUX_FONTS_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_FONTS_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_FONTS_SOURCES})
endif()
