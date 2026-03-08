/*
  ==============================================================================

    SampleConsolidation.cpp
    Implementation of sample deduplication and consolidation

  ==============================================================================
*/

#include "SampleConsolidation.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace ai {

//==============================================================================
// SampleConsolidation helper methods
//==============================================================================

juce::String SampleConsolidation::toString() const
{
    juce::StringArray lines;
    lines.add("Sample Consolidation Entry:");
    lines.add("  Source: " + sourceFile.getFullPathName());
    lines.add("  Destination: " + destFile.getFullPathName());
    lines.add("  Clip ID: " + clipId);
    return lines.joinIntoString("\n");
}

bool SampleConsolidation::isValid() const
{
    return sourceFile.existsAsFile() &&
           destFile.getFullPathName().isNotEmpty() &&
           clipId.isNotEmpty();
}

bool SampleConsolidation::needsCopy() const
{
    return sourceFile != destFile && sourceFile.exists();
}

juce::int64 SampleConsolidation::getSourceFileSize() const
{
    return sourceFile.getSize();
}

juce::int64 SampleConsolidation::getEstimatedDiskSpaceSaved() const
{
    // If source is outside project and dest is inside project,
    // we're not really saving space - we're consolidating
    // Return 0 for consolidation operations
    return 0;
}

//==============================================================================
// SampleConsolidationManager implementation
//==============================================================================

SampleConsolidationManager::SampleConsolidationManager() = default;

SampleConsolidationManager::~SampleConsolidationManager() = default;

void SampleConsolidationManager::addSampleToConsolidate(const SampleConsolidation& sample)
{
    samplesToConsolidate.push_back(sample);
}

void SampleConsolidationManager::addSampleToConsolidate(const juce::File& sourceFile,
                                                         const juce::File& destFile,
                                                         const juce::String& clipId)
{
    SampleConsolidation sample;
    sample.sourceFile = sourceFile;
    sample.destFile = destFile;
    sample.clipId = clipId;
    samplesToConsolidate.push_back(sample);
}

int SampleConsolidationManager::getCount() const
{
    return static_cast<int>(samplesToConsolidate.size());
}

bool SampleConsolidationManager::isEmpty() const
{
    return samplesToConsolidate.empty();
}

void SampleConsolidationManager::clear()
{
    samplesToConsolidate.clear();
}

const std::vector<SampleConsolidation>& SampleConsolidationManager::getSamples() const
{
    return samplesToConsolidate;
}

std::vector<SampleConsolidation>& SampleConsolidationManager::getSamples()
{
    return samplesToConsolidate;
}

juce::int64 SampleConsolidationManager::getTotalSourceSize() const
{
    juce::int64 total = 0;
    for (const auto& sample : samplesToConsolidate)
        total += sample.getSourceFileSize();
    return total;
}

juce::String SampleConsolidationManager::getTotalSourceSizeString() const
{
    return formatFileSize(getTotalSourceSize());
}

juce::String SampleConsolidationManager::formatFileSize(juce::int64 bytes)
{
    const juce::int64 KB = 1024;
    const juce::int64 MB = KB * 1024;
    const juce::int64 GB = MB * 1024;

    if (bytes >= GB)
        return juce::String(bytes / (double)GB, 2) + " GB";
    else if (bytes >= MB)
        return juce::String(bytes / (double)MB, 1) + " MB";
    else if (bytes >= KB)
        return juce::String(bytes / (double)KB, 0) + " KB";
    else
        return juce::String(bytes) + " bytes";
}

juce::String SampleConsolidationManager::getSummary() const
{
    juce::StringArray lines;
    lines.add("=== Samples to Consolidate ===");
    lines.add("Total: " + juce::String(getCount()));
    lines.add("Total Size: " + getTotalSourceSizeString());
    lines.add("");

    if (isEmpty())
    {
        lines.add("[OK] All samples are already in the project folder");
        return lines.joinIntoString("\n");
    }

    // Count samples by source directory
    std::map<juce::String, int> dirCounts;
    for (const auto& sample : samplesToConsolidate)
    {
        juce::String dir = sample.sourceFile.getParentDirectory().getFullPathName();
        dirCounts[dir]++;
    }

    lines.add("External Sample Locations:");
    for (const auto& [dir, count] : dirCounts)
        lines.add("  • " + dir + ": " + juce::String(count) + " files");

    return lines.joinIntoString("\n");
}

juce::Result SampleConsolidationManager::execute(juce::ThreadPool* threadPool)
{
    if (isEmpty())
        return juce::Result::ok();

    // Ensure destination directories exist
    std::set<juce::File> destDirs;
    for (const auto& sample : samplesToConsolidate)
        destDirs.insert(sample.destFile.getParentDirectory());

    for (const auto& dir : destDirs)
    {
        if (!dir.exists())
        {
            auto result = dir.createDirectory();
            if (!result.wasOk())
                return juce::Result::fail("Failed to create directory: " +
                                          dir.getFullPathName());
        }
    }

    // Copy files
    for (const auto& sample : samplesToConsolidate)
    {
        if (sample.needsCopy())
        {
            // Check if destination already exists
            if (sample.destFile.exists())
            {
                // Compare file sizes
                if (sample.sourceFile.getSize() != sample.destFile.getSize())
                {
                    // Files are different - need to handle conflict
                    // For now, skip
                    continue;
                }
                // Files are same - skip copy
                continue;
            }

            // Perform the copy
            auto result = sample.sourceFile.copyFileTo(sample.destFile);
            if (!result.wasOk())
                return juce::Result::fail("Failed to copy file: " +
                                          sample.sourceFile.getFullPathName() +
                                          " → " + sample.destFile.getFullPathName());
        }
    }

    return juce::Result::ok();
}

juce::var SampleConsolidationManager::toJSON() const
{
    auto obj = new juce::DynamicObject();

    auto sampleArray = juce::Array<juce::var>();
    for (const auto& sample : samplesToConsolidate)
    {
        auto sampleObj = new juce::DynamicObject();
        sampleObj->setProperty("sourceFile", sample.sourceFile.getFullPathName());
        sampleObj->setProperty("destFile", sample.destFile.getFullPathName());
        sampleObj->setProperty("clipId", sample.clipId);
        sampleObj->setProperty("fileSize", sample.getSourceFileSize());
        sampleArray.add(juce::var(sampleObj));
    }

    obj->setProperty("samples", sampleArray);
    obj->setProperty("count", getCount());
    obj->setProperty("totalSize", getTotalSourceSize());

    return juce::var(obj);
}

bool SampleConsolidationManager::fromJSON(const juce::var& json)
{
    samplesToConsolidate.clear();

    if (!json.isObject())
        return false;

    auto obj = json.getDynamicObject();

    if (obj->hasProperty("samples"))
    {
        auto sampleArray = obj->getProperty("samples").getArray();
        if (sampleArray)
        {
            for (const auto& item : *sampleArray)
            {
                if (item.isObject())
                {
                    SampleConsolidation sample;
                    auto sampleObj = item.getDynamicObject();
                    sample.sourceFile = juce::File(
                        sampleObj->getProperty("sourceFile").toString());
                    sample.destFile = juce::File(
                        sampleObj->getProperty("destFile").toString());
                    sample.clipId = sampleObj->getProperty("clipId").toString();
                    samplesToConsolidate.push_back(sample);
                }
            }
        }
    }

    return true;
}

} // namespace ai
} // namespace zenith
