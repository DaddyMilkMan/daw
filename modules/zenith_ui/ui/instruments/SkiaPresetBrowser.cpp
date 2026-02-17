/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "SkiaPresetBrowser.h"
#include "../design-system/FontManager.h"
#include <algorithm>
#include <fstream>

namespace zenith {

namespace {
void drawUtf8Text(SkCanvas* canvas, const juce::String& text, float x, float y,
                  const SkFont& font, const SkPaint& paint) {
    const std::string utf8 = text.toStdString();
    canvas->drawSimpleText(utf8.c_str(), utf8.size(), SkTextEncoding::kUTF8, x, y,
                           font, paint);
}
} // namespace

//==============================================================================
// PresetFilter Implementation
//==============================================================================

bool PresetFilter::matches(const PresetMetadata& metadata) const {
    // Search query
    if (searchQuery.isNotEmpty()) {
        juce::String query = searchQuery.toLowerCase();
        if (!metadata.name.toLowerCase().contains(query) &&
            !metadata.author.toLowerCase().contains(query) &&
            !metadata.tags.toLowerCase().contains(query) &&
            !metadata.comment.toLowerCase().contains(query)) {
            return false;
        }
    }

    // Categories
    if (!categories.isEmpty()) {
        bool categoryMatch = false;
        for (const auto& cat : categories) {
            if (cat == PresetCategories::Favorites && metadata.isFavorite) {
                categoryMatch = true;
                break;
            } else if (cat == PresetCategories::All) {
                categoryMatch = true;
                break;
            } else if (metadata.category == cat) {
                categoryMatch = true;
                break;
            }
        }
        if (!categoryMatch) return false;
    }

    // Tags
    if (!tags.isEmpty()) {
        juce::StringArray presetTags;
        presetTags.addTokens(metadata.tags, ",;");
        bool tagMatch = false;
        for (const auto& tag : tags) {
            if (presetTags.contains(tag, true)) {
                tagMatch = true;
                break;
            }
        }
        if (!tagMatch) return false;
    }

    // Author
    if (authorFilter.isNotEmpty() && metadata.author != authorFilter) {
        return false;
    }

    // Factory/User
    if (factoryOnly && !metadata.isFactory) return false;
    if (userOnly && metadata.isFactory) return false;

    // Favorites
    if (favoritesOnly && !metadata.isFavorite) return false;

    // Rating
    if (metadata.rating < ratingMin || metadata.rating > ratingMax) {
        return false;
    }

    if (unratedOnly && metadata.rating > 0) return false;

    // Hidden
    if (metadata.isHidden) return false;

    return true;
}

//==============================================================================
// PresetItem Implementation
//==============================================================================

SkColor PresetItem::getCategoryColor() const {
    if (!data) return categoryColor;

    const auto& cat = data->metadata.category;

    if (cat == PresetCategories::Bass) return SkColorSetRGB(255, 80, 80);
    if (cat == PresetCategories::Lead) return SkColorSetRGB(80, 200, 80);
    if (cat == PresetCategories::Pad) return SkColorSetRGB(100, 150, 255);
    if (cat == PresetCategories::Pluck) return SkColorSetRGB(255, 200, 80);
    if (cat == PresetCategories::Keys) return SkColorSetRGB(200, 100, 255);
    if (cat == PresetCategories::FX) return SkColorSetRGB(255, 100, 200);
    if (cat == PresetCategories::Sequencer) return SkColorSetRGB(100, 255, 200);
    if (cat == PresetCategories::Drum) return SkColorSetRGB(255, 150, 50);
    if (cat == PresetCategories::Vocal) return SkColorSetRGB(150, 100, 255);
    if (cat == PresetCategories::Ambient) return SkColorSetRGB(100, 200, 255);
    if (cat == PresetCategories::Experimental) return SkColorSetRGB(200, 100, 200);
    if (cat == PresetCategories::Init) return SkColorSetRGB(150, 150, 150);
    if (cat == PresetCategories::Favorites) return SkColorSetRGB(255, 215, 0);

    return categoryColor;
}

//==============================================================================
// PresetLibrary Implementation
//==============================================================================

PresetLibrary::PresetLibrary() {
    // Set default library path
    libraryPath_ = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("zenithdaw")
        .getChildFile("presets");

    // Create library directory if it doesn't exist
    if (!libraryPath_.exists()) {
        libraryPath_.createDirectories();
    }

    // Load or initialize presets
    juce::File presetFile = libraryPath_.getChildFile("library.zyn");
    if (presetFile.exists()) {
        loadFromFile(presetFile);
    } else {
        initializeFactoryPresets();
    }
}

bool PresetLibrary::loadFromFile(const juce::File& file) {
    // Parse preset library file
    // Format: JSON or XML - implementing JSON

    std::unique_ptr<juce::FileInputStream> stream(file.createInputStream());
    if (!stream) return false;

    auto json = juce::JSON::parse(*stream);
    if (!json.isObject()) return false;

    auto presetsArray = json.getProperty("presets", juce::var());
    if (!presetsArray.isArray()) return false;

    for (const auto& presetVar : *presetsArray.getArray()) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();

        auto metadataObj = presetVar.getProperty("metadata", juce::var());
        if (metadataObj.isObject()) {
            preset->metadata.id = metadataObj.getProperty("id", "").toString();
            preset->metadata.name = metadataObj.getProperty("name", "").toString();
            preset->metadata.author = metadataObj.getProperty("author", "Zenith").toString();
            preset->metadata.category = metadataObj.getProperty("category", "Lead").toString();
            preset->metadata.tags = metadataObj.getProperty("tags", "").toString();
            preset->metadata.rating = metadataObj.getProperty("rating", 0);
            preset->metadata.isFavorite = metadataObj.getProperty("isFavorite", false);
            preset->metadata.isFactory = metadataObj.getProperty("isFactory", true);
        }

        auto paramsObj = presetVar.getProperty("parameters", juce::var());
        if (paramsObj.isObject()) {
            for (const auto& key : paramsObj.getProperties()->names) {
                preset->parameters.set(key, paramsObj.getProperty(key).toString());
            }
        }

        presets_[preset->metadata.id] = preset;
    }

