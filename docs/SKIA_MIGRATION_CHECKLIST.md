# Skia Migration Checklist

Scope: compiled UI sources from `cmake/ZenithUIUnified.cmake` plus app entry UI shell.
Goal: eliminate JUCE widget UI usage and replace with Skia-rendered/Skia-controlled implementations.

## Completed

- [x] Replace app shell JUCE widgets in `apps/desktop/Source/ui/common/MainWindow.cpp` with Skia controls.
- [x] Add Skia modal/chooser wiring in `apps/desktop/Source/ui/common/MainWindow.h`.
- [x] Fix recursive `SkiaSmartKnob::drawSkia` call in `modules/zenith_ui/ui/instruments/SkiaSmartKnob.cpp`.
- [x] Restore missing text drawing in `modules/zenith_ui/ui/instruments/SkiaPresetBrowser.cpp`.
- [x] Replace JUCE `TextEditor` in `modules/zenith_ui/ui/mixer/PluginBrowser.h` with `SkiaTextInput`.
- [x] Rewire search callbacks in `modules/zenith_ui/ui/mixer/PluginBrowser.cpp` to `SkiaTextInput`.
- [x] Remove JUCE `TextButton` color ID dependence in `modules/zenith_ui/ui/unified/UnifiedComponent.cpp`.
- [x] Replace placeholder type indicator rendering in `modules/zenith_ui/ui/mixer/PluginBrowser.cpp`.
- [x] Replace placeholder waveform seed with deterministic preset-derived visualization in `modules/zenith_ui/ui/panels/PresetBrowserComponent.cpp`.
- [x] Rebuild `views2` Skia plugin browser into production behavior in `modules/zenith_ui/ui/views2/panels/SkiaPluginBrowser.cpp` and `modules/zenith_ui/ui/views2/panels/SkiaPluginBrowser.h`.
- [x] Wire real plugin instantiation + track insertion from browser selection in `modules/zenith_ui/ui/views2/panels/SkiaPluginBrowser.cpp`.
- [x] Add pro workflow layers to `SkiaPluginBrowser`: scoped library views (All/Favorites/Recent), sort modes, keyboard power-user shortcuts, and right-side detail pane.
- [x] Add top-vendor quick-filter chips in `SkiaPluginBrowser` for faster narrowing without typing.
- [x] Add left navigation tree in `SkiaPluginBrowser` with hierarchical sections: Collections, Smart Folders, Vendors.
- [x] Add drag-start behavior from plugin rows in `SkiaPluginBrowser` with JUCE drag container integration and plugin-drag callback hook.
- [x] Add editable collection behavior: pinned vendor collections (persisted + keyboard toggle + nav tree section) in `SkiaPluginBrowser`.
- [x] Add persistent browser state for favorites + recent plugins in `SkiaPluginBrowser` (`plugin_browser_state.json` under user app data).
- [x] Add plugin drop targets across host surfaces for `zenith_plugin|...` payloads: mixer channel inserts, session view track drops, arranger track drops.
- [x] Add custom named collections with persisted metadata (scope/category/vendor/sort/tags), plus apply/rename/reorder/delete workflows in `SkiaPluginBrowser`.
- [x] Remove demo-plugin fallback from `SkiaPluginBrowser`; now truthfully reports real scan state.

## In Progress

- [ ] Runtime UX validation pass for browser panels (`PluginBrowser`, `SkiaPluginBrowser`, `PresetBrowserComponent`).
- [ ] Full `ZenithDAW` app target still blocked by pre-existing `MainWindow.cpp` structural mismatch (`MainComponent` / `MainWindow` declaration issues).
- [ ] Full interaction QA for newly added custom collection workflows (create, rename, tag, reorder, delete) under real plugin libraries.

## Execution Plan (Implemented)

