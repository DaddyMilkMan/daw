# Test Suite for Grok AI Systems

# Test executable
add_executable(GrokAITests
    GrokAITestSuite.cpp
)

# Link with main application
target_link_libraries(GrokAITests PRIVATE
    ZenithDAW
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(GrokAITests PRIVATE
    ${CMAKE_BINARY_DIR}/ZenithDAW_artefacts/JuceLibraryCode
    apps/desktop/Source
    apps/desktop/Source/ai
)

# Test-specific definitions
target_compile_definitions(GrokAITests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    ZENITH_TEST_MODE=1
)

# Add test to CTest
add_test(NAME GrokAIUnitTests COMMAND GrokAITests)

# Set test properties
set_tests_properties(GrokAIUnitTests PROPERTIES
    TIMEOUT 300  # 5 minutes timeout
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target for running all AI tests
add_custom_target(run_ai_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R GrokAI
    DEPENDS GrokAITests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Grok AI Test Suite"
)

# Performance benchmark target
add_custom_target(ai_performance_tests
    COMMAND GrokAITests --performance
    DEPENDS GrokAITests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running AI Performance Benchmarks"
)
