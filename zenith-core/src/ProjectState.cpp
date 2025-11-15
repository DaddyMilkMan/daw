/**
 * @file ProjectState.cpp
 * @brief Project state implementation
 */

#include "../include/ProjectState.h"

#include <functional>

namespace
{
//==============================================================================
int extractNumericSuffix(const juce::String& identifier)
{
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
const juce::Identifier ProjectState::ID_POINT("POINT");

const juce::Identifier ProjectState::PROP_NAME("name");
const juce::Identifier ProjectState::PROP_TEMPO("tempo");
const juce::Identifier ProjectState::PROP_TIME_SIG_NUM("timeSignatureNumerator");
const juce::Identifier ProjectState::PROP_TIME_SIG_DEN("timeSignatureDenominator");
const juce::Identifier ProjectState::PROP_SAMPLE_RATE("sampleRate");

const juce::Identifier ProjectState::PROP_ID("id");
const juce::Identifier ProjectState::PROP_TYPE("type");
const juce::Identifier ProjectState::PROP_VOLUME("volume");
const juce::Identifier ProjectState::PROP_PAN("pan");
const juce::Identifier ProjectState::PROP_MUTE("mute");
const juce::Identifier ProjectState::PROP_SOLO("solo");

const juce::Identifier ProjectState::PROP_START("start");
const juce::Identifier ProjectState::PROP_LENGTH("length");

// Phase 13: Automation properties
const juce::Identifier ProjectState::PROP_PARAM("param");
const juce::Identifier ProjectState::PROP_TIME_BEATS("timeBeats");
const juce::Identifier ProjectState::PROP_VALUE("value");

// Phase 15: Tempo map and marker identifiers
const juce::Identifier ProjectState::ID_TEMPO_MAP("TEMPO_MAP");
const juce::Identifier ProjectState::ID_TEMPO_CHANGE("TEMPO_CHANGE");
const juce::Identifier ProjectState::ID_MARKERS("MARKERS");
const juce::Identifier ProjectState::ID_MARKER("MARKER");

// Phase 15: Tempo map and marker properties
const juce::Identifier ProjectState::PROP_BEAT_POSITION("beatPosition");
const juce::Identifier ProjectState::PROP_BPM("bpm");
const juce::Identifier ProjectState::PROP_TIME_SIG_NUM_CHANGE("timeSigNumerator");
const juce::Identifier ProjectState::PROP_TIME_SIG_DEN_CHANGE("timeSigDenominator");
const juce::Identifier ProjectState::PROP_COLOR("color");

//==============================================================================
ProjectState::ProjectState()
{
    DBG("ProjectState: Constructor");
    newProject();
}

ProjectState::~ProjectState()
{
    DBG("ProjectState: Destructor");
}

//==============================================================================
// Project Management
//==============================================================================

void ProjectState::newProject()
{
    DBG("ProjectState: Creating new project");

    // Clear undo history
    undoManager.clearUndoHistory();

    // Create default state
    createDefaultState();

    // Reset ID counter for a fresh project
    idCounter.store(0);

    DBG("ProjectState: New project created");
}

bool ProjectState::loadFromFile(const juce::File& file)
{
    DBG("ProjectState: Loading from " + file.getFullPathName());

    if (!file.existsAsFile())
    {
        DBG("ProjectState: File does not exist");
        return false;
    }

    // Parse XML
    auto xml = juce::parseXML(file);

    if (xml == nullptr)
    {
        DBG("ProjectState: Failed to parse XML");
        return false;
    }

    // Create ValueTree from XML
    auto newState = juce::ValueTree::fromXml(*xml);

    if (!newState.isValid() || newState.getType() != ID_PROJECT)
    {
        DBG("ProjectState: Invalid project file");
        return false;
    }

    // Replace current state
    state = newState;

    // Ensure future IDs do not clash with those loaded from disk
    rebuildIdCounter();

    // Clear undo history (fresh start)
    undoManager.clearUndoHistory();

    DBG("ProjectState: Loaded successfully");
    return true;
}

bool ProjectState::saveToFile(const juce::File& file)
{
    DBG("ProjectState: Saving to " + file.getFullPathName());

    // Convert ValueTree to XML
    auto xml = state.createXml();

    if (xml == nullptr)
    {
        DBG("ProjectState: Failed to create XML");
        return false;
    }

    // Save to file
    if (!xml->writeTo(file))
    {
        DBG("ProjectState: Failed to write file");
        return false;
    }

    DBG("ProjectState: Saved successfully");
    return true;
}

//==============================================================================
// Project Properties
//==============================================================================

juce::String ProjectState::getProjectName() const
{
    return state[PROP_NAME].toString();
}

void ProjectState::setProjectName(const juce::String& name)
{
    state.setProperty(PROP_NAME, name, &undoManager);
}

double ProjectState::getTempo() const
{
    return state[PROP_TEMPO];
}

void ProjectState::setTempo(double tempo)
{
    tempo = juce::jlimit(20.0, 999.0, tempo);
    state.setProperty(PROP_TEMPO, tempo, &undoManager);
}

int ProjectState::getTimeSignatureNumerator() const
{
    return state[PROP_TIME_SIG_NUM];
}

int ProjectState::getTimeSignatureDenominator() const
{
    return state[PROP_TIME_SIG_DEN];
}

void ProjectState::setTimeSignature(int numerator, int denominator)
{
    state.setProperty(PROP_TIME_SIG_NUM, numerator, &undoManager);
    state.setProperty(PROP_TIME_SIG_DEN, denominator, &undoManager);
}

//==============================================================================
// Track Management
//==============================================================================

juce::String ProjectState::addTrack(const juce::String& name, const juce::String& type)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
    {
        // Create TRACKS node if it doesn't exist
        tracksNode = juce::ValueTree(ID_TRACKS);
        state.appendChild(tracksNode, &undoManager);
    }

    // Generate unique ID
    auto trackId = generateUniqueId("track");

    // Create track
    juce::ValueTree track(ID_TRACK);
    track.setProperty(PROP_ID, trackId, nullptr);
    track.setProperty(PROP_NAME, name, nullptr);
    track.setProperty(PROP_TYPE, type, nullptr);
    track.setProperty(PROP_VOLUME, 0.8, nullptr);
    track.setProperty(PROP_PAN, 0.0, nullptr);
    track.setProperty(PROP_MUTE, false, nullptr);
    track.setProperty(PROP_SOLO, false, nullptr);

    // Create empty CLIPS node
    track.appendChild(juce::ValueTree(ID_CLIPS), nullptr);

    // Add to project
    tracksNode.appendChild(track, &undoManager);

    DBG("ProjectState: Added track '" + name + "' with ID " + trackId);

    return trackId;
}

void ProjectState::removeTrack(const juce::String& trackId)
{
    auto track = findTrack(trackId);

    if (track.isValid())
    {
        auto tracksNode = state.getChildWithName(ID_TRACKS);
        tracksNode.removeChild(track, &undoManager);

        DBG("ProjectState: Removed track " + trackId);
    }
}

int ProjectState::getNumTracks() const
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (tracksNode.isValid())
        return tracksNode.getNumChildren();

