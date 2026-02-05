/*
  ==============================================================================

    SessionGraph.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    Project state serializer implementation

  ==============================================================================
*/

#include "SessionGraph.h"
#include "Engine.h"
#include "ProjectState.h"
#include "../engine/Track.h"
#include "../engine/Clip.h"

namespace zenith {

//==============================================================================
SessionGraph::SessionGraph(Engine& eng, ProjectState& state)
    : engine(eng), projectState(state)
{
}

SessionGraph::~SessionGraph()
{
}

//==============================================================================
juce::var SessionGraph::generateGraph()
{
    auto* root = new juce::DynamicObject();

    // Serialize transport
    root->setProperty("transport", serializeTransport());

    // Serialize tracks
    root->setProperty("tracks", serializeTracks());

    return juce::var(root);
}

juce::String SessionGraph::generateGraphString(bool prettyPrint)
{
    juce::var graph = generateGraph();

    if (prettyPrint)
    {
        // Use 2-space indentation for readability
        return juce::JSON::toString(graph, true, 2);
    }
    else
    {
        return juce::JSON::toString(graph);
    }
}

//==============================================================================
// Serialization Helpers
//==============================================================================

juce::var SessionGraph::serializeTransport()
{
    auto* transport = new juce::DynamicObject();

    transport->setProperty("isPlaying", engine.isPlaying());
    transport->setProperty("playheadSamples", (juce::int64)engine.getPlaybackPosition());
    transport->setProperty("tempo", projectState.getTempo());
    transport->setProperty("timeSigNumerator", projectState.getTimeSignatureNumerator());
    transport->setProperty("timeSigDenominator", projectState.getTimeSignatureDenominator());
    transport->setProperty("sampleRate", engine.getSampleRate());

    return juce::var(transport);
}

juce::var SessionGraph::serializeTracks()
{
    juce::var tracksArray;
    // Removed tracksArray.getArray() call which returns nullptr

    const auto& tracks = engine.tracks();

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        const auto* track = tracks[i].get();
        if (track != nullptr)
        {
            tracksArray.append(serializeTrack(track, (int)i));
        }
    }

    return tracksArray;
}

juce::var SessionGraph::serializeTrack(const Track* track, int trackIndex)
{
    auto* trackObj = new juce::DynamicObject();

    trackObj->setProperty("id", "track_" + juce::String(trackIndex));
    trackObj->setProperty("name", track->getName());
    trackObj->setProperty("type", track->getTypeString());
    trackObj->setProperty("volume", track->getVolume());
    trackObj->setProperty("volumeDb", juce::Decibels::gainToDecibels(track->getVolume()));
    trackObj->setProperty("pan", track->getPan());
    trackObj->setProperty("muted", track->isMuted());
    trackObj->setProperty("soloed", track->isSolo());
    trackObj->setProperty("armed", track->isArmed());
    trackObj->setProperty("enabled", track->isEnabled());

    // Add plugins
    trackObj->setProperty("plugins", serializePlugins(track));

    // Add clips
    trackObj->setProperty("clips", serializeClips(track));

    // Add level meters (current state)
    trackObj->setProperty("currentLevel", track->getCurrentLevel());
    trackObj->setProperty("peakLevel", track->getPeakLevel());

    return juce::var(trackObj);
}

juce::var SessionGraph::serializePlugins(const Track* track)
{
    juce::var pluginsArray;
    // Removed pluginsArray.getArray() call which returns nullptr

    for (int i = 0; i < track->getNumPlugins(); ++i)
    {
        auto* plugin = track->getPlugin(i);
        if (plugin == nullptr)
            continue;

        auto* pluginObj = new juce::DynamicObject();
        pluginObj->setProperty("index", i);
        pluginObj->setProperty("name", plugin->getName());

        // Get plugin description
        auto description = plugin->getPluginDescription();
        pluginObj->setProperty("format", description.pluginFormatName);
        pluginObj->setProperty("manufacturer", description.manufacturerName);
        pluginObj->setProperty("category", description.category);
        pluginObj->setProperty("isInstrument", description.isInstrument);

        // Get bypass state (if supported)
        pluginObj->setProperty("bypassed", plugin->isSuspended());

        // Get latency
        pluginObj->setProperty("latencySamples", plugin->getLatencySamples());

        // Get parameter count
        pluginObj->setProperty("numParameters", plugin->getParameters().size());

        pluginsArray.append(juce::var(pluginObj));
    }

    return pluginsArray;
}

