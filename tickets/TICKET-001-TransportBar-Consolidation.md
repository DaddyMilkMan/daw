# TICKET-001: Consolidate Duplicate Transport Bars

**Priority**: P0 (Critical)  
**Type**: Refactoring / Bug Fix  
**Estimated Effort**: 2-3 days  
**Component**: UI / Main Window  

---

## Problem Statement

The application currently renders **two transport bars simultaneously**:

1. **Legacy `TransportBar`** (`apps/desktop/Source/ui/transport/TransportBar.h`)
   - Instantiated in `MainComponent` constructor (MainWindow.cpp:89-158)
   - Drawn in `MainComponent::drawSkiaContent()` (MainWindow.cpp:467-472)
   
2. **New `SkiaTransportBar`** (`apps/desktop/Source/ui/views2/common/SkiaTransportBar.h`)
   - Instantiated in `ZenithMainLayout` constructor (ZenithMainLayout.cpp:29-31)
   - Drawn as part of new UI layout

This creates visual duplication, maintenance overhead, and callback confusion.

---

## Evidence

### Current Rendering Order (MainWindow.cpp:401-491)
```cpp
void MainComponent::drawSkia(SkCanvas* canvas) {
    // ... hub mode ...
    
    // 2. Draw Main Layout (contains SkiaTransportBar via ZenithMainLayout)
    if (mainLayout && mainLayout->isVisible()) { ... }
    
    // 3. Draw New UI Layout (also contains SkiaTransportBar)
    if (newUILayout && newUILayout->isVisible()) { ... }
    
    // 5. Draw Top Bar Elements (contains legacy TransportBar)
    if (transportBar && transportBar->isVisible()) {  // <-- DUPLICATION
        canvas->save();
        canvas->translate(transportBar->getX(), transportBar->getY());
        transportBar->drawSkia(canvas);  // Legacy transport drawn here
        canvas->restore();
    }
}
```

### Layout Conflicts
- Legacy transport: Y position = 40 (below title bar)
- New SkiaTransportBar: Y position = 0 (top of ZenithMainLayout)
- Both attempt to show transport state and handle user input

---

## Requirements

### Functional Requirements

1. **Single Source of Truth**: Only `SkiaTransportBar` should be rendered
2. **Feature Parity**: All legacy transport features must work in new transport
3. **Callback Migration**: All callbacks wired to legacy transport must work with new
4. **Hub Mode**: Transport should be hidden in Hub mode (correct current behavior)

### Non-Functional Requirements

1. **No Visual Regression**: Transport must look identical or better
2. **Performance**: Maintain 60fps during transport animations
3. **Accessibility**: Maintain accessibility support

---

## Implementation Plan

### Step 1: Audit Legacy Transport Features (2 hours)

Document all features in legacy `TransportBar`:

```cpp
// From TransportBar.h analysis:

// State setters
void setPlaying(bool);
void setRecording(bool);
void setTempo(double);
void setCPU(float);
void setPosition(double);
void setTimeSignature(int, int);
void setLooping(bool);

// Callbacks
std::function<void()> onPlayClicked;
std::function<void()> onStopClicked;
std::function<void()> onRecordClicked;
std::function<void()> onLoopToggled;
std::function<void()> onRewind;
std::function<void()> onViewToggleClicked;
std::function<void()> onSettingsClicked;
std::function<void()> onUpdateAvailable;  // Special: UpdateService integration
std::function<void()> onClearAllSolos;
std::function<void()> onWingmanClicked;
std::function<void(double)> onTempoChanged;
std::function<void(int, int)> onTimeSignatureChanged;

// Update service
std::unique_ptr<zenith::network::UpdateService> updateService_;
bool isUpdateAvailable_ = false;
```

### Step 2: Feature Gap Analysis (1 hour)

Compare with `SkiaTransportBar`:

| Feature | Legacy | New | Status |
|---------|--------|-----|--------|
| Play/Stop/Record | ✅ | ✅ | Match |
| Tempo display/edit | ✅ | ✅ | Match |
| CPU meter | ✅ | ✅ | Match |
| Time signature | ✅ | ✅ | Match |
| Loop toggle | ✅ | ✅ | Match |
| Position display | ✅ | ✅ | Match |
| View toggle | ✅ | ✅ | Match |
| Settings button | ✅ | ❌ | **MISSING** |
| Wingman button | ✅ | ❌ | **MISSING** |
| Update check | ✅ | ❌ | **MISSING** |
| Clear all solos | ✅ | ❌ | **MISSING** |

