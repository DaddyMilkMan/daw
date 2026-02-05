/*
  ==============================================================================

    PluginBlacklist.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    Implementation of persistent plugin blacklist management.

  ==============================================================================
*/

#include "PluginBlacklist.h"

#if JUCE_MODULE_AVAILABLE_juce_xml
#include <juce_xml/juce_xml.h>
#endif

namespace zenith {

//==============================================================================
// BlacklistEntry Implementation
//==============================================================================

juce::String PluginBlacklist::BlacklistEntry::getSummary() const
{
    juce::String summary;
    summary << filePath << "\n";
    summary << "  Error: " << errorType << "\n";
    if (errorMessage.isNotEmpty())
        summary << "  Message: " << errorMessage << "\n";
    summary << "  Failures: " << failureCount << "\n";
    summary << "  Last seen: " << timestamp.toString(true, true, false, true) << "\n";
    if (userBlacklisted)
        summary << "  [User blacklisted]\n";
    return summary;
}

//==============================================================================
// PluginBlacklist Implementation
//==============================================================================

PluginBlacklist::PluginBlacklist()
{
    loadFromDisk();
}

PluginBlacklist::~PluginBlacklist()
{
    if (needsSave_)
        saveToDisk();
}

juce::File PluginBlacklist::getBlacklistFile()
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto zenithDir = appData.getChildFile("Zenith");
    return zenithDir.getChildFile("PluginBlacklist.xml");
}

void PluginBlacklist::loadFromDisk()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto file = getBlacklistFile();
    if (!file.existsAsFile())
        return;
#if JUCE_MODULE_AVAILABLE_juce_xml
    juce::XmlDocument doc(file);
    std::unique_ptr<juce::XmlElement> root(doc.getDocumentElement());
    
    if (root != nullptr)
    {
        juce::ValueTree tree = juce::ValueTree::fromXml(*root);
        fromValueTree(tree);
    }
    
    DBG("PluginBlacklist: Loaded " + juce::String(entries_.size()) + " entries from " + file.getFullPathName());
#else
    juce::ignoreUnused(file);
    DBG("PluginBlacklist: juce_xml not available; skipping load");
#endif
}

void PluginBlacklist::saveToDisk()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto file = getBlacklistFile();
    file.getParentDirectory().createDirectory();

#if JUCE_MODULE_AVAILABLE_juce_xml
    auto tree = toValueTree();
    std::unique_ptr<juce::XmlElement> xml(tree.createXml());
    
    if (xml != nullptr)
    {
        xml->writeTo(file, {});
        DBG("PluginBlacklist: Saved " + juce::String(entries_.size()) + " entries to " + file.getFullPathName());
    }
#else
    juce::ignoreUnused(file);
    DBG("PluginBlacklist: juce_xml not available; skipping save");
#endif
    
    needsSave_ = false;
}

void PluginBlacklist::addToBlacklist(const juce::String& filePath, 
                                     const juce::String& errorType,
                                     const juce::String& errorMessage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    BlacklistEntry entry;
    entry.filePath = filePath;
    entry.errorType = errorType;
    entry.errorMessage = errorMessage;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.failureCount = 1;
    entry.userBlacklisted = true;
    
    entries_[filePath] = entry;
    needsSave_ = true;
    
    DBG("PluginBlacklist: Added " + filePath + " (" + errorType + ")");
    
    // Auto-save immediately for crashes
    if (errorType == "crash" || errorType == "timeout")
        saveToDisk();
}

void PluginBlacklist::removeFromBlacklist(const juce::String& filePath)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entries_.find(filePath);
    if (it != entries_.end())
    {
        entries_.erase(it);
        needsSave_ = true;
        DBG("PluginBlacklist: Removed " + filePath);
        saveToDisk();
    }
}

bool PluginBlacklist::isBlacklisted(const juce::String& filePath) const
{
    if (ignoreBlacklist_)
        return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.find(filePath) != entries_.end();
}

