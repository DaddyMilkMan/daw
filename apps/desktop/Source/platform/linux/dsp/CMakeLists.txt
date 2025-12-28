# Platform Linux dsp sources
file(GLOB PLATFORM_LINUX_DSP_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_DSP_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_DSP_SOURCES})
endif()
