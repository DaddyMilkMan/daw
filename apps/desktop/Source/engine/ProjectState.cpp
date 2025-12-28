/**
 * @file ProjectState.cpp
 * @brief Project state implementation
 */

#include "ProjectState.h"
#include "AutomationStateManager.h"
#include "ClipStateManager.h"
#include "ProjectFileIO.h"
#include "TrackStateManager.h"

#include <functional>

namespace {
//==============================================================================
int extractNumericSuffix(const juce::String &identifier) {
  auto trimmed = identifier.trim();

  if (trimmed.isEmpty())
    return -1;

  const int lastUnderscore = trimmed.lastIndexOfChar('_');

  if (lastUnderscore < 0)
    return -1;

  auto numericPart = trimmed.substring(lastUnderscore + 1).trim();

  if (numericPart.isEmpty() || !numericPart.containsOnly("0123456789"))
    return -1;

  return numericPart.getIntValue();
}
} // namespace

namespace zenith {

//==============================================================================
// Static identifier definitions
//==============================================================================

const juce::Identifier ProjectState::ID_PROJECT("PROJECT");
const juce::Identifier ProjectState::ID_TRACKS("TRACKS");
const juce::Identifier ProjectState::ID_TRACK("TRACK");
const juce::Identifier ProjectState::ID_CLIPS("CLIPS");
const juce::Identifier ProjectState::ID_CLIP("CLIP");
const juce::Identifier ProjectState::ID_MIXER("MIXER");
const juce::Identifier ProjectState::ID_AUTOMATION("AUTOMATION");
const juce::Identifier ProjectState::ID_ENVELOPE("ENVELOPE");
const juce::Identifier ProjectState::ID_POINTS("POINTS");
const juce::Identifier ProjectState::ID_POINT("POINT");
const juce::Identifier ProjectState::ID_NOTES("NOTES");
const juce::Identifier ProjectState::ID_NOTE("NOTE");
const juce::Identifier ProjectState::ID_TEMPO_MAP("TEMPO_MAP");
const juce::Identifier ProjectState::ID_TEMPO_POINT("TEMPO_POINT");
const juce::Identifier ProjectState::ID_MARKERS("MARKERS");
const juce::Identifier ProjectState::ID_MARKER("MARKER");
const juce::Identifier ProjectState::ID_SECTIONS("SECTIONS");
const juce::Identifier ProjectState::ID_SECTION("SECTION");
const juce::Identifier ProjectState::ID_TAKE_FOLDER("TAKE_FOLDER");
const juce::Identifier ProjectState::ID_COMP_REGIONS("COMP_REGIONS");
const juce::Identifier ProjectState::ID_COMP_REGION("COMP_REGION");

const juce::Identifier ProjectState::PROP_NAME("name");
const juce::Identifier ProjectState::PROP_TEMPO("tempo");
const juce::Identifier
    ProjectState::PROP_TIME_SIG_NUM("timeSignatureNumerator");
const juce::Identifier
    ProjectState::PROP_TIME_SIG_DEN("timeSignatureDenominator");
const juce::Identifier ProjectState::PROP_SAMPLE_RATE("sampleRate");

const juce::Identifier ProjectState::PROP_ID("id");
const juce::Identifier ProjectState::PROP_TYPE("type");
const juce::Identifier ProjectState::PROP_VOLUME("volume");
const juce::Identifier ProjectState::PROP_PAN("pan");
const juce::Identifier ProjectState::PROP_MUTE("mute");
const juce::Identifier ProjectState::PROP_SOLO("solo");
const juce::Identifier ProjectState::PROP_ARMED("armed");
const juce::Identifier ProjectState::PROP_INPUT_MONITOR("inputMonitor");
const juce::Identifier ProjectState::PROP_ACTIVE_TAKE("activeTake");
const juce::Identifier ProjectState::PROP_EXPANDED("expanded");

const juce::Identifier ProjectState::PROP_START("start");
const juce::Identifier ProjectState::PROP_LENGTH("length");
const juce::Identifier ProjectState::PROP_OFFSET("offset");
const juce::Identifier ProjectState::PROP_AUDIO_FILE("audioFile");
const juce::Identifier ProjectState::PROP_LOOP_LENGTH("loopLength");
const juce::Identifier ProjectState::PROP_FADE_IN("fadeIn");
const juce::Identifier ProjectState::PROP_FADE_OUT("fadeOut");

// Beat-based clip properties
const juce::Identifier ProjectState::PROP_START_BEATS("startBeats");
const juce::Identifier ProjectState::PROP_LENGTH_BEATS("lengthBeats");
const juce::Identifier ProjectState::PROP_LANE_INDEX("laneIndex");

// MIDI note properties
const juce::Identifier ProjectState::PROP_PITCH("pitch");
const juce::Identifier ProjectState::PROP_VELOCITY("velocity");
const juce::Identifier ProjectState::PROP_PROBABILITY("probability");
const juce::Identifier ProjectState::PROP_CONDITION("condition");
const juce::Identifier ProjectState::PROP_RECURRENCE("recurrence");
const juce::Identifier ProjectState::PROP_ARTICULATION_ID("articulationId");
const juce::Identifier ProjectState::PROP_NOTE_TENSION("tension");

// Automation properties
const juce::Identifier ProjectState::PROP_PARAM("param");
const juce::Identifier ProjectState::PROP_PARAM_ID("paramId");
const juce::Identifier ProjectState::PROP_TIME_BEATS("timeBeats");

const juce::Identifier ProjectState::PROP_VALUE("value");
const juce::Identifier ProjectState::PROP_CURVE_TYPE("curveType");
const juce::Identifier ProjectState::PROP_TENSION("tension");
const juce::Identifier ProjectState::PROP_TAKE_INDEX("takeIndex");

// Plugin Automation Properties
const juce::Identifier ProjectState::PROP_PLUGIN_INDEX("pluginIndex");
const juce::Identifier ProjectState::PROP_PARAM_INDEX("paramIndex");
const juce::Identifier ProjectState::PROP_PARAM_NAME("paramName");


// Tempo/Marker properties
const juce::Identifier ProjectState::PROP_BPM("bpm");
const juce::Identifier ProjectState::PROP_COLOR("color");
const juce::Identifier ProjectState::PROP_NEXT_ID("nextId");
const juce::Identifier ProjectState::PROP_INPUT_CHANNEL("inputChannel");
const juce::Identifier ProjectState::PROP_MANUALLY_COLORED("manuallyColored");
const juce::Identifier ProjectState::PROP_IS_QUARANTINE("isQuarantine");

const juce::Identifier ProjectState::PROP_SELECTED_TRACK_ID("selectedTrackId");

//==============================================================================
ProjectState::ProjectState() : state(Zenith::IDs::PROJECT) {
  DBG("ProjectState: Constructor");

  state.setProperty(PROP_ID, "root", nullptr);
  state.getOrCreateChildWithName(Zenith::IDs::TRACKS, nullptr);

  trackStateManager = std::make_unique<TrackStateManager>(*this);
  clipStateManager = std::make_unique<ClipStateManager>(*this);
  automationStateManager = std::make_unique<AutomationStateManager>(*this);
  projectFileIO = std::make_unique<ProjectFileIO>(*this);

  createDefaultState(); // newProject calls this via IO, but we need state
                        // initialized before listeners?
  // newProject logic is now in ProjectFileIO.
  // We should call newProject() on the IO manager.
  // But newProject() in ProjectState calls createDefaultState().
  // Let's delegate.

  newProject();
  state.addListener(this);

  // Start autosave timer by default (5 minutes)
  startAutosaveTimer(5);
}

ProjectState::~ProjectState() {
  DBG("ProjectState: Destructor");
  stopTimer();
  state.removeListener(this);
}

//==============================================================================
// Project Management
//==============================================================================

void ProjectState::rebuildTrackMap() {
  trackIdMap_.clear();
  nodeCache_.clear();
  
  auto tracksNode = state.getChildWithName(ID_TRACKS);

  if (tracksNode.isValid()) {
    for (const auto &track : tracksNode) {
      if (track.hasType(ID_TRACK)) {
        juce::String id = track.getProperty(PROP_ID).toString();
        if (id.isNotEmpty())
          trackIdMap_[id] = track;
      }
    }
  }

  // RECURSIVE LIGHTNING CACHE (God Mode Optimization)
  std::function<void(juce::ValueTree)> cacheNode = [&](juce::ValueTree n) {
      juce::String id = n.getProperty(PROP_ID).toString();
      if (id.isNotEmpty()) nodeCache_[id] = n;
      for (int i = 0; i < n.getNumChildren(); ++i) cacheNode(n.getChild(i));
  };
  cacheNode(state);
}

void ProjectState::newProject() {
  if (projectFileIO)
    projectFileIO->newProject();
}

bool ProjectState::loadFromFile(const juce::File &file) {
  if (projectFileIO)
    return projectFileIO->loadFromFile(file) == FileIOError::Success;
  return false;
}

bool ProjectState::saveToFile(const juce::File &file) {
  if (projectFileIO)
    return projectFileIO->saveToFile(file) == FileIOError::Success;
  return false;
}

juce::File ProjectState::saveCrashDump() {
  if (projectFileIO) {
    projectFileIO->autoSave();
    return projectFileIO->getRecoveryFile();
  }
  return juce::File();
}

void ProjectState::timerCallback() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (isDirty && projectFile.existsAsFile()) {
    DBG("ProjectState: Autosaving...");

    auto autosaveFile = projectFile.getSiblingFile(
        projectFile.getFileNameWithoutExtension() + "_autosave" +
        projectFile.getFileExtension());

    if (projectFileIO) {
      ProjectFileIO::IOSettings settings;
      settings.format =
          ProjectFileIO::SerializationFormat::MessagePack; // favor speed for
                                                           // autosave
      settings.useAtomicWrite = true;

      projectFileIO->saveToFileAsync(
          autosaveFile, settings, [this](bool success, juce::String error) {
            if (success) {
              DBG("ProjectState: Autosave successful");
            } else {
              DBG("ProjectState: Autosave failed: " + error);
            }
          });
    }
  }
}

void ProjectState::startAutosaveTimer(int intervalMinutes) {
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
      if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(intervalMinutes * 60 * 1000);
}

void ProjectState::stopAutosaveTimer() { stopTimer(); }

void ProjectState::valueTreeChildAdded(juce::ValueTree &parent,
                                       juce::ValueTree &child) {
  isDirty = true;

  // Add to cache
  juce::String id = child.getProperty(PROP_ID).toString();
  if (id.isNotEmpty()) nodeCache_[id] = child;

  if (child.hasType(ID_TRACK)) {
    if (id.isNotEmpty())
      trackIdMap_[id] = child;
  }
}

void ProjectState::valueTreeChildRemoved(juce::ValueTree &parent,
                                         juce::ValueTree &child, int) {
  isDirty = true;
  
  // Remove from cache
  juce::String id = child.getProperty(PROP_ID).toString();
  if (id.isNotEmpty()) nodeCache_.erase(id);
}