    return 0;
}

//==============================================================================
// Undo/Redo
//==============================================================================

void ProjectState::undo()
{
    if (undoManager.canUndo())
    {
        undoManager.undo();
        DBG("ProjectState: Undo");
    }
}

void ProjectState::redo()
{
    if (undoManager.canRedo())
    {
        undoManager.redo();
        DBG("ProjectState: Redo");
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void ProjectState::createDefaultState()
{
    // Create root PROJECT node
    state = juce::ValueTree(ID_PROJECT);

    // Set default properties
    state.setProperty(PROP_NAME, "Untitled Project", nullptr);
    state.setProperty(PROP_TEMPO, 120.0, nullptr);
    state.setProperty(PROP_TIME_SIG_NUM, 4, nullptr);
    state.setProperty(PROP_TIME_SIG_DEN, 4, nullptr);
    state.setProperty(PROP_SAMPLE_RATE, 44100.0, nullptr);

    // Create TRACKS node
    state.appendChild(juce::ValueTree(ID_TRACKS), nullptr);

    // Create MIXER node
    juce::ValueTree mixer(ID_MIXER);
    mixer.setProperty(PROP_VOLUME, 0.8, nullptr);
    state.appendChild(mixer, nullptr);

    DBG("ProjectState: Default state created");
}

juce::String ProjectState::generateUniqueId(const juce::String& prefix)
{
    int id = idCounter.fetch_add(1);
    return prefix + "_" + juce::String(id);
}

juce::ValueTree ProjectState::findTrack(const juce::String& trackId)
{
    auto tracksNode = state.getChildWithName(ID_TRACKS);

    if (!tracksNode.isValid())
        return {};

    for (auto track : tracksNode)
    {
        if (track[PROP_ID].toString() == trackId)
            return track;
    }

    return {};
}

void ProjectState::rebuildIdCounter()
{
    int highestId = -1;

    std::function<void(const juce::ValueTree&)> scanTree = [&](const juce::ValueTree& node)
    {
        if (!node.isValid())
            return;

        if (node.hasProperty(PROP_ID))
        {
            const int suffix = extractNumericSuffix(node[PROP_ID].toString());

            if (suffix > highestId)
                highestId = suffix;
        }

        for (int i = 0; i < node.getNumChildren(); ++i)
            scanTree(node.getChild(i));
    };

    scanTree(state);

    idCounter.store(highestId + 1);
}

//==============================================================================
// Phase 13: Automation Management
//==============================================================================

juce::ValueTree ProjectState::getOrCreateAutomationEnvelope(const juce::String& trackId, const juce::String& paramId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(paramId == "volume" || paramId == "pan" || paramId == "mute");

    auto track = findTrack(trackId);
    if (!track.isValid())
    {
        DBG("ProjectState: Track not found: " + trackId);
        return {};
    }

    // Get or create AUTOMATION node
    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
    {
        automationNode = juce::ValueTree(ID_AUTOMATION);
        track.appendChild(automationNode, &undoManager);
        DBG("ProjectState: Created AUTOMATION node for track " + trackId);
    }

    // Find envelope for this parameter
    for (auto envelope : automationNode)
    {
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
            return envelope;
    }

    // Create new envelope
    juce::ValueTree envelope(ID_ENVELOPE);
    envelope.setProperty(PROP_PARAM, paramId, nullptr);
    automationNode.appendChild(envelope, &undoManager);

    DBG("ProjectState: Created envelope for " + trackId + "/" + paramId);
    return envelope;
}

juce::ValueTree ProjectState::getAutomationEnvelope(const juce::String& trackId, const juce::String& paramId) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto track = const_cast<ProjectState*>(this)->findTrack(trackId);
    if (!track.isValid())
        return {};

    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
        return {};

    // Find envelope for this parameter
    for (auto envelope : automationNode)
    {
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
            return envelope;
    }

    return {};
}

bool ProjectState::hasAutomation(const juce::String& trackId, const juce::String& paramId) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    return getAutomationEnvelope(trackId, paramId).isValid();
}

