/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>
#include <array>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkTypeface.h>
#include <core/SkFont.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Preset Metadata
//==============================================================================

struct PresetMetadata {
    juce::String id;
    juce::String name;
    juce::String author;
    juce::String category;
    juce::String tags;              // Comma-separated
    juce::String comment;
    juce::int64 creationTime = 0;
    juce::int64 lastModified = 0;
    int rating = 0;                 // 0-5 stars
    int plays = 0;                  // Play count
    bool isFactory = true;
    bool isFavorite = false;
    bool isHidden = false;

    // Audio preview
    int previewLength = 0;          // In samples
    float previewStart = 0.0f;      // Normalized position

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetMetadata)
};

//==============================================================================
// Preset Data
//==============================================================================

struct PresetData {
    PresetMetadata metadata;

    // All parameter values stored as key-value pairs
    juce::StringPairArray parameters;

    // Modulation matrix state
    std::vector<juce::String> modulationRoutes;

    // Macro states
    std::array<float, 8> macroValues;
    juce::String macroNames[8];
    juce::uint32 macroColors[8];

    void clear() {
        parameters.clear();
        modulationRoutes.clear();
        macroValues.fill(0.5f);
        for (auto& name : macroNames) {
            name = "Macro " + juce::String(&name - macroNames + 1);
        }
        for (auto& color : macroColors) {
            color = 0xFF2196F3;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetData)
};

//==============================================================================
// Preset Categories
//==============================================================================

class PresetCategories {
public:
    static const char* Bass;
    static const char* Lead;
    static const char* Pad;
    static const char* Pluck;
    static const char* Keys;
    static const char* FX;
    static const char* Sequencer;
    static const char* Drum;
    static const char* Vocal;
    static const char* Ambient;
    static const char* Experimental;
    static const char* Init;
    static const char* Favorites;
    static const char* All;
    static const char* User;
    static const char* Factory;

    static juce::StringArray getAll() {
        return {Bass, Lead, Pad, Pluck, Keys, FX, Sequencer, Drum,
                Vocal, Ambient, Experimental, Init, Favorites, All, User, Factory};
    }
};

// Static definitions
inline const char* PresetCategories::Bass = "Bass";
inline const char* PresetCategories::Lead = "Lead";
inline const char* PresetCategories::Pad = "Pad";
inline const char* PresetCategories::Pluck = "Pluck";
inline const char* PresetCategories::Keys = "Keys";
inline const char* PresetCategories::FX = "FX";
inline const char* PresetCategories::Sequencer = "Sequencer";
inline const char* PresetCategories::Drum = "Drum";
inline const char* PresetCategories::Vocal = "Vocal";
inline const char* PresetCategories::Ambient = "Ambient";
inline const char* PresetCategories::Experimental = "Experimental";
inline const char* PresetCategories::Init = "Init";
inline const char* PresetCategories::Favorites = "Favorites";
inline const char* PresetCategories::All = "All";
inline const char* PresetCategories::User = "User";
inline const char* PresetCategories::Factory = "Factory";

//==============================================================================
// Preset Sort Order
//==============================================================================

enum class PresetSortOrder {
    AlphabeticalAsc,
    AlphabeticalDesc,
    NewestFirst,
    OldestFirst,
    MostPlayed,
    LeastPlayed,
    HighestRated,
    LowestRated,
    RecentlyAdded,
    RecentlyModified
};

//==============================================================================
// Preset Filter
//==============================================================================

struct PresetFilter {
    juce::String searchQuery;           // Text search
    juce::StringArray categories;       // Selected categories
    juce::StringArray tags;             // Selected tags
    juce::String authorFilter;          // Filter by author
    bool factoryOnly = false;           // Show only factory presets
    bool userOnly = false;              // Show only user presets
    bool favoritesOnly = false;         // Show only favorites
    int ratingMin = 0;                  // Minimum rating (0-5)
    int ratingMax = 5;                  // Maximum rating (0-5)
    bool unratedOnly = false;           // Show only unrated presets

    bool matches(const PresetMetadata& metadata) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetFilter)
};

//==============================================================================
// Preset Item (for list display)
//==============================================================================

struct PresetItem {
    std::shared_ptr<PresetData> data;
    juce::String displayName;          // Name with rating indicator
    SkColor categoryColor;
    bool isPlayingPreview = false;
    float previewPosition = 0.0f;       // For progress bar during preview

    PresetItem() : categoryColor(0xFF4CAF50) {}

