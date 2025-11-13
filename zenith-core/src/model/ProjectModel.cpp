/**
 * @file ProjectModel.cpp
 * @brief ValueTree ↔ ProjectModel conversion
 */

#include "../../include/model/ProjectModel.h"
#include "../../include/model/ProjectIDs.h"
#include <JuceHeader.h>

namespace zenith
{

using namespace ProjectIDs;

//==============================================================================
// ValueTree → ProjectModel (Deserialization)
//==============================================================================

static ClipModel clipFromValueTree(const juce::ValueTree& vt)
{
    ClipModel cm;
    cm.id             = (int64_t) vt.getProperty(clipId, (juce::int64) 0);
    cm.filePath       = vt.getProperty(clipFilePath, "").toString().toStdString();
    cm.startSample    = (int64_t) vt.getProperty(clipStartSample, (juce::int64) 0);
    cm.lengthSamples  = (int64_t) vt.getProperty(clipLength, (juce::int64) 0);
    cm.srcOffset      = (int64_t) vt.getProperty(clipSrcOffset, (juce::int64) 0);
    cm.gain           = (float) vt.getProperty(clipGain, 1.0f);
    cm.fadeInSamples  = (int) vt.getProperty(clipFadeIn, 0);
    cm.fadeOutSamples = (int) vt.getProperty(clipFadeOut, 0);
    cm.muted          = (bool) vt.getProperty(clipMuted, false);
    cm.loopEnabled    = (bool) vt.getProperty(clipLoopEnabled, false);
    cm.loopLength     = (int64_t) vt.getProperty(clipLoopLength, (juce::int64) 0);
    return cm;
}

static TrackModel trackFromValueTree(const juce::ValueTree& vt)
{
    TrackModel tm;
    tm.id    = (int32_t) vt.getProperty(trackId, 0);
    tm.name  = vt.getProperty(trackName, "").toString().toStdString();
    tm.gain  = (float) vt.getProperty(trackGain, 1.0f);
    tm.pan   = (float) vt.getProperty(trackPan, 0.0f);
    tm.muted = (bool) vt.getProperty(trackMuted, false);
    tm.solo  = (bool) vt.getProperty(trackSolo, false);

    // Load clips
    for (int i = 0; i < vt.getNumChildren(); ++i)
    {
        auto child = vt.getChild(i);
        if (child.hasType(audioClip))
            tm.clips.push_back(clipFromValueTree(child));
        // FX slots not persisted in v0.1
    }

    return tm;
}

ProjectModel projectFromValueTree(const juce::ValueTree& vt)
{
    if (!vt.hasType(project))
        return {}; // Invalid tree

    ProjectModel pm;
    pm.name        = vt.getProperty(projName, "Untitled").toString().toStdString();
    pm.sampleRate  = (double) vt.getProperty(projSampleRate, 48000.0);
    pm.blockSize   = (int) vt.getProperty(projBlockSize, 512);
    pm.lengthHint  = (int64_t) vt.getProperty(projLengthHint, (juce::int64) 0);
    pm.nextClipId  = (int64_t) vt.getProperty(projNextClipId, (juce::int64) 1);

    // Load tracks
    for (int i = 0; i < vt.getNumChildren(); ++i)
    {
        auto child = vt.getChild(i);
        if (child.hasType(track))
            pm.tracks.push_back(trackFromValueTree(child));
    }

    return pm;
}

//==============================================================================
// ProjectModel → ValueTree (Serialization)
//==============================================================================

static juce::ValueTree clipToValueTree(const ClipModel& cm)
{
    juce::ValueTree vt(audioClip);
    vt.setProperty(clipId, (juce::int64) cm.id, nullptr);
    vt.setProperty(clipFilePath, juce::String(cm.filePath), nullptr);
    vt.setProperty(clipStartSample, (juce::int64) cm.startSample, nullptr);
    vt.setProperty(clipLength, (juce::int64) cm.lengthSamples, nullptr);
    vt.setProperty(clipSrcOffset, (juce::int64) cm.srcOffset, nullptr);
    vt.setProperty(clipGain, cm.gain, nullptr);
    vt.setProperty(clipFadeIn, cm.fadeInSamples, nullptr);
    vt.setProperty(clipFadeOut, cm.fadeOutSamples, nullptr);
    vt.setProperty(clipMuted, cm.muted, nullptr);
    vt.setProperty(clipLoopEnabled, cm.loopEnabled, nullptr);
    vt.setProperty(clipLoopLength, (juce::int64) cm.loopLength, nullptr);
    return vt;
}

static juce::ValueTree trackToValueTree(const TrackModel& tm)
{
    juce::ValueTree vt(track);
    vt.setProperty(trackId, tm.id, nullptr);
    vt.setProperty(trackName, juce::String(tm.name), nullptr);
    vt.setProperty(trackGain, tm.gain, nullptr);
    vt.setProperty(trackPan, tm.pan, nullptr);
    vt.setProperty(trackMuted, tm.muted, nullptr);
    vt.setProperty(trackSolo, tm.solo, nullptr);

    // Add clips
    for (const auto& cm : tm.clips)
        vt.appendChild(clipToValueTree(cm), nullptr);

    // FX slots not persisted in v0.1

    return vt;
}

juce::ValueTree projectToValueTree(const ProjectModel& pm)
{
    juce::ValueTree vt(project);
    vt.setProperty(projName, juce::String(pm.name), nullptr);
    vt.setProperty(projSampleRate, pm.sampleRate, nullptr);
    vt.setProperty(projBlockSize, pm.blockSize, nullptr);
    vt.setProperty(projLengthHint, (juce::int64) pm.lengthHint, nullptr);
    vt.setProperty(projNextClipId, (juce::int64) pm.nextClipId, nullptr);
    vt.setProperty(projVersion, "0.1.0", nullptr);

    // Add tracks
    for (const auto& tm : pm.tracks)
        vt.appendChild(trackToValueTree(tm), nullptr);

    return vt;
}

} // namespace zenith
