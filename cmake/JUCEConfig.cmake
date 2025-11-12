# ============================================================================
# JUCE Framework Configuration
# ============================================================================
# Centralized JUCE setup to avoid duplication across subprojects

include(FetchContent)

# ============================================================================
# Fetch JUCE (only once)
# ============================================================================

if(NOT TARGET juce::juce_core)
    message(STATUS "Fetching JUCE 8.0.9...")

    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE
        GIT_PROGRESS FALSE  # Reduce CMake output noise
        SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/juce-src
    )

    # Fetch but don't add to build tree yet
    FetchContent_GetProperties(JUCE)
    if(NOT juce_POPULATED)
        FetchContent_Populate(JUCE)
        add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    message(STATUS "JUCE configured successfully")
endif()

# ============================================================================
# JUCE Module Bundles
# ============================================================================
# Predefined module groups for different project types

# Audio Engine Modules
set(JUCE_AUDIO_ENGINE_MODULES
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

# GUI Modules
set(JUCE_GUI_MODULES
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# DSP Modules
set(JUCE_DSP_MODULES
    juce::juce_dsp
)

# Complete DAW Application
set(JUCE_DAW_MODULES
    ${JUCE_AUDIO_ENGINE_MODULES}
    ${JUCE_GUI_MODULES}
    ${JUCE_DSP_MODULES}
)

# ============================================================================
# Helper Function: Add JUCE Audio Engine Library
# ============================================================================

function(vexel_add_audio_engine_library target)
    target_link_libraries(${target} PRIVATE
        ${JUCE_AUDIO_ENGINE_MODULES}
        ${JUCE_DSP_MODULES}
    )

    target_compile_definitions(${target} PRIVATE
        JUCE_USE_OGGVORBIS=1
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=$<BOOL:${APPLE}>
    )
endfunction()

# ============================================================================
# Helper Function: Add JUCE GUI Application
# ============================================================================

function(vexel_add_gui_app target)
    juce_add_gui_app(${target}
        PRODUCT_NAME ${target}
        COMPANY_NAME "VexelAudio"
        BUNDLE_ID "com.vexelaudio.${target}"
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "Vexel DAW needs microphone access for recording."
    )

    target_link_libraries(${target} PRIVATE
        ${JUCE_DAW_MODULES}
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    juce_generate_juce_header(${target})
endfunction()