    return true;
}

bool PresetLibrary::saveToFile(const juce::File& file) const {
    juce::var json;
    juce::DynamicObject::Ptr jsonObj = new juce::DynamicObject();
    juce::Array<juce::var> presetsArray;

    for (const auto& [id, preset] : presets_) {
        juce::DynamicObject::Ptr presetObj = new juce::DynamicObject();

        // Metadata
        juce::DynamicObject::Ptr metadataObj = new juce::DynamicObject();
        metadataObj->setProperty("id", preset->metadata.id);
        metadataObj->setProperty("name", preset->metadata.name);
        metadataObj->setProperty("author", preset->metadata.author);
        metadataObj->setProperty("category", preset->metadata.category);
        metadataObj->setProperty("tags", preset->metadata.tags);
        metadataObj->setProperty("rating", preset->metadata.rating);
        metadataObj->setProperty("isFavorite", preset->metadata.isFavorite);
        metadataObj->setProperty("isFactory", preset->metadata.isFactory);
        presetObj->setProperty("metadata", juce::var(metadataObj));

        // Parameters
        juce::DynamicObject::Ptr paramsObj = new juce::DynamicObject();
        for (const auto& param : preset->parameters) {
            paramsObj->setProperty(param.name, param.value);
        }
        presetObj->setProperty("parameters", juce::var(paramsObj));

        presetsArray.add(juce::var(presetObj));
    }

    jsonObj->setProperty("presets", presetsArray);
    jsonObj->setProperty("version", 1);

    juce::String jsonString = juce::JSON::toString(jsonObj);

    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
    if (!stream) return false;

    stream->writeText(jsonString, false, false, nullptr);
    stream->flush();

    return true;
}

std::shared_ptr<PresetData> PresetLibrary::getPreset(const juce::String& id) const {
    auto it = presets_.find(id);
    return (it != presets_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<PresetData>> PresetLibrary::getAllPresets() const {
    std::vector<std::shared_ptr<PresetData>> result;
    result.reserve(presets_.size());

    for (const auto& [id, preset] : presets_) {
        result.push_back(preset);
    }

    return result;
}

std::vector<std::shared_ptr<PresetData>> PresetLibrary::getPresetsByCategory(
    const juce::String& category) const {

    PresetFilter filter;
    filter.categories.add(category);

    return searchPresets(filter);
}

std::vector<std::shared_ptr<PresetData>> PresetLibrary::searchPresets(
    const PresetFilter& filter) const {

    std::vector<std::shared_ptr<PresetData>> result;

    for (const auto& [id, preset] : presets_) {
        if (filter.matches(preset->metadata)) {
            result.push_back(preset);
        }
    }

    return result;
}

bool PresetLibrary::addPreset(std::shared_ptr<PresetData> preset) {
    if (!preset || preset->metadata.id.isEmpty()) {
        return false;
    }

    presets_[preset->metadata.id] = preset;
    return true;
}

bool PresetLibrary::removePreset(const juce::String& id) {
    auto it = presets_.find(id);
    if (it == presets_.end()) return false;

    presets_.erase(it);
    return true;
}

bool PresetLibrary::updatePreset(const juce::String& id, const PresetData& updated) {
    auto it = presets_.find(id);
    if (it == presets_.end()) return false;

    *(it->second) = updated;
    return true;
}

bool PresetLibrary::toggleFavorite(const juce::String& id) {
    auto it = presets_.find(id);
    if (it == presets_.end()) return false;

    it->second->metadata.isFavorite = !it->second->metadata.isFavorite;
    return true;
}

bool PresetLibrary::setRating(const juce::String& id, int rating) {
    auto it = presets_.find(id);
    if (it == presets_.end()) return false;

    it->second->metadata.rating = juce::jlimit(0, 5, rating);
    return true;
}

void PresetLibrary::incrementPlayCount(const juce::String& id) {
    auto it = presets_.find(id);
    if (it != presets_.end()) {
        it->second->metadata.plays++;
    }
}

juce::StringArray PresetLibrary::getAllCategories() const {
    juce::StringArray categories = PresetCategories::getAll();
    categories.addStrings(getAllTags());
    return categories;
}

juce::StringArray PresetLibrary::getAllTags() const {
    std::set<juce::String> tagSet;

    for (const auto& [id, preset] : presets_) {
        juce::StringArray tags;
        tags.addTokens(preset->metadata.tags, ",;");
        for (const auto& tag : tags) {
            tagSet.insert(tag.trim());
        }
    }

    juce::StringArray result;
    for (const auto& tag : tagSet) {
        result.add(tag);
    }
    return result;
}

juce::StringArray PresetLibrary::getAllAuthors() const {
    std::set<juce::String> authorSet;

    for (const auto& [id, preset] : presets_) {
        authorSet.insert(preset->metadata.author);
    }

    juce::StringArray result;
    for (const auto& author : authorSet) {
        result.add(author);
    }
    return result;
}

int PresetLibrary::getFactoryCount() const {
    int count = 0;
    for (const auto& [id, preset] : presets_) {
        if (preset->metadata.isFactory) count++;
    }
    return count;
}

int PresetLibrary::getUserCount() const {
    int count = 0;
    for (const auto& [id, preset] : presets_) {
        if (!preset->metadata.isFactory) count++;
    }
    return count;
}

int PresetLibrary::getFavoriteCount() const {
    int count = 0;
    for (const auto& [id, preset] : presets_) {
        if (preset->metadata.isFavorite) count++;
    }
    return count;
}

void PresetLibrary::initializeFactoryPresets() {
    presets_.clear();

    createDefaultPresets();
    createBassPresets();
    createLeadPresets();
    createPadPresets();
    createPluckPresets();
    createFXPresets();
}

void PresetLibrary::createDefaultPresets() {
    auto initPreset = std::make_shared<PresetData>();
    initPreset->clear();
    initPreset->metadata.id = "init";
    initPreset->metadata.name = "Init";
    initPreset->metadata.author = "Zenith DAW";
    initPreset->metadata.category = PresetCategories::Init;
    initPreset->metadata.tags = "clean, basic, starting point";
    initPreset->metadata.isFactory = true;
    initPreset->metadata.rating = 0;

    presets_[initPreset->metadata.id] = initPreset;
}

void PresetLibrary::createBassPresets() {
    const char* bassPresets[] = {
        "Deep Sub", "Reese Bass", "Acid Bass", "Funk Bass",
        "Growl Bass", "Square Bass", "Saw Bass", "FM Bass"
    };

    for (int i = 0; i < 8; ++i) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();
        preset->metadata.id = "bass_" + juce::String(i);
        preset->metadata.name = bassPresets[i];
        preset->metadata.author = "Zenith DAW";
        preset->metadata.category = PresetCategories::Bass;
        preset->metadata.tags = "bass, low, sub, 808, reese";
        preset->metadata.isFactory = true;
        preset->metadata.rating = 3 + (i % 3);

        // Set typical bass parameters
        preset->parameters.set("osc1_type", "1");  // Saw
        preset->parameters.set("osc2_detune", "5.0");
        preset->parameters.set("filter_cutoff", juce::String(200 + i * 100).toStdString());
        preset->parameters.set("filter_resonance", juce::String(0.2f + i * 0.1).toStdString());
        preset->parameters.set("env1_attack", "0.01");
        preset->parameters.set("env1_decay", "0.2");
        preset->parameters.set("env1_sustain", "0.6");
        preset->parameters.set("env1_release", "0.3");

        presets_[preset->metadata.id] = preset;
    }
}

void PresetLibrary::createLeadPresets() {
    const char* leadPresets[] = {
        "Saw Lead", "Square Lead", "Supersaw Lead", "Trance Lead",
        "Detune Lead", "FM Lead", "Pluck Lead", "Vintage Lead"
    };

    for (int i = 0; i < 8; ++i) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();
        preset->metadata.id = "lead_" + juce::String(i);
        preset->metadata.name = leadPresets[i];
        preset->metadata.author = "Zenith DAW";
        preset->metadata.category = PresetCategories::Lead;
        preset->metadata.tags = "lead, synth, melody, hook";
        preset->metadata.isFactory = true;
        preset->metadata.rating = 4 + (i % 2);

        preset->parameters.set("osc1_type", juce::String(i % 3).toStdString());
        preset->parameters.set("osc2_detune", juce::String(7.0f).toStdString());
        preset->parameters.set("filter_cutoff", juce::String(3000).toStdString());
        preset->parameters.set("filter_resonance", "0.3");
        preset->parameters.set("env1_attack", "0.01");
        preset->parameters.set("env1_decay", "0.1");
        preset->parameters.set("env1_sustain", "0.8");
        preset->parameters.set("env1_release", "0.4");

        presets_[preset->metadata.id] = preset;
    }
}

