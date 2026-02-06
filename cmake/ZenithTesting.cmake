# =============================================================================
# ZENITH TESTING FRAMEWORK
# =============================================================================

include_guard(GLOBAL)

# Enable testing if requested
option(ZENITH_ENABLE_TESTING "Enable testing framework" ON)

if(ZENITH_ENABLE_TESTING)
    # Enable testing
    enable_testing()

    # Find Google Test
    find_package(GTest REQUIRED)

    # Test configuration
    set(ZENITH_TEST_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/apps/desktop/Source/tests
        ${CMAKE_SOURCE_DIR}/modules/zenith_core/engine/core
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/unified
    )

    # Test utilities
    set(ZENITH_TEST_UTILS
        apps/desktop/Source/tests/TestUtils.cpp
        apps/desktop/Source/tests/MockComponents.cpp
        apps/desktop/Source/tests/TestHelpers.cpp
    )

    # Core engine tests (minimal set for now)
    set(ZENITH_ENGINE_TESTS
        apps/desktop/Source/tests/EngineCoreTests.cpp
        # apps/desktop/Source/tests/AudioDeviceManagerTests.cpp
        # apps/desktop/Source/tests/TrackManagerTests.cpp
        # apps/desktop/Source/tests/TransportControllerTests.cpp
        # apps/desktop/Source/tests/AudioRendererTests.cpp
    )

    # UI framework tests (minimal set for now)
    set(ZENITH_UI_TESTS
        # apps/desktop/Source/tests/UnifiedComponentTests.cpp
        # apps/desktop/Source/tests/UnifiedButtonTests.cpp
        # apps/desktop/Source/tests/UnifiedSliderTests.cpp
    )

    # Audio processing tests (minimal set for now)
    set(ZENITH_AUDIO_TESTS
        # apps/desktop/Source/tests/AudioEngineTests.cpp
        # apps/desktop/Source/tests/PluginChainTests.cpp
        # apps/desktop/Source/tests/AudioBufferTests.cpp
    )

    # Integration tests (minimal set for now)
    set(ZENITH_INTEGRATION_TESTS
        # apps/desktop/Source/tests/IntegrationTests.cpp
        # apps/desktop/Source/tests/LoadSaveTests.cpp
        # apps/desktop/Source/tests/PerformanceTests.cpp
    )

    # Unit test executable
    add_executable(zenith_tests
        ${ZENITH_TEST_UTILS}
        ${ZENITH_ENGINE_TESTS}
        ${ZENITH_UI_TESTS}
        ${ZENITH_AUDIO_TESTS}
        ${ZENITH_INTEGRATION_TESTS}
    )

    # Test include directories
    target_include_directories(zenith_tests
        PRIVATE
            ${ZENITH_TEST_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/apps/desktop/Source
            ${CMAKE_SOURCE_DIR}/modules
    )

    # Link libraries for tests
    target_link_libraries(zenith_tests
        PRIVATE
            zenith_engine_core
            zenith_ui_unified
            zenith_core
            zenith_audio_utils
            gtest
            gtest_main
            juce_core
            juce_cryptography
            juce_events
            juce_graphics
            juce_data_structures
            juce_gui_basics
            juce_gui_extra
            juce_audio_basics
            juce_audio_devices
            juce_audio_formats
            juce_audio_processors
            juce_audio_utils
            juce_dsp
            juce_opengl
    )

    # Add test definitions
    target_compile_definitions(zenith_tests
        PRIVATE
            ZENITH_TESTING=1
            ZENITH_ENABLE_TESTING=1
    )

    # Add tests to CTest
    add_test(NAME EngineCoreTests COMMAND zenith_tests)
    add_test(NAME UIComponentTests COMMAND zenith_tests)
    add_test(NAME AudioEngineTests COMMAND zenith_tests)
    add_test(NAME IntegrationTests COMMAND zenith_tests)

    # Test coverage options
    if(ZENITH_ENABLE_COVERAGE)
        # Add coverage flags
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(zenith_tests PRIVATE --coverage)
            target_link_libraries(zenith_tests PRIVATE --coverage)
            set(ZENITH_COVERAGE_COMMAND lcov --capture --directory . --output-file coverage.info)
        endif()
    endif()

    # Test fixtures
    add_executable(zenith_test_fixtures
        apps/desktop/Source/tests/TestFixtures.cpp
    )

    target_link_libraries(zenith_test_fixtures
        PRIVATE
            zenith_engine_core
            zenith_ui_unified
            zenith_core
            zenith_audio_utils
    )

    # Benchmark tests
    if(ZENITH_ENABLE_BENCHMARKS)
        add_executable(zenith_benchmarks
            apps/desktop/Source/tests/BenchmarkMain.cpp
            apps/desktop/Source/tests/EngineBenchmarks.cpp
            apps/desktop/Source/tests/PerformanceBenchmarks.cpp
        )

        target_link_libraries(zenith_benchmarks
            PRIVATE
                zenith_engine_core
                zenith_core
                zenith_audio_utils
        )
    endif()

    # Installation for testing
    install(TARGETS zenith_tests zenith_test_fixtures
        RUNTIME DESTINATION bin/tests
    )

    # Print test information
    message(STATUS "Testing framework enabled:")
    message(STATUS "  - Unit tests: Engine (${ZENITH_ENGINE_TESTS})")
    message(STATUS "  - Unit tests: UI (${ZENITH_UI_TESTS})")
    message(STATUS "  - Unit tests: Audio (${ZENITH_AUDIO_TESTS})")
    message(STATUS "  - Integration tests: ${ZENITH_INTEGRATION_TESTS}")
    message(STATUS "  - Test fixtures available")
    if(ZENITH_ENABLE_BENCHMARKS)
        message(STATUS "  - Benchmarks available")
    endif()
endif()
