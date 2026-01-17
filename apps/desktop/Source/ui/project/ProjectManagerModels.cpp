/*
  ==============================================================================
    ProjectManagerModels.cpp
    Project management ListBoxModel implementations
  ==============================================================================
*/

#include "ProjectManagerUI.h"

namespace zenith {
namespace ui {

//==============================================================================
// TrackListModel Implementation
//==============================================================================

TrackListModel::TrackListModel(const std::vector<Track>& tracks) : tracks(tracks) {
}

int TrackListModel::getNumRows() {
    return static_cast<int>(tracks.size());
}

void TrackListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= tracks.size()) return;
    
    const auto& track = tracks[rowNumber];
    
    // Track name
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(track.name, 10, 0, width - 100, height, juce::Justification::centredLeft, false);
    
    // Track type
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText(track.type, width - 90, 0, 80, height, juce::Justification::centredLeft, false);
    
    // Status indicators
    int x = width - 10;
    if (track.isMuted) {
        g.setColour(juce::Colours::orange);
        g.drawText("M", x, 0, 10, height, juce::Justification::centred, false);
        x -= 15;
    }
    if (track.isSolo) {
        g.setColour(juce::Colours::yellow);
        g.drawText("S", x, 0, 10, height, juce::Justification::centred, false);
        x -= 15;
    }
    if (track.isArmed) {
        g.setColour(juce::Colours::red);
        g.drawText("R", x, 0, 10, height, juce::Justification::centred, false);
    }
}

juce::Component* TrackListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                       juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void TrackListModel::setTracks(const std::vector<Track>& newTracks) {
    tracks = newTracks;
}

void TrackListModel::addTrack(const Track& track) {
    tracks.push_back(track);
}

void TrackListModel::removeTrack(int index) {
    if (index >= 0 && index < tracks.size()) {
        tracks.erase(tracks.begin() + index);
    }
}

//==============================================================================
// EffectListModel Implementation
//==============================================================================

EffectListModel::EffectListModel(const std::vector<Effect>& effects) : effects(effects) {
}

int EffectListModel::getNumRows() {
    return static_cast<int>(effects.size());
}

void EffectListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= effects.size()) return;
    
    const auto& effect = effects[rowNumber];
    
    // Effect name
    g.setColour(effect.isEnabled ? juce::Colours::white : juce::Colours::grey);
    g.setFont(14.0f);
    g.drawText(effect.name, 10, 0, width - 100, height, juce::Justification::centredLeft, false);
    
    // Effect type
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText(effect.type, width - 90, 0, 80, height, juce::Justification::centredLeft, false);
    
    // Position
    g.drawText(juce::String(effect.position), width - 10, 0, 10, height, juce::Justification::centred, false);
}

juce::Component* EffectListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                       juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void EffectListModel::setEffects(const std::vector<Effect>& newEffects) {
    effects = newEffects;
}

void EffectListModel::addEffect(const Effect& effect) {
    effects.push_back(effect);
}

void EffectListModel::removeEffect(int index) {
    if (index >= 0 && index < effects.size()) {
        effects.erase(effects.begin() + index);
    }
}

//==============================================================================
// RecentProjectsListModel Implementation
//==============================================================================

RecentProjectsListModel::RecentProjectsListModel(const std::vector<Project>& projects) : projects(projects) {
}

int RecentProjectsListModel::getNumRows() {
    return static_cast<int>(projects.size());
}

void RecentProjectsListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= projects.size()) return;
    
    const auto& project = projects[rowNumber];
    
    // Project name
    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(project.name, 10, 0, width - 10, height / 2, juce::Justification::centredLeft, false);
    
    // Modified date
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText(project.modified.toString(true, false), 10, height / 2, width - 10, height / 2, 
               juce::Justification::centredLeft, false);
}

juce::Component* RecentProjectsListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                               juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void RecentProjectsListModel::setProjects(const std::vector<Project>& newProjects) {
    projects = newProjects;
}

//==============================================================================
// TemplatesListModel Implementation
//==============================================================================

TemplatesListModel::TemplatesListModel(const std::vector<ProjectTemplate>& templates) : templates(templates) {
}

int TemplatesListModel::getNumRows() {
    return static_cast<int>(templates.size());
}

void TemplatesListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) {
    if (rowNumber >= templates.size()) return;
    
    const auto& template_ = templates[rowNumber];
    
    // Template name
    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(template_.name, 10, 0, width - 10, height / 2, juce::Justification::centredLeft, false);
    
    // Description
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText(template_.description, 10, height / 2, width - 10, height / 2, 
               juce::Justification::centredLeft, false);
}

juce::Component* TemplatesListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, 
                                                           juce::Component* existingComponentToUpdate) {
    return existingComponentToUpdate;
}

void TemplatesListModel::setTemplates(const std::vector<ProjectTemplate>& newTemplates) {
    templates = newTemplates;
}

} // namespace ui
} // namespace zenith
