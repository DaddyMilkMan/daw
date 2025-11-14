/*
  ==============================================================================

    CommandAPI.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    JSON command processor implementation

  ==============================================================================
*/

#include "CommandAPI.h"
#include "SessionGraph.h"
#include "../Engine.h"
#include "../ProjectState.h"
#include "engine/Track.h"
#include "engine/Clip.h"

namespace zenith {

//==============================================================================
CommandAPI::CommandAPI(Engine& eng, ProjectState& state)
    : engine(eng), projectState(state)
{
    DBG("CommandAPI: Initialized");
}

CommandAPI::~CommandAPI()
{
}

//==============================================================================
juce::var CommandAPI::executeCommand(const juce::var& request)
{
    // Validate request structure
    if (!request.isObject())
        return createErrorResponse("Invalid request: must be JSON object");

    if (!request.hasProperty("command"))
        return createErrorResponse("Missing 'command' field");

    juce::String command = request["command"].toString();
    juce::var params = request.getProperty("params", juce::var());

    DBG("CommandAPI: Executing command: " + command);

    // Route to appropriate handler
    if (command == "list_tracks")
        return listTracks(params);
    else if (command == "create_track")
        return createTrack(params);
    else if (command == "delete_track")
        return deleteTrack(params);
    else if (command == "rename_track")
        return renameTrack(params);
    else if (command == "list_clips")
        return listClips(params);
    else if (command == "split_clip")
        return splitClip(params);
    else if (command == "move_clip")
        return moveClip(params);
    else if (command == "set_track_volume")
        return setTrackVolume(params);
    else if (command == "set_track_pan")
        return setTrackPan(params);
    else if (command == "get_session_graph")
        return getSessionGraph(params);
    else
        return createErrorResponse("Unknown command: " + command);
}

juce::String CommandAPI::executeCommandString(const juce::String& jsonRequest)
{
    // Parse JSON string to var
    juce::var parsedJson;
    auto result = juce::JSON::parse(jsonRequest, parsedJson);

    if (result.failed())
        return juce::JSON::toString(createErrorResponse("JSON parse error: " + result.getErrorMessage()));

    // Execute command
    juce::var response = executeCommand(parsedJson);

    // Convert back to string
    return juce::JSON::toString(response);
}

//==============================================================================
// Command Handlers
//==============================================================================

juce::var CommandAPI::listTracks(const juce::var& params)
{
    juce::ignoreUnused(params);

    juce::var tracksArray;
    auto* tracksArrayPtr = tracksArray.getArray();

    const auto& tracks = engine.tracks();

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        const auto* track = tracks[i].get();
        if (track == nullptr)
            continue;

        auto* trackObj = new juce::DynamicObject();
        trackObj->setProperty("id", "track_" + juce::String((int)i));
        trackObj->setProperty("name", track->getName());
        trackObj->setProperty("type", track->getTypeString());
        trackObj->setProperty("volume", track->getVolume());
        trackObj->setProperty("pan", track->getPan());
        trackObj->setProperty("muted", track->isMuted());
        trackObj->setProperty("soloed", track->isSolo());
        trackObj->setProperty("numClips", track->getNumClips());
        trackObj->setProperty("numPlugins", track->getNumPlugins());

        tracksArrayPtr->add(juce::var(trackObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("tracks", tracksArray);
    resultObj->setProperty("count", (int)tracks.size());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::createTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("type"))
        return createErrorResponse("Missing 'type' parameter (must be 'audio' or 'midi')");

    juce::String type = params["type"].toString().toLowerCase();
    juce::String name = params.getProperty("name", "New Track").toString();

    // Validate type
    Track::Type trackType;
    if (type == "audio")
        trackType = Track::Type::Audio;
    else if (type == "midi")
        trackType = Track::Type::MIDI;
    else
        return createErrorResponse("Invalid type: must be 'audio' or 'midi'");

    // Create track
    auto newTrack = std::make_unique<Track>(name, trackType);

    // Prepare it (use current engine sample rate)
    newTrack->prepareToPlay(512, engine.getSampleRate());

    // Get track ID before adding (will be index)
    int trackIndex = engine.getNumTracks();
    juce::String trackId = "track_" + juce::String(trackIndex);

    // Add to engine (this modifies the tracks vector)
    // Note: We're directly adding to engine for MVP. In future, this should go through ProjectState
    const_cast<std::vector<std::unique_ptr<Track>>&>(engine.tracks()).push_back(std::move(newTrack));

    // Create result
    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", name);
    resultObj->setProperty("type", type);

    DBG("CommandAPI: Created track: " + trackId + " (" + name + ")");

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::deleteTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Extract track index from ID (format: "track_N")
    int trackIndex = trackId.fromLastOccurrenceOf("_", false, false).getIntValue();

    // Validate index
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
        return createErrorResponse("Invalid track index");

    // Remove from engine
    // Note: Direct vector manipulation for MVP
    auto& tracks = const_cast<std::vector<std::unique_ptr<Track>>&>(engine.tracks());
    tracks.erase(tracks.begin() + trackIndex);

    DBG("CommandAPI: Deleted track: " + trackId);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("deleted", true);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::renameTrack(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("name"))
        return createErrorResponse("Missing 'name' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String newName = params["name"].toString();

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Rename track
    track->setName(newName);

    DBG("CommandAPI: Renamed track: " + trackId + " to " + newName);

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("name", newName);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::listClips(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");

    juce::String trackId = params["trackId"].toString();

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Build clips array
    juce::var clipsArray;
    auto* clipsArrayPtr = clipsArray.getArray();

    for (int i = 0; i < track->getNumClips(); ++i)
    {
        Track::Clip* clip = track->getClip(i);
        if (clip == nullptr)
            continue;

        auto* clipObj = new juce::DynamicObject();
        clipObj->setProperty("id", "clip_" + juce::String(i));
        clipObj->setProperty("name", clip->getName());
        clipObj->setProperty("type", clip->getType() == Track::Clip::Type::Audio ? "audio" : "midi");
        clipObj->setProperty("startSamples", (juce::int64)clip->getStartPosition());
        clipObj->setProperty("lengthSamples", (juce::int64)clip->getLength());

        // Add type-specific info
        if (clip->getType() == Track::Clip::Type::Audio)
        {
            clipObj->setProperty("audioFile", clip->getAudioFile().getFullPathName());
        }
        else if (clip->getType() == Track::Clip::Type::MIDI)
        {
            const auto* midiSeq = clip->getMidiSequence();
            int noteCount = 0;
            if (midiSeq != nullptr)
            {
                for (int j = 0; j < midiSeq->getNumEvents(); ++j)
                {
                    if (midiSeq->getEventPointer(j)->message.isNoteOn())
                        noteCount++;
                }
            }
            clipObj->setProperty("midiNoteCount", noteCount);
        }

        clipsArrayPtr->add(juce::var(clipObj));
    }

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("clips", clipsArray);
    resultObj->setProperty("count", track->getNumClips());

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::splitClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");
    if (!params.hasProperty("splitSamples"))
        return createErrorResponse("Missing 'splitSamples' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();
    juce::int64 splitSamples = params["splitSamples"];

    // Find track and clip
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    Track::Clip* clip = findClipById(track, clipId);
    if (clip == nullptr)
        return createErrorResponse("Clip not found: " + clipId);

    // Validate split position
    juce::int64 clipStart = clip->getStartPosition();
    juce::int64 clipEnd = clip->getEndPosition();

    if (splitSamples <= clipStart || splitSamples >= clipEnd)
        return createErrorResponse("Split position must be within clip bounds");

    // Create two new clips
    auto leftClip = std::make_unique<Track::Clip>();
    auto rightClip = std::make_unique<Track::Clip>();

    leftClip->setType(clip->getType());
    leftClip->setName(clip->getName() + " (L)");
    leftClip->setStartPosition(clipStart);
    leftClip->setLength(splitSamples - clipStart);
    leftClip->setOffset(clip->getOffset());

    rightClip->setType(clip->getType());
    rightClip->setName(clip->getName() + " (R)");
    rightClip->setStartPosition(splitSamples);
    rightClip->setLength(clipEnd - splitSamples);
    rightClip->setOffset(clip->getOffset() + (splitSamples - clipStart));

    // Copy content (audio or MIDI)
    if (clip->getType() == Track::Clip::Type::Audio && clip->getAudioBuffer() != nullptr)
    {
        leftClip->setAudioBuffer(*clip->getAudioBuffer());
        rightClip->setAudioBuffer(*clip->getAudioBuffer());
    }
    else if (clip->getType() == Track::Clip::Type::MIDI && clip->getMidiSequence() != nullptr)
    {
        leftClip->setMidiSequence(*clip->getMidiSequence());
        rightClip->setMidiSequence(*clip->getMidiSequence());
    }

    // Prepare clips
    leftClip->prepareToPlay(512, engine.getSampleRate());
    rightClip->prepareToPlay(512, engine.getSampleRate());

    // Remove original clip and add new ones
    track->removeClip(clip);
    track->addClip(std::move(leftClip));
    track->addClip(std::move(rightClip));

    DBG("CommandAPI: Split clip: " + clipId + " at " + juce::String(splitSamples));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("originalClipId", clipId);
    resultObj->setProperty("splitSamples", splitSamples);
    resultObj->setProperty("leftClipId", "clip_" + juce::String(track->getNumClips() - 2));
    resultObj->setProperty("rightClipId", "clip_" + juce::String(track->getNumClips() - 1));

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::moveClip(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("clipId"))
        return createErrorResponse("Missing 'clipId' parameter");
    if (!params.hasProperty("newStartSamples"))
        return createErrorResponse("Missing 'newStartSamples' parameter");

    juce::String trackId = params["trackId"].toString();
    juce::String clipId = params["clipId"].toString();
    juce::int64 newStartSamples = params["newStartSamples"];

    // Find track and clip
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    Track::Clip* clip = findClipById(track, clipId);
    if (clip == nullptr)
        return createErrorResponse("Clip not found: " + clipId);

    // Validate position (must be >= 0)
    if (newStartSamples < 0)
        return createErrorResponse("Clip position must be >= 0");

    // Move clip
    clip->setStartPosition(newStartSamples);

    DBG("CommandAPI: Moved clip: " + clipId + " to " + juce::String(newStartSamples));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("clipId", clipId);
    resultObj->setProperty("newStartSamples", newStartSamples);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setTrackVolume(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("volumeDb"))
        return createErrorResponse("Missing 'volumeDb' parameter");

    juce::String trackId = params["trackId"].toString();
    double volumeDb = params["volumeDb"];

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Convert dB to linear gain
    float gain = juce::Decibels::decibelsToGain((float)volumeDb);

    // Clamp to reasonable range (0.0 to 2.0 linear = -inf to +6dB)
    gain = juce::jlimit(0.0f, 2.0f, gain);

    // Set volume
    track->setVolume(gain);

    DBG("CommandAPI: Set track volume: " + trackId + " to " + juce::String(volumeDb) + " dB");

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("volumeDb", volumeDb);
    resultObj->setProperty("volumeLinear", gain);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::setTrackPan(const juce::var& params)
{
    // Validate params
    if (!params.hasProperty("trackId"))
        return createErrorResponse("Missing 'trackId' parameter");
    if (!params.hasProperty("pan"))
        return createErrorResponse("Missing 'pan' parameter");

    juce::String trackId = params["trackId"].toString();
    double pan = params["pan"];

    // Find track
    Track* track = findTrackById(trackId);
    if (track == nullptr)
        return createErrorResponse("Track not found: " + trackId);

    // Clamp to valid range (-1.0 to 1.0)
    float panValue = juce::jlimit(-1.0f, 1.0f, (float)pan);

    // Set pan
    track->setPan(panValue);

    DBG("CommandAPI: Set track pan: " + trackId + " to " + juce::String(panValue));

    auto* resultObj = new juce::DynamicObject();
    resultObj->setProperty("trackId", trackId);
    resultObj->setProperty("pan", panValue);

    return createSuccessResponse(juce::var(resultObj));
}

juce::var CommandAPI::getSessionGraph(const juce::var& params)
{
    juce::ignoreUnused(params);

    // Use SessionGraph to generate full project state
    SessionGraph graph(engine, projectState);
    juce::var graphData = graph.generateGraph();

    DBG("CommandAPI: Generated session graph");

    return createSuccessResponse(graphData);
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::var CommandAPI::createSuccessResponse(const juce::var& result)
{
    auto* response = new juce::DynamicObject();
    response->setProperty("success", true);
    response->setProperty("result", result);
    return juce::var(response);
}

juce::var CommandAPI::createErrorResponse(const juce::String& errorMessage)
{
    auto* response = new juce::DynamicObject();
    response->setProperty("success", false);
    response->setProperty("error", errorMessage);
    return juce::var(response);
}

Track* CommandAPI::findTrackById(const juce::String& trackId)
{
    // Track ID format: "track_N"
    if (!trackId.startsWith("track_"))
        return nullptr;

    int trackIndex = trackId.fromLastOccurrenceOf("_", false, false).getIntValue();

    if (trackIndex < 0 || trackIndex >= engine.getNumTracks())
        return nullptr;

    return const_cast<Track*>(engine.tracks()[trackIndex].get());
}

Track::Clip* CommandAPI::findClipById(Track* track, const juce::String& clipId)
{
    if (track == nullptr)
        return nullptr;

    // Clip ID format: "clip_N"
    if (!clipId.startsWith("clip_"))
        return nullptr;

    int clipIndex = clipId.fromLastOccurrenceOf("_", false, false).getIntValue();

    if (clipIndex < 0 || clipIndex >= track->getNumClips())
        return nullptr;

    return track->getClip(clipIndex);
}

} // namespace zenith
