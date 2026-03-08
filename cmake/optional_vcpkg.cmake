# Optional vcpkg toolchain loader.
# If C:/zenith/daw/external/vcpkg/scripts/buildsystems/vcpkg.cmake exists, use it.
# Otherwise, do nothing and allow local CMake to continue without vcpkg.

set(_VCPKG_TOOLCHAIN "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(EXISTS "${_VCPKG_TOOLCHAIN}")
  message(STATUS "Found vcpkg toolchain at ${_VCPKG_TOOLCHAIN}. Setting CMAKE_TOOLCHAIN_FILE.")
  set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_TOOLCHAIN}" CACHE STRING "Vcpkg toolchain file" FORCE)
else()
  message(WARNING "vcpkg toolchain not found at ${_VCPKG_TOOLCHAIN}. Continuing without vcpkg.")
endif()
