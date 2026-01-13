cmake_minimum_required(VERSION 3.25)

project(TransportProtocolAgent
    VERSION 0.1.0
    DESCRIPTION "Transport Protocol Agent for Zenith DAW"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    transport_protocol.cpp
)

set(HEADERS
    transport_protocol.h
)

# Create library
add_library(transport_protocol_agent STATIC ${SOURCES})

# Include directories
target_include_directories(transport_protocol_agent
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler warnings
if(MSVC)
    target_compile_options(transport_protocol_agent PRIVATE /W4)
else()
    target_compile_options(transport_protocol_agent PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test executable
add_executable(test_transport_protocol
    ../tests/test_transport_protocol.cpp
)

# Link test with agent library
target_link_libraries(test_transport_protocol PRIVATE transport_protocol_agent)

# Installation (optional)
install(TARGETS transport_protocol_agent
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
