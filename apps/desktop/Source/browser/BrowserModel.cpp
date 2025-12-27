/*
  ==============================================================================

    BrowserModel.cpp
    Created: 2025-12-05
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserModel.h"
#include "../engine/PluginHost.h"
#include "../engine/ZenithLogger.h"
#include "../instruments/InstrumentRegistry.h"
#include "FuzzyMatcher.h"


namespace zenith {

BrowserModel::BrowserModel(InstrumentRegistry &registry, PluginHost &host)
    : instrumentRegistry_(registry), pluginHost_(host) {
  // Default paths
  // userLibraryPaths_.add(
  //    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
  //        .getFullPathName());
  // userLibraryPaths_.add(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName());

  refresh();
}

BrowserModel::~BrowserModel() {}

void BrowserModel::refresh() {
  buildStructure();

  populateInternalInstruments();
  populatePlugins();
  populateUserLibrary();

  // Notify listeners (UI) that the tree changed
  sendChangeMessage();
}

void BrowserModel::setUserLibraryPaths(const juce::StringArray &paths) {
  userLibraryPaths_ = paths;
  refresh();
}

//==============================================================================
// Structure Building
//==============================================================================

void BrowserModel::buildStructure() {
  rootItem =
      std::make_shared<BrowserItem>("root", "Root", BrowserItemType::Folder);
  allIndexableItems_.clear();

  // 0. Favorites (always at top)
  favoritesNode_ = std::make_shared<BrowserItem>(
      "favorites_root", "* Favorites", BrowserItemType::Folder);
  rootItem->addChild(favoritesNode_);

  // 1. Internal Instruments Group
  instrumentsNode = std::make_shared<BrowserItem>(
      "instruments_root", "Instruments", BrowserItemType::Folder);
  rootItem->addChild(instrumentsNode);

  // 2. Plugins Group
  pluginsNode = std::make_shared<BrowserItem>("plugins_root", "Plugins",
                                              BrowserItemType::Folder);
  pluginsNode->metadata.description = "VST3 and AU Plugins";
  rootItem->addChild(pluginsNode);

  // 3. User Library
  userLibraryNode = std::make_shared<BrowserItem>(
      "library_root", "User Library", BrowserItemType::Folder);
  rootItem->addChild(userLibraryNode);

  // 4. Presets
  presetsNode = std::make_shared<BrowserItem>("presets_root", "Presets",
                                              BrowserItemType::Folder);
  rootItem->addChild(presetsNode);

  // Load saved favorites, tags, ratings, and recent items
  loadFavorites();
  loadTags();
  loadRatings();
  loadRecent();
}

void BrowserModel::populateInternalInstruments() {
  auto list = instrumentRegistry_.getInstrumentList();

  for (const auto &item : list) {
    juce::String id = item.getProperty("id", "").toString();
    juce::String name = item.getProperty("name", "Unknown").toString();
    juce::String category =
        item.getProperty("category", "Uncategorized").toString();

    auto browserItem =
        std::make_shared<BrowserItem>(id, name, BrowserItemType::Instrument);
    browserItem->metadata.category = category;
    browserItem->metadata.author = "Zenith DAW";
    browserItem->metadata.format = "Native";
    browserItem->metadata.isInstrument = true;
    // Use explicit bool variable to avoid implicit string/bool conversion
    bool isFav = isFavorite(id);
    browserItem->isFavorite = isFav;

    instrumentsNode->addChild(browserItem);
    allIndexableItems_.push_back(browserItem);

    // Add to favorites node if it's a favorite
    if (browserItem->isFavorite) {
      favoritesNode_->addChild(browserItem);
    }
  }
}

void BrowserModel::populatePlugins() {
  auto &knownPlugins = pluginHost_.getKnownPlugins();

  // Group by Category -> Manufacturer
  std::map<juce::String, std::shared_ptr<BrowserItem>> categoryNodes;

  for (const auto &desc : knownPlugins.getTypes()) {
    juce::String category = desc.category;
    if (category.isEmpty())
      category = "Uncategorized";

    // Ensure category exists
    if (categoryNodes.find(category) == categoryNodes.end()) {
      auto catNode = std::make_shared<BrowserItem>("cat_" + category, category,
                                                   BrowserItemType::Folder);
      pluginsNode->addChild(catNode);
      categoryNodes[category] = catNode;
    }

    // Create Plugin Item
    auto pluginItem = std::make_shared<BrowserItem>(
        desc.fileOrIdentifier, desc.name, BrowserItemType::Plugin);
    pluginItem->metadata.author = desc.manufacturerName;
    // Version is stored as string, not int
    pluginItem->metadata.version = desc.version;
    pluginItem->metadata.format = desc.pluginFormatName;
    pluginItem->metadata.category = category;
    pluginItem->metadata.isInstrument = desc.isInstrument;

    // Use explicit bool variable for clarity
    bool isFav = isFavorite(desc.fileOrIdentifier);
    pluginItem->isFavorite = isFav;

    // Add to category
    auto it = categoryNodes.find(category);
    if (it != categoryNodes.end()) {
      it->second->addChild(pluginItem);
    }

    allIndexableItems_.push_back(pluginItem);

    if (pluginItem->isFavorite) {
      favoritesNode_->addChild(pluginItem);
    }
  }
}

void BrowserModel::populateUserLibrary() {
  // Real file scanning
  juce::StringArray supportedExtensions = {".wav", ".aif", ".aiff", ".mp3",
                                           ".ogg", ".mid", ".midi"};

  for (const auto &path : userLibraryPaths_) {
    juce::File dir(path);
    if (dir.isDirectory()) {
      auto dirNode = std::make_shared<BrowserItem>(path, dir.getFileName(),
                                                   BrowserItemType::Folder);
      userLibraryNode->addChild(dirNode);

      // Scan 1 level deep for now to avoid freezing
      // Phase 2: Move this to background thread with deeper recursion
      juce::RangedDirectoryIterator iter(dir, false, "*",
                                         juce::File::findFilesAndDirectories);

      for (const auto &entry : iter) {
        if (entry.isDirectory()) {
          auto subDir = std::make_shared<BrowserItem>(
              entry.getFile().getFullPathName(), entry.getFile().getFileName(),
              BrowserItemType::Folder);
          dirNode->addChild(subDir);
        } else {
          juce::String ext = entry.getFile().getFileExtension().toLowerCase();
          if (supportedExtensions.contains(ext)) {
            BrowserItemType type = (ext == ".mid" || ext == ".midi")
                                       ? BrowserItemType::MidiFile
                                       : BrowserItemType::AudioFile;

            auto fileItem = std::make_shared<BrowserItem>(
                entry.getFile().getFullPathName(),
                entry.getFile().getFileName(), type);
            // Use explicit bool variable for clarity
            bool isFav = isFavorite(entry.getFile().getFullPathName());
            fileItem->isFavorite = isFav;

            dirNode->addChild(fileItem);
            allIndexableItems_.push_back(fileItem);

            if (fileItem->isFavorite) {
              favoritesNode_->addChild(fileItem);
            }
          }
        }
      }
    }
  }
}

//==============================================================================
// Search & Filter
//==============================================================================

#include "FuzzyMatcher.h"

std::vector<std::shared_ptr<BrowserItem>>
BrowserModel::search(const juce::String &queryText) {
  std::vector<std::shared_ptr<BrowserItem>> results;
  juce::String query = queryText.toLowerCase().trim();

  if (query.isEmpty())
    return results;

  struct ScoredItem {
      std::shared_ptr<BrowserItem> item;
      int score;
  };
  std::vector<ScoredItem> scoredItems;

  for (const auto &item : allIndexableItems_) {
    if (!item) continue;

    int nameScore = FuzzyMatcher::score(query, item->name);
    int catScore = FuzzyMatcher::score(query, item->metadata.category);
    
    int tagScore = 0;
    for (const auto &tag : item->metadata.tags) {
        tagScore = std::max(tagScore, FuzzyMatcher::score(query, tag));
    }

    int finalScore = std::max({nameScore, catScore, tagScore});
    
    if (finalScore > 0) {
      scoredItems.push_back({item, finalScore});
    }
  }

  // Sort by score descending
  std::sort(scoredItems.begin(), scoredItems.end(), [](const ScoredItem &a, const ScoredItem &b) {
      if (a.score != b.score) return a.score > b.score;
      return a.item->name.compareNatural(b.item->name) < 0;
  });

  for (const auto &si : scoredItems) {
      results.push_back(si.item);
  }

  return getFilteredItems(results);
}

std::vector<std::shared_ptr<BrowserItem>>
BrowserModel::getItemsByType(BrowserItemType type) {
  std::vector<std::shared_ptr<BrowserItem>> results;

  for (const auto &item : allIndexableItems_) {
    if (item->type == type)
      results.push_back(item);
  }

  return results;
}

void BrowserModel::addScannedItem(std::shared_ptr<BrowserItem> item) {
  if (item) {
    allIndexableItems_.push_back(item);
  }
}

//==============================================================================
// Favorites
//==============================================================================

void BrowserModel::addToFavorites(std::shared_ptr<BrowserItem> item) {
  if (!item)
    return;

  if (favoriteIds_.find(item->id) == favoriteIds_.end()) {
    favoriteIds_.insert(item->id);
    item->isFavorite = true;
    favoritesNode_->addChild(item);
    saveFavorites();
    sendChangeMessage();
  }
}

void BrowserModel::removeFromFavorites(std::shared_ptr<BrowserItem> item) {
  if (!item)
    return;

  auto it = favoriteIds_.find(item->id);
  if (it != favoriteIds_.end()) {
    favoriteIds_.erase(it);
    item->isFavorite = false;

    // Remove from favorites node
    auto &children = favoritesNode_->children;
    children.erase(
        std::remove_if(children.begin(), children.end(),
                       [&](const std::shared_ptr<BrowserItem> &child) {
                         return child->id == item->id;
                       }),
        children.end());

    saveFavorites();
    sendChangeMessage();
  }
}

bool BrowserModel::isFavorite(const juce::String &itemId) const {
  return favoriteIds_.find(itemId) != favoriteIds_.end();
}

std::vector<std::shared_ptr<BrowserItem>> BrowserModel::getFavorites() const {
  return favoritesNode_->children;
}

void BrowserModel::saveFavorites() {
  juce::StringArray ids;
  for (const auto &id : favoriteIds_)
    ids.add(id);

  juce::File file = getFavoritesFile();
  file.replaceWithText(ids.joinIntoString("\n"));
}

void BrowserModel::loadFavorites() {
  juce::File file = getFavoritesFile();
  if (file.existsAsFile()) {
    juce::StringArray ids;
    ids.addTokens(file.loadFileAsString(), "\n", "");

    for (const auto &id : ids) {
      if (id.isNotEmpty())
        favoriteIds_.insert(id);
    }
  }
}

juce::File BrowserModel::getFavoritesFile() const {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("ZenithDAW")
      .getChildFile("browser_favorites.txt");
}

//==============================================================================
// Tagging
//==============================================================================

void BrowserModel::addTagToItem(std::shared_ptr<BrowserItem> item,
                                const juce::String &tag) {
  if (!item || tag.isEmpty())
    return;

  juce::String normalizedTag = tag.toLowerCase().trim();

  // Add to item's metadata
  auto &tags = item->metadata.tags;
  if (std::find(tags.begin(), tags.end(), normalizedTag) == tags.end()) {
    tags.push_back(normalizedTag);
  }

  // Add to global tag map
  itemTags_[item->id].push_back(normalizedTag);
  allTags_.insert(normalizedTag);

  saveTags();
  sendChangeMessage();
}

void BrowserModel::removeTagFromItem(std::shared_ptr<BrowserItem> item,
                                     const juce::String &tag) {
  if (!item)
    return;

  juce::String normalizedTag = tag.toLowerCase().trim();

  // Remove from item metadata
  auto &tags = item->metadata.tags;
  tags.erase(std::remove(tags.begin(), tags.end(), normalizedTag), tags.end());

  // Remove from map
  auto &itemTagList = itemTags_[item->id];
  itemTagList.erase(
      std::remove(itemTagList.begin(), itemTagList.end(), normalizedTag),
      itemTagList.end());

  saveTags();
  sendChangeMessage();
}

std::vector<juce::String>
BrowserModel::getItemTags(std::shared_ptr<BrowserItem> item) const {
  if (!item)
    return {};

  auto it = itemTags_.find(item->id);
  if (it != itemTags_.end())
    return it->second;

  return {};
}

std::vector<std::shared_ptr<BrowserItem>>
BrowserModel::getItemsByTag(const juce::String &tag) {
  std::vector<std::shared_ptr<BrowserItem>> results;
  juce::String normalizedTag = tag.toLowerCase().trim();

  for (const auto &item : allIndexableItems_) {
    for (const auto &itemTag : item->metadata.tags) {
      if (itemTag == normalizedTag) {
        results.push_back(item);
        break;
      }
    }
  }

  return results;
}

void BrowserModel::setTagColor(const juce::String &tag, const juce::String &hexColor) {
  if (tag.isEmpty()) return;
  tagColors_[tag.toLowerCase().trim()] = hexColor;
  saveTags();
  sendChangeMessage();
}

juce::String BrowserModel::getTagColor(const juce::String &tag) const {
  auto it = tagColors_.find(tag.toLowerCase().trim());
  if (it != tagColors_.end()) return it->second;
  return ""; // Default
}

void BrowserModel::saveTags() {
  juce::File file = getTagsFile();
  juce::var jsonRoot(new juce::DynamicObject());

  juce::var itemsObj(new juce::DynamicObject());
  for (const auto &[itemId, tags] : itemTags_) {
    juce::var tagArray;
    for (const auto &tag : tags) tagArray.append(tag);
    itemsObj.getDynamicObject()->setProperty(itemId, tagArray);
  }
  jsonRoot.getDynamicObject()->setProperty("items", itemsObj);

  juce::var colorsObj(new juce::DynamicObject());
  for (const auto &[tag, color] : tagColors_) {
    colorsObj.getDynamicObject()->setProperty(tag, color);
  }
  jsonRoot.getDynamicObject()->setProperty("colors", colorsObj);

  file.replaceWithText(juce::JSON::toString(jsonRoot));
}

void BrowserModel::loadTags() {
  juce::File file = getTagsFile();
  if (file.existsAsFile()) {
    auto json = juce::JSON::parse(file.loadFileAsString());
    if (auto *obj = json.getDynamicObject()) {
      // Load item tags
      if (obj->hasProperty("items")) {
          auto items = obj->getProperty("items");
          if (auto* itemsObj = items.getDynamicObject()) {
              for (const auto& prop : itemsObj->getProperties()) {
                  juce::String itemId = prop.name.toString();
                  if (prop.value.isArray()) {
                      for (int i = 0; i < prop.value.size(); ++i) {
                          juce::String tag = prop.value[i].toString();
                          itemTags_[itemId].push_back(tag);
                          allTags_.insert(tag);
                      }
                  }
              }
          }
      }
      // Load tag colors
      if (obj->hasProperty("colors")) {
          auto colors = obj->getProperty("colors");
          if (auto* colorsObj = colors.getDynamicObject()) {
              for (const auto& prop : colorsObj->getProperties()) {
                  tagColors_[prop.name.toString()] = prop.value.toString();
              }
          }
      }
    }
  }
}

juce::File BrowserModel::getTagsFile() const {
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
      .getChildFile("ZenithDAW")
      .getChildFile("browser_tags_v2.json"); // Bump version for new format
}

//==============================================================================
// Ratings
//==============================================================================

void BrowserModel::setItemRating(std::shared_ptr<BrowserItem> item, int rating) {
  if (!item) return;
  itemRatings_[item->id] = juce::jlimit(0, 5, rating);
  item->metadata.rating = itemRatings_[item->id];
  saveRatings();
  sendChangeMessage();
}

int BrowserModel::getItemRating(const juce::String &itemId) const {
  auto it = itemRatings_.find(itemId);
  if (it != itemRatings_.end()) return it->second;
  return 0;
}

void BrowserModel::saveRatings() {
  juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("ZenithDAW")
                        .getChildFile("browser_ratings.json");
  juce::var jsonRoot(new juce::DynamicObject());
  for (const auto &[itemId, rating] : itemRatings_) {
    jsonRoot.getDynamicObject()->setProperty(itemId, rating);
  }
  file.replaceWithText(juce::JSON::toString(jsonRoot));
}

void BrowserModel::loadRatings() {
  juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("ZenithDAW")
                        .getChildFile("browser_ratings.json");
  if (file.existsAsFile()) {
    auto json = juce::JSON::parse(file.loadFileAsString());
    if (auto *obj = json.getDynamicObject()) {
      for (const auto &prop : obj->getProperties()) {
        itemRatings_[prop.name.toString()] = (int)prop.value;
      }
    }
  }
}

//==============================================================================
// History
//==============================================================================

void BrowserModel::pushHistory(std::shared_ptr<BrowserItem> folder) {
  if (!folder)
    return;

  history_.push_back(folder);

  // Limit history size
  while (history_.size() > maxHistorySize_) {
    history_.pop_front();
  }
}

std::shared_ptr<BrowserItem> BrowserModel::popHistory() {
  if (history_.empty())
    return nullptr;

  auto item = history_.back();
  history_.pop_back();
  return item;
}

std::shared_ptr<BrowserItem> BrowserModel::peekHistory() const {
  if (history_.empty())
    return nullptr;

  return history_.back();
}

//==============================================================================
// Category Filtering
//==============================================================================

void BrowserModel::setActiveFilter(BrowserItemType typeFilter) {
  if (activeFilter_ != typeFilter) {
    activeFilter_ = typeFilter;
    sendChangeMessage();
  }
}

void BrowserModel::clearFilter() {
  if (activeFilter_ != BrowserItemType::Unknown) {
    activeFilter_ = BrowserItemType::Unknown;
    sendChangeMessage();
  }
}

std::vector<std::shared_ptr<BrowserItem>> BrowserModel::getFilteredItems(
    const std::vector<std::shared_ptr<BrowserItem>> &items) const {
  if (activeFilter_ == BrowserItemType::Unknown)
    return items;

  std::vector<std::shared_ptr<BrowserItem>> filtered;

  for (const auto &item : items) {
    // Always show folders
    if (item->isDirectory || item->type == activeFilter_) {
      filtered.push_back(item);
    }
  }

  return filtered;
}

void BrowserModel::addToRecent(std::shared_ptr<BrowserItem> item) {
  if (!item || item->id.isEmpty() || item->isDirectory) return;
  recentItemIds_.erase(std::remove(recentItemIds_.begin(), recentItemIds_.end(), item->id), recentItemIds_.end());
  recentItemIds_.push_front(item->id);
  while (recentItemIds_.size() > maxRecentSize_) recentItemIds_.pop_back();
  saveRecent();
  sendChangeMessage();
}

std::vector<std::shared_ptr<BrowserItem>> BrowserModel::getRecentItems() const {
  std::vector<std::shared_ptr<BrowserItem>> results;
  for (const auto &id : recentItemIds_) {
    for (const auto &item : allIndexableItems_) {
      if (item && item->id == id) {
        results.push_back(item);
        break;
      }
    }
  }
  return results;
}

void BrowserModel::clearRecent() {
  recentItemIds_.clear();
  saveRecent();
  sendChangeMessage();
}

void BrowserModel::saveRecent() {
  juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("ZenithDAW")
                        .getChildFile("browser_recent.txt");
  juce::StringArray ids;
  for (const auto& id : recentItemIds_)
    ids.add(id);
  file.create();
  file.replaceWithText(ids.joinIntoString("\n"));
}

void BrowserModel::loadRecent() {
  juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("ZenithDAW")
                        .getChildFile("browser_recent.txt");
  if (file.existsAsFile()) {
    juce::StringArray ids;
    ids.addTokens(file.loadFileAsString(), "\n", "");
    for (const auto& id : ids) {
      if (id.isNotEmpty())
        recentItemIds_.push_back(id);
    }
  }
}

} // namespace zenith