### Step 3: Extend SkiaTransportBar (4 hours)

Add missing features to `SkiaTransportBar`:

```cpp
// SkiaTransportBar.h - Additions

class SkiaTransportBar : public SkiaComponent {
public:
    // ... existing ...
    
    // NEW: Additional callbacks
    std::function<void()> onSettingsClicked;
    std::function<void()> onWingmanClicked;
    std::function<void()> onClearAllSolos;
    std::function<void()> onUpdateAvailable;  // Called when update detected
    
    // NEW: Update service integration
    void setUpdateService(zenith::network::UpdateService* service);
    bool isUpdateAvailable() const { return updateAvailable_; }
    
private:
    // ... existing ...
    
    // NEW: Additional buttons
    static constexpr int BTN_SETTINGS = 9;
    static constexpr int BTN_WINGMAN = 10;
    static constexpr int BTN_CLEAR_SOLOS = 11;
    
    bool updateAvailable_ = false;
    zenith::network::UpdateService* updateService_ = nullptr;
    
    void drawExtraButtons(SkCanvas* canvas);
};
```

### Step 4: Migrate MainComponent (4 hours)

Remove legacy transport and wire callbacks to new:

```cpp
// MainWindow.cpp - MainComponent constructor

MainComponent::MainComponent(...) {
    // ... hub component setup ...
    
    // REMOVE: Legacy transport bar creation (lines 89-158)
    // transportBar = std::make_unique<TransportBar>();  // DELETE
    
    // REMOVE: All transportBar callback wiring (lines 91-245)
    // All callbacks move to ZenithMainLayout wiring
    
    // ... rest of setup ...
}

// MainWindow.cpp - drawSkiaContent

void MainComponent::drawSkia(SkCanvas* canvas) {
    // ... hub mode ...
    
    // 2. Draw Main Layout (legacy - hidden when useNewUI_)
    if (mainLayout && mainLayout->isVisible()) { ... }
    
    // 3. Draw New UI Layout (contains SkiaTransportBar)
    if (newUILayout && newUILayout->isVisible()) {
        canvas->save();
        canvas->translate(newUILayout->getX(), newUILayout->getY());
        newUILayout->drawSkia(canvas);  // SkiaTransportBar drawn here
        canvas->restore();
    }
    
    // 4. Draw Wingman Panel
    if (rightSidePanel_ && rightSidePanel_->isVisible()) { ... }
    
    // 5. Draw Top Bar (TitleBar only - NO TransportBar)
    if (titleBar && titleBar->isVisible()) { ... }
    
    // REMOVE: Legacy transport bar drawing
    // if (transportBar && transportBar->isVisible()) { ... }  // DELETE
}

// MainWindow.cpp - resized()

void MainComponent::resized() {
    // ... title bar ...
    
    // REMOVE: Legacy transport bar layout (line 584-586)
    // if (transportBar && transportBar->isVisible()) {
    //     transportBar->setBounds(bounds.removeFromTop(52));
    // }
    
    // ... rest of layout ...
}
```

### Step 5: Wire Callbacks in ZenithMainLayout (4 hours)

```cpp
// ZenithMainLayout.cpp

void ZenithMainLayout::setupCallbacks() {
    // ... existing transport callbacks ...
    
    // NEW: Settings button
    transportBar_->onSettingsClicked = [this]() {
        // Signal to MainComponent to show settings
        if (onSettingsRequested) {
            onSettingsRequested();
        }
    };
    
    // NEW: Wingman button
    transportBar_->onWingmanClicked = [this]() {
        if (onWingmanToggleRequested) {
            onWingmanToggleRequested();
        }
    };
    
    // NEW: Update service
    transportBar_->setUpdateService(updateService_);
}

// ZenithMainLayout.h - Add callbacks

class ZenithMainLayout : public juce::Component, 
                          public ViewSwitcher::Listener {
public:
    // ... existing ...
    
    // NEW: Outward callbacks to MainComponent
    std::function<void()> onSettingsRequested;
    std::function<void()> onWingmanToggleRequested;
    
    void setUpdateService(zenith::network::UpdateService* service) {
        updateService_ = service;
        if (transportBar_) {
            transportBar_->setUpdateService(service);
        }
    }
    
private:
    zenith::network::UpdateService* updateService_ = nullptr;
};
```