//==============================================================================
// Project Properties
//==============================================================================

juce::String ProjectState::getProjectName() const {
  return state[PROP_NAME].toString();
}

void ProjectState::setProjectName(const juce::String &name) {
  state.setProperty(PROP_NAME, name, &undoManager);
}

double ProjectState::getTempo() const { 
    double tempo = state[PROP_TEMPO]; 
    return std::max(0.1, tempo);
}

void ProjectState::setTempo(double tempo) {
  tempo = juce::jlimit(20.0, 999.0, tempo);
  state.setProperty(PROP_TEMPO, tempo, &undoManager);
}

int ProjectState::getTimeSignatureNumerator() const {
  return state[PROP_TIME_SIG_NUM];
}

int ProjectState::getTimeSignatureDenominator() const {
  return state[PROP_TIME_SIG_DEN];
}

void ProjectState::setTimeSignature(int numerator, int denominator) {
  state.setProperty(PROP_TIME_SIG_NUM, numerator, &undoManager);
  state.setProperty(PROP_TIME_SIG_DEN, denominator, &undoManager);
}

//==============================================================================
// Track Management
//==============================================================================

juce::String ProjectState::addTrack(const juce::String &name,
                                    const juce::String &type) {
  if (trackStateManager)
    return trackStateManager->addTrack(name, type);
  return {};
}

void ProjectState::removeTrack(const juce::String &trackId,
                               const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->removeTrack(trackId);
}

juce::String ProjectState::duplicateTrack(const juce::String &trackId,
                                          const juce::String &actionName) {
  if (trackStateManager)
    return trackStateManager->duplicateTrack(trackId, actionName);
  return {};
}

juce::String ProjectState::insertTrackAbove(const juce::String &targetTrackId,
                                            const juce::String &type,
                                            const juce::String &actionName) {
  if (trackStateManager)
    return trackStateManager->insertTrackAbove(targetTrackId, type, actionName);
  return {};
}

juce::String ProjectState::insertTrackBelow(const juce::String &targetTrackId,
                                            const juce::String &type,
                                            const juce::String &actionName) {
  if (trackStateManager)
    return trackStateManager->insertTrackBelow(targetTrackId, type, actionName);
  return {};
}

int ProjectState::getNumTracks() const {
  if (trackStateManager)
    return trackStateManager->getNumTracks();
  return 0;
}

void ProjectState::moveTrack(const juce::String& trackId, int newIndex, const juce::String& actionName) {
  if (trackStateManager)
    trackStateManager->moveTrack(trackId, newIndex, actionName);
}

juce::ValueTree ProjectState::getTrack(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->getTrack(trackId);
  return {};
}

juce::ValueTree ProjectState::getTrackByIndex(int index) {
  if (trackStateManager)
    return trackStateManager->getTrackByIndex(index);
  return {};
}

//==============================================================================
// Track Mixer Properties (Getters)
//==============================================================================

float ProjectState::getTrackVolume(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->getTrackVolume(trackId);
  return 1.0f;
}

float ProjectState::getTrackPan(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->getTrackPan(trackId);
  return 0.0f;
}

bool ProjectState::isTrackMuted(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->isTrackMuted(trackId);
  return false;
}

bool ProjectState::isTrackSolo(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->isTrackSolo(trackId);
  return false;
}

bool ProjectState::isTrackArmed(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->isTrackArmed(trackId);
  return false;
}

void ProjectState::setTrackInputMonitor(const juce::String &trackId,
                                        bool monitoring,
                                        const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackInputMonitor(trackId, monitoring, actionName);
}

bool ProjectState::isTrackInputMonitoring(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->isTrackInputMonitoring(trackId);
  return false;
}

juce::String ProjectState::getTrackName(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->getTrackName(trackId);
  return {};
}

juce::String ProjectState::getTrackType(const juce::String &trackId) const {
  if (trackStateManager)
    return trackStateManager->getTrackType(trackId);
  return {};
}

//==============================================================================
// Clip Management
//==============================================================================

juce::String ProjectState::addClip(const juce::String &trackId,
                                   double startBeats, double lengthBeats,
                                   const juce::String &actionName) {
  if (clipStateManager)
    return clipStateManager->createEmptyClip(trackId, startBeats, lengthBeats,
                                             false, "Clip", actionName);
  return {};
}

juce::String ProjectState::createEmptyClip(const juce::String &trackId,
                                           double startBeats,
                                           double lengthBeats, bool isMidi,
                                           const juce::String &name,
                                           const juce::String &actionName) {
  if (clipStateManager)
    return clipStateManager->createEmptyClip(trackId, startBeats, lengthBeats,
                                             isMidi, name, actionName);
  return {};
}

juce::String ProjectState::createAudioClip(const juce::String &trackId,
                                           double startBeats,
                                           double lengthBeats,
                                           const juce::String &audioFilePath,
                                           const juce::String &name,
                                           const juce::String &actionName) {
  // First create an empty audio clip
  if (!clipStateManager) return {};

  juce::String clipId = clipStateManager->createEmptyClip(trackId, startBeats, lengthBeats,
                                                          false, name, actionName);
  if (clipId.isEmpty()) return {};

  // Set the audio file path
  auto [track, clip] = findClip(clipId);
  if (clip.isValid()) {
    clip.setProperty(PROP_AUDIO_FILE, audioFilePath, &undoManager);
    clip.setProperty(PROP_TYPE, "audio", &undoManager);
  }

  DBG("ProjectState: Created audio clip " + clipId + " with file " + audioFilePath);
  return clipId;
}

bool ProjectState::removeClip(const juce::String &trackId,
                              const juce::String &clipId,
                              const juce::String &actionName) {
  if (clipStateManager)
    return clipStateManager->removeClip(trackId, clipId, actionName);
  return false;
}

void ProjectState::moveClip(const juce::String &clipId,
                            const juce::String &newTrackId,
                            double newStartBeats,
                            const juce::String &actionName) {
  if (clipStateManager)
    clipStateManager->moveClipToTrack(clipId, newTrackId, newStartBeats,
                                      actionName);
}

void ProjectState::setClipRange(const juce::String &clipId,
                                double newStartBeats, double newLengthBeats,
                                const juce::String &actionName) {
  if (clipStateManager)
    clipStateManager->setClipRange(clipId, newStartBeats, newLengthBeats,
                                   actionName);
}

void ProjectState::resizeClip(const juce::String &trackId,
                              const juce::String &clipId,
                              juce::int64 newLengthSamples,
                              const juce::String &actionName) {
  if (clipStateManager)
    clipStateManager->resizeClip(
        trackId, clipId, (double)newLengthSamples,
        actionName); // Cast to double as manager uses double
}

void ProjectState::deleteClip(const juce::String &clipId,
                              const juce::String &actionName) {
  if (clipStateManager)
    clipStateManager->deleteClip(clipId, actionName);
}

// Retained setClipAudioFile and getClipAudioFile for now or delegate
bool ProjectState::setClipAudioFile(const juce::String &trackId,
                                    const juce::String &clipId,
                                    const juce::File &audioFile,
                                    const juce::String &actionName) {
  // Delegate to manager - note relative path logic is lost for now
  if (clipStateManager)
    return clipStateManager->setClipAudioFile(trackId, clipId, audioFile,
                                              actionName);
  return false;
}

juce::String ProjectState::getClipAudioFile(const juce::String &trackId,
                                            const juce::String &clipId) const {
  if (clipStateManager)
    return clipStateManager->getClipAudioFile(trackId, clipId);
  return {};
}

juce::String ProjectState::getClipName(const juce::String &clipId) const {
  auto [track, clip] = findClip(clipId);
  if (clip.isValid())
    return clip[PROP_NAME].toString();
  return {};
}

void ProjectState::renameClip(const juce::String &clipId,
                              const juce::String &newName) {
  auto [track, clip] = findClip(clipId);
  if (clip.isValid()) {
    undoManager.beginNewTransaction("Rename Clip");
    clip.setProperty(PROP_NAME, newName, &undoManager);
  }
}

juce::ValueTree ProjectState::getClip(const juce::String &trackId,
                                      const juce::String &clipId) const {
  if (clipStateManager)
    return clipStateManager->getClip(trackId, clipId);
  return {};
}

std::pair<juce::ValueTree, juce::ValueTree>
ProjectState::findClip(const juce::String &clipId) const {
  if (clipStateManager)
    return clipStateManager->findClip(clipId);
  return {{}, {}};
}

void ProjectState::setClipFade(const juce::String &clipId, double fadeInBeats,
                               double fadeOutBeats,
                               const juce::String &actionName) {
  auto [track, clip] = findClip(clipId);
  if (clip.isValid()) {
    undoManager.beginNewTransaction(actionName);
    // Validate range? Max fade = length/2 or length?
    // Let's clamps to length/2 or just length? 
    // Logic Pro allows crossing.
    // We'll trust caller or clamp to 0 min.
    fadeInBeats = std::max(0.0, fadeInBeats);
    fadeOutBeats = std::max(0.0, fadeOutBeats);
    
    // Check against length?
    double length = clip[PROP_LENGTH_BEATS];
    if (fadeInBeats + fadeOutBeats > length) {
        // Simple clamp if desired, but user might want crossfade logic.
        // For now, allow but maybe warn or clamp?
        // Let's not clamp rigidly to prevent stuck UI.
    }

    clip.setProperty(PROP_FADE_IN, fadeInBeats, &undoManager);
    clip.setProperty(PROP_FADE_OUT, fadeOutBeats, &undoManager);
  }
}

void ProjectState::setClipColor(const juce::String &clipId, const juce::Colour &color) {
  auto [track, clip] = findClip(clipId);
  if (clip.isValid()) {
    undoManager.beginNewTransaction("Set Clip Color");
    clip.setProperty(PROP_COLOR, color.toString(), &undoManager);
  }
}

//==============================================================================
// Automation Management
//==============================================================================

juce::ValueTree
ProjectState::getOrCreateAutomationEnvelope(const juce::String &trackId,
                                            const juce::String &paramId) {
  if (automationStateManager)
    return automationStateManager->getOrCreateAutomationEnvelope(trackId,
                                                                 paramId);
  return {};
}

juce::ValueTree
ProjectState::getAutomationEnvelope(const juce::String &trackId,
                                    const juce::String &paramId) const {
  if (automationStateManager)
    return automationStateManager->getAutomationEnvelope(trackId, paramId);
  return {};
}

bool ProjectState::hasAutomation(const juce::String &trackId,
                                 const juce::String &paramId) const {
  if (automationStateManager)
    return automationStateManager->hasAutomation(trackId, paramId);
  return false;
}

juce::String ProjectState::addAutomationPoint(const juce::String &trackId,
                                              const juce::String &paramId,
                                              double timeBeats, double value,
                                              float tension, int curveType,
                                              const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->addAutomationPoint(
        trackId, paramId, timeBeats, value, tension, curveType, actionName);
  return {};
}

