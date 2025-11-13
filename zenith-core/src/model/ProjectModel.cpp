/**
 * @file ProjectModel.cpp
 * @brief Project model implementation (v0.1)
 *
 * Conversion between ValueTree and ProjectModel structs.
 * Message thread only - no engine interaction.
 */

#include "../../include/model/ProjectModel.h"
#include "../../include/model/ProjectIDs.h"

namespace zenith::model
{

//==============================================================================
// makeEmptyProject
//==============================================================================

ProjectModel makeEmptyProject(double sampleRate, const juce::String& name)
{
    ProjectModel project;
    project.name = name;
    project.sampleRate = sampleRate;
    project.tracks.clear(); // 0 tracks

    return project;
}

//==============================================================================
// projectToValueTree
//==============================================================================

juce::ValueTree projectToValueTree(const ProjectModel& project,
                                   const juce::File& projectFileDirectory)
{
    using namespace ids;

    // Create root node
    juce::ValueTree root(ID_PROJECT);
    root.setProperty(attrName, project.name, nullptr);
    root.setProperty(attrSampleRate, project.sampleRate, nullptr);

    // Add tracks
    for (const auto& track : project.tracks)
    {
        juce::ValueTree trackVT(ID_TRACK);
        trackVT.setProperty(attrTrackId, track.trackId, nullptr);
        trackVT.setProperty(attrTrackName, track.name, nullptr);

        // Add clips
        for (const auto& clip : track.clips)
        {
            juce::ValueTree clipVT(ID_AUDIO_CLIP);
            clipVT.setProperty(attrClipId, (juce::int64) clip.id, nullptr);
            clipVT.setProperty(attrStartSample, (juce::int64) clip.startSample, nullptr);
            clipVT.setProperty(attrLengthSamples, (juce::int64) clip.lengthSamples, nullptr);
            clipVT.setProperty(attrSrcOffset, (juce::int64) clip.srcOffset, nullptr);
            clipVT.setProperty(attrGain, clip.gain, nullptr);
            clipVT.setProperty(attrFadeInSamples, clip.fadeInSamples, nullptr);
            clipVT.setProperty(attrFadeOutSamples, clip.fadeOutSamples, nullptr);
            clipVT.setProperty(attrMuted, clip.muted, nullptr);

            // Handle file path: store relative if inside project directory, else absolute
            juce::String pathToStore;
            if (projectFileDirectory.isDirectory() &&
                clip.file.isAChildOf(projectFileDirectory))
            {
                // Store relative path
                pathToStore = clip.file.getRelativePathFrom(projectFileDirectory);
            }
            else
            {
                // Store absolute path
                pathToStore = clip.file.getFullPathName();
            }
            clipVT.setProperty(attrFilePath, pathToStore, nullptr);

            trackVT.addChild(clipVT, -1, nullptr);
        }

        root.addChild(trackVT, -1, nullptr);
    }

    return root;
}

//==============================================================================
// projectFromValueTree
//==============================================================================

ProjectModel projectFromValueTree(const juce::ValueTree& root,
                                  const juce::File& projectFileDirectory)
{
    using namespace ids;

    // If invalid or not ID_PROJECT, return default empty project
    if (!root.isValid() || root.getType() != ID_PROJECT)
    {
        DBG("ProjectModel: Invalid or empty ValueTree, returning default empty project");
        return makeEmptyProject(48000.0, "Untitled");
    }

    ProjectModel project;

    // Read project properties with defaults
    project.name = root.getProperty(attrName, "Untitled").toString();
    project.sampleRate = root.getProperty(attrSampleRate, 48000.0);

    // Validate sample rate
    if (project.sampleRate <= 0.0)
    {
        DBG("ProjectModel: Invalid sample rate " + juce::String(project.sampleRate) + ", defaulting to 48000.0");
        project.sampleRate = 48000.0;
    }

    // Parse tracks
    int autoTrackId = 0;
    for (int trackIdx = 0; trackIdx < root.getNumChildren(); ++trackIdx)
    {
        juce::ValueTree trackVT = root.getChild(trackIdx);

        // Skip non-track nodes
        if (trackVT.getType() != ID_TRACK)
            continue;

        TrackModel track;

        // Read track properties with defaults
        if (trackVT.hasProperty(attrTrackId))
        {
            track.trackId = trackVT.getProperty(attrTrackId);
        }
        else
        {
            // Auto-assign trackId if missing
            track.trackId = autoTrackId;
        }
        autoTrackId = track.trackId + 1; // Ensure next auto-ID doesn't conflict

        track.name = trackVT.getProperty(attrTrackName, "Track " + juce::String(track.trackId)).toString();

        // Parse clips
        for (int clipIdx = 0; clipIdx < trackVT.getNumChildren(); ++clipIdx)
        {
            juce::ValueTree clipVT = trackVT.getChild(clipIdx);

            // Skip non-clip nodes
            if (clipVT.getType() != ID_AUDIO_CLIP)
                continue;

            ClipModel clip;

            // Read clip properties with defaults
            clip.id = clipVT.getProperty(attrClipId, (juce::int64) 0);
            clip.startSample = clipVT.getProperty(attrStartSample, (juce::int64) 0);
            clip.lengthSamples = clipVT.getProperty(attrLengthSamples, (juce::int64) 0);
            clip.srcOffset = clipVT.getProperty(attrSrcOffset, (juce::int64) 0);
            clip.gain = clipVT.getProperty(attrGain, 1.0f);
            clip.fadeInSamples = clipVT.getProperty(attrFadeInSamples, 0);
            clip.fadeOutSamples = clipVT.getProperty(attrFadeOutSamples, 0);
            clip.muted = clipVT.getProperty(attrMuted, false);

            // Clamp values to sensible ranges
            clip.startSample = juce::jmax((juce::int64) 0, clip.startSample);
            clip.lengthSamples = juce::jmax((juce::int64) 0, clip.lengthSamples);
            clip.srcOffset = juce::jmax((juce::int64) 0, clip.srcOffset);
            clip.gain = juce::jlimit(0.0f, 2.0f, clip.gain);
            clip.fadeInSamples = juce::jmax(0, clip.fadeInSamples);
            clip.fadeOutSamples = juce::jmax(0, clip.fadeOutSamples);

            // Resolve file path
            juce::String pathStr = clipVT.getProperty(attrFilePath, juce::String()).toString();

            if (pathStr.isEmpty())
            {
                // Skip clip with no file path
                DBG("ProjectModel: Skipping clip " + juce::String(clip.id) + " with empty file path");
                continue;
            }

            juce::File clipFile(pathStr);

            // If not absolute, treat as relative to project directory
            if (!clipFile.isAbsolute() && projectFileDirectory.isDirectory())
            {
                clipFile = projectFileDirectory.getChildFile(pathStr);
            }

            clip.file = clipFile;

            // Note: We keep clips even if file doesn't exist (for "missing file" UI handling)
            // But log a warning
            if (!clip.file.existsAsFile())
            {
                DBG("ProjectModel: Warning - clip " + juce::String(clip.id) +
                    " references missing file: " + clip.file.getFullPathName());
            }

            // Skip clips with invalid length (but keep missing files)
            if (clip.lengthSamples <= 0)
            {
                DBG("ProjectModel: Skipping clip " + juce::String(clip.id) + " with invalid length");
                continue;
            }

            track.clips.push_back(clip);
        }

        project.tracks.push_back(track);
    }

    DBG("ProjectModel: Loaded project '" + project.name + "' with " +
        juce::String(project.tracks.size()) + " tracks");

    return project;
}

} // namespace zenith::model
