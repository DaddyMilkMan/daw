#include "TrackCommands.h"
#include "../dsp/ONNXStemSeparator.h"
#include "../engine/MixerChannel.h"
#include "../engine/Track.h"
#include "../engine/TrackFreeze.h"
#include "CommandUtils.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"

namespace zenith {

TrackCommands::TrackCommands(Engine &eng, ProjectState &state)
    : engine(eng), projectState(state) {}

juce::var TrackCommands::listTracks(const juce::var &params) {
  juce::ignoreUnused(params);

  juce::var tracksArray;
  auto *tracksArrayPtr = tracksArray.getArray();

  const auto &tracks = engine.tracks();

  for (size_t i = 0; i < tracks.size(); ++i) {
    const auto *track = tracks[i].get();
    if (track == nullptr)
      continue;

    auto *trackObj = new juce::DynamicObject();
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

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("tracks", tracksArray);
  resultObj->setProperty("count", (int)tracks.size());

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::createTrack(const juce::var &params) {
  if (!params.hasProperty("type"))
    return createErrorResponse(
        "Missing 'type' parameter (must be 'audio' or 'midi')");

  juce::String type = params["type"].toString().toLowerCase();
  juce::String name = params.hasProperty("name") ? params["name"].toString()
                                                 : juce::String("New Track");

  if (type != "audio" && type != "midi")
    return createErrorResponse("Invalid type: must be 'audio' or 'midi'");

  juce::String trackId = projectState.addTrack(name, type);

  if (trackId.isEmpty())
    return createErrorResponse("Failed to create track");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("name", name);
  resultObj->setProperty("type", type);

  DBG("TrackCommands: Created track: " + trackId + " (" + name + ")");

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::deleteTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  projectState.removeTrack(trackId);

  DBG("TrackCommands: Deleted track: " + trackId);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("deleted", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::renameTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("name"))
    return createErrorResponse("Missing 'name' parameter");

  juce::String trackId = params["trackId"].toString();
  juce::String newName = params["name"].toString();

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  juce::String actionName = "rename_track " + trackId + " to '" + newName + "'";
  projectState.renameTrack(trackId, newName, actionName);

  DBG("TrackCommands: Renamed track: " + trackId + " to " + newName);

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("newName", newName);
  resultObj->setProperty("success", true);

  return juce::var(resultObj);
}

juce::var TrackCommands::setTrackVolume(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("volumeDb"))
    return createErrorResponse("Missing 'volumeDb' parameter");

  juce::String trackId = params["trackId"].toString();
  double volumeDb = params["volumeDb"];

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  float gain = juce::Decibels::decibelsToGain((float)volumeDb);
  gain = juce::jlimit(0.0f, 2.0f, gain);

  Track *track = findTrackById(engine, trackId);
  if (track) {
    int trackIndex = -1;
    for (int i = 0; i < engine.getNumTracks(); ++i) {
      if (engine.tracks()[i].get() == track) {
        trackIndex = i;
        break;
      }
    }
    if (trackIndex >= 0) {
      zenith::EngineEvent e(zenith::EngineEvent::Type::SetTrackVolume);
      e.trackIndex = trackIndex;
      e.value = gain;
      engine.queueEvent(e);
    }
  }

  juce::String actionName = "set_track_volume " + trackId + " to " +
                            juce::String(volumeDb, 1) + " dB";
  projectState.setTrackVolume(trackId, gain, actionName);

  DBG("TrackCommands: Set track volume: " + trackId + " to " +
      juce::String(volumeDb) + " dB");

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("volumeDb", volumeDb);
  resultObj->setProperty("volumeLinear", gain);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::setTrackPan(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");
  if (!params.hasProperty("pan"))
    return createErrorResponse("Missing 'pan' parameter");

  juce::String trackId = params["trackId"].toString();
  double pan = params["pan"];

  auto trackTree = projectState.getTrack(trackId);
  if (!trackTree.isValid())
    return createErrorResponse("Track not found: " + trackId);

  float panValue = juce::jlimit(-1.0f, 1.0f, (float)pan);

  Track *track = findTrackById(engine, trackId);
  if (track) {
    int trackIndex = -1;
    for (int i = 0; i < engine.getNumTracks(); ++i) {
      if (engine.tracks()[i].get() == track) {
        trackIndex = i;
        break;
      }
    }
    if (trackIndex >= 0) {
      zenith::EngineEvent e(zenith::EngineEvent::Type::SetTrackPan);
      e.trackIndex = trackIndex;
      e.value = panValue;
      engine.queueEvent(e);
    }
  }

  juce::String actionName =
      "set_track_pan " + trackId + " to " + juce::String(panValue, 2);
  projectState.setTrackPan(trackId, panValue, actionName);

  DBG("TrackCommands: Set track pan: " + trackId + " to " +
      juce::String(panValue));

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("trackId", trackId);
  resultObj->setProperty("pan", panValue);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::setTrackSend(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("sendIndex"))
    return createErrorResponse("Missing 'sendIndex'");
  if (!params.hasProperty("level"))
    return createErrorResponse("Missing 'level'");

  juce::String trackId = params["trackId"].toString();
  int sendIndex = (int)params["sendIndex"];
  float level = (float)params["level"];
  bool preFader =
      params.hasProperty("preFader") ? (bool)params["preFader"] : false;

  Track *track = findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  if (sendIndex < 0 || sendIndex >= 4)
    return createErrorResponse("Invalid sendIndex (0-3)");

  auto &mixer = track->getMixerChannel();
  mixer.setSendLevel(sendIndex, level);
  mixer.setSendPreFader(sendIndex, preFader);

  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::setTrackEQ(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");
  if (!params.hasProperty("bandIndex"))
    return createErrorResponse("Missing 'bandIndex'");

  juce::String trackId = params["trackId"].toString();
  int bandIndex = (int)params["bandIndex"];

  Track *track = findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  if (bandIndex < 0 || bandIndex >= 4)
    return createErrorResponse("Invalid bandIndex (0-3)");

  auto &mixer = track->getMixerChannel();
  auto &band = mixer.getEQBand(bandIndex);

  if (params.hasProperty("enabled"))
    band.enabled = (bool)params["enabled"];
  if (params.hasProperty("frequency"))
    band.frequency = (float)params["frequency"];
  if (params.hasProperty("gain"))
    band.gain = (float)params["gain"];
  if (params.hasProperty("q"))
    band.q = (float)params["q"];

  if (params.hasProperty("type")) {
    juce::String typeStr = params["type"].toString();
    if (typeStr == "low_shelf")
      band.type = MixerChannel::EQBand::Type::LowShelf;
    else if (typeStr == "high_shelf")
      band.type = MixerChannel::EQBand::Type::HighShelf;
    else
      band.type = MixerChannel::EQBand::Type::Peak;
  }

  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::setTrackCompressor(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId'");

  juce::String trackId = params["trackId"].toString();
  Track *track = findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  auto &mixer = track->getMixerChannel();

  if (params.hasProperty("enabled"))
    mixer.setCompressorEnabled((bool)params["enabled"]);
  if (params.hasProperty("threshold"))
    mixer.setCompressorThreshold((float)params["threshold"]);
  if (params.hasProperty("ratio"))
    mixer.setCompressorRatio((float)params["ratio"]);
  if (params.hasProperty("attack"))
    mixer.setCompressorAttack((float)params["attack"]);
  if (params.hasProperty("release"))
    mixer.setCompressorRelease((float)params["release"]);
  if (params.hasProperty("makeup"))
    mixer.setCompressorMakeup((float)params["makeup"]);

  return createSuccessResponse(juce::var());
}

juce::var TrackCommands::separateTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  Track *track = findTrackById(engine, trackId);

  if (track == nullptr)
    return createErrorResponse("Track not found: " + trackId);

  juce::int64 maxEnd = 0;
  for (int i = 0; i < track->getNumClips(); ++i) {
    auto *clip = track->getClip(i);
    if (clip) {
      maxEnd =
          juce::jmax(maxEnd, static_cast<juce::int64>(clip->getStartPosition() +
                                                       clip->getLength()));
    }
  }

  if (maxEnd == 0)
    return createErrorResponse("Track is empty");

  // Safety Limit: Prevent huge allocations (e.g. > 15 mins @ 48kHz) to avoid crash
  const juce::int64 kMaxSeparationSamples = 45000000; 
  if (maxEnd > kMaxSeparationSamples)
      return createErrorResponse("Track too long for separation (limit: ~15 mins). Please split the clip.");

  double sampleRate = engine.getSampleRate();
  if (sampleRate <= 0)
    sampleRate = 44100.0;

  juce::String trackName = track->getName();

  // Capture references for async task. 
  // NOTE: TrackCommands instance might be destroyed, so we capture engine and projectState refs directly.
  Engine& engineRef = engine;
  ProjectState& stateRef = projectState;

  // Launch background thread for rendering and AI processing
  juce::Thread::launch([&engineRef, &stateRef, trackId, trackName, maxEnd, sampleRate]() {
      // 1. Render Track Audio (Background Thread)
      juce::AudioBuffer<float> trackBuffer(2, (int)maxEnd);
      trackBuffer.clear();

      // We need to find the track again on this thread since capturing a raw pointer is unsafe
      Track* trackPtr = findTrackById(engineRef, trackId);
      if (!trackPtr) return;

      int blockSize = 1024;
      juce::int64 samplesRendered = 0;
      juce::AudioBuffer<float> blockBuffer(2, blockSize);

      while (samplesRendered < maxEnd) {
        int numSamples = (int)juce::jmin((juce::int64)blockSize, maxEnd - samplesRendered);
        blockBuffer.clear();
        juce::AudioSourceChannelInfo info(&blockBuffer, 0, numSamples);

        trackPtr->getNextAudioBlock(info, samplesRendered, nullptr);

        for (int ch = 0; ch < 2; ++ch) {
          trackBuffer.copyFrom(ch, (int)samplesRendered, blockBuffer, ch, 0, numSamples);
        }
        samplesRendered += numSamples;
      }

      // 2. AI STEM SEPARATION (Background Thread - Intensive)
      ONNXStemSeparator separator;
      separator.initialize(juce::File());
      
      auto result = std::make_shared<ONNXStemSeparator::SeparationResult>();
      *result = separator.separate(trackBuffer, sampleRate);

      if (!result->success) {
          DBG("Separation failed: " + result->error);
          return;
      }

      // 3. Update Project State (Main Thread Async)
      // We must move back to message thread for ProjectState updates + track creation
      juce::MessageManager::callAsync([&stateRef, trackName, trackId, result, sampleRate]() {
          juce::File recordingsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
              .getChildFile("ZenithDAW/Stems");

          if (!recordingsDir.exists())
            recordingsDir.createDirectory();

          juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
          juce::String baseName = trackName + "_" + timestamp;

          struct StemInfo {
            juce::String suffix;
            juce::AudioBuffer<float> &buffer;
          };

          StemInfo stems[] = {{"Vocals", result->vocals},
                              {"Drums", result->drums},
                              {"Bass", result->bass},
                              {"Other", result->other}};

          juce::WavAudioFormat wavFormat;

          for (const auto &stem : stems) {
            juce::File stemFile = recordingsDir.getChildFile(baseName + "_" + stem.suffix + ".wav");
            auto fileStream = std::make_unique<juce::FileOutputStream>(stemFile);

            if (fileStream->openedOk()) {
              auto writerOptions = juce::AudioFormatWriter::Options()
                                       .withSampleRate(sampleRate)
                                       .withNumChannels(2)
                                       .withBitsPerSample(24);

              std::unique_ptr<juce::OutputStream> outputStream = std::move(fileStream);
              std::unique_ptr<juce::AudioFormatWriter> writer = wavFormat.createWriterFor(outputStream, writerOptions);

              if (writer) {
                writer->writeFromAudioSampleBuffer(stem.buffer, 0, stem.buffer.getNumSamples());
                writer.reset();

                juce::String newTrackName = trackName + " (" + stem.suffix + ")";
                juce::String newTrackId = stateRef.addTrack(newTrackName, "audio");

                juce::String clipId = stateRef.createClip(
                    newTrackId, "audio", 0, stem.buffer.getNumSamples(), stem.suffix, "create_stem_clip");

                auto clipTree = stateRef.getClip(newTrackId, clipId);
                if (clipTree.isValid()) {
                  clipTree.setProperty(ProjectState::PROP_AUDIO_FILE, stemFile.getFullPathName(), &stateRef.getUndoManager());
                }
              }
            }
          }
          
          DBG("Stem separation complete for: " + trackName);
      });
  });

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("originalTrackId", trackId);
  resultObj->setProperty("status", "started_async");
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TrackCommands::freezeTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  Track *track = findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  juce::File freezeDir = projectState.getAssetDirectory("Freeze");
  
  // Use Engine's freeze manager
  if (engine.getTrackFreezeManager().freezeTrack(*track, engine, freezeDir, nullptr)) {
      auto *resultObj = new juce::DynamicObject();
      resultObj->setProperty("trackId", trackId);
      resultObj->setProperty("status", "freezing");
      return createSuccessResponse(juce::var(resultObj));
  }

  return createErrorResponse("Failed to start freeze operation");
}

juce::var TrackCommands::unfreezeTrack(const juce::var &params) {
  if (!params.hasProperty("trackId"))
    return createErrorResponse("Missing 'trackId' parameter");

  juce::String trackId = params["trackId"].toString();
  Track *track = findTrackById(engine, trackId);
  if (!track)
    return createErrorResponse("Track not found: " + trackId);

  if (engine.getTrackFreezeManager().unfreezeTrack(*track)) {
      auto *resultObj = new juce::DynamicObject();
      resultObj->setProperty("trackId", trackId);
      resultObj->setProperty("status", "unfrozen");
      return createSuccessResponse(juce::var(resultObj));
  }

  return createErrorResponse("Failed to unfreeze track");
}

} // namespace zenith