juce::String ProjectState::addAutomationPoint(const juce::String &trackId,
                                              const juce::String &paramId,
                                              double timeBeats, double value,
                                              const juce::String &actionName) {
  // Delegate to 7-arg version
  return addAutomationPoint(trackId, paramId, timeBeats, value, 0.0f, 0,
                            actionName);
}

bool ProjectState::moveAutomationPoint(const juce::String &trackId,
                                       const juce::String &paramId,
                                       const juce::String &pointId,
                                       double newTimeBeats, double newValue,
                                       const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->moveAutomationPoint(
        trackId, paramId, pointId, newTimeBeats, newValue, actionName);
  return false;
}

bool ProjectState::deleteAutomationPoint(const juce::String &trackId,
                                         const juce::String &paramId,
                                         const juce::String &pointId,
                                         const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->deleteAutomationPoint(trackId, paramId,
                                                         pointId, actionName);
  return false;
}

bool ProjectState::clearAutomation(const juce::String &trackId,
                                   const juce::String &paramId,
                                   const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->clearAutomation(trackId, paramId,
                                                   actionName);
  return false;
}

bool ProjectState::setAutomationTension(const juce::String &trackId,
                                        const juce::String &paramId,
                                        const juce::String &pointId,
                                        float tension,
                                        const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->setAutomationTension(
        trackId, paramId, pointId, tension, actionName);
  return false;
}

bool ProjectState::setAutomationCurveType(const juce::String &trackId,
                                          const juce::String &paramId,
                                          const juce::String &pointId,
                                          int curveType,
                                          const juce::String &actionName) {
  if (automationStateManager)
    return automationStateManager->setAutomationCurveType(
        trackId, paramId, pointId, curveType, actionName);
  return false;
}

//==============================================================================
// Undo/Redo
//==============================================================================

void ProjectState::undo() {
  if (undoManager.canUndo()) {
    undoManager.undo();
    DBG("ProjectState: Undo");
  }
}

void ProjectState::redo() {
  if (undoManager.canRedo()) {
    undoManager.redo();
    DBG("ProjectState: Redo");
  }
}

//==============================================================================
// MIDI Note Management (Phase 8)
//==============================================================================

juce::Array<ProjectState::MidiNoteSpec>
ProjectState::getMidiNotesForClip(const juce::String &clipId) const {
  juce::Array<MidiNoteSpec> notes;

  auto [track, clip] = findClip(clipId);
  if (!clip.isValid())
    return notes;

  auto midiNotesNode = clip.getChildWithName(ID_NOTES);
  if (!midiNotesNode.isValid())
    return notes;

  for (const auto &noteTree : midiNotesNode) {
    if (!noteTree.hasType(ID_NOTE))
      continue;

    MidiNoteSpec note;
    note.id = noteTree[PROP_ID].toString();
    note.pitch = noteTree[PROP_PITCH];
    note.startBeats = noteTree[PROP_START_BEATS];
    note.lengthBeats = noteTree[PROP_LENGTH_BEATS];
    note.velocity = noteTree[PROP_VELOCITY];
    note.muted = noteTree.getProperty(PROP_MUTE, false);
    note.probability = noteTree.getProperty(PROP_PROBABILITY, 1.0f);
    note.condition = noteTree.getProperty(PROP_CONDITION).toString();
    note.recurrence = noteTree.getProperty(PROP_RECURRENCE).toString();
    note.articulationId = noteTree.getProperty(PROP_ARTICULATION_ID, 0);
    note.tension = noteTree.getProperty(PROP_NOTE_TENSION, 0.0f);

    notes.add(note);
  }

  return notes;
}

juce::String ProjectState::addMidiNote(const juce::String &clipId,
                                       const MidiNoteSpec &note,
                                       const juce::String &actionName) {
  auto [track, clip] = findClip(clipId);
  if (!clip.isValid()) {
    DBG("ProjectState: Cannot add MIDI note - clip not found: " + clipId);
    return {};
  }

  // Get or create MIDI_NOTES container
  auto midiNotesNode = clip.getChildWithName(ID_NOTES);
  if (!midiNotesNode.isValid()) {
    midiNotesNode = juce::ValueTree(ID_NOTES);
    clip.appendChild(midiNotesNode, &undoManager);
  }

  // Generate note ID if not provided
  juce::String noteId = note.id;
  if (noteId.isEmpty())
    noteId = generateUniqueId("note");

  // Validate note properties
  int pitch = juce::jlimit(0, 127, note.pitch);
  int velocity = juce::jlimit(0, 127, note.velocity);
  double startBeats = juce::jmax(0.0, note.startBeats);
  double lengthBeats = juce::jmax(0.0, note.lengthBeats);

  // Create note tree
  juce::ValueTree noteTree(ID_NOTE);
  noteTree.setProperty(PROP_ID, noteId, nullptr);
  noteTree.setProperty(PROP_PITCH, pitch, nullptr);
  noteTree.setProperty(PROP_START_BEATS, startBeats, nullptr);
  noteTree.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
  noteTree.setProperty(PROP_VELOCITY, velocity, nullptr);
  if (note.muted)
    noteTree.setProperty(PROP_MUTE, true, nullptr);
  if (note.probability < 1.0f)
    noteTree.setProperty(PROP_PROBABILITY, note.probability, nullptr);
  if (note.condition.isNotEmpty())
    noteTree.setProperty(PROP_CONDITION, note.condition, nullptr);
  if (note.recurrence.isNotEmpty())
    noteTree.setProperty(PROP_RECURRENCE, note.recurrence, nullptr);
  if (note.articulationId != 0)
    noteTree.setProperty(PROP_ARTICULATION_ID, note.articulationId, nullptr);
  if (std::abs(note.tension) > 0.001f)
    noteTree.setProperty(PROP_NOTE_TENSION, note.tension, nullptr);

  // Begin transaction
  undoManager.beginNewTransaction(actionName);
  midiNotesNode.appendChild(noteTree, &undoManager);

  DBG("ProjectState: Added MIDI note " + noteId + " to clip " + clipId);

  return noteId;
}

void ProjectState::removeMidiNote(const juce::String &clipId,
                                  const juce::String &noteId,
                                  const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot remove MIDI note - note not found: " + noteId);
    return;
  }

  auto parent = noteTree.getParent();
  if (parent.isValid()) {
    undoManager.beginNewTransaction(actionName);
    parent.removeChild(noteTree, &undoManager);
    DBG("ProjectState: Removed MIDI note " + noteId + " from clip " + clipId);
  }
}

void ProjectState::moveMidiNote(const juce::String &clipId,
                                const juce::String &noteId,
                                double newStartBeats, int newPitch,
                                const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot move MIDI note - note not found: " + noteId);
    return;
  }

  // Validate new values
  int pitch = juce::jlimit(0, 127, newPitch);
  double startBeats = juce::jmax(0.0, newStartBeats);

  undoManager.beginNewTransaction(actionName);
  noteTree.setProperty(PROP_PITCH, pitch, &undoManager);
  noteTree.setProperty(PROP_START_BEATS, startBeats, &undoManager);

  DBG("ProjectState: Moved MIDI note " + noteId + " in clip " + clipId);
}

void ProjectState::quantizeClip(const juce::String &clipId, double gridBeats,
                                const juce::String &actionName) {
  auto notes = getMidiNotesForClip(clipId);
  if (notes.isEmpty())
    return;

  if (gridBeats <= 0.0) {
    DBG("ProjectState: Invalid grid size for quantization: " +
        juce::String(gridBeats));
    return;
  }

  undoManager.beginNewTransaction(actionName);

  for (const auto &note : notes) {
    // Quantize start time to nearest grid point
    double quantizedStart = std::round(note.startBeats / gridBeats) * gridBeats;
    quantizedStart = juce::jmax(0.0, quantizedStart);

    auto noteTree = findMidiNote(clipId, note.id);
    if (noteTree.isValid()) {
      noteTree.setProperty(PROP_START_BEATS, quantizedStart, &undoManager);
    }
  }

  DBG("ProjectState: Quantized clip " + clipId + " to grid " +
      juce::String(gridBeats) + " beats");
}

void ProjectState::setMidiNoteVelocity(const juce::String &clipId,
                                       const juce::String &noteId,
                                       int newVelocity,
                                       const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot set velocity - note not found: " + noteId);
    return;
  }

  // Clamp velocity (1-127, never 0)
  int velocity = juce::jlimit(1, 127, newVelocity);

  undoManager.beginNewTransaction(actionName);
  noteTree.setProperty(PROP_VELOCITY, velocity, &undoManager);

  DBG("ProjectState: Set velocity for note " + noteId + " to " +
      juce::String(velocity));
}

void ProjectState::setMidiNoteLength(const juce::String &clipId,
                                     const juce::String &noteId,
                                     double newLengthBeats,
                                     const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot set length - note not found: " + noteId);
    return;
  }

  // Ensure positive length (minimum 0.01 beats)
  double lengthBeats = juce::jmax(0.01, newLengthBeats);

  undoManager.beginNewTransaction(actionName);
  noteTree.setProperty(PROP_LENGTH_BEATS, lengthBeats, &undoManager);

  DBG("ProjectState: Set length for note " + noteId + " to " +
      juce::String(lengthBeats) + " beats");
}

void ProjectState::setMidiNoteMuted(const juce::String &clipId,
                                    const juce::String &noteId, bool muted,
                                    const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot set muted - note not found: " + noteId);
    return;
  }

  undoManager.beginNewTransaction(actionName);

  if (muted)
    noteTree.setProperty(PROP_MUTE, true, &undoManager);
  else
    noteTree.removeProperty(
        PROP_MUTE, &undoManager); // Remove property when false to save space

  DBG("ProjectState: Set muted for note " + noteId + " to " +
      juce::String(muted ? "true" : "false"));
}

void ProjectState::setMidiNoteProbability(const juce::String &clipId,
                                          const juce::String &noteId,
                                          float probability,
                                          const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot set probability - note not found: " + noteId);
    return;
  }

  undoManager.beginNewTransaction(actionName);

  // Clamp probability
  probability = juce::jlimit(0.0f, 1.0f, probability);

  noteTree.setProperty(PROP_PROBABILITY, probability, &undoManager);

  DBG("ProjectState: Set probability for note " + noteId + " to " +
      juce::String(probability));
}

void ProjectState::setMidiNoteTension(const juce::String &clipId,
                                      const juce::String &noteId,
                                      float tension,
                                      const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (!noteTree.isValid()) {
    DBG("ProjectState: Cannot set tension - note not found: " + noteId);
    return;
  }

  undoManager.beginNewTransaction(actionName);
  noteTree.setProperty(PROP_NOTE_TENSION, tension, &undoManager);
}

