# Platform Linux network sources
include(FindPkgConfig)
pkg_check_modules(LIBSECRET libsecret-1)

file(GLOB PLATFORM_LINUX_NETWORK_SOURCES "*.cpp" "*.h")
if(PLATFORM_LINUX_NETWORK_SOURCES)
    target_sources(ZenithDAW PRIVATE ${PLATFORM_LINUX_NETWORK_SOURCES})
endif()

if(LIBSECRET_FOUND)
    target_include_directories(ZenithDAW PRIVATE ${LIBSECRET_INCLUDE_DIRS})
    target_link_libraries(ZenithDAW PRIVATE ${LIBSECRET_LIBRARIES})
    target_compile_definitions(ZenithDAW PRIVATE HAVE_LIBSECRET=1)
    message(STATUS "libsecret found - secure credential storage enabled")
else()
    message(STATUS "libsecret not found - using fallback encryption")
endif()
