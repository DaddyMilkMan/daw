/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <array>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Randomization Target
//==============================================================================

struct RandomizationTarget {
    juce::String parameterId;
    juce::String displayName;
    float minValue;
    float maxValue;
    bool isLogarithmic = false;
    bool isInteger = false;
    float weight = 1.0f;         // Probability of being included

    // Current lock state
    bool isLocked = false;
    bool wasLockedBeforeRandom = false;
    float preRandomValue = 0.0f;  // For undo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizationTarget)
};

//==============================================================================
// Randomization Group
//==============================================================================

class RandomizationGroup {
public:
    RandomizationGroup(const juce::String& name);

    juce::String getName() const { return name_; }
    void setName(const juce::String& name) { name_ = name; }

    void addTarget(const RandomizationTarget& target);
    void removeTarget(const juce::String& parameterId);
    const std::vector<RandomizationTarget>& getTargets() const { return targets_; }

    void setGroupLocked(bool locked) { groupLocked_ = locked; }
    bool isGroupLocked() const { return groupLocked_; }

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void randomize(float amount);
    void lockAll();
    void unlockAll();
    void clear();

private:
    juce::String name_;
    std::vector<RandomizationTarget> targets_;
    bool groupLocked_ = false;
    bool enabled_ = true;

    juce::Random random_;
};

//==============================================================================
// Randomization Preset
//==============================================================================

struct RandomizationPreset {
    juce::String name;
    juce::String description;

    // Group states
    std::vector<juce::String> groupNames;
    juce::Array<juce::String> lockedGroups;
    juce::Array<juce::String> unlockedGroups;

    // Global settings
    float amount = 0.5f;
    bool lockCurrentState = false;
    bool includeHidden = false;

    // Individual parameter locks
    juce::StringArray lockedParameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizationPreset)
};

//==============================================================================
// Randomization History
//==============================================================================

class RandomizationHistory {
public:
    struct HistoryEntry {
        juce::StringArray parameterIds;
        std::vector<float> previousValues;
        juce::String description;
        juce::int64 timestamp;
    };

    void saveState(const std::vector<RandomizationTarget>& targets);
    void pushEntry(const HistoryEntry& entry);
    HistoryEntry undo();
    HistoryEntry redo();
    bool canUndo() const { return historyPosition_ > 0; }
    bool canRedo() const { return historyPosition_ < history_.size(); }
    void clear();

private:
    std::vector<HistoryEntry> history_;
    size_t historyPosition_ = 0;
};

//==============================================================================
// SkiaRandomizationPanel
//==============================================================================

class SkiaRandomizationPanel : public SkiaComponent {
public:
    SkiaRandomizationPanel();
    ~SkiaRandomizationPanel() override = default;

    void drawSkia(SkCanvas* canvas) override;
    std::vector<AIElementInfo> getInspectableElements() override;

    // Configuration
    void setTargets(const std::vector<RandomizationTarget>& targets);
    void addGroup(const RandomizationGroup& group);

    // Randomization control
    void setRandomizeAmount(float amount) { randomAmount_ = amount; markDirty(); }
    float getRandomizeAmount() const { return randomAmount_; }

    void randomize();
    void randomizeGroup(const juce::String& groupName);
    void lockAll();
    void unlockAll();
    void clearLocks();

    // History
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    // Presets
    void setRandomizationPreset(const RandomizationPreset& preset);
    RandomizationPreset getCurrentPreset() const;
    void saveAsPreset(const juce::String& name, const juce::String& description = {});

    // Display options
    void setShowGroups(bool show) { showGroups_ = show; markDirty(); }
    void setShowWeights(bool show) { showWeights_ = show; markDirty(); }
    void setShowValueSliders(bool show) { showValueSliders_ = show; markDirty(); }
    void setCompactMode(bool compact) { compactMode_ = compact; markDirty(); }