void ProjectState::humanizeClip(const juce::String &clipId, double velocityRange,
                                double timeRangeBeats,
                                const juce::String &actionName) {
  auto notes = getMidiNotesForClip(clipId);
  if (notes.isEmpty())
    return;

  undoManager.beginNewTransaction(actionName);
  
  juce::Random rng;
  rng.setSeedRandomly();

  for (const auto &note : notes) {
    auto noteTree = findMidiNote(clipId, note.id);
    if (!noteTree.isValid())
      continue;
      
    // Humanize Velocity
    // Use ceil to ensure non-zero range if input > 0 (handle 0.1 test case)
    int iRange = (int)std::ceil(velocityRange);
    if (iRange > 0) {
        int velOffset = rng.nextInt(juce::Range<int>(-iRange, iRange + 1));
        int newVel = juce::jlimit(1, 127, note.velocity + velOffset);
        noteTree.setProperty(PROP_VELOCITY, newVel, &undoManager);
    }
    
    // Humanize Time (small random offset)
    // -timeRangeBeats/2 to +timeRangeBeats/2
    if (timeRangeBeats > 0.0) {
        double timeOffset = (rng.nextDouble() - 0.5) * timeRangeBeats;
        double newStart = juce::jmax(0.0, note.startBeats + timeOffset);
        noteTree.setProperty(PROP_START_BEATS, newStart, &undoManager);
    }
  }
}

void ProjectState::legatoClip(const juce::String &clipId, bool adjustOverlap,
                              const juce::String &actionName) {
  auto notes = getMidiNotesForClip(clipId);
  if (notes.isEmpty())
    return;
    
  undoManager.beginNewTransaction(actionName);
  
  // Sort notes by start time
  std::sort(notes.begin(), notes.end(), [](const MidiNoteSpec &a, const MidiNoteSpec &b) {
      if (std::abs(a.startBeats - b.startBeats) < 0.0001)
          return a.pitch < b.pitch; // Secondary sort by pitch
      return a.startBeats < b.startBeats;
  });
  
  // We need to group them if we were doing Logic-style legato across voices,
  // but for basic legato we often just want "monophonic" style or per-voice.
  // A simple approach: Extend note until the next note starts. 
  // If polyphonic, this is tricky. Logic has "Legato" which extends until the *very next note event* regardless of pitch.
  // Let's implement that standard behavior first.
  
  for (int i = 0; i < notes.size() - 1; ++i) {
      const auto& current = notes.getReference(i);
      const auto& next = notes.getReference(i + 1);
      
      double distToNext = next.startBeats - current.startBeats;
      
      if (distToNext > 0) {
          double newLen = distToNext;
          
          if (adjustOverlap && current.lengthBeats > newLen) {
             // Shorten if it overlaps
             auto noteTree = findMidiNote(clipId, current.id);
             if (noteTree.isValid())
                 noteTree.setProperty(PROP_LENGTH_BEATS, newLen, &undoManager);
          } else if (current.lengthBeats < newLen) {
             // Extend if gap
             auto noteTree = findMidiNote(clipId, current.id);
             if (noteTree.isValid())
                 noteTree.setProperty(PROP_LENGTH_BEATS, newLen, &undoManager);
          }
      }
  }
}


//==============================================================================
// Helper Methods
//==============================================================================

juce::File ProjectState::getAssetDirectory(const juce::String& name) {
    if (projectFile.exists())
        return projectFile.getSiblingFile(name);
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Documents/Zenith/Untitled/" + name);
}

void ProjectState::createDefaultState() {
  // Create root PROJECT node
  state = juce::ValueTree(ID_PROJECT);

  // Set default properties
  state.setProperty(PROP_ID, "project_root", nullptr);
  state.setProperty(PROP_NAME, "Untitled Project", nullptr);
  state.setProperty(PROP_TEMPO, 120.0, nullptr);
  state.setProperty(PROP_TIME_SIG_NUM, 4, nullptr);
  state.setProperty(PROP_TIME_SIG_DEN, 4, nullptr);
  state.setProperty(PROP_SAMPLE_RATE, 44100.0, nullptr);

  // Create TRACKS node
  juce::ValueTree tracks(ID_TRACKS);
  tracks.setProperty(PROP_ID, "tracks_list", nullptr);
  state.appendChild(tracks, nullptr);

  // Create MIXER node
  juce::ValueTree mixer(ID_MIXER);
  mixer.setProperty(PROP_ID, "mixer_node", nullptr);
  mixer.setProperty(PROP_VOLUME, 0.8, nullptr);
  state.appendChild(mixer, nullptr);

  // Create TEMPO_MAP node
  juce::ValueTree tempoMap(ID_TEMPO_MAP);
  tempoMap.setProperty(PROP_ID, "tempo_map", nullptr);
  state.appendChild(tempoMap, nullptr);

  DBG("ProjectState: Default state created");
}

juce::String ProjectState::generateUniqueId(const juce::String &prefix) {
  int id = idCounter.fetch_add(1);
  // Persist the next ID so we don't have to scan on load (O(1) lookup vs O(N)
  // scan) We don't use undoManager here to avoid polluting the undo stack with
  // ID increments
  state.setProperty(PROP_NEXT_ID, id + 1, nullptr);
  return prefix + "_" + juce::String(id);
}

juce::ValueTree
ProjectState::findTrackInternal(const juce::String &trackId) const {
  // O(1) Lookup
  auto it = trackIdMap_.find(trackId);
  if (it != trackIdMap_.end())
    return it->second;

  return {};
}

juce::ValueTree ProjectState::findTrack(const juce::String &trackId) const {
  // O(1) Lookup (const version)
  auto it = trackIdMap_.find(trackId);
  if (it != trackIdMap_.end())
    return it->second;

  return {};
}

// findMidiNote is defined later with const qualifier

juce::ValueTree
ProjectState::findAutomationPoint(const juce::ValueTree &envelope,
                                  const juce::String &pointId) const {
  if (!envelope.isValid())
    return {};

  // Points are stored in the ID_POINTS container
  auto pointsNode = envelope.getChildWithName(ID_POINTS);
  if (!pointsNode.isValid())
    return {};

  for (const auto &point : pointsNode) {
    if (point.hasType(ID_POINT) && point[PROP_ID].toString() == pointId)
      return point;
  }

  return {};
}

void ProjectState::rebuildIdCounter() {
  // Check if we have a stored nextId to avoid O(N) tree scan on load
  if (state.hasProperty(PROP_NEXT_ID)) {
    idCounter.store(static_cast<int>(state[PROP_NEXT_ID]));
    return;
  }

  int highestId = -1;

  std::function<void(const juce::ValueTree &)> scanTree =
      [&](const juce::ValueTree &node) {
        if (!node.isValid())
          return;

        if (node.hasProperty(PROP_ID)) {
          const int suffix = extractNumericSuffix(node[PROP_ID].toString());

          if (suffix > highestId)
            highestId = suffix;
        }

        for (int i = 0; i < node.getNumChildren(); ++i)
          scanTree(node.getChild(i));
      };

  scanTree(state);

  idCounter.store(highestId + 1);
  // Store it for next time
  state.setProperty(PROP_NEXT_ID, highestId + 1, nullptr);
}

juce::ValueTree ProjectState::findClip(const juce::String &trackId,
                                       const juce::String &clipId) const {
  auto track = findTrack(trackId);
  if (!track.isValid())
    return {};

  auto clipsNode = track.getChildWithName(ID_CLIPS);
  if (!clipsNode.isValid())
    return {};

  for (const auto &clip : clipsNode) {
    if (clip.hasType(ID_CLIP) && clip[PROP_ID].toString() == clipId)
      return clip;
  }

  return {};
}

juce::ValueTree ProjectState::findNote(const juce::String &trackId,
                                       const juce::String &clipId,
                                       const juce::String &noteId) {
  auto clip = findClip(trackId, clipId);
  if (!clip.isValid())
    return {};

  auto notesNode = clip.getChildWithName(ID_NOTES);
  if (!notesNode.isValid())
    return {};

  for (const auto &note : notesNode) {
    if (note.hasType(ID_NOTE) && note[PROP_ID].toString() == noteId)
      return note;
  }

  return {};
}

juce::ValueTree ProjectState::findMidiNote(const juce::String &clipId,
                                           const juce::String &noteId) const {
  // Find the clip first using the single-argument findClip
  auto [track, clip] = findClip(clipId);
  if (!clip.isValid())
    return {};

  auto notesNode = clip.getChildWithName(ID_NOTES);
  if (!notesNode.isValid())
    return {};

  for (const auto &note : notesNode) {
    if (note.hasType(ID_NOTE) && note[PROP_ID].toString() == noteId)
      return note;
  }

  return {};
}

//==============================================================================
// U4.1: Clip Management (Beat-Based)
//==============================================================================

juce::String ProjectState::addClip(const juce::String &trackId,
                                   const juce::String &clipType,
                                   double startBeats, double lengthBeats,
                                   int laneIndex) {
  if (clipStateManager)
    return clipStateManager->addClip(trackId, clipType, startBeats, lengthBeats,
                                     laneIndex);
  return {};
}

void ProjectState::deleteClip(const juce::String &trackId,
                              const juce::String &clipId,
                              const juce::String &actionName) {
  if (clipStateManager)
    clipStateManager->removeClip(trackId, clipId, actionName);
}

bool ProjectState::removeClip(const juce::String &trackId,
                              const juce::String &clipId) {
  if (clipStateManager)
    return clipStateManager->removeClip(trackId, clipId);
  return false;
}

bool ProjectState::moveClip(const juce::String &trackId,
                            const juce::String &clipId, double newStartBeats) {
  if (clipStateManager)
    return clipStateManager->moveClip(trackId, clipId, newStartBeats);
  return false;
}

bool ProjectState::resizeClip(const juce::String &trackId,
                              const juce::String &clipId,
                              double newLengthBeats) {
  if (clipStateManager)
    return clipStateManager->resizeClip(trackId, clipId, newLengthBeats);
  return false;
}

//==============================================================================
// U4.1: MIDI Note Management (Matching Header API - clipId-only)
//==============================================================================

juce::ValueTree
ProjectState::getOrCreateNotesContainer(const juce::String &clipId) {
  auto [track, clip] = findClip(clipId);
  if (!clip.isValid())
    return {};

  auto notesNode = clip.getChildWithName(ID_NOTES);
  if (!notesNode.isValid()) {
    notesNode = juce::ValueTree(ID_NOTES);
    clip.appendChild(notesNode, &undoManager);
  }

  return notesNode;
}

