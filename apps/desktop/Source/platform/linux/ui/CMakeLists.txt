# Platform Linux ui sources
file(GLOB PLATFORM_LINUX_UI_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_UI_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_UI_SOURCES})
endif()
