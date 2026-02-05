# =============================================================================
# ZENITH UI UNIFIED MODULE
# =============================================================================

# Unified UI framework sources
set(ZENITH_UI_UNIFIED_SOURCES
    modules/zenith_ui/ui/unified/UnifiedComponent.cpp
    modules/zenith_ui/ui/unified/controls/UnifiedButton.cpp
    modules/zenith_ui/ui/unified/controls/UnifiedSlider.cpp
)

# UI framework headers
set(ZENITH_UI_UNIFIED_HEADERS
    modules/zenith_ui/ui/unified/IUnifiedComponent.h
    modules/zenith_ui/ui/unified/UnifiedComponent.h
    modules/zenith_ui/ui/unified/controls/UnifiedButton.h
    modules/zenith_ui/ui/unified/controls/UnifiedSlider.h
)

# Create UI unified library
add_library(zenith_ui_unified STATIC
    ${ZENITH_UI_UNIFIED_SOURCES}
    ${ZENITH_UI_UNIFIED_HEADERS}
)

# Target properties
target_include_directories(zenith_ui_unified
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/unified>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/framework
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/design-system
)

target_compile_features(zenith_ui_unified
    PUBLIC
        cxx_std_17
)

target_link_libraries(zenith_ui_unified
    PUBLIC
        zenith_ui_framework
        zenith_core
        zenith_audio_utils
    PRIVATE
        skia
        ${ZENITH_JUCE_MODULES}
)

# Add Skia rendering support
if(ZENITH_ENABLE_SKIA)
    target_compile_definitions(zenith_ui_unified PRIVATE ZENITH_ENABLE_SKIA=1)
    target_link_libraries(zenith_ui_unified PRIVATE skia)
endif()

# Installation
install(
    TARGETS zenith_ui_unified
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    FILES ${ZENITH_UI_UNIFIED_HEADERS}
    DESTINATION include/zenith/ui/unified
)