juce::String ProjectState::addNote(const juce::String &clipId,
                                   double startBeats, double lengthBeats,
                                   int pitch, int velocity,
                                   const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Validate parameters
  if (lengthBeats <= 0.0) {
    DBG("ProjectState: Invalid note length: " + juce::String(lengthBeats));
    return {};
  }

  if (startBeats < 0.0)
    startBeats = 0.0;

  pitch = juce::jlimit(0, 127, pitch);
  velocity = juce::jlimit(0, 127, velocity);

  auto [track, clip] = findClip(clipId);
  if (!clip.isValid()) {
    DBG("ProjectState: Clip not found: " + clipId);
    return {};
  }

  // Verify it's a MIDI clip
  if (clip[PROP_TYPE].toString() != "midi") {
    DBG("ProjectState: Cannot add note to non-MIDI clip");
    return {};
  }

  // Get or create NOTES node
  auto notesNode = clip.getChildWithName(ID_NOTES);
  if (!notesNode.isValid()) {
    notesNode = juce::ValueTree(ID_NOTES);
    clip.appendChild(notesNode, &undoManager);
  }

  // Generate unique note ID
  auto noteId = generateUniqueId("note");

  // Create note ValueTree
  juce::ValueTree note(ID_NOTE);
  note.setProperty(PROP_ID, noteId, nullptr);
  note.setProperty(PROP_START_BEATS, startBeats, nullptr);
  note.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
  note.setProperty(PROP_PITCH, pitch, nullptr);
  note.setProperty(PROP_VELOCITY, velocity, nullptr);

  // Insert in sorted order by startBeats, then by pitch
  int insertIndex = 0;
  for (int i = 0; i < notesNode.getNumChildren(); ++i) {
    auto existingNote = notesNode.getChild(i);
    double existingStart = existingNote[PROP_START_BEATS];
    int existingPitch = existingNote[PROP_PITCH];

    if (startBeats > existingStart ||
        (startBeats == existingStart && pitch >= existingPitch))
      insertIndex = i + 1;
    else
      break;
  }

  undoManager.beginNewTransaction(actionName);
  notesNode.addChild(note, insertIndex, &undoManager);

  DBG("ProjectState: Added note " + noteId + " (pitch=" + juce::String(pitch) +
      ", start=" + juce::String(startBeats) + " beats)");
  return noteId;
}

void ProjectState::addNotes(const juce::String &clipId,
                            const std::vector<MidiNoteSpec> &notes,
                            const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto [track, clip] = findClip(clipId);
  if (!clip.isValid()) {
    DBG("ProjectState: Clip not found for addNotes: " + clipId);
    return;
  }

  undoManager.beginNewTransaction(actionName);

  for (const auto &noteSpec : notes) {
    addNote(clipId, noteSpec.startBeats, noteSpec.lengthBeats, noteSpec.pitch,
            noteSpec.velocity,
            ""); // Empty action since we already started transaction
  }
}

bool ProjectState::moveNote(const juce::String &clipId,
                            const juce::String &noteId, double newStartBeats,
                            double newLengthBeats, int newPitch,
                            int newVelocity, const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (newStartBeats < 0.0)
    newStartBeats = 0.0;
  if (newLengthBeats <= 0.0)
    newLengthBeats = 0.1; // Minimum length

  newPitch = juce::jlimit(0, 127, newPitch);
  newVelocity = juce::jlimit(0, 127, newVelocity);

  auto note = findMidiNote(clipId, noteId);
  if (!note.isValid())
    return false;

  undoManager.beginNewTransaction(actionName);
  note.setProperty(PROP_START_BEATS, newStartBeats, &undoManager);
  note.setProperty(PROP_LENGTH_BEATS, newLengthBeats, &undoManager);
  note.setProperty(PROP_PITCH, newPitch, &undoManager);
  note.setProperty(PROP_VELOCITY, newVelocity, &undoManager);

  // Re-sort the notes in the parent container
  auto [track, clip] = findClip(clipId);
  if (clip.isValid()) {
    auto notesNode = clip.getChildWithName(ID_NOTES);
    if (notesNode.isValid()) {
      // Find current index
      int currentIndex = -1;
      for (int i = 0; i < notesNode.getNumChildren(); ++i) {
        if (notesNode.getChild(i)[PROP_ID].toString() == noteId) {
          currentIndex = i;
          break;
        }
      }

      if (currentIndex >= 0) {
        // Remove from current position
        notesNode.removeChild(currentIndex, &undoManager);

        // Find new sorted position
        int insertIndex = 0;
        for (int i = 0; i < notesNode.getNumChildren(); ++i) {
          auto existingNote = notesNode.getChild(i);
          double existingStart = existingNote[PROP_START_BEATS];
          int existingPitch = existingNote[PROP_PITCH];

          if (newStartBeats > existingStart ||
              (newStartBeats == existingStart && newPitch >= existingPitch))
            insertIndex = i + 1;
          else
            break;
        }

        // Re-insert at new position
        notesNode.addChild(note, insertIndex, &undoManager);
      }
    }
  }

  DBG("ProjectState: Moved note " + noteId);
  return true;
}

bool ProjectState::deleteNote(const juce::String &clipId,
                              const juce::String &noteId,
                              const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto [track, clip] = findClip(clipId);
  if (!clip.isValid())
    return false;

  auto notesNode = clip.getChildWithName(ID_NOTES);
  if (!notesNode.isValid())
    return false;

  for (int i = 0; i < notesNode.getNumChildren(); ++i) {
    auto note = notesNode.getChild(i);
    if (note.hasType(ID_NOTE) && note[PROP_ID].toString() == noteId) {
      undoManager.beginNewTransaction(actionName);
      notesNode.removeChild(i, &undoManager);
      DBG("ProjectState: Deleted note " + noteId);
      return true;
    }
  }

  return false;
}

juce::ValueTree ProjectState::getNotes(const juce::String &clipId) const {
  auto [track, clip] = findClip(clipId);
  if (!clip.isValid())
    return {};

  return clip.getChildWithName(ID_NOTES);
}

//==============================================================================
// U4.1: Debug Helpers
//==============================================================================

#if JUCE_DEBUG
void ProjectState::dumpClipStructureToLog() const {
  DBG("========================================");
  DBG("ProjectState Clip & Note Structure Dump");
  DBG("========================================");

  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);
  if (!tracksNode.isValid()) {
    DBG("No tracks found");
    return;
  }

  for (const auto &track : tracksNode) {
    if (!track.hasType(ProjectState::ID_TRACK))
      continue;

    juce::String trackId = track[ProjectState::PROP_ID].toString();
    juce::String trackName = track[ProjectState::PROP_NAME].toString();
    juce::String trackType = track[ProjectState::PROP_TYPE].toString();

    DBG("TRACK: " + trackId + " (" + trackName + ", type=" + trackType + ")");

    auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
    if (!clipsNode.isValid() || clipsNode.getNumChildren() == 0) {
      DBG("  (no clips)");
      continue;
    }

    for (const auto &clip : clipsNode) {
      if (!clip.hasType(ProjectState::ID_CLIP))
        continue;

      juce::String clipId = clip[ProjectState::PROP_ID].toString();
      juce::String clipType = clip[ProjectState::PROP_TYPE].toString();
      double startBeats = clip[ProjectState::PROP_START_BEATS];
      double lengthBeats = clip[ProjectState::PROP_LENGTH_BEATS];
      int laneIndex = clip[ProjectState::PROP_LANE_INDEX];

      DBG("  CLIP: " + clipId + " (type=" + clipType +
          ", start=" + juce::String(startBeats, 2) + " beats" +
          ", length=" + juce::String(lengthBeats, 2) + " beats" +
          ", lane=" + juce::String(laneIndex) + ")");

      // Show notes for MIDI clips
      if (clipType == "midi") {
        auto notesNode = clip.getChildWithName(ProjectState::ID_NOTES);
        if (!notesNode.isValid() || notesNode.getNumChildren() == 0) {
          DBG("    (no notes)");
          continue;
        }

        for (const auto &note : notesNode) {
          if (!note.hasType(ProjectState::ID_NOTE))
            continue;

          juce::String noteId = note[ProjectState::PROP_ID].toString();
          double noteStart = note[ProjectState::PROP_START_BEATS];
          double noteLength = note[ProjectState::PROP_LENGTH_BEATS];
          int pitch = note[ProjectState::PROP_PITCH];
          int velocity = note[ProjectState::PROP_VELOCITY];

          DBG("    NOTE: " + noteId + " (start=" + juce::String(noteStart, 2) +
              " beats" + ", length=" + juce::String(noteLength, 2) + " beats" +
              ", pitch=" + juce::String(pitch) +
              ", vel=" + juce::String(velocity) + ")");
        }
      }
    }
  }

  DBG("========================================");
}
#endif

//==============================================================================
// Track Property Management (Undoable)
//==============================================================================

void ProjectState::renameTrack(const juce::String &trackId,
                               const juce::String &newName,
                               const juce::String &actionName) {
  undoManager.beginNewTransaction(actionName);

  auto track = findTrack(trackId);
  if (track.isValid()) {
    track.setProperty(PROP_NAME, newName, &undoManager);
    DBG("ProjectState: Renamed track " + trackId + " to '" + newName + "'");
  }
}

void ProjectState::setTrackVolume(const juce::String &trackId,
                                  float volumeLinear,
                                  const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackVolume(trackId, volumeLinear, actionName);
}

void ProjectState::setTrackPan(const juce::String &trackId, float pan,
                               const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackPan(trackId, pan, actionName);
}

void ProjectState::setTrackMute(const juce::String &trackId, bool muted,
                                const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackMute(trackId, muted, actionName);
}

void ProjectState::setTrackSolo(const juce::String &trackId, bool soloed,
                                const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackSolo(trackId, soloed, actionName);
}

void ProjectState::setTrackArmed(const juce::String &trackId, bool armed,
                                 const juce::String &actionName) {
  if (trackStateManager)
    trackStateManager->setTrackArmed(trackId, armed, actionName);
}

//==============================================================================
// Clip Management (Undoable)
//==============================================================================

juce::String ProjectState::createClip(const juce::String &trackId,
                                      const juce::String &clipType,
                                      juce::int64 startSamples,
                                      juce::int64 lengthSamples,
                                      const juce::String &name,
                                      const juce::String &actionName) {
  undoManager.beginNewTransaction(actionName);

  auto track = findTrack(trackId);
  if (!track.isValid()) {
    DBG("ProjectState: Track not found: " + trackId);
    return {};
  }

  // Get or create CLIPS node
  auto clipsNode = track.getChildWithName(ID_CLIPS);
  if (!clipsNode.isValid()) {
    clipsNode = juce::ValueTree(ID_CLIPS);
    track.appendChild(clipsNode, &undoManager);
  }

  // Generate unique clip ID
  auto clipId = generateUniqueId("clip");

  // Create clip ValueTree
  juce::ValueTree clip(ID_CLIP);
  clip.setProperty(PROP_ID, clipId, nullptr);
  clip.setProperty(PROP_NAME, name, nullptr);
  clip.setProperty(PROP_TYPE, clipType, nullptr);
  clip.setProperty(PROP_START, startSamples, nullptr);
  clip.setProperty(PROP_LENGTH, lengthSamples, nullptr);
  clip.setProperty(PROP_OFFSET, 0, nullptr);

  // Add to track
  clipsNode.appendChild(clip, &undoManager);

  DBG("ProjectState: Created clip '" + name + "' with ID " + clipId +
      " on track " + trackId);

  return clipId;
}

void ProjectState::moveClip(const juce::String &trackId,
                            const juce::String &clipId,
                            juce::int64 newStartSamples,
                            const juce::String &actionName) {
  undoManager.beginNewTransaction(actionName);

  auto clip = getClip(trackId, clipId);
  if (clip.isValid()) {
    clip.setProperty(PROP_START, newStartSamples, &undoManager);
    DBG("ProjectState: Moved clip " + clipId + " to " +
        juce::String(newStartSamples));
  }
}