- [x] Phase 1: Complete organization model inside `SkiaPluginBrowser` with custom collections and persistent tags.
- [x] Phase 2: Expose collection lifecycle controls in-panel (save/apply/rename/delete/reorder) and keyboard power workflows.
- [x] Phase 3: Integrate collection-aware filtering and sort behavior with persistent browser state.
- [x] Phase 4: Rebuild and regression-check `zenith_ui_views2` and `zenith_ui_unified`.

## Verification

- [x] Build `zenith_ui_unified` target: success.
- [x] Build `zenith_ui_views2` target: success.
- [x] Compiled-source scan for JUCE widget usage in inventory scope: zero matches.
- [x] Rebuild `zenith_ui_views2` after favorites/recents persistence + detail-pane upgrade: success.
- [x] Build browser object directly (`SkiaPluginBrowser.cpp.o`) after vendor filter + persistence upgrades: success.
- [x] Full `zenith_ui_views2` rebuild after hierarchy + drag updates: success.
- [x] Clean rebuild `zenith_ui_unified` after plugin drag/drop target integration: success.
- [x] Rebuild `zenith_ui_views2` after plugin drag/drop target integration: success.

## Compiled Source Inventory

| File | JUCE Widget Usage | Placeholder Debt | Status |
|---|---:|---:|---|
| `apps/desktop/Source/ui/common/MainWindow.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ContextMenuManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/DebugConsoleComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ExportProgressBar.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/FreezeProgressOverlay.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/MarkdownComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaAlertWindow.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaComboBox.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaFileChooser.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaLabel.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaListBox.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaPopupMenu.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/SkiaTextEditor.cpp` | 0 | 6 | PLACEHOLDER-DEBT |
| `modules/zenith_ui/ui/controls/SkiaTextInput.cpp` | 0 | 2 | PLACEHOLDER-DEBT |
| `modules/zenith_ui/ui/controls/SpectraAnalyzerComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ToastNotificationManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithButton.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithControl.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithDropdown.cpp` | 0 | 1 | PLACEHOLDER-DEBT |
| `modules/zenith_ui/ui/controls/ZenithKnob.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithModMatrix.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithSlider.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithTextInput.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithToggle.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithTooltipOverlay.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/controls/ZenithVisualizer.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/FontManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/MeterRenderer.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/PlatformFontUtils.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/SvgIcon.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/ThemeManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/ZenithDesignSystem.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/ZenithLayout.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/design-system/ZenithTheme.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/AnimationCoordinator.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/AuroraBackground.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/ComponentLifecycleManager.cpp` | 0 | 5 | PLACEHOLDER-DEBT |
| `modules/zenith_ui/ui/framework/ConfigurationManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/LayoutManager.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/PlatformDisplayUtils.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/PlatformPathUtils.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/PlatformWindowUtils.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/SkiaComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/SkiaD3D12Context.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/SkiaLayout.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/SkiaMainWindowIntegration.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/framework/UIErrorHandler.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/legacy/ZenithLookAndFeel.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/mixer/MixerChannelComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/mixer/MixerComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/mixer/PluginBrowser.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/panels/BrowserFilterBar.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/panels/BrowserListView.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/panels/BrowserSearchBar.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/panels/PresetBrowserComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/transport/TransportBar.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/unified/UnifiedComponent.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/unified/controls/UnifiedButton.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/AdvancedGestureSystem.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/AudioBufferSource.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/AudioReactiveSystem.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/InternationalizationSystem.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/ZenithMainLayout.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/arranger/SkiaArrangementView.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/browser/SkiaPresetBrowser.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/common/SkiaTransportBar.cpp` | 0 | 6 | PLACEHOLDER-DEBT |
| `modules/zenith_ui/ui/views2/core/ViewSwitcher.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/panels/SkiaPluginBrowser.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/panels/SkiaSettingsPanel.cpp` | 0 | 0 | OK |
| `modules/zenith_ui/ui/views2/session/SkiaSessionView.cpp` | 0 | 0 | OK |