juce::String ProjectState::addAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                               double timeBeats, double value, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(timeBeats >= 0.0);

    // Validate parameter ID and value range
    if (paramId == "volume")
        value = juce::jlimit(0.0, 1.0, value);
    else if (paramId == "pan")
        value = juce::jlimit(-1.0, 1.0, value);
    else if (paramId == "mute")
        value = value >= 0.5 ? 1.0 : 0.0;  // Binary
    else
    {
        DBG("ProjectState: Invalid parameter ID: " + paramId);
        return {};
    }

    auto envelope = getOrCreateAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
    {
        DBG("ProjectState: Failed to get/create envelope");
        return {};
    }

    // Generate unique point ID
    auto pointId = generateUniqueId("point");

    // Create point
    juce::ValueTree point(ID_POINT);
    point.setProperty(PROP_ID, pointId, nullptr);
    point.setProperty(PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(PROP_VALUE, value, nullptr);

    // Insert in sorted order by time
    int insertIndex = 0;
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto existingPoint = envelope.getChild(i);
        double existingTime = existingPoint[PROP_TIME_BEATS];
        if (timeBeats >= existingTime)
            insertIndex = i + 1;
        else
            break;
    }

    envelope.addChild(point, insertIndex, &undoManager);

    DBG("ProjectState: Added automation point " + pointId + " at " + juce::String(timeBeats) + " beats");
    return pointId;
}