juce::String ProjectState::createTrack(const juce::String &type,
                                       const juce::String &name,
                                       const juce::String &actionName) {
  undoManager.beginNewTransaction(actionName);

  juce::String trackId = generateUniqueId("track_");
  juce::ValueTree newTrack(ID_TRACK);

  newTrack.setProperty(PROP_ID, trackId, &undoManager);
  newTrack.setProperty(PROP_NAME, name, &undoManager);
  newTrack.setProperty(PROP_TYPE, type, &undoManager);
  newTrack.setProperty(PROP_VOLUME, 1.0f, &undoManager);
  newTrack.setProperty(PROP_PAN, 0.0f, &undoManager);
  newTrack.setProperty(PROP_MUTE, false, &undoManager);
  newTrack.setProperty(PROP_SOLO, false, &undoManager);
  newTrack.setProperty(PROP_ARMED, false, &undoManager);

  // Create CLIPS container
  juce::ValueTree clips(ID_CLIPS);
  newTrack.addChild(clips, -1, &undoManager);

  auto tracksNode = state.getChildWithName(ID_TRACKS);
  if (!tracksNode.isValid()) {
    tracksNode = juce::ValueTree(ID_TRACKS);
    state.addChild(tracksNode, -1, &undoManager);
  }

  tracksNode.addChild(newTrack, -1, &undoManager);

  // Update map
  trackIdMap_[trackId] = newTrack;

  DBG("ProjectState: Created new track " + trackId + " (" + name + ")");
  return trackId;
}

std::pair<juce::String, juce::String>
ProjectState::splitClip(const juce::String &trackId, const juce::String &clipId,
                        juce::int64 splitSamples,
                        const juce::String &actionName) {
  undoManager.beginNewTransaction(actionName);

  auto originalClip = getClip(trackId, clipId);
  if (!originalClip.isValid()) {
    DBG("ProjectState: Clip not found: " + clipId);
    return {};
  }

  // Get original clip properties
  juce::int64 clipStart = originalClip[PROP_START];
  juce::int64 clipLength = originalClip[PROP_LENGTH];
  double clipLengthBeats = originalClip[PROP_LENGTH_BEATS];
  bool isBeatBased = (clipLengthBeats > 0.0);

  // If missing sample properties but has beats, infer them
  if (clipLength == 0 && isBeatBased) {
    auto inferred = inferSamplePropertiesFromBeats(originalClip);
    if (inferred.first < 0) return {};
    clipStart = inferred.first;
    clipLength = inferred.second;
  }

  juce::int64 clipEnd = clipStart + clipLength;

  // Validate split position with minimum segment guard (e.g. 10 samples)
  constexpr juce::int64 minSegmentSamples = 10;
  if (splitSamples < clipStart + minSegmentSamples || splitSamples > clipEnd - minSegmentSamples) {
    DBG("ProjectState: Split position too close to edge or invalid: " << splitSamples 
        << " (Clip: " << clipStart << " to " << clipEnd << ")");
    return {};
  }

  if (isBeatBased) {
    return splitBeatBasedClip(trackId, originalClip, splitSamples);
  } else {
    return splitSampleBasedClip(trackId, originalClip, splitSamples);
  }
}

std::pair<juce::int64, juce::int64> ProjectState::inferSamplePropertiesFromBeats(const juce::ValueTree& clip) {
  if (!state.hasProperty(PROP_SAMPLE_RATE)) {
    DBG("ProjectState: Cannot split beat-based clip without valid Project Sample Rate.");
    return {-1, -1};
  }
  
  double bpm = getTempo();
  double sampleRate = state[PROP_SAMPLE_RATE];
  if (sampleRate <= 0.0) return {-1, -1};
  
  double samplesPerBeat = (60.0 / bpm) * sampleRate;
  juce::int64 start = (juce::int64)((double)clip[PROP_START_BEATS] * samplesPerBeat);
  juce::int64 length = (juce::int64)((double)clip[PROP_LENGTH_BEATS] * samplesPerBeat);
  
  return {start, length};
}

std::pair<juce::String, juce::String> ProjectState::splitBeatBasedClip(const juce::String &trackId, juce::ValueTree originalClip, juce::int64 splitSamples) {
  juce::String clipId = originalClip[PROP_ID];
  juce::int64 clipStart = originalClip[PROP_START];
  if (clipStart == 0 && (double)originalClip[PROP_LENGTH_BEATS] > 0) {
      auto inferred = inferSamplePropertiesFromBeats(originalClip);
      clipStart = inferred.first;
  }
  
  juce::int64 leftLength = splitSamples - clipStart;
  juce::int64 clipLength = originalClip[PROP_LENGTH];
  if (clipLength == 0) {
      clipLength = inferSamplePropertiesFromBeats(originalClip).second;
  }
  
  double splitRatio = (clipLength > 0) ? (double)leftLength / (double)clipLength : 0.0;
  double clipLengthBeats = originalClip[PROP_LENGTH_BEATS];
  double leftBeats = clipLengthBeats * splitRatio;
  
  // Reuse sample-based split but update beats
  auto ids = splitSampleBasedClip(trackId, originalClip, splitSamples);
  
  auto leftClip = getClip(trackId, ids.first);
  auto rightClip = getClip(trackId, ids.second);
  
  if (leftClip.isValid()) {
    leftClip.setProperty(PROP_LENGTH_BEATS, leftBeats, &undoManager);
  }
  if (rightClip.isValid()) {
    rightClip.setProperty(PROP_START_BEATS, (double)originalClip[PROP_START_BEATS] + leftBeats, &undoManager);
    rightClip.setProperty(PROP_LENGTH_BEATS, clipLengthBeats - leftBeats, &undoManager);
  }
  
  return ids;
}

std::pair<juce::String, juce::String> ProjectState::splitSampleBasedClip(const juce::String &trackId, juce::ValueTree originalClip, juce::int64 splitSamples) {
  juce::String clipId = originalClip[PROP_ID];
  juce::String clipType = originalClip[PROP_TYPE];
  juce::String clipName = originalClip[PROP_NAME];
  juce::int64 clipStart = originalClip[PROP_START];
  juce::int64 clipLength = originalClip[PROP_LENGTH];
  juce::int64 clipOffset = originalClip[PROP_OFFSET];
  
  juce::int64 leftLength = splitSamples - clipStart;
  juce::int64 rightLength = clipLength - leftLength;

  juce::String leftId = createClip(trackId, clipType, clipStart, leftLength, clipName + " (L)", "");
  juce::String rightId = createClip(trackId, clipType, splitSamples, rightLength, clipName + " (R)", "");

  auto leftClip = getClip(trackId, leftId);
  auto rightClip = getClip(trackId, rightId);

  if (leftClip.isValid()) {
    leftClip.setProperty(PROP_OFFSET, clipOffset, &undoManager);
    migrateProperties(originalClip, leftClip);
  }
  if (rightClip.isValid()) {
    rightClip.setProperty(PROP_OFFSET, clipOffset + leftLength, &undoManager);
    migrateProperties(originalClip, rightClip);
  }

  deleteClip(trackId, clipId, "");
  return {leftId, rightId};
}

void ProjectState::migrateProperties(const juce::ValueTree& source, juce::ValueTree& dest) {
    // Copy all properties except core identity ones which are already set
    for (int i = 0; i < source.getNumProperties(); ++i) {
        auto prop = source.getPropertyName(i);
        if (prop != PROP_ID && prop != PROP_START && prop != PROP_LENGTH && prop != PROP_NAME && 
            prop != PROP_START_BEATS && prop != PROP_LENGTH_BEATS && prop != PROP_OFFSET) {
            dest.setProperty(prop, source.getProperty(prop), &undoManager);
        }
    }
    
    // Copy children (Automation, MIDI notes, etc.)
    for (int i = 0; i < source.getNumChildren(); ++i) {
        dest.addChild(source.getChild(i).createCopy(), -1, &undoManager);
    }
}

//==============================================================================
// MIDI Note Management (implementations are in U4.1 section above)
//==============================================================================
// NOTE: Function implementations moved to U4.1 section to avoid duplication

//==============================================================================
// Tempo Map & Markers (Phase 15)
//==============================================================================

juce::String ProjectState::addTempoChange(double beatPosition, double bpm,
                                          const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto tempoMap = state.getChildWithName(ID_TEMPO_MAP);
  if (!tempoMap.isValid()) {
    tempoMap = juce::ValueTree(ID_TEMPO_MAP);
    state.addChild(tempoMap, -1, &undoManager);
  }

  // Create point
  juce::ValueTree point(ID_TEMPO_POINT);
  juce::String pointId = generateUniqueId("tempo_point");

  point.setProperty(PROP_ID, pointId, nullptr);
  point.setProperty(PROP_TIME_BEATS, beatPosition, nullptr);
  point.setProperty(PROP_BPM, bpm, nullptr);

  // Insert in sorted order
  int insertIndex = 0;
  for (int i = 0; i < tempoMap.getNumChildren(); ++i) {
    if ((double)tempoMap.getChild(i)[PROP_TIME_BEATS] > beatPosition) {
      insertIndex = i;
      break;
    }
    insertIndex = i + 1;
  }

  undoManager.beginNewTransaction(actionName);
  tempoMap.addChild(point, insertIndex, &undoManager);

  DBG("ProjectState: Added tempo change at " + juce::String(beatPosition) +
      " beats: " + juce::String(bpm) + " BPM");
  return pointId;
}

bool ProjectState::deleteTempoChange(const juce::String &pointId,
                                     const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto tempoMap = getTempoMap();
  if (!tempoMap.isValid())
    return false;

  for (int i = 0; i < tempoMap.getNumChildren(); ++i) {
    if (tempoMap.getChild(i)[PROP_ID].toString() == pointId) {
      undoManager.beginNewTransaction(actionName);
      tempoMap.removeChild(i, &undoManager);
      DBG("ProjectState: Deleted tempo change " + pointId);
      return true;
    }
  }
  return false;
}

void ProjectState::moveTempoChange(const juce::String &pointId,
                                   double newBeats, double newBpm,
                                   const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto tempoMap = getTempoMap();
  if (!tempoMap.isValid())
    return;

  for (int i = 0; i < tempoMap.getNumChildren(); ++i) {
    auto point = tempoMap.getChild(i);
    if (point[PROP_ID].toString() == pointId) {
      undoManager.beginNewTransaction(actionName);
      
      // Update properties
      point.setProperty(PROP_TIME_BEATS, newBeats, &undoManager);
      point.setProperty(PROP_BPM, newBpm, &undoManager);

      // Re-sort
      tempoMap.removeChild(i, &undoManager);
      
      int insertIndex = 0;
      for (int j = 0; j < tempoMap.getNumChildren(); ++j) {
        if ((double)tempoMap.getChild(j)[PROP_TIME_BEATS] > newBeats) {
          insertIndex = j;
          break;
        }
        insertIndex = j + 1;
      }
      tempoMap.addChild(point, insertIndex, &undoManager);

      DBG("ProjectState: Moved tempo point " + pointId);
      return;
    }
  }
}

