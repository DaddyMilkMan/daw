/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    ProjectManagerIO.cpp
    Project management I/O and Template implementation
  ==============================================================================
*/


namespace zenith {
namespace ui {

//==============================================================================
// Project I/O Operations
//==============================================================================

bool ProjectManagerUI::loadProject(const juce::File& projectFile) {
    try {
        // Parse project file
        auto content = projectFile.loadFileAsString();
        auto data = juce::JSON::parse(content);
        
        if (!data.isObject()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                  "Error", "Invalid project file.");
            return false;
        }
        
        // Create project from data
        currentProject = std::make_unique<Project>();
        currentProject->projectId = data.getProperty("projectId", "");
        currentProject->name = data.getProperty("name", "Untitled");
        currentProject->path = projectFile.getParentDirectory().getFullPathName();
        currentProject->created = juce::Time(data.getProperty("created", 0));
        currentProject->modified = juce::Time(data.getProperty("modified", 0));
        currentProject->isModified = false;
        
        // Load tracks
        auto tracksData = data.getProperty("tracks", juce::var());
        if (tracksData.isArray()) {
            for (int i = 0; i < tracksData.size(); ++i) {
                auto trackData = tracksData[i];
                if (trackData.isObject()) {
                    Track track;
                    track.trackId = trackData.getProperty("trackId", "");
                    track.name = trackData.getProperty("name", "Track " + juce::String(i + 1));
                    track.type = trackData.getProperty("type", "audio");
                    track.isMuted = trackData.getProperty("isMuted", false);
                    track.isSolo = trackData.getProperty("isSolo", false);
                    track.isArmed = trackData.getProperty("isArmed", false);
                    track.volume = trackData.getProperty("volume", 0.0f);
                    track.pan = trackData.getProperty("pan", 0.0f);
                    
                    currentProject->tracks.push_back(track);
                }
            }
        }
        
        // Load effects
        auto effectsData = data.getProperty("effects", juce::var());
        if (effectsData.isArray()) {
            for (int i = 0; i < effectsData.size(); ++i) {
                auto effectData = effectsData[i];
                if (effectData.isObject()) {
                    Effect effect;
                    effect.effectId = effectData.getProperty("effectId", "");
                    effect.name = effectData.getProperty("name", "Effect " + juce::String(i + 1));
                    effect.type = effectData.getProperty("type", "unknown");
                    effect.trackId = effectData.getProperty("trackId", "");
                    effect.isEnabled = effectData.getProperty("isEnabled", true);
                    effect.position = effectData.getProperty("position", 0);
                    
                    currentProject->effects.push_back(effect);
                }
            }
        }
        
        // Add to recent projects
        addToRecentProjects(*currentProject);
        
        // Update UI
        updateProjectList();
        updateTrackList();
        updateEffectList();
        updateRecentProjects();
        updateStatusBar();
        
        notifyProjectOpened(*currentProject);
        return true;
    } catch (const std::exception& e) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error", "Failed to load project: " + juce::String(e.what()));
        return false;
    }
}

bool ProjectManagerUI::saveProjectToFile() {
    if (!currentProject) return false;
    
    // Create project data
    juce::DynamicObject::Ptr projectData = new juce::DynamicObject();
    projectData->setProperty("projectId", currentProject->projectId);
    projectData->setProperty("name", currentProject->name);
    projectData->setProperty("created", currentProject->created.toMilliseconds());
    projectData->setProperty("modified", currentProject->modified.toMilliseconds());
    
    // Save tracks
    juce::Array<juce::var> tracksArray;
    for (const auto& track : currentProject->tracks) {
        juce::DynamicObject::Ptr trackObj = new juce::DynamicObject();
        trackObj->setProperty("trackId", track.trackId);
        trackObj->setProperty("name", track.name);
        trackObj->setProperty("type", track.type);
        trackObj->setProperty("isMuted", track.isMuted);
        trackObj->setProperty("isSolo", track.isSolo);
        trackObj->setProperty("isArmed", track.isArmed);
        trackObj->setProperty("volume", track.volume);
        trackObj->setProperty("pan", track.pan);
        tracksArray.add(trackObj);
    }
    projectData->setProperty("tracks", tracksArray);
    
    // Save effects
    juce::Array<juce::var> effectsArray;
    for (const auto& effect : currentProject->effects) {
        juce::DynamicObject::Ptr effectObj = new juce::DynamicObject();
        effectObj->setProperty("effectId", effect.effectId);
        effectObj->setProperty("name", effect.name);
        effectObj->setProperty("type", effect.type);
        effectObj->setProperty("trackId", effect.trackId);
        effectObj->setProperty("isEnabled", effect.isEnabled);
        effectObj->setProperty("position", effect.position);
        effectsArray.add(effectObj);
    }
    projectData->setProperty("effects", effectsArray);
    
    // Write to file
    juce::File projectFile = juce::File(currentProject->path).getChildFile(currentProject->name + ".zenith");
    return projectFile.replaceWithText(juce::JSON::toString(projectData));
}

//==============================================================================
// Recent Projects Management
//==============================================================================