bool ProjectState::moveAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                        const juce::String& pointId, double newTimeBeats, double newValue,
                                        const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newTimeBeats >= 0.0);

    // Validate value range
    if (paramId == "volume")
        newValue = juce::jlimit(0.0, 1.0, newValue);
    else if (paramId == "pan")
        newValue = juce::jlimit(-1.0, 1.0, newValue);
    else if (paramId == "mute")
        newValue = newValue >= 0.5 ? 1.0 : 0.0;

    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    // Find point
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto point = envelope.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            double oldTime = point[PROP_TIME_BEATS];

            // Update properties
            point.setProperty(PROP_TIME_BEATS, newTimeBeats, &undoManager);
            point.setProperty(PROP_VALUE, newValue, &undoManager);

            // If time changed, re-sort
            if (newTimeBeats != oldTime)
            {
                // Remove and re-insert in sorted position
                // Use undoManager for both operations to ensure proper undo/redo support
                envelope.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < envelope.getNumChildren(); ++j)
                {
                    auto existingPoint = envelope.getChild(j);
                    double existingTime = existingPoint[PROP_TIME_BEATS];
                    if (newTimeBeats >= existingTime)
                        insertIndex = j + 1;
                    else
                        break;
                }

                envelope.addChild(point, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved automation point " + pointId);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteAutomationPoint(const juce::String& trackId, const juce::String& paramId,
                                          const juce::String& pointId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    // Find and remove point
    for (int i = 0; i < envelope.getNumChildren(); ++i)
    {
        auto point = envelope.getChild(i);
        if (point[PROP_ID].toString() == pointId)
        {
            envelope.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted automation point " + pointId);
            return true;
        }
    }

    return false;
}

bool ProjectState::clearAutomation(const juce::String& trackId, const juce::String& paramId,
                                    const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto track = findTrack(trackId);
    if (!track.isValid())
        return false;

    auto automationNode = track.getChildWithName(ID_AUTOMATION);
    if (!automationNode.isValid())
        return false;

    // Find and remove envelope
    for (int i = 0; i < automationNode.getNumChildren(); ++i)
    {
        auto envelope = automationNode.getChild(i);
        if (envelope.hasType(ID_ENVELOPE) && envelope[PROP_PARAM].toString() == paramId)
        {
            automationNode.removeChild(i, &undoManager);
            DBG("ProjectState: Cleared automation for " + trackId + "/" + paramId);
            return true;
        }
    }

    return false;
}

//==============================================================================
// Phase 15: Tempo Map Management
//==============================================================================

juce::ValueTree ProjectState::getTempoMapNode()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
    {
        // Create tempo map node
        tempoMapNode = juce::ValueTree(ID_TEMPO_MAP);
        state.appendChild(tempoMapNode, &undoManager);

        // Add default tempo at beat 0
        juce::ValueTree defaultTempo(ID_TEMPO_CHANGE);
        defaultTempo.setProperty(PROP_ID, generateUniqueId("tempo"), nullptr);
        defaultTempo.setProperty(PROP_BEAT_POSITION, 0.0, nullptr);
        defaultTempo.setProperty(PROP_BPM, getTempo(), nullptr);  // Use project tempo
        defaultTempo.setProperty(PROP_TIME_SIG_NUM_CHANGE, getTimeSignatureNumerator(), nullptr);
        defaultTempo.setProperty(PROP_TIME_SIG_DEN_CHANGE, getTimeSignatureDenominator(), nullptr);
        tempoMapNode.appendChild(defaultTempo, &undoManager);

        DBG("ProjectState: Created TEMPO_MAP with default tempo");
    }

    return tempoMapNode;
}