juce::ValueTree ProjectState::getTempoMap() const {
  return state.getChildWithName(ID_TEMPO_MAP);
}

juce::String ProjectState::addMarker(double beatPosition,
                                     const juce::String &name,
                                     const juce::String &color,
                                     const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto markers = state.getChildWithName(ID_MARKERS);
  if (!markers.isValid()) {
    markers = juce::ValueTree(ID_MARKERS);
    state.addChild(markers, -1, &undoManager);
  }

  // Create marker
  juce::ValueTree marker(ID_MARKER);
  juce::String markerId = generateUniqueId("marker");

  marker.setProperty(PROP_ID, markerId, nullptr);
  marker.setProperty(PROP_TIME_BEATS, beatPosition, nullptr);
  marker.setProperty(PROP_NAME, name, nullptr);
  marker.setProperty(PROP_COLOR, color, nullptr);

  // Insert in sorted order
  int insertIndex = 0;
  for (int i = 0; i < markers.getNumChildren(); ++i) {
    if ((double)markers.getChild(i)[PROP_TIME_BEATS] > beatPosition) {
      insertIndex = i;
      break;
    }
    insertIndex = i + 1;
  }

  undoManager.beginNewTransaction(actionName);
  markers.addChild(marker, insertIndex, &undoManager);

  DBG("ProjectState: Added marker '" + name + "' at " +
      juce::String(beatPosition) + " beats");
  return markerId;
}

bool ProjectState::deleteMarker(const juce::String &markerId,
                                const juce::String &actionName) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  auto markers = state.getChildWithName(ID_MARKERS);
  if (!markers.isValid())
    return false;

  for (int i = 0; i < markers.getNumChildren(); ++i) {
    if (markers.getChild(i)[PROP_ID].toString() == markerId) {
      undoManager.beginNewTransaction(actionName);
      markers.removeChild(i, &undoManager);
      DBG("ProjectState: Deleted marker " + markerId);
      return true;
    }
  }

  return false;
}

void ProjectState::moveMarker(const juce::String &markerId, double newBeats,
                              const juce::String &actionName) {
  auto markers = getMarkers();
  if (!markers.isValid())
    return;

  for (int i = 0; i < markers.getNumChildren(); ++i) {
    auto marker = markers.getChild(i);
    if (marker[PROP_ID].toString() == markerId) {
      undoManager.beginNewTransaction(actionName);
      marker.setProperty(PROP_TIME_BEATS, newBeats, &undoManager);

      // Re-sort
      markers.removeChild(i, &undoManager);
      int insertIndex = 0;
      for (int j = 0; j < markers.getNumChildren(); ++j) {
        if ((double)markers.getChild(j)[PROP_TIME_BEATS] > newBeats) {
          insertIndex = j;
          break;
        }
        insertIndex = j + 1;
      }
      markers.addChild(marker, insertIndex, &undoManager);

      DBG("ProjectState: Moved marker " + markerId + " to " +
          juce::String(newBeats));
      return;
    }
  }
}

void ProjectState::renameMarker(const juce::String &markerId,
                                const juce::String &newName,
                                const juce::String &actionName) {
  auto markers = getMarkers();
  if (!markers.isValid())
    return;

  for (int i = 0; i < markers.getNumChildren(); ++i) {
    auto marker = markers.getChild(i);
    if (marker[PROP_ID].toString() == markerId) {
      undoManager.beginNewTransaction(actionName);
      marker.setProperty(PROP_NAME, newName, &undoManager);
      DBG("ProjectState: Renamed marker " + markerId + " to " + newName);
      return;
    }
  }
}

juce::ValueTree ProjectState::getMarkers() const {
  return state.getChildWithName(ID_MARKERS);
}

void ProjectState::setTrackColor(const juce::String &trackId,
                                 const juce::Colour &color, bool manuallySet,
                                 const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (track.isValid()) {
    undoManager.beginNewTransaction(actionName);
    track.setProperty(PROP_COLOR, color.toString(), &undoManager);
    if (manuallySet) {
      track.setProperty(PROP_MANUALLY_COLORED, true, &undoManager);
    }
  }
}

//==============================================================================
// Arranger Sections
//==============================================================================

juce::String ProjectState::addSection(double startBeats, double lengthBeats,
                                      const juce::String &name,
                                      const juce::String &color,
                                      const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid()) {
    sectionsNode = juce::ValueTree(ID_SECTIONS);
    state.appendChild(sectionsNode, &undoManager);
  }

  auto sectionId = generateUniqueId("section");
  juce::ValueTree section(ID_SECTION);
  section.setProperty(PROP_ID, sectionId, nullptr);
  section.setProperty(PROP_START, startBeats, nullptr);
  section.setProperty(PROP_LENGTH, lengthBeats, nullptr);
  section.setProperty(PROP_NAME, name, nullptr);
  section.setProperty(PROP_COLOR, color, nullptr);

  undoManager.beginNewTransaction(actionName);
  sectionsNode.appendChild(section, &undoManager);

  DBG("ProjectState: Added section " + sectionId + " '" + name + "'");
  return sectionId;
}

bool ProjectState::deleteSection(const juce::String &sectionId,
                                 const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return false;

  for (int i = 0; i < sectionsNode.getNumChildren(); ++i) {
    auto section = sectionsNode.getChild(i);
    if (section[PROP_ID].toString() == sectionId) {
      undoManager.beginNewTransaction(actionName);
      sectionsNode.removeChild(i, &undoManager);
      DBG("ProjectState: Deleted section " + sectionId);
      return true;
    }
  }
  return false;
}

void ProjectState::moveSection(const juce::String &sectionId,
                               double newStartBeats,
                               const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return;

  for (auto section : sectionsNode) {
    if (section[PROP_ID].toString() == sectionId) {
      undoManager.beginNewTransaction(actionName);
      section.setProperty(PROP_START, newStartBeats, &undoManager);
      return;
    }
  }
}

void ProjectState::resizeSection(const juce::String &sectionId,
                                 double newLengthBeats,
                                 const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return;

  for (auto section : sectionsNode) {
    if (section[PROP_ID].toString() == sectionId) {
      undoManager.beginNewTransaction(actionName);
      section.setProperty(PROP_LENGTH, juce::jmax(0.25, newLengthBeats),
                          &undoManager);
      return;
    }
  }
}

void ProjectState::renameSection(const juce::String &sectionId,
                                 const juce::String &newName,
                                 const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return;

  for (auto section : sectionsNode) {
    if (section[PROP_ID].toString() == sectionId) {
      undoManager.beginNewTransaction(actionName);
      section.setProperty(PROP_NAME, newName, &undoManager);
      return;
    }
  }
}

void ProjectState::setSectionColor(const juce::String &sectionId,
                                   const juce::String &newColor,
                                   const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return;

  for (auto section : sectionsNode) {
    if (section[PROP_ID].toString() == sectionId) {
      undoManager.beginNewTransaction(actionName);
      section.setProperty(PROP_COLOR, newColor, &undoManager);
      return;
    }
  }
}

juce::ValueTree ProjectState::getSections() const {
  return state.getChildWithName(ID_SECTIONS);
}

