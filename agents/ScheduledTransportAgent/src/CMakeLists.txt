# ScheduledTransportAgent module

# Create an executable for the agent scaffold
add_executable(ScheduledTransportAgent
    scheduled_transport.cpp
)

# C++20 standard required
target_compile_features(ScheduledTransportAgent PUBLIC cxx_std_20)

# Compiler warnings
if(MSVC)
    target_compile_options(ScheduledTransportAgent PRIVATE /W4)
else()
    target_compile_options(ScheduledTransportAgent PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Note: This is a minimal scaffold. Future versions will be structured as a library
# with separate executable targets for testing and demos.