juce::Array<ProjectState::TempoChangeSpec> ProjectState::getTempoChanges() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::Array<TempoChangeSpec> result;

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return result;

    // Collect all tempo changes
    for (auto child : tempoMapNode)
    {
        if (child.hasType(ID_TEMPO_CHANGE))
        {
            TempoChangeSpec spec;
            spec.id = child[PROP_ID].toString();
            spec.beatPosition = child[PROP_BEAT_POSITION];
            spec.bpm = child[PROP_BPM];
            spec.timeSigNumerator = child[PROP_TIME_SIG_NUM_CHANGE];
            spec.timeSigDenominator = child[PROP_TIME_SIG_DEN_CHANGE];
            result.add(spec);
        }
    }

    // Sort by beat position
    std::sort(result.begin(), result.end(),
              [](const TempoChangeSpec& a, const TempoChangeSpec& b) {
                  return a.beatPosition < b.beatPosition;
              });

    return result;
}

juce::String ProjectState::addTempoChange(double beatPosition, double bpm,
                                           int numerator, int denominator,
                                           const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(beatPosition >= 0.0);
    jassert(bpm > 0.0);
    jassert(numerator > 0);
    jassert(denominator > 0);

    auto tempoMapNode = getTempoMapNode();

    // Generate unique ID
    auto tempoId = generateUniqueId("tempo");

    // Create tempo change node
    juce::ValueTree tempoChange(ID_TEMPO_CHANGE);
    tempoChange.setProperty(PROP_ID, tempoId, nullptr);
    tempoChange.setProperty(PROP_BEAT_POSITION, beatPosition, nullptr);
    tempoChange.setProperty(PROP_BPM, bpm, nullptr);
    tempoChange.setProperty(PROP_TIME_SIG_NUM_CHANGE, numerator, nullptr);
    tempoChange.setProperty(PROP_TIME_SIG_DEN_CHANGE, denominator, nullptr);

    // Insert in sorted order by beat position
    int insertIndex = 0;
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto existing = tempoMapNode.getChild(i);
        if (existing.hasType(ID_TEMPO_CHANGE))
        {
            double existingBeat = existing[PROP_BEAT_POSITION];
            if (beatPosition >= existingBeat)
                insertIndex = i + 1;
            else
                break;
        }
    }

    tempoMapNode.addChild(tempoChange, insertIndex, &undoManager);

    DBG("ProjectState: Added tempo change " + tempoId + " at beat " + juce::String(beatPosition));
    return tempoId;
}

bool ProjectState::moveTempoChange(const juce::String& tempoId, double newBeatPosition,
                                    const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newBeatPosition >= 0.0);

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    // Find tempo change
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto tempoChange = tempoMapNode.getChild(i);
        if (tempoChange.hasType(ID_TEMPO_CHANGE) && tempoChange[PROP_ID].toString() == tempoId)
        {
            double oldBeat = tempoChange[PROP_BEAT_POSITION];

            // Check if trying to move the first tempo away from beat 0
            if (oldBeat == 0.0 && newBeatPosition != 0.0)
            {
                // Check if there are other tempos
                int numTempos = 0;
                for (auto child : tempoMapNode)
                    if (child.hasType(ID_TEMPO_CHANGE))
                        ++numTempos;

                if (numTempos == 1)
                {
                    DBG("ProjectState: Cannot move the only tempo change away from beat 0");
                    return false;
                }
            }

            tempoChange.setProperty(PROP_BEAT_POSITION, newBeatPosition, &undoManager);

            // Re-sort if necessary
            if (newBeatPosition != oldBeat)
            {
                tempoMapNode.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < tempoMapNode.getNumChildren(); ++j)
                {
                    auto existing = tempoMapNode.getChild(j);
                    if (existing.hasType(ID_TEMPO_CHANGE))
                    {
                        double existingBeat = existing[PROP_BEAT_POSITION];
                        if (newBeatPosition >= existingBeat)
                            insertIndex = j + 1;
                        else
                            break;
                    }
                }

                tempoMapNode.addChild(tempoChange, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved tempo change " + tempoId);
            return true;
        }
    }

    return false;
}

