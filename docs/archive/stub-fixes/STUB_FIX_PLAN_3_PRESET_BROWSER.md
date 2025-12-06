# 🎯 CRITICAL STUB FIX PLAN #3: PRESET BROWSER
**Date**: 2025-11-30 20:50 PST
**Complexity**: LOW-MEDIUM
**Risk**: LOW
**Est. Time**: 30 minutes

---

## 📋 CURRENT STATE

**File**: `PresetBrowserComponent.h`
**Stubs**:
- Line 139: `refreshPresetList()` - Just clears, doesn't load
- Line 175: `loadPresetAtIndex()` - Creates dummy preset
- Line 185: `showSaveDialog()` - Shows "coming soon" alert
- Line 197: `deleteSelectedPreset()` - Shows "coming soon" alert

**Available Resources**:
- `PresetGenerator::generateFactoryPresets()` - Generates 500 presets
- `ZenithPresetManager` - Has `savePreset()`, `loadPreset()`, `getPresets()`
- `ZenithInstrumentPreset` struct - Full preset data structure

---

## 🔧 FIX #1: refreshPresetList() (Line 137-143)

### Current Code:
```cpp
void refreshPresetList()
{
    // TODO: Get actual presets from PresetManager
    // For now, just clear
    allPresets_.clear();
    updateFilteredPresets();
}
```

### New Code:
```cpp
void refreshPresetList()
{
    allPresets_.clear();
    
    // Get presets from PresetManager
    auto presets = presetManager_.getPresetsForInstrument(instrumentId_);
    
    for (const auto& preset : presets)
    {
        PresetInfo info;
        info.name = preset.name;
        info.category = preset.category;
        info.file = presetManager_.getPresetFile(preset.id);
        allPresets_.add(info);
    }
    
    // If no presets exist, generate factory presets
    if (allPresets_.isEmpty())
    {
        DBG("No presets found, generating factory presets...");
        PresetGenerator::generateFactoryPresets();
        
        // Reload after generation
        auto newPresets = presetManager_.getPresetsForInstrument(instrumentId_);
        for (const auto& preset : newPresets)
        {
            PresetInfo info;
            info.name = preset.name;
            info.category = preset.category;
            info.file = presetManager_.getPresetFile(preset.id);
            allPresets_.add(info);
        }
    }
    
    updateFilteredPresets();
}
```

### Risk: LOW
- Uses existing PresetManager API
- Fallback to factory generation if empty
- No file system manipulation

---

## 🔧 FIX #2: loadPresetAtIndex() (Line 171-181)

### Current Code:
```cpp
void loadPresetAtIndex(int index)
{
    if (onLoadPreset && index >= 0 && index < filteredPresets_.size())
    {
        // TODO: Load actual preset from file
        // For now, just call the callback with a dummy preset
        ZenithInstrumentPreset preset;
        preset.name = filteredPresets_[index].name.toStdString();
        onLoadPreset(preset);
    }
}
```

### New Code:
```cpp
void loadPresetAtIndex(int index)
{
    if (onLoadPreset && index >= 0 && index < filteredPresets_.size())
    {
        auto& presetInfo = filteredPresets_[index];
        
        // Load preset from PresetManager
        auto preset = presetManager_.loadPresetFromFile(presetInfo.file);
        
        if (preset.has_value())
        {
            DBG("Loaded preset: " + preset->name);
            onLoadPreset(*preset);
        }
        else
        {
            DBG("Failed to load preset: " + presetInfo.name);
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Load Error",
                "Failed to load preset: " + presetInfo.name);
        }
    }
}
```

### Risk: LOW
- Uses PresetManager's load method
- Has error handling
- Shows user feedback on failure

---

## 🔧 FIX #3: showSaveDialog() (Line 183-190)

### Current Code:
```cpp
void showSaveDialog()
{
    // TODO: Show save dialog
    // For now, just show alert
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                           "Save Preset",
                                           "Save preset functionality coming soon!");
}
```

