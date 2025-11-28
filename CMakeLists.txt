cmake_minimum_required(VERSION 3.22)

project(Zenith VERSION 0.1.0 LANGUAGES CXX)

# ============================================================================
# Zenith DAW - Canonical JUCE-based DAW
# ============================================================================
#
# All previous Qt/QML prototypes and donor code have been removed.
# The canonical DAW implementation lives in ./zenith-core/
#
# To build:
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#
# ============================================================================

add_subdirectory(modules/zenith-core)
