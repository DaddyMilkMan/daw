# Skia Purebred Migration Audit

Date: 2026-02-17
Scope: `apps/desktop/Source/ui`, `apps/desktop/Source/instruments`

## Goal
No hybrid UI surfaces in shipping paths.
All visible product UI should be Skia-rendered components, with JUCE used only for platform/window/event plumbing.

## Runtime Surface Audit (Code-Verified)

### Primary runtime surfaces
- `apps/desktop/Source/ui/common/MainLayoutComponent.cpp`
- `apps/desktop/Source/ui/panels/BrowserPanel.cpp`
- `apps/desktop/Source/ui/arranger/ArrangerComponent.cpp`
- `apps/desktop/Source/ui/views/SessionViewComponent.cpp`
- `apps/desktop/Source/ui/sample-editor/SampleEditorComponent.cpp`
- `apps/desktop/Source/ui/piano-roll/PianoRollComponent.cpp`
- `apps/desktop/Source/ui/instruments/ZenithPolySynthUI.cpp`
- `apps/desktop/Source/ui/mixer/MixerComponent.cpp`
- `apps/desktop/Source/ui/mixer/MixerChannelComponent.cpp`

Status:
- Core canvas rendering is mostly Skia (`drawSkia` path).
- Hybrid violations still exist in several panels via JUCE widgets/dialog APIs.

## Hybrid Violations (High Impact)

### P0: Must remove for purebred claim
- `apps/desktop/Source/ui/panels/PluginBrowserComponent.h`
  Uses `juce::TextEditor`, `juce::TableListBox`, `juce::ComboBox`, `juce::Label`.
- `apps/desktop/Source/ui/panels/SettingsPanel.h`
  Uses `juce::Viewport`, `juce::TextEditor`, `juce::ComboBox`, `juce::Slider`, `juce::Label`.
- `apps/desktop/Source/ui/common/WingmanPanel.h`
  Uses JUCE text input/button/combo paths.
- `apps/desktop/Source/ui/mixer/PluginBrowser.h`
  Uses `juce::TextEditor`.
- `apps/desktop/Source/ui/project/ProjectManagerUI.h`
  Heavy JUCE widget tree (`ListBox`, `ComboBox`, `TextEditor`, `Slider`, `Label`, `FileChooser`).

### P1: User-facing but not always in first path
- `apps/desktop/Source/ui/visualizations/WaveformDisplay.h`
  JUCE button/combo/slider controls.
- `apps/desktop/Source/instruments/ZenithSamplerEditor.h`
  `juce::AudioProcessorEditor` + JUCE table/list controls.
- `apps/desktop/Source/instruments/ZenithPolySynthEditor.h`
  Legacy JUCE component path still exists alongside Skia UI.

### P2: Non-core or utility paths
- Various alert/file-chooser call sites in non-browser surfaces.

## Completed In This Iteration

### Browser panel moved to Skia-native dialog stack
- Replaced JUCE `AlertWindow` tag dialog with `SkiaAlertWindow`.
- Replaced JUCE `FileChooser` add-folder flow with `SkiaFileChooser`.
- Added explicit overlay lifecycle cleanup.

Files:
- `apps/desktop/Source/ui/panels/BrowserPanel.h`
- `apps/desktop/Source/ui/panels/BrowserPanel.cpp`

## Enforcement Plan

### Phase A (P0) - no hybrids in primary user flow
1. Replace `PluginBrowserComponent` with `Skia` list/search table implementation.
2. Replace `SettingsPanel` control stack with `ZenithControl` primitives.
3. Replace `WingmanPanel` input/selector controls with `SkiaTextEditor`/`SkiaComboBox`/`ZenithButton`.
4. Replace mixer plugin browser text input with `SkiaTextEditor`.

### Phase B (P1) - complete product-level consistency
1. Migrate sampler editor to Skia controls and canvas.
2. Remove legacy `ZenithPolySynthEditor` path or route fully to `ZenithPolySynthUI`.
3. Migrate `ProjectManagerUI` away from JUCE list/viewport controls.

### Phase C (P2) - clean residual JUCE UI calls
1. Remove remaining JUCE `AlertWindow` and `FileChooser` usages in UI layer.
2. Keep JUCE only for system-level window/event integration.

## Build Verification
- `cmake --build build --target ZenithDAW -j4` passes after current changes.