void ProjectState::moveSectionContent(const juce::String &sectionId,
                                      double newStartBeats,
                                      const juce::String &actionName) {
  auto sectionsNode = state.getChildWithName(ID_SECTIONS);
  if (!sectionsNode.isValid())
    return;

  juce::ValueTree section;
  for (auto s : sectionsNode) {
    if (s[PROP_ID].toString() == sectionId) {
      section = s;
      break;
    }
  }

  if (!section.isValid())
    return;

  double oldStart = section[PROP_START];
  double length = section[PROP_LENGTH];
  double oldEnd = oldStart + length;
  double delta = newStartBeats - oldStart;

  if (std::abs(delta) < 0.001)
    return;

  undoManager.beginNewTransaction(actionName);

  // 1. Move the section itself
  section.setProperty(PROP_START, newStartBeats, &undoManager);

  // 2. Identify and Slice Clips across ALL tracks
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  if (!tracksNode.isValid())
    return;

  // Helper to convert samples to beats (primitive approximation if needed)
  auto sampleRate = state[PROP_SAMPLE_RATE];
  if (double(sampleRate) <= 0.0)
    sampleRate = 44100.0;
  double tempo = getTempo();
  if (tempo <= 0.0)
    tempo = 120.0;

  auto toBeats = [&](const juce::var &val) -> double {
    if (val.isDouble())
      return (double)val;
    if (val.isInt64())
      return (double)((juce::int64)val) / (double)sampleRate * (tempo / 60.0);
    return 0.0;
  };

  // Pass 1: Split at Start and End
  for (int t = 0; t < tracksNode.getNumChildren(); ++t) {
    auto track = tracksNode.getChild(t);
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
      continue;

    // Iterate clips to find overlaps
    // We restart loop if splits occur to deal with iterator invalidation safety
    bool splitsOccurred = true;
    while (splitsOccurred) {
      splitsOccurred = false;
      for (int c = 0; c < clipsNode.getNumChildren(); ++c) {
        auto clip = clipsNode.getChild(c);
        double cStart = toBeats(clip[PROP_START]);
        double len = toBeats(clip[PROP_LENGTH]);
        if (clip[PROP_LENGTH].isInt64())
          len = (double)((juce::int64)clip[PROP_LENGTH]) / (double)sampleRate *
                (tempo / 60.0);
        double cEnd = cStart + len;

        // Tolerance
        double tolerance = 0.001;

        // Check strict overlap with oldStart (Start within clip body)
        if (cStart < oldStart - tolerance && cEnd > oldStart + tolerance) {
          // Split at oldStart
          double newLen1 = oldStart - cStart;

          // We need to know if we are updating beats or samples
          if (clip[PROP_LENGTH].isInt64()) {
            // Samples mode
            // Convert newLen1 to samples
            juce::int64 samples1 =
                (juce::int64)(newLen1 * (60.0 / tempo) * (double)sampleRate);
            clip.setProperty(PROP_LENGTH, samples1, &undoManager);

            // Remainder
            double newLen2 = len - newLen1;
            juce::int64 samples2 =
                (juce::int64)(newLen2 * (60.0 / tempo) * (double)sampleRate);

            juce::ValueTree newClip = clip.createCopy();
            newClip.setProperty(PROP_ID, generateUniqueId("clip"), nullptr);
            // New start in samples
            juce::int64 startSamples =
                (juce::int64)(oldStart * (60.0 / tempo) * (double)sampleRate);
            newClip.setProperty(PROP_START, startSamples, nullptr);
            newClip.setProperty(PROP_LENGTH, samples2, nullptr);

            if (clip.hasProperty(PROP_OFFSET)) {
              juce::int64 offset = clip[PROP_OFFSET];
              newClip.setProperty(PROP_OFFSET, offset + samples1, nullptr);
            }
            clipsNode.appendChild(newClip, &undoManager);
          } else {
            // Beats mode
            clip.setProperty(PROP_LENGTH, newLen1, &undoManager);

            double newLen2 = len - newLen1;
            juce::ValueTree newClip = clip.createCopy();
            newClip.setProperty(PROP_ID, generateUniqueId("clip"), nullptr);
            newClip.setProperty(PROP_START, oldStart, nullptr);
            newClip.setProperty(PROP_LENGTH, newLen2, nullptr);

            if (clip.hasProperty(PROP_OFFSET)) {
              double offset = clip[PROP_OFFSET];
              newClip.setProperty(PROP_OFFSET, offset + newLen1, nullptr);
            }
            clipsNode.appendChild(newClip, &undoManager);
          }

          splitsOccurred = true;
          break;
        }

        // Check strict overlap with oldEnd
        if (cStart < oldEnd - tolerance && cEnd > oldEnd + tolerance) {
          // Split at oldEnd
          double newLen1 = oldEnd - cStart;

          if (clip[PROP_LENGTH].isInt64()) {
            juce::int64 samples1 =
                (juce::int64)(newLen1 * (60.0 / tempo) * (double)sampleRate);
            clip.setProperty(PROP_LENGTH, samples1, &undoManager);

            double newLen2 = len - newLen1;
            juce::int64 samples2 =
                (juce::int64)(newLen2 * (60.0 / tempo) * (double)sampleRate);

            juce::ValueTree newClip = clip.createCopy();
            newClip.setProperty(PROP_ID, generateUniqueId("clip"), nullptr);
            juce::int64 startSamples =
                (juce::int64)(oldEnd * (60.0 / tempo) * (double)sampleRate);
            newClip.setProperty(PROP_START, startSamples, nullptr);
            newClip.setProperty(PROP_LENGTH, samples2, nullptr);

            if (clip.hasProperty(PROP_OFFSET)) {
              juce::int64 offset = clip[PROP_OFFSET];
              newClip.setProperty(PROP_OFFSET, offset + samples1, nullptr);
            }
            clipsNode.appendChild(newClip, &undoManager);
          } else {
            clip.setProperty(PROP_LENGTH, newLen1, &undoManager);

            double newLen2 = len - newLen1;
            juce::ValueTree newClip = clip.createCopy();
            newClip.setProperty(PROP_ID, generateUniqueId("clip"), nullptr);
            newClip.setProperty(PROP_START, oldEnd, nullptr);
            newClip.setProperty(PROP_LENGTH, newLen2, nullptr);

            if (clip.hasProperty(PROP_OFFSET)) {
              double offset = clip[PROP_OFFSET];
              newClip.setProperty(PROP_OFFSET, offset + newLen1, nullptr);
            }
            clipsNode.appendChild(newClip, &undoManager);
          }

          splitsOccurred = true;
          break;
        }
      }
    }
  }

  // Pass 2: Move clips fully within [oldStart, oldEnd]
  for (int t = 0; t < tracksNode.getNumChildren(); ++t) {
    auto track = tracksNode.getChild(t);
    auto clipsNode = track.getChildWithName(ID_CLIPS);
    if (!clipsNode.isValid())
      continue;

    for (int c = 0; c < clipsNode.getNumChildren(); ++c) {
      auto clip = clipsNode.getChild(c);
      double cStart = toBeats(clip[PROP_START]);
      double cLen = toBeats(clip[PROP_LENGTH]);
      if (clip[PROP_LENGTH].isInt64())
        cLen = (double)((juce::int64)clip[PROP_LENGTH]) / (double)sampleRate *
               (tempo / 60.0);
      double cEnd = cStart + cLen;

      if (cStart >= oldStart - 0.001 && cEnd <= oldEnd + 0.001) {
        if (clip[PROP_START].isInt64()) {
          juce::int64 newStartSamples =
              (juce::int64)((cStart + delta) * (60.0 / tempo) *
                            double(sampleRate));
          clip.setProperty(PROP_START, newStartSamples, &undoManager);
        } else {
          clip.setProperty(PROP_START, cStart + delta, &undoManager);
        }
      }
    }
  }
}

//==========================================================================
// Take Folder Management
//==========================================================================

juce::String ProjectState::createTakeFolder(const juce::String &trackId,
                                            double startBeats,
                                            double lengthBeats,
                                            const juce::String &actionName) {
  auto track = findTrack(trackId);
  if (!track.isValid())
    return {};

  auto clipsNode = track.getOrCreateChildWithName(ID_CLIPS, nullptr);

  undoManager.beginNewTransaction(actionName);

  juce::String folderId = generateUniqueId("folder");
  juce::ValueTree folder(ID_TAKE_FOLDER);
  folder.setProperty(PROP_ID, folderId, nullptr);
  folder.setProperty(PROP_START_BEATS, startBeats, nullptr);
  folder.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
  folder.setProperty(PROP_ACTIVE_TAKE, -1, nullptr); // -1 = comp mode
  folder.setProperty(PROP_EXPANDED, true, nullptr);

  clipsNode.addChild(folder, -1, &undoManager);

  DBG("ProjectState: Created Take Folder " + folderId + " on track " + trackId);
  return folderId;
}

void ProjectState::addTakeToFolder(const juce::String &folderId,
                                   const juce::String &clipId) {
  auto [oldTrack, clip] = findClip(clipId);
  if (!clip.isValid())
    return;

  // Find folder
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  juce::ValueTree folder;
  for (auto t : tracksNode) {
    auto clips = t.getChildWithName(ID_CLIPS);
    folder = clips.getChildWithProperty(PROP_ID, folderId);
    if (folder.isValid())
      break;
  }

  if (!folder.isValid())
    return;

  undoManager.beginNewTransaction("Add take to folder");

  // Move clip to folder
  auto parent = clip.getParent();
  parent.removeChild(clip, &undoManager);
  folder.addChild(clip, -1, &undoManager);

  DBG("ProjectState: Added clip " + clipId + " to folder " + folderId);
}

void ProjectState::removeTakeFromFolder(const juce::String &folderId,
                                        const juce::String &clipId) {
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  juce::ValueTree folder;
  juce::ValueTree track;
  for (auto t : tracksNode) {
    auto clips = t.getChildWithName(ID_CLIPS);
    folder = clips.getChildWithProperty(PROP_ID, folderId);
    if (folder.isValid()) {
      track = t;
      break;
    }
  }

  if (!folder.isValid())
    return;

  auto clip = folder.getChildWithProperty(PROP_ID, clipId);
  if (!clip.isValid())
    return;

  undoManager.beginNewTransaction("Remove take from folder");

  folder.removeChild(clip, &undoManager);
  track.getOrCreateChildWithName(ID_CLIPS, nullptr)
      .addChild(clip, -1, &undoManager);

  DBG("ProjectState: Removed clip " + clipId + " from folder " + folderId);
}

void ProjectState::setCompRegion(const juce::String &folderId, double startBeats,
                                 double lengthBeats, int takeIndex,
                                 const juce::String &actionName) {
  // Find folder
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  juce::ValueTree folder;
  for (auto t : tracksNode) {
    auto clips = t.getChildWithName(ID_CLIPS);
    folder = clips.getChildWithProperty(PROP_ID, folderId);
    if (folder.isValid())
      break;
  }

  if (!folder.isValid())
    return;

  undoManager.beginNewTransaction(actionName);

  // Simple implementation: Add a new region.
  // Real implementation in TakeFolder.cpp handles overlaps.
  // Here we just update the model; the engine implementation is primary truth.
  // But for UI/Persistence, we need something.

  juce::ValueTree region(ID_COMP_REGION);
  region.setProperty(PROP_ID, generateUniqueId("region"), nullptr);
  region.setProperty(PROP_START_BEATS, startBeats, nullptr);
  region.setProperty(PROP_LENGTH_BEATS, lengthBeats, nullptr);
  region.setProperty(PROP_TAKE_INDEX, takeIndex, nullptr);

  folder.addChild(region, -1, &undoManager);
}

void ProjectState::setTakeFolderExpanded(const juce::String &folderId,
                                         bool expanded) {
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  juce::ValueTree folder;
  for (auto t : tracksNode) {
    auto clips = t.getChildWithName(ID_CLIPS);
    folder = clips.getChildWithProperty(PROP_ID, folderId);
    if (folder.isValid())
      break;
  }

  if (folder.isValid()) {
    folder.setProperty(PROP_EXPANDED, expanded, &undoManager);
  }
}

void ProjectState::setActiveTake(const juce::String &folderId, int takeIndex) {
  auto tracksNode = state.getChildWithName(ID_TRACKS);
  juce::ValueTree folder;
  for (auto t : tracksNode) {
    auto clips = t.getChildWithName(ID_CLIPS);
    folder = clips.getChildWithProperty(PROP_ID, folderId);
    if (folder.isValid())
      break;
  }

  if (folder.isValid()) {
    folder.setProperty(PROP_ACTIVE_TAKE, takeIndex, &undoManager);
  }
}

juce::File ProjectState::getAssetDirectory(const juce::String &subfolder) const {
    auto assetsDir = projectFile.getParentDirectory().getChildFile("Assets");
    if (subfolder.isNotEmpty())
        return assetsDir.getChildFile(subfolder);
    return assetsDir;
}



juce::var ProjectState::getProperty(const juce::String& nodeId, const juce::String& propId) const {
    auto it = nodeCache_.find(nodeId);
    if (it != nodeCache_.end()) {
        return it->second.getProperty(propId);
    }
    return {};
}

void ProjectState::setProperty(const juce::String& nodeId, const juce::String& propId, const juce::var& value) {
    auto it = nodeCache_.find(nodeId);
    if (it != nodeCache_.end()) {
        it->second.setProperty(propId, value, &undoManager);
    }
}

juce::var ProjectState::getProjectHierarchy() const {
    auto* rootObj = new juce::DynamicObject();
    
    auto buildHierarchy = [&](auto& self, juce::ValueTree node) -> juce::var {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", node.getProperty(PROP_ID).toString());
        obj->setProperty("name", node.getProperty(PROP_NAME).toString());
        obj->setProperty("type", node.getType().toString());
        
        juce::Array<juce::var> children;
        for (int i = 0; i < node.getNumChildren(); ++i) {
            children.add(self(self, node.getChild(i)));
        }
        
        if (children.size() > 0) obj->setProperty("children", children);
        return juce::var(obj);
    };
    
    return buildHierarchy(buildHierarchy, state);
}

juce::StringArray ProjectState::getUndoHistory() const {
    // JUCE 8 doesn't have getUndoNames(), so we build the list manually
    juce::StringArray history;
    
    // Get current undo description if available
    auto desc = undoManager.getUndoDescription();
    if (desc.isNotEmpty()) {
        history.add(desc);
    }
    
    // Note: JUCE UndoManager doesn't expose full history stack directly
    // This returns just the current undo action name
    return history;
}

void ProjectState::undoTo(int index) {
    // Undo back to a specific point - since we can't get full stack,
    // we undo (index+1) times to reach that point
    for (int i = 0; i <= index && undoManager.canUndo(); ++i) {
        undoManager.undo();
    }
}

} // namespace zenith
