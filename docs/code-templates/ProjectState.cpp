/*
  ==============================================================================

    ProjectState.cpp
    Created: 2025-11-10

    Implementation of project state management.

  ==============================================================================
*/

#include "ProjectState.h"

//==============================================================================
// Static Identifiers

const juce::Identifier ProjectState::ID_PROJECT       ("PROJECT");
const juce::Identifier ProjectState::ID_TRACKS        ("TRACKS");
const juce::Identifier ProjectState::ID_TRACK         ("TRACK");
const juce::Identifier ProjectState::ID_CLIPS         ("CLIPS");
const juce::Identifier ProjectState::ID_CLIP          ("CLIP");
const juce::Identifier ProjectState::ID_PLUGINS       ("PLUGINS");
const juce::Identifier ProjectState::ID_PLUGIN        ("PLUGIN");
const juce::Identifier ProjectState::ID_AUTOMATION    ("AUTOMATION");
const juce::Identifier ProjectState::ID_SETTINGS      ("SETTINGS");

const juce::Identifier ProjectState::PROP_ID          ("id");
const juce::Identifier ProjectState::PROP_NAME        ("name");
const juce::Identifier ProjectState::PROP_TYPE        ("type");
const juce::Identifier ProjectState::PROP_NUM_CHANNELS("numChannels");
const juce::Identifier ProjectState::PROP_COLOR       ("color");
const juce::Identifier ProjectState::PROP_VOLUME      ("volume");
const juce::Identifier ProjectState::PROP_PAN         ("pan");
const juce::Identifier ProjectState::PROP_MUTE        ("mute");
const juce::Identifier ProjectState::PROP_SOLO        ("solo");

const juce::Identifier ProjectState::PROP_START_TIME  ("startTime");
const juce::Identifier ProjectState::PROP_LENGTH      ("length");
const juce::Identifier ProjectState::PROP_OFFSET      ("offset");
const juce::Identifier ProjectState::PROP_FILE        ("file");

const juce::Identifier ProjectState::PROP_PATH        ("path");
const juce::Identifier ProjectState::PROP_STATE       ("state");

const juce::Identifier ProjectState::PROP_TEMPO       ("tempo");
const juce::Identifier ProjectState::PROP_TIME_SIG_NUM("timeSigNumerator");
const juce::Identifier ProjectState::PROP_TIME_SIG_DEN("timeSigDenominator");
const juce::Identifier ProjectState::PROP_SAMPLE_RATE ("sampleRate");

//==============================================================================
ProjectState::ProjectState()
{
    createDefaultProjectTree();
}

ProjectState::~ProjectState()
{
}

//==============================================================================
// Save/Load

bool ProjectState::saveToFile(const juce::File& file)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Convert ValueTree to XML
    auto xml = projectTree.createXml();

    if (xml == nullptr)
        return false;

    // Write to file
    if (xml->writeTo(file))
    {
        markAsSaved();
        return true;
    }

    return false;
}

bool ProjectState::loadFromFile(const juce::File& file)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Parse XML file
    auto xml = juce::parseXML(file);

    if (xml == nullptr)
        return false;

    // Convert XML to ValueTree
    auto loadedTree = juce::ValueTree::fromXml(*xml);

    if (!loadedTree.isValid() || loadedTree.getType() != ID_PROJECT)
        return false;

    // Replace current project tree
    projectTree = loadedTree;

    // Clear undo history (fresh start)
    undoManager.clearUndoHistory();

    return true;
}

void ProjectState::markAsSaved()
{
    // Clear the undo manager's transaction count
    // Note: You may want a more sophisticated "saved state" tracking
    undoManager.beginNewTransaction();
}

//==============================================================================
// Project Settings

void ProjectState::setTempo(double bpm)
{
    auto settings = getSettingsTree();
    settings.setProperty(PROP_TEMPO, bpm, &undoManager);
}

double ProjectState::getTempo() const
{
    auto settings = getSettingsTree();
    return settings.getProperty(PROP_TEMPO, 120.0);
}

void ProjectState::setTimeSignature(int numerator, int denominator)
{
    auto settings = getSettingsTree();
    settings.setProperty(PROP_TIME_SIG_NUM, numerator, &undoManager);
    settings.setProperty(PROP_TIME_SIG_DEN, denominator, &undoManager);
}

void ProjectState::getTimeSignature(int& numerator, int& denominator) const
{
    auto settings = getSettingsTree();
    numerator = settings.getProperty(PROP_TIME_SIG_NUM, 4);
    denominator = settings.getProperty(PROP_TIME_SIG_DEN, 4);
}

void ProjectState::setSampleRate(double rate)
{
    auto settings = getSettingsTree();
    settings.setProperty(PROP_SAMPLE_RATE, rate, &undoManager);
}

double ProjectState::getSampleRate() const
{
    auto settings = getSettingsTree();
    return settings.getProperty(PROP_SAMPLE_RATE, 44100.0);
}

//==============================================================================
// Track Management