bool PluginBlacklist::recordFailure(const juce::String& filePath,
                                    const juce::String& errorType,
                                    const juce::String& errorMessage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entries_.find(filePath);
    if (it != entries_.end())
    {
        // Already blacklisted, increment count
        it->second.failureCount++;
        it->second.timestamp = juce::Time::getCurrentTime();
        needsSave_ = true;
        return true; // Already blacklisted
    }
    else
    {
        // Check if we have transient failures
        // For now, blacklist immediately on crash/timeout
        if (errorType == "crash" || errorType == "timeout" || errorType == "exception")
        {
            BlacklistEntry entry;
            entry.filePath = filePath;
            entry.errorType = errorType;
            entry.errorMessage = errorMessage;
            entry.timestamp = juce::Time::getCurrentTime();
            entry.failureCount = 1;
            entry.userBlacklisted = false;
            
            entries_[filePath] = entry;
            needsSave_ = true;
            DBG("PluginBlacklist: Auto-blacklisted " + filePath + " after " + errorType);
            saveToDisk();
            return true;
        }
        
        return false;
    }
}

void PluginBlacklist::clearBlacklist()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = entries_.size();
    entries_.clear();
    needsSave_ = true;
    
    DBG("PluginBlacklist: Cleared " + juce::String(count) + " entries");
    saveToDisk();
}

int PluginBlacklist::getBlacklistCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(entries_.size());
}

juce::StringArray PluginBlacklist::getBlacklistedPaths() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    juce::StringArray paths;
    for (const auto& [path, entry] : entries_)
        paths.add(path);
    return paths;
}

std::optional<PluginBlacklist::BlacklistEntry> PluginBlacklist::getEntry(const juce::String& filePath) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entries_.find(filePath);
    if (it != entries_.end())
        return it->second;
    
    return std::nullopt;
}

std::vector<PluginBlacklist::BlacklistEntry> PluginBlacklist::getAllEntries() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<BlacklistEntry> result;
    result.reserve(entries_.size());
    
    for (const auto& [path, entry] : entries_)
        result.push_back(entry);
    
    // Sort by timestamp (most recent first)
    std::sort(result.begin(), result.end(), 
              [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });
    
    return result;
}

//==============================================================================
// Serialization
//==============================================================================

juce::ValueTree PluginBlacklist::toValueTree() const
{
    juce::ValueTree tree("PLUGIN_BLACKLIST");
    tree.setProperty("version", 1, nullptr);
    tree.setProperty("savedAt", juce::Time::getCurrentTime().toISO8601(true), nullptr);
    
    for (const auto& [path, entry] : entries_)
    {
        juce::ValueTree entryTree("ENTRY");
        entryTree.setProperty("filePath", entry.filePath, nullptr);
        entryTree.setProperty("errorType", entry.errorType, nullptr);
        entryTree.setProperty("errorMessage", entry.errorMessage, nullptr);
        entryTree.setProperty("timestamp", entry.timestamp.toISO8601(true), nullptr);
        entryTree.setProperty("failureCount", entry.failureCount, nullptr);
        entryTree.setProperty("userBlacklisted", entry.userBlacklisted, nullptr);
        tree.appendChild(entryTree, nullptr);
    }
    
    return tree;
}

void PluginBlacklist::fromValueTree(const juce::ValueTree& tree)
{
    entries_.clear();
    
    if (!tree.hasType("PLUGIN_BLACKLIST"))
        return;
    
    for (const auto& entryTree : tree)
    {
        if (!entryTree.hasType("ENTRY"))
            continue;
        
        BlacklistEntry entry;
        entry.filePath = entryTree.getProperty("filePath").toString();
        entry.errorType = entryTree.getProperty("errorType", "unknown").toString();
        entry.errorMessage = entryTree.getProperty("errorMessage", "").toString();
        
        juce::String timestampStr = entryTree.getProperty("timestamp", "").toString();
        if (timestampStr.isNotEmpty())
            entry.timestamp = juce::Time::fromISO8601(timestampStr);
        
        entry.failureCount = entryTree.getProperty("failureCount", 1);
        entry.userBlacklisted = entryTree.getProperty("userBlacklisted", false);
        
        if (entry.filePath.isNotEmpty())
            entries_[entry.filePath] = entry;
    }
}

} // namespace zenith