void ProjectManagerUI::saveRecentProjects() {
    juce::File recentFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("ZenithDAW")
                           .getChildFile("recent_projects.json");
    
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    juce::Array<juce::var> projectArray;
    
    for (const auto& project : recentProjects) {
        juce::DynamicObject::Ptr projectObj = new juce::DynamicObject();
        projectObj->setProperty("projectId", project.projectId);
        projectObj->setProperty("name", project.name);
        projectObj->setProperty("path", project.path);
        projectObj->setProperty("modified", project.modified.toMilliseconds());
        projectArray.add(projectObj);
    }
    
    data->setProperty("projects", projectArray);
    recentFile.replaceWithText(juce::JSON::toString(data));
}

void ProjectManagerUI::loadRecentProjects() {
    juce::File recentFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("ZenithDAW")
                           .getChildFile("recent_projects.json");
    
    if (recentFile.exists()) {
        try {
            auto content = recentFile.loadFileAsString();
            auto data = juce::JSON::parse(content);
            
            if (data.hasProperty("projects") && data["projects"].isArray()) {
                auto projects = data["projects"];
                recentProjects.clear();
                
                for (int i = 0; i < projects.size(); ++i) {
                    auto projectData = projects[i];
                    if (projectData.isObject()) {
                        Project project;
                        project.projectId = projectData.getProperty("projectId", "");
                        project.name = projectData.getProperty("name", "");
                        project.path = projectData.getProperty("path", "");
                        project.modified = juce::Time(projectData.getProperty("modified", 0));
                        
                        recentProjects.push_back(project);
                    }
                }
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Failed to load recent projects: " + juce::String(e.what()));
        }
    }
}

//==============================================================================
// ProjectTemplateManager Implementation
//==============================================================================

ProjectTemplateManager::ProjectTemplateManager() {
    loadDefaultTemplates();
}

ProjectTemplateManager::~ProjectTemplateManager() = default;

void ProjectTemplateManager::saveTemplate(const ProjectTemplate& template_) {
    templates.push_back(template_);
    saveTemplates();
}

void ProjectTemplateManager::deleteTemplate(const juce::String& templateId) {
    templates.erase(std::remove_if(templates.begin(), templates.end(),
                                  [&templateId](const ProjectTemplate& t) { return t.templateId == templateId; }),
                   templates.end());
    saveTemplates();
}

std::vector<ProjectTemplate> ProjectTemplateManager::getAllTemplates() const {
    return templates;
}

ProjectTemplate* ProjectTemplateManager::getTemplate(const juce::String& templateId) {
    for (auto& template_ : templates) {
        if (template_.templateId == templateId) {
            return &template_;
        }
    }
    return nullptr;
}

void ProjectTemplateManager::loadDefaultTemplates() {
    // Create default templates
    ProjectTemplate emptyTemplate;
    emptyTemplate.templateId = juce::Uuid().toString();
    emptyTemplate.name = "Empty Project";
    emptyTemplate.description = "Start with a blank project";
    emptyTemplate.created = juce::Time::getCurrentTime();
    templates.push_back(emptyTemplate);
    
    ProjectTemplate musicTemplate;
    musicTemplate.templateId = juce::Uuid().toString();
    musicTemplate.name = "Music Production";
    musicTemplate.description = "Basic setup for music production";
    musicTemplate.created = juce::Time::getCurrentTime();
    
    // Add default tracks
    Track drums;
    drums.trackId = juce::Uuid().toString();
    drums.name = "Drums";
    drums.type = "audio";
    musicTemplate.tracks.push_back(drums);
    
    Track bass;
    bass.trackId = juce::Uuid().toString();
    bass.name = "Bass";
    bass.type = "audio";
    musicTemplate.tracks.push_back(bass);
    
    Track guitar;
    guitar.trackId = juce::Uuid().toString();
    guitar.name = "Guitar";
    guitar.type = "audio";
    musicTemplate.tracks.push_back(guitar);
    
    Track vocals;
    vocals.trackId = juce::Uuid().toString();
    vocals.name = "Vocals";
    vocals.type = "audio";
    musicTemplate.tracks.push_back(vocals);
    
    templates.push_back(musicTemplate);
}

void ProjectTemplateManager::saveTemplates() {
    juce::File templatesFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("ZenithDAW")
                              .getChildFile("project_templates.json");
    
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    juce::Array<juce::var> templateArray;
    
    for (const auto& template_ : templates) {
        juce::DynamicObject::Ptr templateObj = new juce::DynamicObject();
        templateObj->setProperty("templateId", template_.templateId);
        templateObj->setProperty("name", template_.name);
        templateObj->setProperty("description", template_.description);
        templateObj->setProperty("created", template_.created.toMilliseconds());
        
        // Save tracks
        juce::Array<juce::var> tracksArray;
        for (const auto& track : template_.tracks) {
            juce::DynamicObject::Ptr trackObj = new juce::DynamicObject();
            trackObj->setProperty("trackId", track.trackId);
            trackObj->setProperty("name", track.name);
            trackObj->setProperty("type", track.type);
            tracksArray.add(trackObj);
        }
        templateObj->setProperty("tracks", tracksArray);
        
        templateArray.add(templateObj);
    }
    
    data->setProperty("templates", templateArray);
    templatesFile.replaceWithText(juce::JSON::toString(data));
}

} // namespace ui
} // namespace zenith
