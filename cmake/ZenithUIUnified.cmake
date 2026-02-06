# =============================================================================
# ZENITH UI UNIFIED MODULE
# =============================================================================

include_guard(GLOBAL)

# Unified UI framework sources
set(ZENITH_UI_UNIFIED_SOURCES
    modules/zenith_ui/ui/unified/UnifiedComponent.cpp
    modules/zenith_ui/ui/unified/controls/UnifiedButton.cpp
    modules/zenith_ui/ui/unified/controls/UnifiedSlider.cpp
    
    # Framework and Design System
    modules/zenith_ui/ui/framework/AnimationCoordinator.cpp
    modules/zenith_ui/ui/framework/AuroraBackground.cpp
    modules/zenith_ui/ui/framework/ComponentLifecycleManager.cpp
    modules/zenith_ui/ui/framework/ConfigurationManager.cpp
    modules/zenith_ui/ui/framework/LayoutManager.cpp
    modules/zenith_ui/ui/framework/PlatformDisplayUtils.cpp
    modules/zenith_ui/ui/framework/PlatformPathUtils.cpp
    modules/zenith_ui/ui/framework/PlatformWindowUtils.cpp
    modules/zenith_ui/ui/framework/SkiaComponent.cpp
    modules/zenith_ui/ui/framework/SkiaD3D12Context.cpp
    modules/zenith_ui/ui/framework/SkiaLayout.cpp
    modules/zenith_ui/ui/framework/SkiaMainWindowIntegration.cpp
    modules/zenith_ui/ui/framework/UIErrorHandler.cpp
    
    modules/zenith_ui/ui/design-system/FontManager.cpp
    modules/zenith_ui/ui/design-system/MeterRenderer.cpp
    modules/zenith_ui/ui/design-system/PlatformFontUtils.cpp
    modules/zenith_ui/ui/design-system/SvgIcon.cpp
    modules/zenith_ui/ui/design-system/ThemeManager.cpp
    modules/zenith_ui/ui/design-system/ZenithDesignSystem.cpp
    modules/zenith_ui/ui/design-system/ZenithLayout.cpp
    modules/zenith_ui/ui/design-system/ZenithTheme.cpp
    
    # Controls
    modules/zenith_ui/ui/controls/ContextMenuManager.cpp
    modules/zenith_ui/ui/controls/DebugConsoleComponent.cpp
    modules/zenith_ui/ui/controls/ExportProgressBar.cpp
    modules/zenith_ui/ui/controls/FreezeProgressOverlay.cpp
    modules/zenith_ui/ui/controls/MarkdownComponent.cpp
    modules/zenith_ui/ui/controls/SkiaAlertWindow.cpp
    modules/zenith_ui/ui/controls/SkiaComboBox.cpp
    modules/zenith_ui/ui/controls/SkiaFileChooser.cpp
    modules/zenith_ui/ui/controls/SkiaLabel.cpp
    modules/zenith_ui/ui/controls/SkiaListBox.cpp
    modules/zenith_ui/ui/controls/SkiaPopupMenu.cpp
    modules/zenith_ui/ui/controls/SkiaTextEditor.cpp
    modules/zenith_ui/ui/controls/SkiaTextInput.cpp
    modules/zenith_ui/ui/controls/SpectraAnalyzerComponent.cpp
    modules/zenith_ui/ui/controls/ToastNotificationManager.cpp
    modules/zenith_ui/ui/controls/ZenithButton.cpp
    modules/zenith_ui/ui/controls/ZenithControl.cpp
    modules/zenith_ui/ui/controls/ZenithDropdown.cpp
    modules/zenith_ui/ui/controls/ZenithKnob.cpp
    modules/zenith_ui/ui/controls/ZenithModMatrix.cpp
    modules/zenith_ui/ui/controls/ZenithSlider.cpp
    modules/zenith_ui/ui/controls/ZenithTextInput.cpp
    modules/zenith_ui/ui/controls/ZenithToggle.cpp
    modules/zenith_ui/ui/controls/ZenithTooltipOverlay.cpp
    modules/zenith_ui/ui/controls/ZenithVisualizer.cpp
    
    # Common Views and Panels
    modules/zenith_ui/ui/panels/SettingsPanel.cpp
    modules/zenith_ui/ui/panels/BrowserFilterBar.cpp
    modules/zenith_ui/ui/panels/BrowserListView.cpp
    modules/zenith_ui/ui/panels/BrowserSearchBar.cpp
    modules/zenith_ui/ui/panels/PluginBrowserComponent.cpp
    modules/zenith_ui/ui/panels/PresetBrowserComponent.cpp
    
    modules/zenith_ui/ui/mixer/MixerChannelComponent.cpp
    modules/zenith_ui/ui/mixer/MixerComponent.cpp
    modules/zenith_ui/ui/mixer/PluginBrowser.cpp
    
    modules/zenith_ui/ui/transport/TransportBar.cpp
    modules/zenith_ui/ui/legacy/ZenithLookAndFeel.cpp
)

