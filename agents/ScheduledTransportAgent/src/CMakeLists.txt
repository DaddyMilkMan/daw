# ScheduledTransportAgent module

# Create a library for the agent
add_library(ScheduledTransportAgent
    scheduled_transport.cpp
)

# Link with JUCE core components
target_link_libraries(ScheduledTransportAgent PUBLIC
    juce::juce_core
    juce::juce_audio_basics
)

# Include directories
target_include_directories(ScheduledTransportAgent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# C++20 standard required
target_compile_features(ScheduledTransportAgent PUBLIC cxx_std_20)

# Compiler warnings
if(MSVC)
    target_compile_options(ScheduledTransportAgent PRIVATE /W4)
else()
    target_compile_options(ScheduledTransportAgent PRIVATE -Wall -Wextra -Wpedantic)
endif()