bool ProjectState::setTempoChangeBpm(const juce::String& tempoId, double newBpm,
                                      const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newBpm > 0.0);

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    for (auto tempoChange : tempoMapNode)
    {
        if (tempoChange.hasType(ID_TEMPO_CHANGE) && tempoChange[PROP_ID].toString() == tempoId)
        {
            tempoChange.setProperty(PROP_BPM, newBpm, &undoManager);
            DBG("ProjectState: Set tempo change " + tempoId + " BPM to " + juce::String(newBpm));
            return true;
        }
    }

    return false;
}

bool ProjectState::setTempoChangeTimeSig(const juce::String& tempoId, int numerator, int denominator,
                                          const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(numerator > 0);
    jassert(denominator > 0);

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    for (auto tempoChange : tempoMapNode)
    {
        if (tempoChange.hasType(ID_TEMPO_CHANGE) && tempoChange[PROP_ID].toString() == tempoId)
        {
            tempoChange.setProperty(PROP_TIME_SIG_NUM_CHANGE, numerator, &undoManager);
            tempoChange.setProperty(PROP_TIME_SIG_DEN_CHANGE, denominator, &undoManager);
            DBG("ProjectState: Set tempo change " + tempoId + " time sig to " +
                juce::String(numerator) + "/" + juce::String(denominator));
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteTempoChange(const juce::String& tempoId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempoMapNode = state.getChildWithName(ID_TEMPO_MAP);
    if (!tempoMapNode.isValid())
        return false;

    // Find the tempo change
    for (int i = 0; i < tempoMapNode.getNumChildren(); ++i)
    {
        auto tempoChange = tempoMapNode.getChild(i);
        if (tempoChange.hasType(ID_TEMPO_CHANGE) && tempoChange[PROP_ID].toString() == tempoId)
        {
            double beatPos = tempoChange[PROP_BEAT_POSITION];
            double bpm = tempoChange[PROP_BPM];
            int num = tempoChange[PROP_TIME_SIG_NUM_CHANGE];
            int den = tempoChange[PROP_TIME_SIG_DEN_CHANGE];

            // Delete the tempo change
            tempoMapNode.removeChild(i, &undoManager);

            // Ensure there's always a tempo at beat 0
            bool hasZeroTempo = false;
            for (auto child : tempoMapNode)
            {
                if (child.hasType(ID_TEMPO_CHANGE))
                {
                    double childBeat = child[PROP_BEAT_POSITION];
                    if (childBeat == 0.0)
                    {
                        hasZeroTempo = true;
                        break;
                    }
                }
            }

            if (!hasZeroTempo)
            {
                // Create a default tempo at beat 0 with the values from the deleted one
                juce::ValueTree defaultTempo(ID_TEMPO_CHANGE);
                defaultTempo.setProperty(PROP_ID, generateUniqueId("tempo"), nullptr);
                defaultTempo.setProperty(PROP_BEAT_POSITION, 0.0, nullptr);
                defaultTempo.setProperty(PROP_BPM, bpm, nullptr);
                defaultTempo.setProperty(PROP_TIME_SIG_NUM_CHANGE, num, nullptr);
                defaultTempo.setProperty(PROP_TIME_SIG_DEN_CHANGE, den, nullptr);
                tempoMapNode.addChild(defaultTempo, 0, &undoManager);
                DBG("ProjectState: Created default tempo at beat 0 after deletion");
            }

            DBG("ProjectState: Deleted tempo change " + tempoId);
            return true;
        }
    }

    return false;
}

double ProjectState::getTempoAtBeat(double beat) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempos = getTempoChanges();
    if (tempos.isEmpty())
        return getTempo();  // Fallback to project tempo

    // Find the tempo change that applies at this beat
    double currentBpm = tempos[0].bpm;
    for (const auto& tempo : tempos)
    {
        if (beat >= tempo.beatPosition)
            currentBpm = tempo.bpm;
        else
            break;
    }

    return currentBpm;
}

double ProjectState::beatToSeconds(double beat) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempos = getTempoChanges();
    if (tempos.isEmpty())
    {
        // Fallback: use project tempo
        double bpm = getTempo();
        return (beat / bpm) * 60.0;
    }

    // Integrate through tempo changes
    double seconds = 0.0;
    double currentBeat = 0.0;

    for (int i = 0; i < tempos.size(); ++i)
    {
        const auto& tempo = tempos[i];
        double nextBeat = (i + 1 < tempos.size()) ? tempos[i + 1].beatPosition : beat;

        if (beat <= tempo.beatPosition)
            break;

        if (beat < nextBeat)
            nextBeat = beat;

        double beatDelta = nextBeat - currentBeat;
        double secondsDelta = (beatDelta / tempo.bpm) * 60.0;
        seconds += secondsDelta;
        currentBeat = nextBeat;

        if (currentBeat >= beat)
            break;
    }

    // Handle remaining beats if we're beyond the last tempo change
    if (currentBeat < beat && !tempos.isEmpty())
    {
        const auto& lastTempo = tempos.getLast();
        double beatDelta = beat - currentBeat;
        double secondsDelta = (beatDelta / lastTempo.bpm) * 60.0;
        seconds += secondsDelta;
    }

    return seconds;
}

double ProjectState::secondsToBeat(double seconds) const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto tempos = getTempoChanges();
    if (tempos.isEmpty())
    {
        // Fallback: use project tempo
        double bpm = getTempo();
        return (seconds / 60.0) * bpm;
    }

    // Integrate through tempo changes
    double beat = 0.0;
    double currentSeconds = 0.0;

    for (int i = 0; i < tempos.size(); ++i)
    {
        const auto& tempo = tempos[i];

        // Calculate seconds at the next tempo change
        double nextBeat = (i + 1 < tempos.size()) ? tempos[i + 1].beatPosition : beat + 1000.0;  // Large number
        double beatDelta = nextBeat - beat;
        double secondsForSegment = (beatDelta / tempo.bpm) * 60.0;
        double nextSeconds = currentSeconds + secondsForSegment;

        if (seconds <= nextSeconds || i + 1 >= tempos.size())
        {
            // Target is within this segment
            double remainingSeconds = seconds - currentSeconds;
            double additionalBeats = (remainingSeconds / 60.0) * tempo.bpm;
            beat += additionalBeats;
            break;
        }

        // Move to next segment
        currentSeconds = nextSeconds;
        beat = nextBeat;
    }

    return juce::jmax(0.0, beat);
}