void PresetLibrary::createPadPresets() {
    const char* padPresets[] = {
        "Ambient Pad", "Ethereal Pad", "Warm Pad", "Space Pad",
        "Choir Pad", "String Pad", "Drone Pad", "Evolving Pad"
    };

    for (int i = 0; i < 8; ++i) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();
        preset->metadata.id = "pad_" + juce::String(i);
        preset->metadata.name = padPresets[i];
        preset->metadata.author = "Zenith DAW";
        preset->metadata.category = PresetCategories::Pad;
        preset->metadata.tags = "pad, ambient, atmosphere, chord, drone";
        preset->metadata.isFactory = true;
        preset->metadata.rating = 5;

        preset->parameters.set("osc1_type", "0");  // Sine
        preset->parameters.set("osc2_detune", juce::String(10.0f).toStdString());
        preset->parameters.set("filter_cutoff", juce::String(4000).toStdString());
        preset->parameters.set("filter_resonance", "0.1");
        preset->parameters.set("env1_attack", "0.5");
        preset->parameters.set("env1_decay", "0.8");
        preset->parameters.set("env1_sustain", "0.7");
        preset->parameters.set("env1_release", "2.0");

        presets_[preset->metadata.id] = preset;
    }
}

void PresetLibrary::createPluckPresets() {
    const char* pluckPresets[] = {
        "Pluck Bass", "Kalimba", "Koto", "Cymbal",
        "Electric Piano", "Clavinet", "Harp", "Marimba"
    };

    for (int i = 0; i < 8; ++i) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();
        preset->metadata.id = "pluck_" + juce::String(i);
        preset->metadata.name = pluckPresets[i];
        preset->metadata.author = "Zenith DAW";
        preset->metadata.category = PresetCategories::Pluck;
        preset->metadata.tags = "pluck, percussive, string, mallet";
        preset->metadata.isFactory = true;
        preset->metadata.rating = 4;

        preset->parameters.set("osc1_type", "0");  // Sine
        preset->parameters.set("filter_cutoff", juce::String(5000).toStdString());
        preset->parameters.set("filter_resonance", "0.2");
        preset->parameters.set("env1_attack", "0.001");
        preset->parameters.set("env1_decay", "0.3");
        preset->parameters.set("env1_sustain", "0.0");
        preset->parameters.set("env1_release", "0.5");

        presets_[preset->metadata.id] = preset;
    }
}

void PresetLibrary::createFXPresets() {
    const char* fxPresets[] = {
        "Metallic", "Bell", "Glockenspiel", "Wind Chimes",
        "Siren", "Alarm", "Sci-fi", "Robot"
    };

    for (int i = 0; i < 8; ++i) {
        auto preset = std::make_shared<PresetData>();
        preset->clear();
        preset->metadata.id = "fx_" + juce::String(i);
        preset->metadata.name = fxPresets[i];
        preset->metadata.author = "Zenith DAW";
        preset->metadata.category = PresetCategories::FX;
        preset->metadata.tags = "fx, special, experimental, sfx";
        preset->metadata.isFactory = true;
        preset->metadata.rating = 3 + (i % 3);

        presets_[preset->metadata.id] = preset;
    }
}

