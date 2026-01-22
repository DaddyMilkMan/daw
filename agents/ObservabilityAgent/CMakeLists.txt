
# ObservabilityAgent Tests - Isolated Backend Test Target
add_executable(ObservabilityAgentTests
    tests/TestMain.cpp
    tests/ObservabilityAgentTest.cpp
    ObservabilityAgent.cpp
    ObservabilityAgent.h
)

# Link ONLY backend dependencies
target_link_libraries(ObservabilityAgentTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
)

# Enable C++20
target_compile_features(ObservabilityAgentTests PRIVATE cxx_std_20)

target_compile_definitions(ObservabilityAgentTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    ZENITH_TEST_MODE=1
)

# Register with CTest
add_test(NAME ObservabilityAgentTests COMMAND ObservabilityAgentTests)