    // Callbacks
    using RandomizationCallback = std::function<void(const juce::String&, float)>;
    void setRandomizationCallback(RandomizationCallback callback) {
        randomizationCallback_ = std::move(callback);
    }

    using LockChangedCallback = std::function<void(const juce::String&, bool)>;
    void setLockChangedCallback(LockChangedCallback callback) {
        lockChangedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void resized() override;

private:
    // Data
    std::vector<RandomizationTarget> targets_;
    std::vector<RandomizationGroup> groups_;
    RandomizationHistory history_;

    // State
    float randomAmount_ = 0.5f;
    bool lockCurrentState_ = false;
    int hoveredTarget_ = -1;
    int draggedTarget_ = -1;
    bool isDraggingAmount_ = false;
    float dragStartY_ = 0.0f;
    float dragStartAmount_ = 0.0f;

    // Display
    bool showGroups_ = true;
    bool showWeights_ = true;
    bool showValueSliders_ = true;
    bool compactMode_ = false;
    bool showAllGroups_ = true;
    int currentGroupIndex_ = 0;

    // Layout rects
    SkRect headerRect_;
    SkRect amountSliderRect_;
    SkRect groupsListRect_;
    SkRect targetsRect_;
    SkRect buttonRowRect_;

    // Animation
    float randomizeAnimation_ = 0.0f;
    float lockAnimation_ = 0.0f;

    // Callbacks
    RandomizationCallback randomizationCallback_;
    LockChangedCallback lockChangedCallback_;

    // Drawing helpers
    void drawBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawHeader(SkCanvas* canvas, const SkRect& bounds);
    void drawAmountSlider(SkCanvas* canvas, const SkRect& bounds);
    void drawGroupsList(SkCanvas* canvas, const SkRect& bounds);
    void drawTargets(SkCanvas* canvas, const SkRect& bounds);
    void drawTargetRow(SkCanvas* canvas, const SkRect& bounds, const RandomizationTarget& target,
                     int index, bool isHovered, bool isSelected);
    void drawLockIcon(SkCanvas* canvas, const SkRect& bounds, bool isLocked, bool isHovered);
    void drawWeightSlider(SkCanvas* canvas, const SkRect& bounds, float weight, bool isHovered);
    void drawValueDisplay(SkCanvas* canvas, const SkRect& bounds, float value);
    void drawButtonRow(SkCanvas* canvas, const SkRect& bounds);

    // Buttons
    SkRect randomizeButtonRect_;
    SkRect lockAllButtonRect_;
    SkRect unlockAllButtonRect_;
    SkRect clearLocksButtonRect_;
    SkRect undoButtonRect_;
    SkRect redoButtonRect_;
    SkRect presetsButtonRect_;

    // Button interactions
    bool isButtonHovered(const SkRect& rect, float x, float y) const;

    // Layout
    void updateLayout();

    // Group filtering
    void filterDisplayedTargets();
    std::vector<const RandomizationTarget*> getDisplayedTargets() const;

    // Colors
    SkColor getBackgroundColor() const { return SkColorSetARGB(255, 15, 15, 20); }
    SkColor getSurfaceColor() const { return SkColorSetARGB(255, 25, 25, 32); }
    SkColor getAccentColor() const { return SkColorSetARGB(255, 0, 200, 255); }
    SkColor getLockColor() const { return SkColorSetARGB(255, 200, 50, 50); }
    SkColor getUnlockColor() const { return SkColorSetARGB(255, 100, 100, 100); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaRandomizationPanel)
};

//==============================================================================
// Factory Presets
//==============================================================================

class RandomizationPresets {
public:
    static RandomizationPreset mild();
    static RandomizationPreset moderate();
    static RandomizationPreset wild();
    static RandomizationPreset chaotic();
    static RandomizationPreset subtle();
    static RandomizationPreset musical();

    static juce::StringArray getPresetNames() {
        return {"Mild", "Moderate", "Wild", "Chaotic", "Subtle", "Musical"};
    }
};

} // namespace zenith