### Step 6: Update MainComponent to Wire ZenithMainLayout Callbacks (2 hours)

```cpp
// MainWindow.cpp - MainComponent constructor

MainComponent::MainComponent(...) {
    // ... setup ...
    
    // Create New UI Layout
    newUILayout = std::make_unique<ui::ZenithMainLayout>(engine, projectState);
    
    // NEW: Wire callbacks from ZenithMainLayout
    newUILayout->onSettingsRequested = [this]() {
        if (settingsPanel) {
            settingsPanel->setVisible(!settingsPanel->isVisible());
            if (settingsPanel->isVisible()) {
                settingsPanel->toFront(true);
                resized();
            }
        }
    };
    
    newUILayout->onWingmanToggleRequested = [this]() {
        toggleWingman();
    };
    
    // NEW: Share update service
    newUILayout->setUpdateService(updateService_.get());
    
    addChildComponent(newUILayout.get());
    
    // ... rest of setup ...
}
```

### Step 7: Cleanup (1 hour)

1. Comment out (don't delete yet) legacy transport member:
```cpp
// MainComponent.h

class MainComponent : public SkiaMainWindowIntegration, ... {
private:
    // std::unique_ptr<TransportBar> transportBar;  // REMOVED
};
```

2. Update CMakeLists.txt if needed (remove TransportBar.cpp if no other uses)

3. Run full build and test

---

## Testing Checklist

### Functional Tests

- [ ] Play button starts playback
- [ ] Stop button stops playback
- [ ] Record button arms/disarms recording
- [ ] Tempo edit changes project tempo
- [ ] Time signature edit changes time signature
- [ ] Loop toggle enables/disables looping
- [ ] View toggle switches Arrangement/Session
- [ ] Settings button opens settings panel
- [ ] Wingman button toggles Wingman panel
- [ ] CPU meter shows engine CPU usage
- [ ] Position display updates during playback
- [ ] Update available indicator appears when applicable

### Visual Tests

- [ ] Transport bar renders at correct position (top of main content)
- [ ] No visual duplication
- [ ] Button hover states work
- [ ] Button press states work
- [ ] Record button pulses when armed
- [ ] CPU meter animates smoothly

### Edge Cases

- [ ] Hub mode hides transport correctly
- [ ] Window resize handles transport correctly
- [ ] Transport works with both old and new UI (if useNewUI_ toggle still exists)
- [ ] All shortcuts still work (Space, R, L)

---

## Rollback Plan

If issues are found:

1. Revert MainWindow.cpp changes
2. Uncomment legacy transport member
3. Re-enable legacy transport drawing
4. New UI stack remains available via feature flag

---

## Definition of Done

- [ ] Legacy `TransportBar` member commented out/removed
- [ ] `SkiaTransportBar` has all legacy features
- [ ] All callbacks functional
- [ ] No visual duplication
- [ ] All tests pass
- [ ] Code review approved
- [ ] No performance regression

---

## Related Files

| File | Lines | Action |
|------|-------|--------|
| `apps/desktop/Source/ui/common/MainWindow.cpp` | 89-158, 467-472, 584-586 | Remove legacy transport |
| `apps/desktop/Source/ui/common/MainWindow.h` | TBD | Remove legacy member |
| `apps/desktop/Source/ui/views2/ZenithMainLayout.cpp` | TBD | Add callback wiring |
| `apps/desktop/Source/ui/views2/ZenithMainLayout.h` | TBD | Add callback declarations |
| `apps/desktop/Source/ui/views2/common/SkiaTransportBar.h` | TBD | Add missing features |
| `apps/desktop/Source/ui/views2/common/SkiaTransportBar.cpp` | TBD | Implement missing features |

---

## Notes

- The legacy `TransportBar` class can remain in codebase for now (other components might use it)
- Focus on removing the **instance** in MainComponent, not the class itself
- Consider removing legacy `MainLayoutComponent` in follow-up ticket