# UI framework headers
set(ZENITH_UI_UNIFIED_HEADERS
    modules/zenith_ui/ui/unified/IUnifiedComponent.h
    modules/zenith_ui/ui/unified/UnifiedComponent.h
    modules/zenith_ui/ui/unified/controls/UnifiedButton.h
    modules/zenith_ui/ui/unified/controls/UnifiedSlider.h
)

# =============================================================================
# ZENITH UI VIEWS2 MODULE (Advanced UI Components)
# =============================================================================

# Advanced gesture and audio-reactive UI sources
set(ZENITH_UI_VIEWS2_SOURCES
    modules/zenith_ui/ui/views2/AudioBufferSource.cpp
    modules/zenith_ui/ui/views2/AudioReactiveSystem.cpp
    modules/zenith_ui/ui/views2/AdvancedGestureSystem.cpp
    modules/zenith_ui/ui/views2/InternationalizationSystem.cpp
    modules/zenith_ui/ui/views2/arranger/SkiaArrangementView.cpp
    modules/zenith_ui/ui/views2/session/SkiaSessionView.cpp
    modules/zenith_ui/ui/views2/ZenithMainLayout.cpp
    modules/zenith_ui/ui/views2/core/ViewSwitcher.cpp
    modules/zenith_ui/ui/views2/common/SkiaTransportBar.cpp
    modules/zenith_ui/ui/views2/panels/SkiaPluginBrowser.cpp
    modules/zenith_ui/ui/views2/panels/SkiaSettingsPanel.cpp
    modules/zenith_ui/ui/views2/browser/SkiaPresetBrowser.cpp
)

# Advanced gesture and audio-reactive UI headers
set(ZENITH_UI_VIEWS2_HEADERS
    modules/zenith_ui/ui/views2/AudioBufferSource.h
    modules/zenith_ui/ui/views2/AudioReactiveSystem.h
    modules/zenith_ui/ui/views2/AdvancedGestureSystem.h
    modules/zenith_ui/ui/views2/InternationalizationSystem.h
)

# [ ... rest of the file stays the same ... ]
# I'll just write the whole file to be safe since I'm changing so much.

# Check for ICU (International Components for Unicode) library
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    # Prefer imported targets so include dirs and link flags propagate correctly.
    pkg_check_modules(ICU_UC QUIET IMPORTED_TARGET icu-uc)
    pkg_check_modules(ICU_I18N QUIET IMPORTED_TARGET icu-i18n)

    if(ICU_UC_FOUND AND ICU_I18N_FOUND)
        set(ZENITH_HAS_ICU TRUE)
        message(STATUS "Found ICU libraries for internationalization support")
    else()
        set(ZENITH_HAS_ICU FALSE)
        message(WARNING "ICU libraries not found - internationalization features will be limited")
    endif()
else()
    set(ZENITH_HAS_ICU FALSE)
endif()

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
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/framework
        ${CMAKE_SOURCE_DIR}/modules/zenith_ui/ui/design-system
        ${CMAKE_SOURCE_DIR}/external/vcpkg/installed/x64-linux/include/skia
)

target_compile_features(zenith_ui_unified
    PUBLIC
        cxx_std_20
)

target_link_libraries(zenith_ui_unified
    PUBLIC
        zenith_core
        zenith_audio_utils
    PRIVATE
        ${ZENITH_JUCE_MODULES}
)

# Add Skia rendering support (disabled until Skia is properly integrated)
# if(ZENITH_ENABLE_SKIA)
#     target_compile_definitions(zenith_ui_unified PRIVATE ZENITH_ENABLE_SKIA=1)
#     target_link_libraries(zenith_ui_unified PRIVATE skia)
# endif()

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

# =============================================================================
# CREATE VIEWS2 LIBRARY
# =============================================================================

add_library(zenith_ui_views2 STATIC
    ${ZENITH_UI_VIEWS2_SOURCES}
    ${ZENITH_UI_VIEWS2_HEADERS}
)

target_include_directories(zenith_ui_views2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/modules>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(zenith_ui_views2
    PUBLIC
        cxx_std_20
)

# Link ICU if available
if(ZENITH_HAS_ICU)
    target_compile_definitions(zenith_ui_views2 PRIVATE ZENITH_HAS_ICU=1)
    # ICU i18n depends on ICU uc; use imported targets to avoid strict-path and ordering issues.
    target_link_libraries(zenith_ui_views2 PRIVATE PkgConfig::ICU_I18N PkgConfig::ICU_UC)
else()
    target_compile_definitions(zenith_ui_views2 PRIVATE JUCE_DISABLE_ICU=1)
endif()

target_link_libraries(zenith_ui_views2
    PUBLIC
        zenith_core
        ${ZENITH_JUCE_MODULES}
)

# Installation
install(
    TARGETS zenith_ui_views2
    EXPORT ZenithTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    FILES ${ZENITH_UI_VIEWS2_HEADERS}
    DESTINATION include/zenith/ui/views2
)
