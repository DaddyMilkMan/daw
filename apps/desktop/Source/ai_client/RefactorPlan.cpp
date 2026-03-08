/*
  ==============================================================================

    RefactorPlan.cpp
    Implementation of project refactoring plan for AI-driven cleanup

  ==============================================================================
*/

#include "RefactorPlan.h"

namespace zenith {
namespace ai {

//==============================================================================
// RefactorPlan helper methods
//==============================================================================

juce::String RefactorPlan::getSummary() const
{
    juce::StringArray lines;
    lines.add("=== Project Refactoring Plan ===");
    lines.add("");

    if (!trackRenames.empty())
    {
        lines.add("Track Renames (" + juce::String(trackRenames.size()) + "):");
        for (const auto& r : trackRenames)
            lines.add("  • " + r.oldName + " → " + r.newName);
        lines.add("");
    }

    if (!colorChanges.empty())
    {
        lines.add("Color Assignments (" + juce::String(colorChanges.size()) + "):");
        lines.add("  (Tracks will be colored by instrument type)");
        lines.add("");
    }

    if (!groupsToCreate.empty())
    {
        lines.add("Track Groups (" + juce::String(groupsToCreate.size()) + "):");
        for (const auto& g : groupsToCreate)
            lines.add("  • " + g.name + " (" + juce::String(g.trackIds.size()) + " tracks)");
        lines.add("");
    }

    if (!clipsToArchive.empty())
    {
        lines.add("Dead Clips to Archive (" + juce::String(clipsToArchive.size()) + "):");
        for (const auto& c : clipsToArchive)
            lines.add("  • " + c.reason + " (Moving to Quarantine)");
        lines.add("");
    }

    if (!samplesToConsolidate.empty())
    {
        lines.add("Samples to Consolidate (" + juce::String(samplesToConsolidate.size()) + "):");
        lines.add("  (External samples will be copied to project folder)");
        lines.add("");
    }

    if (isEmpty())
        lines.add("[OK] Project is already clean - no refactoring needed!");

    return lines.joinIntoString("\n");
}

bool RefactorPlan::isEmpty() const
{
    return trackRenames.empty() && colorChanges.empty() &&
           groupsToCreate.empty() && clipsToArchive.empty() &&
           samplesToConsolidate.empty();
}

void RefactorPlan::clear()
{
    trackRenames.clear();
    colorChanges.clear();
    groupsToCreate.clear();
    clipsToArchive.clear();
    samplesToConsolidate.clear();

    totalIssuesFound = 0;
    tracksToRename = 0;
    tracksToColor = 0;
    groupsToCreate_ = 0;
    clipsToArchiveCount = 0;
    samplesToMove = 0;
}

void RefactorPlan::calculateStatistics()
{
    tracksToRename = static_cast<int>(trackRenames.size());
    tracksToColor = static_cast<int>(colorChanges.size());
    groupsToCreate_ = static_cast<int>(groupsToCreate.size());
    clipsToArchiveCount = static_cast<int>(clipsToArchive.size());
    samplesToMove = static_cast<int>(samplesToConsolidate.size());

    totalIssuesFound = tracksToRename + tracksToColor + groupsToCreate_ +
                       clipsToArchiveCount + samplesToMove;
}

juce::var RefactorPlan::toJSON() const
{
    auto obj = new juce::DynamicObject();

    // Track renames
    auto renameArray = juce::Array<juce::var>();
    for (const auto& rename : trackRenames)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("trackId", rename.trackId);
        item->setProperty("oldName", rename.oldName);
        item->setProperty("newName", rename.newName);
        item->setProperty("category", static_cast<int>(rename.category));
        renameArray.add(juce::var(item));
    }
    obj->setProperty("trackRenames", renameArray);

    // Color changes
    auto colorArray = juce::Array<juce::var>();
    for (const auto& color : colorChanges)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("trackId", color.trackId);
        item->setProperty("color", color.color.toString());
        item->setProperty("category", static_cast<int>(color.category));
        colorArray.add(juce::var(item));
    }
    obj->setProperty("colorChanges", colorArray);

    // Track groups
    auto groupArray = juce::Array<juce::var>();
    for (const auto& group : groupsToCreate)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("name", group.name);

        auto trackIdArray = juce::Array<juce::var>();
        for (const auto& tid : group.trackIds)
            trackIdArray.add(tid);
        item->setProperty("trackIds", trackIdArray);