juce::var SessionGraph::serializeClips(const Track* track)
{
    juce::var clipsArray;
    // Removed clipsArray.getArray() call which returns nullptr

    for (int i = 0; i < track->getNumClips(); ++i)
    {
        Clip* clip = track->getClip(i);
        if (clip != nullptr)
        {
            clipsArray.append(serializeClip(clip, i));
        }
    }

    return clipsArray;
}

juce::var SessionGraph::serializeClip(const Clip* clip, int clipIndex)
{
    auto* clipObj = new juce::DynamicObject();

    clipObj->setProperty("id", "clip_" + juce::String(clipIndex));
    clipObj->setProperty("name", clip->getName());
    clipObj->setProperty("type", clip->getType() == Clip::Type::Audio ? "audio" : "midi");
    clipObj->setProperty("startSamples", (juce::int64)clip->getStartPosition());
    clipObj->setProperty("lengthSamples", (juce::int64)clip->getLength());
    clipObj->setProperty("offsetSamples", (juce::int64)clip->getOffset());
    clipObj->setProperty("isPlaying", clip->isPlaying());
    clipObj->setProperty("isLooping", clip->isLooping());
    clipObj->setProperty("gain", clip->getGain());
    clipObj->setProperty("gainDb", juce::Decibels::gainToDecibels(clip->getGain()));

    // Add fade info
    clipObj->setProperty("fadeInSamples", (juce::int64)clip->getFadeIn());
    clipObj->setProperty("fadeOutSamples", (juce::int64)clip->getFadeOut());

    // Type-specific data
    if (clip->getType() == Clip::Type::Audio)
    {
        // Audio clip - include file path and buffer info
        auto audioFile = clip->getAudioFile();
        clipObj->setProperty("audioFile", audioFile.getFullPathName());
        clipObj->setProperty("audioFileExists", audioFile.existsAsFile());

        const auto* audioBuffer = clip->getAudioBuffer();
        if (audioBuffer != nullptr)
        {
            clipObj->setProperty("numChannels", audioBuffer->getNumChannels());
            clipObj->setProperty("numSamples", audioBuffer->getNumSamples());
        }
    }
    else if (clip->getType() == Clip::Type::MIDI)
    {
        // MIDI clip - count notes and events
        const auto* midiSeq = clip->getMidiSequence();
        if (midiSeq != nullptr)
        {
            int noteOnCount = 0;
            int noteOffCount = 0;
            int ccCount = 0;
            int otherCount = 0;

            for (int j = 0; j < midiSeq->getNumEvents(); ++j)
            {
                const auto* event = midiSeq->getEventPointer(j);
                if (event != nullptr)
                {
                    const auto& msg = event->message;
                    if (msg.isNoteOn())
                        noteOnCount++;
                    else if (msg.isNoteOff())
                        noteOffCount++;
                    else if (msg.isController())
                        ccCount++;
                    else
                        otherCount++;
                }
            }

            clipObj->setProperty("midiNoteCount", noteOnCount);
            clipObj->setProperty("midiNoteOffCount", noteOffCount);
            clipObj->setProperty("midiCCCount", ccCount);
            clipObj->setProperty("midiOtherEventCount", otherCount);
            clipObj->setProperty("midiTotalEvents", midiSeq->getNumEvents());
        }
    }

    // Color (convert to hex string for JSON)
    auto color = clip->getColor();
    juce::String colorHex = "#" +
        juce::String::toHexString((int)(color.getRed() * 255)).paddedLeft('0', 2) +
        juce::String::toHexString((int)(color.getGreen() * 255)).paddedLeft('0', 2) +
        juce::String::toHexString((int)(color.getBlue() * 255)).paddedLeft('0', 2);
    clipObj->setProperty("color", colorHex);

    return juce::var(clipObj);
}

} // namespace zenith