//==============================================================================
// Phase 15: Markers Management
//==============================================================================

juce::Array<ProjectState::MarkerSpec> ProjectState::getMarkers() const
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::Array<MarkerSpec> result;

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return result;

    // Collect all markers
    for (auto child : markersNode)
    {
        if (child.hasType(ID_MARKER))
        {
            MarkerSpec spec;
            spec.id = child[PROP_ID].toString();
            spec.name = child[PROP_NAME].toString();
            spec.beatPosition = child[PROP_BEAT_POSITION];
            spec.color = child[PROP_COLOR].toString();
            result.add(spec);
        }
    }

    // Sort by beat position
    std::sort(result.begin(), result.end(),
              [](const MarkerSpec& a, const MarkerSpec& b) {
                  return a.beatPosition < b.beatPosition;
              });

    return result;
}

juce::String ProjectState::addMarker(double beatPosition, const juce::String& name,
                                      const juce::String& colorHex, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(beatPosition >= 0.0);

    // Get or create markers node
    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
    {
        markersNode = juce::ValueTree(ID_MARKERS);
        state.appendChild(markersNode, &undoManager);
        DBG("ProjectState: Created MARKERS node");
    }

    // Generate unique ID
    auto markerId = generateUniqueId("marker");

    // Create marker node
    juce::ValueTree marker(ID_MARKER);
    marker.setProperty(PROP_ID, markerId, nullptr);
    marker.setProperty(PROP_NAME, name, nullptr);
    marker.setProperty(PROP_BEAT_POSITION, beatPosition, nullptr);
    marker.setProperty(PROP_COLOR, colorHex, nullptr);

    // Insert in sorted order by beat position
    int insertIndex = 0;
    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto existing = markersNode.getChild(i);
        if (existing.hasType(ID_MARKER))
        {
            double existingBeat = existing[PROP_BEAT_POSITION];
            if (beatPosition >= existingBeat)
                insertIndex = i + 1;
            else
                break;
        }
    }

    markersNode.addChild(marker, insertIndex, &undoManager);

    DBG("ProjectState: Added marker " + markerId + " at beat " + juce::String(beatPosition));
    return markerId;
}