    PresetItem(std::shared_ptr<PresetData> d) : data(d) {
        updateDisplay();
    }

    void updateDisplay() {
        if (!data) return;

        displayName = data->metadata.name;

        // Add rating stars
        if (data->metadata.rating > 0) {
            displayName += " ";
            for (int i = 0; i < data->metadata.rating; ++i) {
                displayName += "★";
            }
        }

        // Add favorite indicator
        if (data->metadata.isFavorite) {
            displayName = "♥ " + displayName;
        }
    }

    SkColor getCategoryColor() const;
    bool matchesFilter(const PresetFilter& filter) const {
        return data ? filter.matches(data->metadata) : false;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetItem)
};

//==============================================================================
// Preset Library (Database)
//==============================================================================

class PresetLibrary {
public:
    PresetLibrary();
    ~PresetLibrary() = default;

    // Load/save library
    bool loadFromFile(const juce::File& file);
    bool saveToFile(const juce::File& file) const;

    // Preset management
    std::shared_ptr<PresetData> getPreset(const juce::String& id) const;
    std::vector<std::shared_ptr<PresetData>> getAllPresets() const;
    std::vector<std::shared_ptr<PresetData>> getPresetsByCategory(
        const juce::String& category) const;
    std::vector<std::shared_ptr<PresetData>> searchPresets(
        const PresetFilter& filter) const;

    // Add/remove/update
    bool addPreset(std::shared_ptr<PresetData> preset);
    bool removePreset(const juce::String& id);
    bool updatePreset(const juce::String& id, const PresetData& updated);

    // Favorites
    bool toggleFavorite(const juce::String& id);
    bool setRating(const juce::String& id, int rating);
    void incrementPlayCount(const juce::String& id);

    // Categories
    juce::StringArray getAllCategories() const;
    juce::StringArray getAllTags() const;
    juce::StringArray getAllAuthors() const;

    // Statistics
    int getTotalCount() const { return static_cast<int>(presets_.size()); }
    int getFactoryCount() const;
    int getUserCount() const;
    int getFavoriteCount() const;

    // Factory presets
    void initializeFactoryPresets();

private:
    std::map<juce::String, std::shared_ptr<PresetData>> presets_;
    juce::File libraryPath_;

    void createDefaultPresets();
    void createBassPresets();
    void createLeadPresets();
    void createPadPresets();
    void createPluckPresets();
    void createFXPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetLibrary)
};

//==============================================================================
// Preset Preview Player
//==============================================================================

class PresetPreviewPlayer {
public:
    PresetPreviewPlayer();
    ~PresetPreviewPlayer() = default;

    void setPresetToPreview(std::shared_ptr<PresetData> preset);
    void startPreview();
    void stopPreview();
    bool isPlaying() const { return isPlaying_; }
    float getPosition() const { return position_; }

    void processAudio(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiBuffer);

    void setSampleRate(double sampleRate);
    void setBlockSize(int blockSize);

    // Callbacks
    using PreviewFinishedCallback = std::function<void()>;
    void setPreviewFinishedCallback(PreviewFinishedCallback callback) {
        previewFinishedCallback_ = std::move(callback);
    }

private:
    std::shared_ptr<PresetData> currentPreset_;
    bool isPlaying_ = false;
    float position_ = 0.0f;
    double sampleRate_ = 44100.0;
    int blockSize_ = 512;

    // Preview synthesis (simple sine-based for preview)
    float previewPhase_ = 0.0f;
    int previewNoteIndex_ = 0;
    static constexpr int previewNotes[] = {60, 64, 67, 72, 76, 79}; // Cmaj7

    PreviewFinishedCallback previewFinishedCallback_;

    void generatePreviewNote(juce::AudioBuffer<float>& buffer, int noteIndex);
};

//==============================================================================
// Preset Browser Component
//==============================================================================

class SkiaPresetBrowser : public SkiaComponent {
public:
    SkiaPresetBrowser();
    ~SkiaPresetBrowser() override = default;

    void drawSkia(SkCanvas* canvas) override;
    std::vector<AIElementInfo> getInspectableElements() override;

    // Library access
    void setLibrary(std::shared_ptr<PresetLibrary> library);
    std::shared_ptr<PresetLibrary> getLibrary() const { return library_; }

    // Selection
    void setSelectedPreset(const juce::String& id);
    juce::String getSelectedPresetId() const { return selectedPresetId_; }
    std::shared_ptr<PresetData> getSelectedPreset() const;