### New Code:
```cpp
void showSaveDialog()
{
    // Capture current state
    if (!onCaptureState)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Error",
            "Cannot save: No state capture callback set.");
        return;
    }
    
    // Show input dialog for preset name
    juce::AlertWindow nameDialog("Save Preset", 
                                 "Enter preset name:",
                                 juce::AlertWindow::QuestionIcon);
    
    nameDialog.addTextEditor("name", "", "Preset Name:");
    nameDialog.addComboBox("category", {"Bass", "Lead", "Pad", "Pluck", "Keys", "FX", "Sequence"}, "Category:");
    nameDialog.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    nameDialog.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    
    if (nameDialog.runModalLoop() == 1)
    {
        juce::String presetName = nameDialog.getTextEditorContents("name");
        
        if (presetName.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Error",
                "Preset name cannot be empty.");
            return;
        }
        
        // Create preset
        Preset preset;
        preset.id = juce::Uuid().toString();
        preset.name = presetName;
        preset.instrumentId = instrumentId_;
        preset.category = nameDialog.getComboBoxComponent("category")->getText();
        preset.author = "User";
        preset.description = "User preset";
        
        // Capture parameters
        auto params = onCaptureState();
        for (const auto& [id, value] : params)
        {
            preset.setParameter(juce::String(id), value);
        }
        
        // Save as user preset
        if (presetManager_.savePreset(preset, true))
        {
            DBG("Saved preset: " + presetName);
            refreshPresetList();
            
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Success",
                "Preset saved: " + presetName);
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Error",
                "Failed to save preset.");
        }
    }
}
```

### Risk: MEDIUM
- Uses modal dialog (blocks UI)
- Requires `onCaptureState` callback to be set
- File system write operation

---

## 🔧 FIX #4: deleteSelectedPreset() (Line 192-202)

### Current Code:
```cpp
void deleteSelectedPreset()
{
    int selectedRow = presetList_.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < filteredPresets_.size())
    {
        // TODO: Delete preset file and refresh list
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                               "Delete Preset",
                                               "Delete preset functionality coming soon!");
    }
}
```

### New Code:
```cpp
void deleteSelectedPreset()
{
    int selectedRow = presetList_.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < filteredPresets_.size())
    {
        auto& presetInfo = filteredPresets_[selectedRow];
        
        // Confirm deletion
        bool confirmed = juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Delete Preset",
            "Are you sure you want to delete '" + presetInfo.name + "'?",
            "Delete",
            "Cancel");
        
        if (confirmed)
        {
            // Delete the file
            if (presetInfo.file.deleteFile())
            {
                DBG("Deleted preset: " + presetInfo.name);
                refreshPresetList();
                
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Success",
                    "Preset deleted: " + presetInfo.name);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Delete Error",
                    "Failed to delete preset file.");
            }
        }
    }
}
```

### Risk: MEDIUM
- File deletion is permanent
- Has confirmation dialog
- Error handling included

---

## ✅ TESTING PLAN

### Test 1: Load Factory Presets
1. Open preset browser
2. Verify 500 presets appear
3. Select a preset
4. Click "Load"
5. Verify preset loads correctly

### Test 2: Save User Preset
1. Modify synth parameters
2. Click "Save"
3. Enter name "My Test Preset"
4. Select category "Lead"
5. Click "Save"
6. Verify preset appears in list

### Test 3: Delete Preset
1. Select a user preset
2. Click "Delete"
3. Confirm deletion
4. Verify preset is removed from list
5. Verify file is deleted from disk

### Test 4: Search/Filter
1. Type "Bass" in search box
2. Verify only bass presets show
3. Clear search
4. Verify all presets show again

---

## 📊 RISK ASSESSMENT

| Fix | Risk | Why |
|-----|------|-----|
| refreshPresetList | LOW | Read-only operation |
| loadPresetAtIndex | LOW | Has error handling |
| showSaveDialog | MEDIUM | File write, modal dialog |
| deleteSelectedPreset | MEDIUM | File deletion (permanent) |

**Overall Risk**: LOW-MEDIUM

---

## 🎯 IMPLEMENTATION ORDER

1. **refreshPresetList** - Safest, enables testing
2. **loadPresetAtIndex** - Needed for testing
3. **showSaveDialog** - More complex, test after load works
4. **deleteSelectedPreset** - Last, most destructive

---

## 📝 NOTES

- Requires `PresetManager` to have `getPresetsForInstrument()` method
- Requires `PresetManager` to have `loadPresetFromFile()` method
- Requires `PresetManager` to have `getPresetFile()` method
- May need to add these methods if they don't exist

---

**Ready to implement**: YES
**Requires review**: Modal dialogs (blocking UI)
**Fallback plan**: If PresetManager methods missing, add them first
