/*
  ==============================================================================

    BrowserModel.h
    Created: 2025-12-05
    Author:  Zenith DAW

    The simplified data model for the Universal Media Browser.
     Aggregates:
     - File System (Samples, Projects)
     - Internal Instruments
     - VST3/AU Plugins
     - Presets
     
    Features:
     - Favorites/Collections
     - Tagging System
     - Browser History
     - Category Filtering

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "BrowserData.h"
#include <deque>
#include <set>

namespace zenith {

class InstrumentRegistry;
class PluginHost; 

class BrowserModel : public juce::ChangeBroadcaster
{
public:
    BrowserModel(InstrumentRegistry& instrumentRegistry, PluginHost& pluginHost);
    ~BrowserModel();

    //==============================================================================
    // Initialization & Refresh
    //==============================================================================
    
    void refresh();
    void setUserLibraryPaths(const juce::StringArray& paths);
    juce::StringArray getUserLibraryPaths() const { return userLibraryPaths_; }

    //==============================================================================
    // Accessors
    //==============================================================================

    std::shared_ptr<BrowserItem> getRoot() const { return rootItem; }
    std::shared_ptr<BrowserItem> getFavoritesNode() const { return favoritesNode_; }
    
    std::vector<std::shared_ptr<BrowserItem>> search(const juce::String& queryText);
    std::vector<std::shared_ptr<BrowserItem>> getItemsByType(BrowserItemType type);

    //==============================================================================
    // Favorites / Collections
    //==============================================================================
    
    void addToFavorites(std::shared_ptr<BrowserItem> item);
    void removeFromFavorites(std::shared_ptr<BrowserItem> item);
    bool isFavorite(const juce::String& itemId) const;
    std::vector<std::shared_ptr<BrowserItem>> getFavorites() const;
    
    void saveFavorites();
    void loadFavorites();

    //==============================================================================
    // Tagging System
    //==============================================================================
    
    void addTagToItem(std::shared_ptr<BrowserItem> item, const juce::String& tag);
    void removeTagFromItem(std::shared_ptr<BrowserItem> item, const juce::String& tag);
    std::vector<juce::String> getItemTags(std::shared_ptr<BrowserItem> item) const;
    
    std::vector<std::shared_ptr<BrowserItem>> getItemsByTag(const juce::String& tag);
    std::set<juce::String> getAllTags() const { return allTags_; }

    // Tag Colors
    void setTagColor(const juce::String& tag, const juce::String& hexColor);
    juce::String getTagColor(const juce::String& tag) const;
    
    void saveTags();
    void loadTags();

    //==============================================================================
    // Ratings System
    //==============================================================================

    void setItemRating(std::shared_ptr<BrowserItem> item, int rating);
    int getItemRating(const juce::String& itemId) const;
    void saveRatings();
    void loadRatings();

    //==============================================================================
    // Browser History
    //==============================================================================
    
    void pushHistory(std::shared_ptr<BrowserItem> folder);
    std::shared_ptr<BrowserItem> popHistory();
    std::shared_ptr<BrowserItem> peekHistory() const;
    bool canGoBack() const { return !history_.empty(); }
    void clearHistory() { history_.clear(); }

    // Recent Items
    void addToRecent(std::shared_ptr<BrowserItem> item);
    std::vector<std::shared_ptr<BrowserItem>> getRecentItems() const;
    void clearRecent();
    void saveRecent();
    void loadRecent();
    
    //==============================================================================
    // Category Filtering
    //==============================================================================
    
    void setActiveFilter(BrowserItemType typeFilter);
    void clearFilter();
    BrowserItemType getActiveFilter() const { return activeFilter_; }
    bool hasActiveFilter() const { return activeFilter_ != BrowserItemType::Unknown; }
    
    std::vector<std::shared_ptr<BrowserItem>> getFilteredItems(
        const std::vector<std::shared_ptr<BrowserItem>>& items) const;

    //==============================================================================
    // Async Worker Updates
    //==============================================================================
    
    void addScannedItem(std::shared_ptr<BrowserItem> item);
    bool isScanning() const { return isScanning_; }

private:
    InstrumentRegistry& instrumentRegistry_;
    PluginHost& pluginHost_;

    std::shared_ptr<BrowserItem> rootItem;
    
    // Categorized Roots (Children of rootItem)
    std::shared_ptr<BrowserItem> favoritesNode_;
    std::shared_ptr<BrowserItem> instrumentsNode;
    std::shared_ptr<BrowserItem> pluginsNode;
    std::shared_ptr<BrowserItem> userLibraryNode;
    std::shared_ptr<BrowserItem> presetsNode;

    // Search Cache
    std::vector<std::shared_ptr<BrowserItem>> allIndexableItems_;

    bool isScanning_ = false;
    juce::StringArray userLibraryPaths_;
    
    // Favorites
    std::set<juce::String> favoriteIds_;
    
    // Tags (itemId -> tags)
    std::map<juce::String, std::vector<juce::String>> itemTags_;
    std::set<juce::String> allTags_;
    std::map<juce::String, juce::String> tagColors_;
    
    // Ratings
    std::map<juce::String, int> itemRatings_;

    // Recent Items
    std::deque<juce::String> recentItemIds_;
    static constexpr size_t maxRecentSize_ = 50;
    
    // History stack
    std::deque<std::shared_ptr<BrowserItem>> history_;
    static constexpr size_t maxHistorySize_ = 20;
    
    // Active filter
    BrowserItemType activeFilter_ = BrowserItemType::Unknown;

    // Internal population helpers
    void buildStructure();
    void populateInternalInstruments();
    void populatePlugins(); 
    void populateUserLibrary();
    
    // Persistence helpers
    juce::File getFavoritesFile() const;
    juce::File getTagsFile() const;

    juce::WeakReference<BrowserModel>::Master masterReference;
    friend class juce::WeakReference<BrowserModel>;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserModel)
};

} // namespace zenith
