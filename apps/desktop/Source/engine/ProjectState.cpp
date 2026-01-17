/**
 * @file ProjectState.cpp
 * @brief Project state implementation
 */

#include "ProjectState.h"
#include "AutomationStateManager.h"
#include "ClipStateManager.h"
#include "ProjectFileIO.h"
#include "TrackStateManager.h"
#include "MidiNoteStateManager.h"
#include "IDService.h"

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

// Take Folder identifiers
const juce::Identifier ProjectState::ID_TAKE_FOLDERS("TAKE_FOLDERS");
const juce::Identifier ProjectState::ID_TAKES("TAKES");
const juce::Identifier ProjectState::ID_TAKE("TAKE");

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

// Take Folder properties
// Take Folder properties: defined above
// PROP_TAKE_INDEX, PROP_ACTIVE_TAKE, PROP_EXPANDED are already defined

//==============================================================================
ProjectState::ProjectState() : state(Zenith::IDs::PROJECT) {
  DBG("ProjectState: Constructor");

  state.setProperty(PROP_ID, "root", nullptr);
  state.getOrCreateChildWithName(Zenith::IDs::TRACKS, nullptr);

  trackStateManager = std::make_unique<TrackStateManager>(*this);
  clipStateManager = std::make_unique<ClipStateManager>(*this);
  midiNoteStateManager = std::make_unique<MidiNoteStateManager>(*this);
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
      startTimer(intervalMinutes * 60 * 1000);
}

void ProjectState::stopAutosaveTimer() { stopTimer(); }

void ProjectState::valueTreeChildAdded(juce::ValueTree &parent,
                                       juce::ValueTree &child) {
  isDirty = true;

  if (child.hasType(ID_TRACK)) {
    juce::String id = child.getProperty(PROP_ID).toString();
    if (id.isNotEmpty())
      trackIdMap_[id] = child;
  }
}

void ProjectState::valueTreeChildRemoved(juce::ValueTree &parent,
                                         juce::ValueTree &child, int) {
  isDirty = true;

  if (child.hasType(ID_TRACK)) {
    juce::String id = child.getProperty(PROP_ID).toString();
    if (id.isNotEmpty())
      trackIdMap_.erase(id);
  }
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
  tempo = juce::jlimit<double>(1.0, 999.0, tempo);
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
  if (midiNoteStateManager)
    return midiNoteStateManager->getNotesForClip(clipId);
  return {};
}

juce::String ProjectState::addMidiNote(const juce::String &clipId,
                                       const MidiNoteSpec &note,
                                       const juce::String &actionName) {
  if (midiNoteStateManager)
    return midiNoteStateManager->addNote(clipId, note, actionName);
  return {};
}

void ProjectState::removeMidiNote(const juce::String &clipId,
                                  const juce::String &noteId,
                                  const juce::String &actionName) {
  if (midiNoteStateManager)
    midiNoteStateManager->deleteNote(clipId, noteId, actionName);
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
  int pitch = juce::jlimit<int>(0, 127, newPitch);
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
  if (midiNoteStateManager)
    midiNoteStateManager->moveNote(clipId, noteId, -1.0, -1.0, -1,
                                   MidiNote::fromMidiVelocity(newVelocity), actionName);
}

void ProjectState::setMidiNoteLength(const juce::String &clipId,
                                     const juce::String &noteId,
                                     double newLengthBeats,
                                     const juce::String &actionName) {
  if (midiNoteStateManager)
    midiNoteStateManager->moveNote(clipId, noteId, -1.0, newLengthBeats, -1, -1.0f, actionName);
}

void ProjectState::setMidiNoteMuted(const juce::String &clipId,
                                    const juce::String &noteId, bool muted,
                                    const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (noteTree.isValid()) {
    undoManager.beginNewTransaction(actionName);
    if (muted)
        noteTree.setProperty(PROP_MUTE, true, &undoManager);
    else
        noteTree.removeProperty(PROP_MUTE, &undoManager);
  }
}

void ProjectState::setMidiNoteProbability(const juce::String &clipId,
                                          const juce::String &noteId,
                                          float probability,
                                          const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (noteTree.isValid()) {
    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_PROBABILITY, juce::jlimit<float>(0.0f, 1.0f, probability), &undoManager);
  }
}

void ProjectState::setMidiNoteTension(const juce::String &clipId,
                                      const juce::String &noteId,
                                      float tension,
                                      const juce::String &actionName) {
  auto noteTree = findMidiNote(clipId, noteId);
  if (noteTree.isValid()) {
    undoManager.beginNewTransaction(actionName);
    noteTree.setProperty(PROP_NOTE_TENSION, tension, &undoManager);
  }
}

void ProjectState::humanizeClip(const juce::String &clipId, double velocityRange,
                                double timeRangeBeats,
                                const juce::String &actionName) {
  if (midiNoteStateManager)
    midiNoteStateManager->humanizeNotes(clipId, timeRangeBeats, (int)velocityRange, actionName);
}

void ProjectState::legatoClip(const juce::String &clipId, bool adjustOverlap,
                              const juce::String &actionName) {
  if (midiNoteStateManager)
    midiNoteStateManager->legatoNotes(clipId, adjustOverlap, actionName);
}


//==============================================================================
// Helper Methods
//==============================================================================

juce::File ProjectState::getAssetDirectory(const juce::String &subfolder) const {
    juce::File assetsDir;
    if (projectFile.exists()) {
        assetsDir = projectFile.getParentDirectory().getChildFile("Assets");
    } else {
        assetsDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                        .getChildFile("Documents/Zenith/Untitled/Assets");
    }

    if (subfolder.isNotEmpty())
        return assetsDir.getChildFile(subfolder);
    return assetsDir;
}

} // namespace zenith