//==============================================================================
// PresetPreviewPlayer Implementation
//==============================================================================

PresetPreviewPlayer::PresetPreviewPlayer() {
    previewPhase_ = 0.0f;
}

void PresetPreviewPlayer::setPresetToPreview(std::shared_ptr<PresetData> preset) {
    currentPreset_ = preset;
    previewNoteIndex_ = 0;
    position_ = 0.0f;
}

void PresetPreviewPlayer::startPreview() {
    isPlaying_ = true;
    previewNoteIndex_ = 0;
    position_ = 0.0f;
}

void PresetPreviewPlayer::stopPreview() {
    isPlaying_ = false;
    position_ = 0.0f;

    if (previewFinishedCallback_) {
        previewFinishedCallback_();
    }
}

void PresetPreviewPlayer::processAudio(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiBuffer) {
    if (!isPlaying_) return;

    // Generate simple preview tone based on preset
    // In a real implementation, this would use the actual synth engine

    int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) {
        // Play each note of Cmaj7 for ~1 second each
        int samplesPerNote = static_cast<int>(sampleRate_ * 0.5);
        int currentNoteIdx = static_cast<int>(position_ / samplesPerNote);

        if (currentNoteIdx >= 6) {
            stopPreview();
            break;
        }

        int midiNote = previewNotes[currentNoteIdx];

        // Generate tone with preset characteristics
        float freq = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

        if (currentPreset_) {
            // Apply filter from preset
            float cutoff = currentPreset_->parameters.getValue("filter_cutoff", "2000.0").getFloatValue();
            float resonance = currentPreset_->parameters.getValue("filter_resonance", "0.0").getFloatValue();

            // Simple sine wave
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * freq * previewPhase_);

            // Apply basic filter simulation
            float filterAmount = cutoff / 20000.0f;
            sample = sample * (1.0f - filterAmount * 0.5f);

            // Apply envelope
            float attack = currentPreset_->parameters.getValue("env1_attack", "0.01").getFloatValue();
            float decay = currentPreset_->parameters.getValue("env1_decay", "0.3").getFloatValue();
            float sustain = currentPreset_->parameters.getValue("env1_sustain", "0.7").getFloatValue();

            float notePos = (position_ - currentNoteIdx * samplesPerNote) / sampleRate_;
            float env = 0.0f;

            if (notePos < attack) {
                env = notePos / attack;
            } else if (notePos < attack + decay) {
                env = 1.0f - (notePos - attack) / decay * (1.0f - sustain);
            } else {
                env = sustain;
            }

            sample *= env * 0.3f;
        } else {
            // Default simple tone
            float sample = std::sin(2.0f * juce::MathConstants<float>::pi * freq * previewPhase_);
            sample *= 0.2f;
        }

        // Apply pan
        left[i] = sample * 0.7f;
        right[i] = sample * 0.7f;

        // Update phase
        previewPhase_ += freq / sampleRate_;
        if (previewPhase_ >= 1.0f) {
            previewPhase_ -= 1.0f;
        }

        position_ += 1.0f;
    }
}

void PresetPreviewPlayer::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void PresetPreviewPlayer::setBlockSize(int blockSize) {
    blockSize_ = blockSize;
}

//==============================================================================
// SkiaPresetBrowser Implementation
//==============================================================================

SkiaPresetBrowser::SkiaPresetBrowser() {
    categories_ = PresetCategories::getAll();

    // Create preview player
    previewPlayer_ = std::make_shared<PresetPreviewPlayer>();

    // Set up timer for animation
    startTimerHz(60);
}