    // Filter control
    void setSearchQuery(const juce::String& query);
    void setCategoryFilter(const juce::String& category);
    void setFavoritesOnly(bool favoritesOnly);
    void setRatingFilter(int minRating, int maxRating);

    // Sort order
    void setSortOrder(PresetSortOrder order);

    // Display options
    void setShowPreviewButton(bool show) { showPreviewButton_ = show; markDirty(); }
    void setShowFavoritesButton(bool show) { showFavoritesButton_ = show; markDirty(); }
    void setCompactMode(bool compact) { compactMode_ = compact; markDirty(); }

    // Callbacks
    using PresetSelectedCallback = std::function<void(std::shared_ptr<PresetData>)>;
    void setPresetSelectedCallback(PresetSelectedCallback callback) {
        presetSelectedCallback_ = std::move(callback);
    }

    using PresetPreviewCallback = std::function<void(const juce::String&)>;
    void setPresetPreviewCallback(PresetPreviewCallback callback) {
        presetPreviewCallback_ = std::move(callback);
    }

    // Preview control
    void startPreview(const juce::String& presetId);
    void stopPreview();
    bool isPreviewing() const { return previewPlayer_ && previewPlayer_->isPlaying(); }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void resized() override;
    void timerCallback() override;

private:
    // Data
    std::shared_ptr<PresetLibrary> library_;
    std::shared_ptr<PresetPreviewPlayer> previewPlayer_;
    std::vector<PresetItem> displayedPresets_;
    juce::String selectedPresetId_;

    // Filter state
    PresetFilter currentFilter_;
    PresetSortOrder sortOrder_ = PresetSortOrder::AlphabeticalAsc;

    // Display state
    bool compactMode_ = false;
    bool showPreviewButton_ = true;
    bool showFavoritesButton_ = true;
    int firstVisibleIndex_ = 0;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    float scrollOffset_ = 0.0f;
    float itemHeight_ = 40.0f;

    // UI layout
    SkRect searchBoxRect_;
    SkRect categoryListRect_;
    SkRect presetListRect_;
    SkRect previewButtonRect_;
    SkRect favoritesButtonRect_;
    SkRect sortButtonRect_;

    // Search
    juce::String searchQuery_;
    bool isEditingSearch_ = false;
    int cursorPosition_ = 0;

    // Categories
    int selectedCategoryIndex_ = 0;
    juce::StringArray categories_;

    // Animation
    float hoverTransition_ = 0.0f;
    float selectionTransition_ = 0.0f;

    // Callbacks
    PresetSelectedCallback presetSelectedCallback_;
    PresetPreviewCallback presetPreviewCallback_;

    // Drawing helpers
    void drawBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawSearchBox(SkCanvas* canvas, const SkRect& bounds);
    void drawCategoryList(SkCanvas* canvas, const SkRect& bounds);
    void drawPresetList(SkCanvas* canvas, const SkRect& bounds);
    void drawPresetItem(SkCanvas* canvas, const SkRect& bounds, const PresetItem& item,
                       int index, bool isSelected, bool isHovered);
    void drawPreviewButton(SkCanvas* canvas, const SkRect& bounds);
    void drawFavoritesButton(SkCanvas* canvas, const SkRect& bounds);
    void drawScrollBar(SkCanvas* canvas, const SkRect& bounds);
    void drawNoResults(SkCanvas* canvas, const SkRect& bounds);

    // Layout
    void updateLayout();
    void calculateVisibleItems();

    // Interaction
    int getPresetIndexAtPosition(float x, float y) const;
    int getCategoryIndexAtPosition(float x, float y) const;

    // Font management
    SkFont getRegularFont(float size) const;
    SkFont getBoldFont(float size) const;

    // Colors
    SkColor getBackgroundColor() const { return SkColorSetARGB(255, 18, 18, 23); }
    SkColor getSurfaceColor() const { return SkColorSetARGB(255, 25, 25, 32); }
    SkColor getBorderColor() const { return SkColorSetARGB(100, 60, 60, 70); }
    SkColor getTextColor() const { return SkColorSetARGB(255, 220, 220, 230); }
    SkColor getAccentColor() const { return SkColorSetARGB(255, 0, 200, 255); }
    SkColor getHoverColor() const { return SkColorSetARGB(50, 100, 100, 120); }
    SkColor getSelectionColor() const { return SkColorSetARGB(80, 0, 150, 200); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaPresetBrowser)
};

} // namespace zenith