        item->setProperty("color", group.color.toString());
        groupArray.add(juce::var(item));
    }
    obj->setProperty("groupsToCreate", groupArray);

    // Clips to archive
    auto archiveArray = juce::Array<juce::var>();
    for (const auto& clip : clipsToArchive)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("trackId", clip.trackId);
        item->setProperty("clipId", clip.clipId);
        item->setProperty("reason", clip.reason);
        item->setProperty("targetTrackId", clip.targetTrackId);
        archiveArray.add(juce::var(item));
    }
    obj->setProperty("clipsToArchive", archiveArray);

    // Samples to consolidate
    auto sampleArray = juce::Array<juce::var>();
    for (const auto& sample : samplesToConsolidate)
    {
        auto item = new juce::DynamicObject();
        item->setProperty("sourceFile", sample.sourceFile.getFullPathName());
        item->setProperty("destFile", sample.destFile.getFullPathName());
        item->setProperty("clipId", sample.clipId);
        sampleArray.add(juce::var(item));
    }
    obj->setProperty("samplesToConsolidate", sampleArray);

    // Statistics
    obj->setProperty("totalIssuesFound", totalIssuesFound);
    obj->setProperty("tracksToRename", tracksToRename);
    obj->setProperty("tracksToColor", tracksToColor);
    obj->setProperty("groupsToCreate", groupsToCreate_);
    obj->setProperty("clipsToArchiveCount", clipsToArchiveCount);
    obj->setProperty("samplesToMove", samplesToMove);

    return juce::var(obj);
}

bool RefactorPlan::fromJSON(const juce::var& json)
{
    clear();

    if (!json.isObject())
        return false;

    auto obj = json.getDynamicObject();

    // Parse track renames
    if (obj->hasProperty("trackRenames"))
    {
        auto renameArray = obj->getProperty("trackRenames").getArray();
        if (renameArray)
        {
            for (const auto& item : *renameArray)
            {
                if (item.isObject())
                {
                    TrackRename rename;
                    auto itemObj = item.getDynamicObject();
                    rename.trackId = itemObj->getProperty("trackId").toString();
                    rename.oldName = itemObj->getProperty("oldName").toString();
                    rename.newName = itemObj->getProperty("newName").toString();
                    rename.category = static_cast<InstrumentCategory>(
                        static_cast<int>(itemObj->getProperty("category")));
                    trackRenames.push_back(rename);
                }
            }
        }
    }

    // Parse color changes
    if (obj->hasProperty("colorChanges"))
    {
        auto colorArray = obj->getProperty("colorChanges").getArray();
        if (colorArray)
        {
            for (const auto& item : *colorArray)
            {
                if (item.isObject())
                {
                    TrackColorChange color;
                    auto itemObj = item.getDynamicObject();
                    color.trackId = itemObj->getProperty("trackId").toString();
                    color.color = juce::Colour::fromString(
                        itemObj->getProperty("color").toString());
                    color.category = static_cast<InstrumentCategory>(
                        static_cast<int>(itemObj->getProperty("category")));
                    colorChanges.push_back(color);
                }
            }
        }
    }

    // Parse track groups
    if (obj->hasProperty("groupsToCreate"))
    {
        auto groupArray = obj->getProperty("groupsToCreate").getArray();
        if (groupArray)
        {
            for (const auto& item : *groupArray)
            {
                if (item.isObject())
                {
                    TrackGroupDef group;
                    auto itemObj = item.getDynamicObject();
                    group.name = itemObj->getProperty("name").toString();
                    group.color = juce::Colour::fromString(
                        itemObj->getProperty("color").toString());

                    auto trackIdArray = itemObj->getProperty("trackIds").getArray();
                    if (trackIdArray)
                    {
                        for (const auto& tid : *trackIdArray)
                            group.trackIds.push_back(tid.toString());
                    }

                    groupsToCreate.push_back(group);
                }
            }
        }
    }

    // Parse clips to archive
    if (obj->hasProperty("clipsToArchive"))
    {
        auto archiveArray = obj->getProperty("clipsToArchive").getArray();
        if (archiveArray)
        {
            for (const auto& item : *archiveArray)
            {
                if (item.isObject())
                {
                    ClipArchive clip;
                    auto itemObj = item.getDynamicObject();
                    clip.trackId = itemObj->getProperty("trackId").toString();
                    clip.clipId = itemObj->getProperty("clipId").toString();
                    clip.reason = itemObj->getProperty("reason").toString();
                    clip.targetTrackId = itemObj->getProperty("targetTrackId").toString();
                    clipsToArchive.push_back(clip);
                }
            }
        }
    }

    // Parse samples to consolidate
    if (obj->hasProperty("samplesToConsolidate"))
    {
        auto sampleArray = obj->getProperty("samplesToConsolidate").getArray();
        if (sampleArray)
        {
            for (const auto& item : *sampleArray)
            {
                if (item.isObject())
                {
                    SampleConsolidation sample;
                    auto itemObj = item.getDynamicObject();
                    sample.sourceFile = juce::File(
                        itemObj->getProperty("sourceFile").toString());
                    sample.destFile = juce::File(
                        itemObj->getProperty("destFile").toString());
                    sample.clipId = itemObj->getProperty("clipId").toString();
                    samplesToConsolidate.push_back(sample);
                }
            }
        }
    }

    // Parse statistics
    totalIssuesFound = obj->getProperty("totalIssuesFound");
    tracksToRename = obj->getProperty("tracksToRename");
    tracksToColor = obj->getProperty("tracksToColor");
    groupsToCreate_ = obj->getProperty("groupsToCreate");
    clipsToArchiveCount = obj->getProperty("clipsToArchiveCount");
    samplesToMove = obj->getProperty("samplesToMove");

    return true;
}

} // namespace ai
} // namespace zenith
