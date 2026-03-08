/*
  ==============================================================================

    ClipArchive.cpp
    Implementation of clip archival management for dead/unwanted clips

  ==============================================================================
*/

#include "ClipArchive.h"

namespace zenith {
namespace ai {

//==============================================================================
// ClipArchive helper methods
//==============================================================================

juce::String ClipArchive::getReason() const
{
    return reason;
}

juce::String ClipArchive::toString() const
{
    juce::StringArray lines;
    lines.add("Clip Archive Entry:");
    lines.add("  Track ID: " + trackId);
    lines.add("  Clip ID: " + clipId);
    lines.add("  Reason: " + reason);
    lines.add("  Target Track: " + targetTrackId);
    return lines.joinIntoString("\n");
}

bool ClipArchive::isValid() const
{
    return trackId.isNotEmpty() && clipId.isNotEmpty() &&
           targetTrackId.isNotEmpty() && reason.isNotEmpty();
}

//==============================================================================
// ClipArchiveManager implementation
//==============================================================================

ClipArchiveManager::ClipArchiveManager() = default;

ClipArchiveManager::~ClipArchiveManager() = default;

void ClipArchiveManager::addClipToArchive(const ClipArchive& clip)
{
    clipsToArchive.push_back(clip);
}

void ClipArchiveManager::addClipToArchive(const juce::String& trackId,
                                          const juce::String& clipId,
                                          const juce::String& reason,
                                          const juce::String& targetTrackId)
{
    ClipArchive clip;
    clip.trackId = trackId;
    clip.clipId = clipId;
    clip.reason = reason;
    clip.targetTrackId = targetTrackId;
    clipsToArchive.push_back(clip);
}

int ClipArchiveManager::getCount() const
{
    return static_cast<int>(clipsToArchive.size());
}

bool ClipArchiveManager::isEmpty() const
{
    return clipsToArchive.empty();
}

void ClipArchiveManager::clear()
{
    clipsToArchive.clear();
}

const std::vector<ClipArchive>& ClipArchiveManager::getClips() const
{
    return clipsToArchive;
}

std::vector<ClipArchive>& ClipArchiveManager::getClips()
{
    return clipsToArchive;
}

juce::String ClipArchiveManager::getSummary() const
{
    juce::StringArray lines;
    lines.add("=== Clips to Archive ===");
    lines.add("Total: " + juce::String(getCount()));
    lines.add("");

    if (isEmpty())
    {
        lines.add("[OK] No clips need archiving");
        return lines.joinIntoString("\n");
    }

    // Group by reason
    std::map<juce::String, int> reasonCounts;
    for (const auto& clip : clipsToArchive)
        reasonCounts[clip.reason]++;

    lines.add("By Reason:");
    for (const auto& [reason, count] : reasonCounts)
        lines.add("  • " + reason + ": " + juce::String(count));

    return lines.joinIntoString("\n");
}

juce::var ClipArchiveManager::toJSON() const
{
    auto obj = new juce::DynamicObject();

    auto clipArray = juce::Array<juce::var>();
    for (const auto& clip : clipsToArchive)
    {
        auto clipObj = new juce::DynamicObject();
        clipObj->setProperty("trackId", clip.trackId);
        clipObj->setProperty("clipId", clip.clipId);
        clipObj->setProperty("reason", clip.reason);
        clipObj->setProperty("targetTrackId", clip.targetTrackId);
        clipArray.add(juce::var(clipObj));
    }

    obj->setProperty("clips", clipArray);
    obj->setProperty("count", getCount());

    return juce::var(obj);
}

bool ClipArchiveManager::fromJSON(const juce::var& json)
{
    clipsToArchive.clear();

    if (!json.isObject())
        return false;

    auto obj = json.getDynamicObject();

    if (obj->hasProperty("clips"))
    {
        auto clipArray = obj->getProperty("clips").getArray();
        if (clipArray)
        {
            for (const auto& item : *clipArray)
            {
                if (item.isObject())
                {
                    ClipArchive clip;
                    auto clipObj = item.getDynamicObject();
                    clip.trackId = clipObj->getProperty("trackId").toString();
                    clip.clipId = clipObj->getProperty("clipId").toString();
                    clip.reason = clipObj->getProperty("reason").toString();
                    clip.targetTrackId = clipObj->getProperty("targetTrackId").toString();
                    clipsToArchive.push_back(clip);
                }
            }
        }
    }

    return true;
}

} // namespace ai
} // namespace zenith
