/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <functional>
#include <memory>

namespace zenith {

//==============================================================================
// PRESET METADATA
//==============================================================================
/**
 * Rich metadata for presets matching Serum 2
 */
struct PresetMetadata {
    juce::String name = "Untitled";
    juce::String author = "Unknown";
    juce::String category = "Bass";
    juce::String tags = "";

    // Rating (0-5 stars)
    int rating = 0;

    // Creation and modification dates
    juce::Time creationDate = juce::Time::getCurrentTime();
    juce::Time modificationDate = juce::Time::getCurrentTime();

    // Audio preview (optional)
    std::vector<float> audioPreview;
    int previewSampleRate = 44100;

    // Compatibility info
    int formatVersion = 1;
    juce::String requiredVersion = "1.0.0";

    // User data
    bool isFavorite = false;
    int playCount = 0;
    juce::Time lastPlayed;

    // Color coding for UI
    juce::Colour color = juce::Colours::white;
};

//==============================================================================
// PRESET DATA
//==============================================================================
/**
 * Complete preset data with parameters and metadata
 */
struct Preset {
    PresetMetadata metadata;

    // All parameter values (indexed by parameter ID)
    std::unordered_map<int, float> parameters;

    // Modulation matrix state
    juce::var modulationMatrix;

    // Macro names (4 macros)
    std::array<juce::String, 4> macroNames = {"Macro 1", "Macro 2", "Macro 3", "Macro 4"};

    // WAV table references (by path or index)
    std::vector<juce::String> wavetableFiles;

    // Serialize to/from JSON
    juce::var toJSON() const;
    bool fromJSON(const juce::var& json);
};

//==============================================================================
// PRESET MORPH STATE
//==============================================================================
/**
 * State for morphing between two presets
 */
struct PresetMorphState {
    bool isActive = false;
    float morphPosition = 0.0f;           // 0.0 = A, 1.0 = B
    std::unique_ptr<Preset> presetA;
    std::unique_ptr<Preset> presetB;
    std::vector<float> currentValues;
    std::vector<float> targetValues;
    float smoothingTime = 0.1f;              // Smoothing time in seconds
};

//==============================================================================
// UNDO/REDO ENTRY
//==============================================================================
/**
 * Single undo/redo entry for preset changes
 */
struct PresetUndoEntry {
    juce::String description;
    std::vector<int> changedParameters;
    std::vector<float> oldValues;
    std::vector<float> newValues;
    juce::Time timestamp;

    PresetUndoEntry() : timestamp(juce::Time::getCurrentTime()) {}
};

//==============================================================================
// PRESET MANAGER
//==============================================================================
/**
 * Professional preset management matching Serum 2
 *
 * FEATURES:
 * - Save/load presets with metadata
 * - Preset search and filtering
 * - Preset morphing
 * - Undo/redo support
 * - Auto-save
 * - Import/export
 * - Tag management
 * - Favorites
 */
class ZenithPresetManager {
public:
    ZenithPresetManager();
    ~ZenithPresetManager() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set preset directory path
     */
    void setPresetDirectory(const juce::File& dir);

    /**
     * @brief Get preset directory
     */
    juce::File getPresetDirectory() const { return presetDirectory_; }

    //==========================================================================
    // Preset Loading/Saving
    //==========================================================================

    /**
     * @brief Load preset from file
     */
    bool loadPreset(const juce::File& file, Preset& preset);

    /**
     * @brief Save preset to file
     */
    bool savePreset(const Preset& preset, const juce::File& file);

    /**
     * @brief Load preset by name
     */
    bool loadPresetByName(const juce::String& name, Preset& preset);

    /**
     * @brief Save current synth state as preset
     */
    bool saveCurrentState(const juce::String& name);

    //==========================================================================
    // Preset Browsing
    //==========================================================================

    /**
     * @brief Get all presets
     */
    const std::vector<Preset>& getAllPresets() const { return presets_; }

    /**
     * @brief Get presets by category
     */
    std::vector<Preset> getPresetsByCategory(const juce::String& category) const;

    /**
     * @brief Get favorite presets
     */
    std::vector<Preset> getFavoritePresets() const;

    /**
     * @brief Get recently used presets
     */
    std::vector<Preset> getRecentPresets() const;

    /**
     * @brief Search presets by name/tag
     */
    std::vector<Preset> searchPresets(const juce::String& query) const;

    //==========================================================================
    // Preset Application
    //==========================================================================

    /**
     * @brief Apply preset to synth with smoothing
     */
    void applyPreset(const Preset& preset, float smoothingTime = 0.0f);

    /**
     * @brief Apply preset immediately (no smoothing)
     */
    void applyPresetImmediate(const Preset& preset);

    //==========================================================================
    // Preset Morphing
    //==========================================================================

    /**
     * @brief Start morphing between two presets
     */
    void startMorph(const Preset& from, const Preset& to);

    /**
     * @brief Set morph position (0-1)
     */
    void setMorphPosition(float position);

    /**
     * @brief Get morph position
     */
    float getMorphPosition() const { return morphState_.morphPosition; }

    /**
     * @brief Stop morphing
     */
    void stopMorph();

    /**
     * @brief Process morph smoothing (call each audio callback)
     */
    void processMorph(int numSamples);

    //==========================================================================
    // Undo/Redo
    //==========================================================================

    /**
     * @brief Get undo manager
     */
    juce::UndoManager& getUndoManager() { return undoManager_; }

    /**
     * @brief Begin new undo transaction
     */
    void beginUndoTransaction(const juce::String& description);

    /**
     * @brief End undo transaction
     */
    void endUndoTransaction();

    /**
     * @brief Undo
     */
    void undo();

    /**
     * @brief Redo
     */
    void redo();

    /**
     * @brief Can undo
     */
    bool canUndo() const;

    /**
     * @brief Can redo
     */
    bool canRedo() const;

    //==========================================================================
    // Auto-Save
    //==========================================================================

    /**
     * @brief Enable auto-save
     */
    void setAutoSaveEnabled(bool enabled) { autoSaveEnabled_ = enabled; }

    /**
     * @brief Set auto-save interval (seconds)
     */
    void setAutoSaveInterval(int seconds) { autoSaveInterval_ = seconds; }

    /**
     * @brief Trigger auto-save now
     */
    void triggerAutoSave();

    //==========================================================================
    // Metadata
    //==========================================================================

    /**
     * @brief Get all categories
     */
    std::vector<juce::String> getAllCategories() const;

    /**
     * @brief Get all tags
     */
    std::vector<juce::String> getAllTags() const;

    /**
     * @brief Update preset metadata
     */
    void updateMetadata(const juce::String& presetName, const PresetMetadata& metadata);

private:
    juce::File presetDirectory_;
    std::vector<Preset> presets_;

    // Morphing state
    PresetMorphState morphState_;

    // Undo/Redo
    juce::UndoManager undoManager_;

    // Auto-save
    bool autoSaveEnabled_ = false;
    int autoSaveInterval_ = 300;  // 5 minutes
    juce::Time lastAutoSave_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void scanPresets();
    void indexPresets();
    void updateRecentPresets(const Preset& preset);
};

} // namespace zenith