void SkiaPresetBrowser::setLibrary(std::shared_ptr<PresetLibrary> library) {
    library_ = library;
    updateDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::setSelectedPreset(const juce::String& id) {
    selectedPresetId_ = id;

    // Find index
    for (size_t i = 0; i < displayedPresets_.size(); ++i) {
        if (displayedPresets_[i].data && displayedPresets_[i].data->metadata.id == id) {
            selectedIndex_ = static_cast<int>(i);
            break;
        }
    }

    markDirty();
}

std::shared_ptr<PresetData> SkiaPresetBrowser::getSelectedPreset() const {
    if (selectedPresetId_.isEmpty()) return nullptr;
    return library_ ? library_->getPreset(selectedPresetId_) : nullptr;
}

void SkiaPresetBrowser::setSearchQuery(const juce::String& query) {
    searchQuery_ = query;
    currentFilter_.searchQuery = query;
    updateDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::setCategoryFilter(const juce::String& category) {
    currentFilter_.categories.clear();
    currentFilter_.categories.add(category);
    updateDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::setFavoritesOnly(bool favoritesOnly) {
    currentFilter_.favoritesOnly = favoritesOnly;
    updateDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::setRatingFilter(int minRating, int maxRating) {
    currentFilter_.ratingMin = minRating;
    currentFilter_.ratingMax = maxRating;
    updateDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::setSortOrder(PresetSortOrder order) {
    sortOrder_ = order;
    sortDisplayedPresets();
    markDirty();
}

void SkiaPresetBrowser::updateDisplayedPresets() {
    if (!library_) return;

    auto presets = library_->searchPresets(currentFilter_);

    // Convert to PresetItems
    displayedPresets_.clear();
    for (const auto& preset : presets) {
        displayedPresets_.push_back(PresetItem(preset));
    }

    sortDisplayedPresets();
}

void SkiaPresetBrowser::sortDisplayedPresets() {
    auto compareFunc = [this](const PresetItem& a, const PresetItem& b) {
        switch (sortOrder_) {
            case PresetSortOrder::AlphabeticalAsc:
                return a.displayName.compareIgnoreCase(b.displayName) < 0;
            case PresetSortOrder::AlphabeticalDesc:
                return a.displayName.compareIgnoreCase(b.displayName) > 0;
            case PresetSortOrder::NewestFirst:
                if (a.data->metadata.creationTime != b.data->metadata.creationTime)
                    return a.data->metadata.creationTime > b.data->metadata.creationTime;
                break;
            case PresetSortOrder::OldestFirst:
                if (a.data->metadata.creationTime != b.data->metadata.creationTime)
                    return a.data->metadata.creationTime < b.data->metadata.creationTime;
                break;
            case PresetSortOrder::MostPlayed:
                return a.data->metadata.plays > b.data->metadata.plays;
            case PresetSortOrder::LeastPlayed:
                return a.data->metadata.plays < b.data->metadata.plays;
            case PresetSortOrder::HighestRated:
                return a.data->metadata.rating > b.data->metadata.rating;
            case PresetSortOrder::LowestRated:
                return a.data->metadata.rating < b.data->metadata.rating;
            default:
                break;
        }
        return false;
    };

    std::sort(displayedPresets_.begin(), displayedPresets_.end(), compareFunc);
}

void SkiaPresetBrowser::startPreview(const juce::String& presetId) {
    if (!previewPlayer_) return;

    auto preset = library_ ? library_->getPreset(presetId) : nullptr;
    if (!preset) return;

    // Stop current preview
    previewPlayer_->stopPreview();

    // Start new preview
    previewPlayer_->setPresetToPreview(preset);
    previewPlayer_->startPreview();

    // Mark which preset is playing
    for (auto& item : displayedPresets_) {
        if (item.data && item.data->metadata.id == presetId) {
            item.isPlayingPreview = true;
        } else {
            item.isPlayingPreview = false;
        }
    }

    if (presetPreviewCallback_) {
        presetPreviewCallback_(presetId);
    }

    markDirty();
}

void SkiaPresetBrowser::stopPreview() {
    if (previewPlayer_) {
        previewPlayer_->stopPreview();
    }

    for (auto& item : displayedPresets_) {
        item.isPlayingPreview = false;
    }

    markDirty();
}

void SkiaPresetBrowser::drawSkia(SkCanvas* canvas) {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    drawBackground(canvas, bounds);
    drawSearchBox(canvas, searchBoxRect_);
    drawCategoryList(canvas, categoryListRect_);
    drawPresetList(canvas, presetListRect_);
    drawPreviewButton(canvas, previewButtonRect_);
    drawFavoritesButton(canvas, favoritesButtonRect_);
}

void SkiaPresetBrowser::drawBackground(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint bgPaint;
    bgPaint.setColor(getBackgroundColor());
    canvas->drawRect(bounds, bgPaint);

    // Draw subtle gradient
    SkColor colors[2] = {getBackgroundColor(), getSurfaceColor()};
    SkPoint points[2] = {{bounds.fLeft, bounds.fTop}, {bounds.fLeft, bounds.fBottom}};
    bgPaint.setShader(SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, bgPaint);
}

void SkiaPresetBrowser::drawSearchBox(SkCanvas* canvas, const SkRect& bounds) {
    // Draw background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(getSurfaceColor());
    bgPaint.setStyle(SkPaint::kStroke_Style);
    bgPaint.setStrokeWidth(1.0f);
    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 4, 4), bgPaint);

    // Draw search icon
    SkPaint iconPaint;
    iconPaint.setAntiAlias(true);
    iconPaint.setColor(SkColorSetARGB(150, 150, 150, 150));

    // Simple search icon (magnifying glass)
    float iconX = bounds.fLeft + 10;
    float iconY = bounds.fCenterY;
    float iconSize = 12.0f;

    SkPath searchIcon;
    searchIcon.addCircle(iconX, iconY - iconSize * 0.3f, iconSize * 0.4f);
    searchIcon.moveTo(iconX + iconSize * 0.3f, iconY + iconSize * 0.3f);
    searchIcon.lineTo(iconX + iconSize * 0.7f, iconY + iconSize * 0.8f);
    canvas->drawPath(searchIcon, iconPaint);

    // Draw search text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(getTextColor());
    const SkFont textFont = getRegularFont(12.0f);

    float textX = bounds.fLeft + 30;

    if (searchQuery_.isEmpty() && !isEditingSearch_) {
        // Draw placeholder
        textPaint.setColor(SkColorSetARGB(100, 150, 150, 150));
        drawUtf8Text(canvas, "Search...", textX, bounds.fCenterY + 4.0f, textFont,
                     textPaint);
    } else {
        // Draw actual text with cursor
        const juce::String displayText = searchQuery_;
        drawUtf8Text(canvas, displayText, textX, bounds.fCenterY + 4.0f, textFont,
                     textPaint);

        // Draw cursor
        if (isEditingSearch_) {
            float cursorX = textX + displayText.length() * 7.0f; // Approximate
            SkPaint cursorPaint;
            cursorPaint.setStyle(SkPaint::kStroke_Style);
            cursorPaint.setColor(getAccentColor());
            cursorPaint.setStrokeWidth(1.0f);
            canvas->drawLine(cursorX, bounds.fTop + 8, cursorX, bounds.fBottom - 8, cursorPaint);
        }
    }

    // Draw clear button if has text
    if (!searchQuery_.isEmpty()) {
        float clearX = bounds.fRight - 15;
        float clearY = bounds.fCenterY;
        float clearSize = 10.0f;

        SkPaint clearPaint;
        clearPaint.setAntiAlias(true);
        clearPaint.setStyle(SkPaint::kStroke_Style);
        clearPaint.setColor(SkColorSetARGB(150, 100, 100));
        clearPaint.setStrokeWidth(1.5f);

        SkPath clearIcon;
        clearIcon.moveTo(clearX - clearSize * 0.4f, clearY - clearSize * 0.4f);
        clearIcon.lineTo(clearX + clearSize * 0.4f, clearY + clearSize * 0.4f);
        clearIcon.moveTo(clearX + clearSize * 0.4f, clearY - clearSize * 0.4f);
        clearIcon.lineTo(clearX - clearSize * 0.4f, clearY + clearSize * 0.4f);
        canvas->drawPath(clearIcon, clearPaint);
    }
}

void SkiaPresetBrowser::drawCategoryList(SkCanvas* canvas, const SkRect& bounds) {
    // Draw category pills
    float x = bounds.fLeft;
    float y = bounds.fTop;
    float maxWidth = bounds.width();
    float currentRowWidth = 0;
    float rowHeight = 28.0f;

    SkFont font = getRegularFont(11.0f);

    for (int i = 0; i < categories_.size(); ++i) {
        const auto& cat = categories_[i];

        // Measure text
        float textWidth = cat.length() * 7.0f; // Approximate
        float pillWidth = textWidth + 20.0f;
        float pillHeight = 22.0f;

        // Check if we need to wrap to next row
        if (currentRowWidth + pillWidth > maxWidth && currentRowWidth > 0) {
            x = bounds.fLeft;
            y += rowHeight;
            currentRowWidth = 0;
        }

        // Draw pill background
        bool isSelected = (i == selectedCategoryIndex_);
        bool isHovered = (i == hoveredIndex_); // Would need hover tracking

        SkPaint pillPaint;
        pillPaint.setAntiAlias(true);

        if (isSelected) {
            pillPaint.setColor(getAccentColor());
        } else {
            pillPaint.setColor(SkColorSetARGB(80, 60, 70, 80));
        }

        SkRRect pillRect = SkRRect::MakeRectXY(
            SkRect::MakeXYWH(x, y, pillWidth, pillHeight),
            11.0f, 11.0f
        );
        canvas->drawRRect(pillRect, pillPaint);

        // Draw text
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(isSelected ? SK_ColorWHITE : getTextColor());
        drawUtf8Text(canvas, cat, x + 10.0f, y + 15.0f, font, textPaint);

        x += pillWidth + 8.0f;
        currentRowWidth += pillWidth + 8.0f;
    }
}

void SkiaPresetBrowser::drawPresetList(SkCanvas* canvas, const SkRect& bounds) {
    if (displayedPresets_.empty()) {
        drawNoResults(canvas, bounds);
        return;
    }

    // Determine visible range
    int numVisible = static_cast<int>(bounds.height() / itemHeight_) + 1;
    int startIndex = static_cast<int>(scrollOffset_ / itemHeight_);
    int endIndex = juce::jmin(startIndex + numVisible, static_cast<int>(displayedPresets_.size()));

    for (int i = startIndex; i < endIndex; ++i) {
        if (i < 0 || i >= static_cast<int>(displayedPresets_.size())) continue;

        const auto& item = displayedPresets_[i];
        bool isSelected = (i == selectedIndex_);
        bool isHovered = (i == hoveredIndex_);

        float y = bounds.fTop + (i - startIndex) * itemHeight_ - scrollOffset_ + (scrollOffset_ - startIndex * itemHeight_);

        SkRect itemBounds = SkRect::MakeXYWH(
            bounds.fLeft,
            y,
            bounds.width(),
            itemHeight_ - 2.0f
        );

        drawPresetItem(canvas, itemBounds, item, i, isSelected, isHovered);
    }
}

void SkiaPresetBrowser::drawPresetItem(SkCanvas* canvas, const SkRect& bounds,
                                       const PresetItem& item, int index,
                                       bool isSelected, bool isHovered) {
    // Draw background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (isSelected) {
        bgPaint.setColor(getSelectionColor());
    } else if (isHovered) {
        bgPaint.setColor(getHoverColor());
    } else {
        bgPaint.setColor(SkColorSetARGB(0, 0, 0, 0));
    }

    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 4, 4), bgPaint);

    // Draw category indicator (left stripe)
    SkPaint stripePaint;
    stripePaint.setAntiAlias(true);
    stripePaint.setColor(item.getCategoryColor());

    SkRRect stripeRect = SkRRect::MakeRectXY(
        SkRect::MakeXYWH(bounds.fLeft + 1, bounds.fTop + 4, 3.0f, bounds.height() - 8.0f),
        1.5f, 1.5f
    );
    canvas->drawRRect(stripeRect, stripePaint);

    // Draw preset name
    SkPaint namePaint;
    namePaint.setAntiAlias(true);
    namePaint.setColor(getTextColor());

    float nameX = bounds.fLeft + 15;
    float nameY = bounds.fCenterY + 4;

    // canvas->drawText(item.displayName.toUTF8(), item.displayName.length(), nameX, nameY, namePaint);

    // Draw favorite star if favorited
    if (item.data && item.data->metadata.isFavorite) {
        SkPaint starPaint;
        starPaint.setAntiAlias(true);
        starPaint.setColor(SkColorSetARGB(255, 215, 0));

        float starX = bounds.fRight - 20;
        float starY = bounds.fCenterY;

        // Draw star shape
        SkPath starPath;
        // Simple star shape
        canvas->drawPath(starPath, starPaint);
    }

    // Draw rating stars
    if (item.data && item.data->metadata.rating > 0) {
        SkPaint ratingPaint;
        ratingPaint.setAntiAlias(true);
        ratingPaint.setColor(SkColorSetARGB(255, 200, 100));

        float ratingX = bounds.fRight - 50;
        float starSize = 8.0f;
        for (int r = 0; r < 5; ++r) {
            if (r < item.data->metadata.rating) {
                canvas->drawCircle(ratingX + r * 10, bounds.fCenterY, starSize * 0.5f, ratingPaint);
            } else {
                ratingPaint.setColor(SkColorSetARGB(50, 100, 100));
                canvas->drawCircle(ratingX + r * 10, bounds.fCenterY, starSize * 0.5f, ratingPaint);
            }
        }
    }

    // Draw preview progress
    if (item.isPlayingPreview) {
        SkPaint progressPaint;
        progressPaint.setStyle(SkPaint::kStroke_Style);
        progressPaint.setColor(getAccentColor());
        progressPaint.setStrokeWidth(2.0f);

        SkRect progressRect = SkRect::MakeXYWH(
            bounds.fLeft,
            bounds.fBottom - 2.0f,
            bounds.width() * item.previewPosition,
            2.0f
        );
        canvas->drawRect(progressRect, progressPaint);
    }
}