bool ProjectState::moveMarker(const juce::String& markerId, double newBeatPosition,
                               const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(newBeatPosition >= 0.0);

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    // Find marker
    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto marker = markersNode.getChild(i);
        if (marker.hasType(ID_MARKER) && marker[PROP_ID].toString() == markerId)
        {
            double oldBeat = marker[PROP_BEAT_POSITION];
            marker.setProperty(PROP_BEAT_POSITION, newBeatPosition, &undoManager);

            // Re-sort if necessary
            if (newBeatPosition != oldBeat)
            {
                markersNode.removeChild(i, &undoManager);

                int insertIndex = 0;
                for (int j = 0; j < markersNode.getNumChildren(); ++j)
                {
                    auto existing = markersNode.getChild(j);
                    if (existing.hasType(ID_MARKER))
                    {
                        double existingBeat = existing[PROP_BEAT_POSITION];
                        if (newBeatPosition >= existingBeat)
                            insertIndex = j + 1;
                        else
                            break;
                    }
                }

                markersNode.addChild(marker, insertIndex, &undoManager);
            }

            DBG("ProjectState: Moved marker " + markerId);
            return true;
        }
    }

    return false;
}

bool ProjectState::renameMarker(const juce::String& markerId, const juce::String& newName,
                                 const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    for (auto marker : markersNode)
    {
        if (marker.hasType(ID_MARKER) && marker[PROP_ID].toString() == markerId)
        {
            marker.setProperty(PROP_NAME, newName, &undoManager);
            DBG("ProjectState: Renamed marker " + markerId + " to '" + newName + "'");
            return true;
        }
    }

    return false;
}

bool ProjectState::recolorMarker(const juce::String& markerId, const juce::String& newColorHex,
                                  const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    for (auto marker : markersNode)
    {
        if (marker.hasType(ID_MARKER) && marker[PROP_ID].toString() == markerId)
        {
            marker.setProperty(PROP_COLOR, newColorHex, &undoManager);
            DBG("ProjectState: Recolored marker " + markerId + " to " + newColorHex);
            return true;
        }
    }

    return false;
}

bool ProjectState::deleteMarker(const juce::String& markerId, const juce::String& actionName)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto markersNode = state.getChildWithName(ID_MARKERS);
    if (!markersNode.isValid())
        return false;

    for (int i = 0; i < markersNode.getNumChildren(); ++i)
    {
        auto marker = markersNode.getChild(i);
        if (marker.hasType(ID_MARKER) && marker[PROP_ID].toString() == markerId)
        {
            markersNode.removeChild(i, &undoManager);
            DBG("ProjectState: Deleted marker " + markerId);
            return true;
        }
    }

    return false;
}