juce::ValueTree ProjectState::addTrack(const juce::String& trackId,
                                       const juce::String& trackName,
                                       int numChannels)
{
    auto tracksTree = getTracksTree();

    // Create new track ValueTree
    juce::ValueTree newTrack(ID_TRACK);
    newTrack.setProperty(PROP_ID, trackId, nullptr);
    newTrack.setProperty(PROP_NAME, trackName, nullptr);
    newTrack.setProperty(PROP_NUM_CHANNELS, numChannels, nullptr);
    newTrack.setProperty(PROP_VOLUME, 0.8f, nullptr);  // Default 80% volume
    newTrack.setProperty(PROP_PAN, 0.0f, nullptr);      // Center
    newTrack.setProperty(PROP_MUTE, false, nullptr);
    newTrack.setProperty(PROP_SOLO, false, nullptr);
    newTrack.setProperty(PROP_COLOR, juce::Colour(0xff4080ff).toString(), nullptr);

    // Add empty CLIPS and PLUGINS containers
    newTrack.addChild(juce::ValueTree(ID_CLIPS), -1, nullptr);
    newTrack.addChild(juce::ValueTree(ID_PLUGINS), -1, nullptr);

    // Add to project tree (this is the undoable operation)
    tracksTree.addChild(newTrack, -1, &undoManager);

    return newTrack;
}

void ProjectState::removeTrack(const juce::String& trackId)
{
    auto track = findTrack(trackId);

    if (track.isValid())
    {
        auto parent = track.getParent();
        parent.removeChild(track, &undoManager);
    }
}

juce::ValueTree ProjectState::findTrack(const juce::String& trackId) const
{
    auto tracksTree = getTracksTree();

    for (auto track : tracksTree)
    {
        if (track.getProperty(PROP_ID).toString() == trackId)
            return track;
    }

    return juce::ValueTree();
}

juce::ValueTree ProjectState::getTracksTree() const
{
    return projectTree.getChildWithName(ID_TRACKS);
}

//==============================================================================
// Clip Management

juce::ValueTree ProjectState::addClip(const juce::String& trackId,
                                      const juce::String& clipId,
                                      const juce::String& clipType,
                                      double startTime,
                                      double length)
{
    auto track = findTrack(trackId);

    if (!track.isValid())
        return juce::ValueTree();

    auto clipsTree = track.getChildWithName(ID_CLIPS);

    // Create new clip
    juce::ValueTree newClip(ID_CLIP);
    newClip.setProperty(PROP_ID, clipId, nullptr);
    newClip.setProperty(PROP_TYPE, clipType, nullptr);
    newClip.setProperty(PROP_START_TIME, startTime, nullptr);
    newClip.setProperty(PROP_LENGTH, length, nullptr);
    newClip.setProperty(PROP_OFFSET, 0.0, nullptr);

    clipsTree.addChild(newClip, -1, &undoManager);

    return newClip;
}

void ProjectState::removeClip(const juce::String& trackId, const juce::String& clipId)
{
    auto clip = findClip(trackId, clipId);

    if (clip.isValid())
    {
        auto parent = clip.getParent();
        parent.removeChild(clip, &undoManager);
    }
}

juce::ValueTree ProjectState::findClip(const juce::String& trackId, const juce::String& clipId) const
{
    auto track = findTrack(trackId);

    if (!track.isValid())
        return juce::ValueTree();

    auto clipsTree = track.getChildWithName(ID_CLIPS);

    for (auto clip : clipsTree)
    {
        if (clip.getProperty(PROP_ID).toString() == clipId)
            return clip;
    }

    return juce::ValueTree();
}

//==============================================================================
// Plugin Management

juce::ValueTree ProjectState::addPlugin(const juce::String& trackId,
                                        const juce::String& pluginId,
                                        const juce::String& pluginPath)
{
    auto track = findTrack(trackId);

    if (!track.isValid())
        return juce::ValueTree();

    auto pluginsTree = track.getChildWithName(ID_PLUGINS);

    // Create new plugin
    juce::ValueTree newPlugin(ID_PLUGIN);
    newPlugin.setProperty(PROP_ID, pluginId, nullptr);
    newPlugin.setProperty(PROP_PATH, pluginPath, nullptr);
    newPlugin.setProperty(PROP_STATE, "", nullptr);  // Empty state initially

    pluginsTree.addChild(newPlugin, -1, &undoManager);

    return newPlugin;
}

void ProjectState::removePlugin(const juce::String& trackId, const juce::String& pluginId)
{
    auto track = findTrack(trackId);

    if (!track.isValid())
        return;

    auto pluginsTree = track.getChildWithName(ID_PLUGINS);

    for (auto plugin : pluginsTree)
    {
        if (plugin.getProperty(PROP_ID).toString() == pluginId)
        {
            pluginsTree.removeChild(plugin, &undoManager);
            break;
        }
    }
}

//==============================================================================
// Private Methods

void ProjectState::createDefaultProjectTree()
{
    // Create root project tree
    projectTree = juce::ValueTree(ID_PROJECT);

    // Add TRACKS container
    projectTree.addChild(juce::ValueTree(ID_TRACKS), -1, nullptr);

    // Add AUTOMATION container
    projectTree.addChild(juce::ValueTree(ID_AUTOMATION), -1, nullptr);

    // Add SETTINGS
    juce::ValueTree settings(ID_SETTINGS);
    settings.setProperty(PROP_TEMPO, 120.0, nullptr);
    settings.setProperty(PROP_TIME_SIG_NUM, 4, nullptr);
    settings.setProperty(PROP_TIME_SIG_DEN, 4, nullptr);
    settings.setProperty(PROP_SAMPLE_RATE, 44100.0, nullptr);

    projectTree.addChild(settings, -1, nullptr);

    // Clear undo history (fresh project)
    undoManager.clearUndoHistory();
}

juce::ValueTree ProjectState::getSettingsTree() const
{
    return projectTree.getChildWithName(ID_SETTINGS);
}