void SkiaPresetBrowser::drawPreviewButton(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f);
    paint.setColor(isPreviewing() ? getAccentColor() : getBorderColor());

    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 4, 4), paint);

    // Draw speaker icon or stop icon
    if (isPreviewing()) {
        // Stop icon (square)
        SkPath stopIcon;
        stopIcon.addRect(SkRect::MakeXYWH(
            bounds.centerX() - 4, bounds.centerY() - 4,
            8, 8
        ));
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(stopIcon, paint);
    } else {
        // Play/speaker icon
        SkPath playIcon;
        playIcon.moveTo(bounds.centerX() - 6, bounds.centerY() - 3);
        playIcon.lineTo(bounds.centerX(), bounds.centerY() - 6);
        playIcon.lineTo(bounds.centerX(), bounds.centerY() + 6);
        playIcon.lineTo(bounds.centerX() - 6, bounds.centerY() + 3);
        playIcon.close();

        playIcon.moveTo(bounds.centerX() + 2, bounds.centerY() - 6);
        playIcon.lineTo(bounds.centerX() + 10, bounds.centerY());
        playIcon.lineTo(bounds.centerX() + 2, bounds.centerY() + 6);
        playIcon.close();

        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(playIcon, paint);
    }
}

void SkiaPresetBrowser::drawFavoritesButton(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f);

    bool isActive = currentFilter_.favoritesOnly;
    paint.setColor(isActive ? SkColorSetARGB(255, 215, 0) : getBorderColor());

    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 4, 4), paint);

    // Draw heart icon
    SkPath heartPath;
    // Heart shape
    heartPath.moveTo(bounds.centerX(), bounds.fBottom - 5);
    heartPath.cubicTo(bounds.centerX(), bounds.fBottom - 12,
                     bounds.centerX() - 6, bounds.fBottom - 12);
    heartPath.lineTo(bounds.centerX() - 9, bounds.fBottom - 8);
    heartPath.lineTo(bounds.centerX(), bounds.fBottom - 2);
    heartPath.lineTo(bounds.centerX() + 9, bounds.fBottom - 8);
    heartPath.lineTo(bounds.centerX() + 6, bounds.fBottom - 12);
    heartPath.cubicTo(bounds.centerX() + 6, bounds.fBottom - 12,
                     bounds.centerX(), bounds.fBottom - 5);

    paint.setStyle(isActive ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
    canvas->drawPath(heartPath, paint);
}

void SkiaPresetBrowser::drawScrollBar(SkCanvas* canvas, const SkRect& bounds) {
    // Calculate scrollbar geometry
    float totalHeight = displayedPresets_.size() * itemHeight_;
    float viewHeight = presetListRect_.height();
    float thumbHeight = viewHeight / totalHeight * viewHeight;
    thumbHeight = juce::jmax(30.0f, thumbHeight);

    float thumbY = presetListRect_.fTop + (scrollOffset_ / totalHeight) * viewHeight;

    // Draw track
    SkPaint trackPaint;
    trackPaint.setColor(SkColorSetARGB(30, 60, 60, 70));
    canvas->drawRect(SkRect::MakeXYWH(
        bounds.fRight - 6, presetListRect_.fTop,
        4.0f, presetListRect_.height()
    ), trackPaint);

    // Draw thumb
    SkPaint thumbPaint;
    thumbPaint.setColor(SkColorSetARGB(150, 100, 100));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(
        SkRect::MakeXYWH(
            bounds.fRight - 6, thumbY,
            4.0f, thumbHeight
        ),
        2.0f, 2.0f
    ), thumbPaint);
}

void SkiaPresetBrowser::drawNoResults(SkCanvas* canvas, const SkRect& bounds) {
    SkPaint textPaint;
    textPaint.setColor(getTextColor());
    // canvas->drawText("No presets found", bounds.centerX() - 50, bounds.centerY(), textPaint);

    // Draw helpful message
    juce::String message = "Try adjusting your search or filters";
    // canvas->drawText(message.toUTF8(), message.length(), bounds.centerX() - 70, bounds.centerY() + 20, textPaint);
}

void SkiaPresetBrowser::mouseDown(const juce::MouseEvent& e) {
    float x = e.position.x;
    float y = e.position.y;

    // Check search box
    if (searchBoxRect_.contains(x, y)) {
        isEditingSearch_ = true;
        markDirty();
        return;
    }

    // Check preset list
    if (presetListRect_.contains(x, y)) {
        int index = getPresetIndexAtPosition(x, y);
        if (index >= 0 && index < static_cast<int>(displayedPresets_.size())) {
            selectedIndex_ = index;
            selectedPresetId_ = displayedPresets_[index].data->metadata.id;

            if (presetSelectedCallback_) {
                presetSelectedCallback_(displayedPresets_[index].data);
            }

            // Double click to load
            if (e.mods.isLeftButtonDown() && e.getNumberOfClicks() > 1) {
                // Load preset
            }

            markDirty();
        }
    }

    // Check categories
    if (categoryListRect_.contains(x, y)) {
        int index = getCategoryIndexAtPosition(x, y);
        if (index >= 0 && index < categories_.size()) {
            selectedCategoryIndex_ = index;
            setCategoryFilter(categories_[index]);
        }
    }

    // Check preview button
    if (previewButtonRect_.contains(x, y)) {
        if (isPreviewing()) {
            stopPreview();
        } else if (!selectedPresetId_.isEmpty()) {
            startPreview(selectedPresetId_);
        }
    }

    // Check favorites button
    if (favoritesButtonRect_.contains(x, y)) {
        setFavoritesOnly(!currentFilter_.favoritesOnly);
    }
}

void SkiaPresetBrowser::mouseDrag(const juce::MouseEvent& e) {
    if (presetListRect_.contains(e.position.x, e.position.y)) {
        // Handle scroll
        int newIndex = getPresetIndexAtPosition(e.position.x, e.position.y);
        if (newIndex >= 0) {
            selectedIndex_ = newIndex;
            selectedPresetId_ = displayedPresets_[newIndex].data->metadata.id;
            markDirty();
        }
    }
}

void SkiaPresetBrowser::mouseUp(const juce::MouseEvent& e) {
    isEditingSearch_ = false;
}

void SkiaPresetBrowser::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    // Handle scroll
    float scrollAmount = wheel.deltaY * itemHeight_ * 0.5f;

    scrollOffset_ += scrollAmount;
    scrollOffset_ = juce::jlimit(
        0.0f,
        juce::jmax(0.0f, displayedPresets_.size() * itemHeight_ - presetListRect_.height()),
        scrollOffset_
    );

    markDirty();
}

bool SkiaPresetBrowser::keyPressed(const juce::KeyPress& key) {
    if (isEditingSearch_) {
        if (key.getKeyCode() == juce::KeyPress::escapeKey) {
            isEditingSearch_ = false;
            markDirty();
            return true;
        } else if (key.getKeyCode() == juce::KeyPress::returnKey) {
            isEditingSearch_ = false;
            markDirty();
            return true;
        } else if (key.getTextCharacter() != 0) {
            searchQuery_ += key.getTextCharacter();
            setSearchQuery(searchQuery_);
            return true;
        } else if (key.isKeyCode(juce::KeyPress::backspaceKey)) {
            if (searchQuery_.isNotEmpty()) {
                searchQuery_ = searchQuery_.dropLastCharacters(1);
                setSearchQuery(searchQuery_);
            }
            return true;
        }
    } else {
        // Keyboard navigation
        if (key.getKeyCode() == juce::KeyPress::upKey) {
            if (selectedIndex_ > 0) {
                selectedIndex_--;
                selectedPresetId_ = displayedPresets_[selectedIndex_].data->metadata.id;
                markDirty();
            }
            return true;
        } else if (key.getKeyCode() == juce::KeyPress::downKey) {
            if (selectedIndex_ < static_cast<int>(displayedPresets_.size()) - 1) {
                selectedIndex_++;
                selectedPresetId_ = displayedPresets_[selectedIndex_].data->metadata.id;
                markDirty();
            }
            return true;
        } else if (key.getKeyCode() == juce::KeyPress::returnKey) {
            // Load selected preset
            if (presetSelectedCallback_ && selectedIndex_ >= 0) {
                presetSelectedCallback_(displayedPresets_[selectedIndex_].data);
            }
            return true;
        }
    }

    return false;
}

void SkiaPresetBrowser::resized() {
    updateLayout();
}

void SkiaPresetBrowser::timerCallback() {
    // Update animation states
    if (isPreviewing() && previewPlayer_) {
        // Update progress for currently previewing preset
        for (auto& item : displayedPresets_) {
            if (item.isPlayingPreview) {
                item.previewPosition = previewPlayer_->getPosition();
            }
        }
        markDirty();
    }

    // Update hover animation
    if (hoverTransition_ > 0.0f) {
        hoverTransition_ -= 0.1f;
        if (hoverTransition_ < 0.0f) hoverTransition_ = 0.0f;
        markDirty();
    }

    // Update selection animation
    if (selectionTransition_ > 0.0f) {
        selectionTransition_ -= 0.1f;
        if (selectionTransition_ < 0.0f) selectionTransition_ = 0.0f;
        markDirty();
    }
}

void SkiaPresetBrowser::updateLayout() {
    SkRect bounds = SkRect::MakeXYWH(0, 0, getWidth(), getHeight());

    // Search box
    searchBoxRect_ = SkRect::MakeXYWH(
        bounds.fLeft + 10, bounds.fTop + 10,
        bounds.width() - 20, 30.0f
    );

    // Categories (below search)
    categoryListRect_ = SkRect::MakeXYWH(
        bounds.fLeft + 10, searchBoxRect_.fBottom + 10,
        bounds.width() - 20, 70.0f
    );

    // Buttons (right side)
    float buttonY = bounds.fTop + 10;
    float buttonSize = 30.0f;

    previewButtonRect_ = SkRect::MakeXYWH(
        bounds.fRight - buttonSize - 10, buttonY,
        buttonSize, buttonSize
    );

    favoritesButtonRect_ = SkRect::MakeXYWH(
        previewButtonRect_.fLeft - buttonSize - 5, buttonY,
        buttonSize, buttonSize
    );

    // Preset list (main area)
    presetListRect_ = SkRect::MakeXYWH(
        bounds.fLeft + 10, categoryListRect_.fBottom + 10,
        bounds.width() - 20, bounds.fBottom - categoryListRect_.fBottom - 20
    );

    itemHeight_ = compactMode_ ? 30.0f : 40.0f;
}

int SkiaPresetBrowser::getPresetIndexAtPosition(float x, float y) const {
    if (!presetListRect_.contains(x, y)) return -1;

    float relativeY = y - presetListRect_.fTop + scrollOffset_;
    int index = static_cast<int>(relativeY / itemHeight_);

    if (index >= 0 && index < static_cast<int>(displayedPresets_.size())) {
        return index;
    }

    return -1;
}

int SkiaPresetBrowser::getCategoryIndexAtPosition(float x, float y) const {
    if (!categoryListRect_.contains(x, y)) return -1;

    // Simple row-based calculation
    float rowHeight = 28.0f;
    int row = static_cast<int>((y - categoryListRect_.fTop) / rowHeight);

    // Find category in this row
    float currentX = categoryListRect_.fLeft;
    float maxWidth = categoryListRect_.width();
    int categoryIdx = 0;

    for (int i = 0; i < categories_.size(); ++i) {
        float textWidth = categories_[i].length() * 7.0f + 28.0f;
        if (currentX + textWidth > maxWidth) {
            currentX = categoryListRect_.fLeft;
            row++;
        }

        if (row == static_cast<int>((y - categoryListRect_.fTop) / rowHeight)) {
            if (x >= currentX && x <= currentX + textWidth) {
                return i;
            }
        }

        currentX += textWidth + 8.0f;
    }

    return -1;
}

SkFont SkiaPresetBrowser::getRegularFont(float size) const {
    return design::FontManager::getInstance().getUIFont(size, design::FontWeight::Regular);
}

SkFont SkiaPresetBrowser::getBoldFont(float size) const {
    return design::FontManager::getInstance().getUIFont(size, design::FontWeight::Bold);
}

} // namespace zenith
