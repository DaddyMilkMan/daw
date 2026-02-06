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

#include "ProjectManagerUI.h"

==============================================================================
    ProjectManagerUI.cpp
    Project management UI implementation
  ==============================================================================



namespace zenith {
namespace ui {

// ProjectManagerUI Implementation
ProjectManagerUI::ProjectManagerUI()
    : projectManager(std::make_unique<ProjectManager>()),
      undoRedoManager(std::make_unique<UndoRedoManager>()),
      templateManager(std::make_unique<ProjectTemplateManager>()) {
    
    // Initialize UI
    createProjectControls();
    createTrackControls();
    createEffectControls();
    createSessionControls();
    createRecentProjectsList();
    createTemplatesList();
    createSearchAndFilter();
    createFileBrowser();
    createStatusBar();
    
    // Load templates
    templateManager->loadDefaultTemplates();
    
    // Update UI
    updateProjectList();
    updateTrackList();
    updateEffectList();
    updateRecentProjects();
    updateTemplatesList();
    
    // Start auto-save timer
    startTimerHz(1);
}

ProjectManagerUI::~ProjectManagerUI() {
    stopTimer();
    
    // Save current project if needed
    if (currentProject && currentProject->isModified) {
        saveProject();
    }
}

void ProjectManagerUI::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
}

void ProjectManagerUI::resized() {
    auto bounds = getLocalBounds();
    int margin = 10;
    
    // Layout project controls (top)
    int controlHeight = 30;
    int topY = margin;
    
    if (newProjectButton) {
        newProjectButton->setBounds(margin, topY, 100, controlHeight);
    }
    if (openProjectButton) {
        openProjectButton->setBounds(110, topY, 100, controlHeight);
    }
    if (saveProjectButton) {
        saveProjectButton->setBounds(220, topY, 100, controlHeight);
    }
    if (saveAsProjectButton) {
        saveAsProjectButton->setBounds(330, topY, 100, controlHeight);
    }
    if (closeProjectButton) {
        closeProjectButton->setBounds(440, topY, 100, controlHeight);
    }
    
    // Search bar (below controls)
    topY += controlHeight + margin;
    if (searchBox) {
        searchBox->setBounds(margin, topY, 200, controlHeight);
    }
    if (filterComboBox) {
        filterComboBox->setBounds(210, topY, 150, controlHeight);
    }
    if (sortByComboBox) {
        sortByComboBox->setBounds(370, topY, 150, controlHeight);
    }
    
    // Main area (below search)
    topY += controlHeight + margin;
    int mainHeight = bounds.getHeight() - topY - 60; // Leave room for status bar
    
    // Left panel (tracks and effects)
    int leftPanelWidth = 300;
    if (trackListBox) {
        trackListBox->setBounds(margin, topY, leftPanelWidth, mainHeight / 2 - margin);
    }
    topY += mainHeight / 2 + margin;
    if (effectListBox) {
        effectListBox->setBounds(margin, topY, leftPanelWidth, mainHeight / 2 - margin);
    }
    
    // Center panel (project browser)
    int centerPanelX = margin + leftPanelWidth + margin;
    int centerPanelWidth = bounds.getWidth() - leftPanelWidth - 200 - 3 * margin;
    
    if (projectListBox) {
        projectListBox->setBounds(centerPanelX, topY - mainHeight + margin, 
                                 centerPanelWidth, mainHeight - margin);
    }
    
    // Right panel (recent and templates)
    int rightPanelX = centerPanelX + centerPanelWidth + margin;
    int rightPanelWidth = 200;
    
    if (recentProjectsListBox) {
        recentProjectsListBox->setBounds(rightPanelX, topY - mainHeight + margin, 
                                        rightPanelWidth, mainHeight / 2 - margin);
    }
    if (templatesListBox) {
        templatesListBox->setBounds(rightPanelX, topY - mainHeight / 2, 
                                   rightPanelWidth, mainHeight / 2 - margin);
    }
    
    // Status bar (bottom)
    int statusY = bounds.getHeight() - 50;
    if (statusLabel) {
        statusLabel->setBounds(margin, statusY, bounds.getWidth() - 2 * margin, 20);
    }
    statusY += 20;
    if (progressBar) {
        progressBar->setBounds(margin, statusY, bounds.getWidth() - 2 * margin, 20);
    }
}

void ProjectManagerUI::newProject() {
    // Show new project dialog
    auto dialog = std::make_unique<juce::DialogWindow>("New Project", 
                                                     juce::Colours::darkgrey, 
                                                     true);
    
    auto content = std::make_unique<juce::Component>();
    content->setSize(400, 300);
    
    // Project name
    auto nameLabel = std::make_unique<juce::Label>("Name", "Project Name:");
    nameLabel->setBounds(20, 20, 100, 20);
    content->addAndMakeVisible(*nameLabel);
    
    auto nameEditor = std::make_unique<juce::TextEditor>();
    nameEditor->setBounds(130, 20, 250, 20);
    content->addAndMakeVisible(*nameEditor);
    
    // Template selection
    auto templateLabel = std::make_unique<juce::Label>("Template", "Template:");
    templateLabel->setBounds(20, 50, 100, 20);
    content->addAndMakeVisible(*templateLabel);
    
    auto templateCombo = std::make_unique<juce::ComboBox>();
    auto templates = templateManager->getAllTemplates();
    for (size_t i = 0; i < templates.size(); ++i) {
        templateCombo->addItem(templates[i].name, static_cast<int>(i) + 1);
    }
    templateCombo->setBounds(130, 50, 250, 20);
    content->addAndMakeVisible(*templateCombo);
    
    // Location
    auto locationLabel = std::make_unique<juce::Label>("Location", "Location:");
    locationLabel->setBounds(20, 80, 100, 20);
    content->addAndMakeVisible(*locationLabel);
    
    auto locationEditor = std::make_unique<juce::TextEditor>();
    locationEditor->setBounds(130, 80, 200, 20);
    locationEditor->setText(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getFullPathName());
    content->addAndMakeVisible(*locationEditor);
    
    auto browseButton = std::make_unique<juce::TextButton>("Browse");
    browseButton->setBounds(340, 80, 40, 20);
    content->addAndMakeVisible(*browseButton);
    
    // Create button
    auto createButton = std::make_unique<juce::TextButton>("Create");
    createButton->setBounds(150, 250, 100, 30);
    content->addAndMakeVisible(*createButton);
    
    // Store references
    auto* namePtr = nameEditor.get();
    auto* templatePtr = templateCombo.get();
    auto* locationPtr = locationEditor.get();
    
    // Handle create button
    createButton->onClick = [this, dialogPtr = dialog.get(), namePtr, templatePtr, locationPtr]() {
        juce::String name = namePtr->getText();
        juce::File location = locationPtr->getText();
        
        if (name.isEmpty()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                  "Error", "Please enter a project name.");
            return;
        }
        
        // Get selected template
        int templateIndex = templatePtr->getSelectedId() - 1;
        ProjectTemplate* selectedTemplate = nullptr;
        if (templateIndex >= 0 && templateIndex < templateManager->getAllTemplates().size()) {
            selectedTemplate = &templateManager->getAllTemplates()[templateIndex];
        }
        
        // Create project
        if (createNewProject(name, location, selectedTemplate)) {
            dialogPtr->exitModalState();
        }
    };
    
    // Handle browse button
    browseButton->onClick = [locationPtr]() {
        auto chooser = std::make_unique<juce::FileChooser>("Select Project Location");
        if (chooser->browseForDirectory()) {
            locationPtr->setText(chooser->getResult().getFullPathName());
        }
    };
    
    dialog->setContentOwned(content.release(), true);
    dialog->runModalLoop();
}

bool ProjectManagerUI::createNewProject(const juce::String& name, 
                                       const juce::File& location,
                                       const ProjectTemplate* template_) {
    try {
        // Create project directory
        juce::File projectDir = location.getChildFile(name);
        if (!projectDir.createDirectory()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                  "Error", "Failed to create project directory.");
            return false;
        }
        
        // Create project
        currentProject = std::make_unique<Project>();
        currentProject->projectId = juce::Uuid().toString();
        currentProject->name = name;
        currentProject->path = projectDir.getFullPathName();
        currentProject->created = juce::Time::getCurrentTime();
        currentProject->modified = currentProject->created;
        currentProject->isModified = false;
        
        // Apply template if provided
        if (template_) {
            applyTemplate(*template_);
        }
        
        // Save project file
        saveProjectToFile();
        
        // Add to recent projects
        addToRecentProjects(*currentProject);
        
        // Update UI
        updateProjectList();
        updateTrackList();
        updateEffectList();
        updateStatusBar();
        
        notifyProjectCreated(*currentProject);
        return true;
    } catch (const std::exception& e) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error", "Failed to create project: " + juce::String(e.what()));
        return false;
    }
}

void ProjectManagerUI::openProject() {
    auto chooser = std::make_unique<juce::FileChooser>("Open Project",
                                                      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                                      "*.zenith");
    
    if (chooser->browseForFileToOpen()) {
        loadProject(chooser->getResult());
    }
}

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

bool ProjectManagerUI::saveProject() {
    if (!currentProject) return false;
    
    try {
        currentProject->modified = juce::Time::getCurrentTime();
        currentProject->isModified = false;
        
        return saveProjectToFile();
    } catch (const std::exception& e) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error", "Failed to save project: " + juce::String(e.what()));
        return false;
    }
}

bool ProjectManagerUI::saveProjectAs() {
    if (!currentProject) return false;
    
    auto chooser = std::make_unique<juce::FileChooser>("Save Project As",
                                                      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                                      "*.zenith");
    
    if (chooser->browseForFileToSave(true)) {
        currentProject->path = chooser->getResult().getParentDirectory().getFullPathName();
        currentProject->name = chooser->getResult().getFileNameWithoutExtension();
        
        return saveProject();
    }
    
    return false;
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

void ProjectManagerUI::closeProject() {
    if (currentProject) {
        if (currentProject->isModified) {
            auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                                                           "Save Changes",
                                                           "Do you want to save changes to " + currentProject->name + "?");
            
            if (result) {
                saveProject();
            }
        }
        
        notifyProjectClosed(*currentProject);
        currentProject.reset();
        
        // Update UI
        updateProjectList();
        updateTrackList();
        updateEffectList();
        updateStatusBar();
    }
}

bool ProjectManagerUI::deleteProject(const juce::String& projectId) {
    // Find project
    Project* project = nullptr;
    for (auto& p : allProjects) {
        if (p.projectId == projectId) {
            project = &p;
            break;
        }
    }
    
    if (!project) return false;
    
    auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                                                   "Delete Project",
                                                   "Are you sure you want to delete " + project->name + "?\nThis cannot be undone.");
    
    if (!result) return false;
    
    try {
        // Delete project directory
        juce::File projectDir(project->path);
        if (projectDir.exists()) {
            projectDir.deleteRecursively();
        }
        
        // Remove from list
        allProjects.erase(std::remove_if(allProjects.begin(), allProjects.end(),
                                        [&projectId](const Project& p) { return p.projectId == projectId; }),
                         allProjects.end());
        
        // Remove from recent
        recentProjects.erase(std::remove_if(recentProjects.begin(), recentProjects.end(),
                                          [&projectId](const Project& p) { return p.projectId == projectId; }),
                           recentProjects.end());
        
        // Update UI
        updateProjectList();
        updateRecentProjects();
        
        notifyProjectDeleted(*project);
        return true;
    } catch (const std::exception& e) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                              "Error", "Failed to delete project: " + juce::String(e.what()));
        return false;
    }
}

void ProjectManagerUI::addTrack(const Track& track) {
    if (!currentProject) return;
    
    // Add to current project
    currentProject->tracks.push_back(track);
    currentProject->isModified = true;
    
    // Update UI
    updateTrackList();
    updateStatusBar();
    
    notifyTrackAdded(track);
}

void ProjectManagerUI::removeTrack(const juce::String& trackId) {
    if (!currentProject) return;
    
    // Remove from current project
    currentProject->tracks.erase(std::remove_if(currentProject->tracks.begin(),
                                               currentProject->tracks.end(),
                                               [&trackId](const Track& t) { return t.trackId == trackId; }),
                                currentProject->tracks.end());
    
    currentProject->isModified = true;
    
    // Update UI
    updateTrackList();
    updateStatusBar();
    
    notifyTrackRemoved(trackId);
}

void ProjectManagerUI::updateTrack(const Track& track) {
    if (!currentProject) return;
    
    // Find and update track
    for (auto& t : currentProject->tracks) {
        if (t.trackId == track.trackId) {
            t = track;
            currentProject->isModified = true;
            
            // Update UI
            updateTrackList();
            updateStatusBar();
            
            notifyTrackUpdated(track);
            break;
        }
    }
}

void ProjectManagerUI::addEffect(const Effect& effect) {
    if (!currentProject) return;
    
    // Add to current project
    currentProject->effects.push_back(effect);
    currentProject->isModified = true;
    
    // Update UI
    updateEffectList();
    updateStatusBar();
    
    notifyEffectAdded(effect);
}

void ProjectManagerUI::removeEffect(const juce::String& effectId) {
    if (!currentProject) return;
    
    // Remove from current project
    currentProject->effects.erase(std::remove_if(currentProject->effects.begin(),
                                                currentProject->effects.end(),
                                                [&effectId](const Effect& e) { return e.effectId == effectId; }),
                                 currentProject->effects.end());
    
    currentProject->isModified = true;
    
    // Update UI
    updateEffectList();
    updateStatusBar();
    
    notifyEffectRemoved(effectId);
}

void ProjectManagerUI::updateEffect(const Effect& effect) {
    if (!currentProject) return;
    
    // Find and update effect
    for (auto& e : currentProject->effects) {
        if (e.effectId == effect.effectId) {
            e = effect;
            currentProject->isModified = true;
            
            // Update UI
            updateEffectList();
            updateStatusBar();
            
            notifyEffectUpdated(effect);
            break;
        }
    }
}

void ProjectManagerUI::startAutoSave() {
    autoSaveEnabled = true;
    autoSaveInterval = 300; // 5 minutes
}

void ProjectManagerUI::stopAutoSave() {
    autoSaveEnabled = false;
}

bool ProjectManagerUI::isAutoSaveEnabled() const {
    return autoSaveEnabled;
}

void ProjectManagerUI::setAutoSaveInterval(int seconds) {
    autoSaveInterval = seconds;
}

void ProjectManagerUI::loadProjectTemplate(const ProjectTemplate& template_) {
    if (!currentProject) return;
    
    applyTemplate(template_);
    currentProject->isModified = true;
    
    updateTrackList();
    updateEffectList();
    updateStatusBar();
}

void ProjectManagerUI::saveAsTemplate(const juce::String& name, const juce::String& description) {
    if (!currentProject) return;
    
    ProjectTemplate template_;
    template_.templateId = juce::Uuid().toString();
    template_.name = name;
    template_.description = description;
    template_.tracks = currentProject->tracks;
    template_.effects = currentProject->effects;
    template_.created = juce::Time::getCurrentTime();
    
    templateManager->saveTemplate(template_);
    updateTemplatesList();
    
    notifyTemplateSaved(template_);
}

void ProjectManagerUI::searchProjects(const juce::String& query) {
    searchQuery = query;
    updateProjectList();
}

void ProjectManagerUI::filterProjects(const juce::String& filter) {
    currentFilter = filter;
    updateProjectList();
}

void ProjectManagerUI::sortProjects(SortMethod method) {
    currentSortMethod = method;
    updateProjectList();
}

std::vector<Project> ProjectManagerUI::getProjects() const {
    return allProjects;
}

Project* ProjectManagerUI::getCurrentProject() {
    return currentProject.get();
}

std::vector<Project> ProjectManagerUI::getRecentProjects() const {
    return recentProjects;
}

std::vector<ProjectTemplate> ProjectManagerUI::getTemplates() const {
    return templateManager->getAllTemplates();
}

void ProjectManagerUI::timerCallback() {
    // Auto-save check
    if (autoSaveEnabled && currentProject && currentProject->isModified) {
        auto now = juce::Time::getCurrentTime();
        auto elapsed = (now - lastAutoSave).inSeconds();
        
        if (elapsed >= autoSaveInterval) {
            saveProject();
            lastAutoSave = now;
        }
    }
}

void ProjectManagerUI::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void ProjectManagerUI::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void ProjectManagerUI::buttonClicked(juce::Button* button) {
    if (button == newProjectButton.get()) {
        newProject();
    } else if (button == openProjectButton.get()) {
        openProject();
    } else if (button == saveProjectButton.get()) {
        saveProject();
    } else if (button == saveAsProjectButton.get()) {
        saveProjectAs();
    } else if (button == closeProjectButton.get()) {
        closeProject();
    } else if (button == addTrackButton.get()) {
        Track track;
        track.trackId = juce::Uuid().toString();
        track.name = "New Track";
        track.type = "audio";
        addTrack(track);
    } else if (button == removeTrackButton.get()) {
        int selectedRow = trackListBox->getSelectedRow();
        if (selectedRow >= 0 && currentProject) {
            removeTrack(currentProject->tracks[selectedRow].trackId);
        }
    } else if (button == addEffectButton.get()) {
        Effect effect;
        effect.effectId = juce::Uuid().toString();
        effect.name = "New Effect";
        effect.type = "unknown";
        addEffect(effect);
    } else if (button == removeEffectButton.get()) {
        int selectedRow = effectListBox->getSelectedRow();
        if (selectedRow >= 0 && currentProject) {
            removeEffect(currentProject->effects[selectedRow].effectId);
        }
    }
}

void ProjectManagerUI::textEditorTextChanged(juce::TextEditor& editor) {
    if (&editor == searchBox.get()) {
        searchProjects(editor.getText());
    }
}

void ProjectManagerUI::comboBoxChanged(juce::ComboBox* comboBox) {
    if (comboBox == filterComboBox.get()) {
        filterProjects(comboBox->getText());
    } else if (comboBox == sortByComboBox.get()) {
        SortMethod method = SortMethod::ByName;
        if (comboBox->getSelectedId() == 2) method = SortMethod::ByDate;
        else if (comboBox->getSelectedId() == 3) method = SortMethod::BySize;
        else if (comboBox->getSelectedId() == 4) method = SortMethod::ByType;
        
        sortProjects(method);
    }
}

void ProjectManagerUI::createProjectControls() {
    // Project buttons
    newProjectButton = std::make_unique<juce::TextButton>("New");
    newProjectButton->addListener(this);
    addAndMakeVisible(*newProjectButton);
    
    openProjectButton = std::make_unique<juce::TextButton>("Open");
    openProjectButton->addListener(this);
    addAndMakeVisible(*openProjectButton);
    
    saveProjectButton = std::make_unique<juce::TextButton>("Save");
    saveProjectButton->addListener(this);
    addAndMakeVisible(*saveProjectButton);
    
    saveAsProjectButton = std::make_unique<juce::TextButton>("Save As");
    saveAsProjectButton->addListener(this);
    addAndMakeVisible(*saveAsProjectButton);
    
    closeProjectButton = std::make_unique<juce::TextButton>("Close");
    closeProjectButton->addListener(this);
    addAndMakeVisible(*closeProjectButton);
}

void ProjectManagerUI::createTrackControls() {
    // Track list
    trackListModel = std::make_unique<TrackListModel>(currentProject ? currentProject->tracks : std::vector<Track>());
    trackListBox = std::make_unique<juce::ListBox>("Tracks");
    trackListBox->setModel(trackListModel.get());
    trackListBox->setRowHeight(25);
    addAndMakeVisible(*trackListBox);
    
    // Track buttons
    addTrackButton = std::make_unique<juce::TextButton>("Add Track");
    addTrackButton->addListener(this);
    addAndMakeVisible(*addTrackButton);
    
    removeTrackButton = std::make_unique<juce::TextButton>("Remove");
    removeTrackButton->addListener(this);
    addAndMakeVisible(*removeTrackButton);
}

void ProjectManagerUI::createEffectControls() {
    // Effect list
    effectListModel = std::make_unique<EffectListModel>(currentProject ? currentProject->effects : std::vector<Effect>());
    effectListBox = std::make_unique<juce::ListBox>("Effects");
    effectListBox->setModel(effectListModel.get());
    effectListBox->setRowHeight(25);
    addAndMakeVisible(*effectListBox);
    
    // Effect buttons
    addEffectButton = std::make_unique<juce::TextButton>("Add Effect");
    addEffectButton->addListener(this);
    addAndMakeVisible(*addEffectButton);
    
    removeEffectButton = std::make_unique<juce::TextButton>("Remove");
    removeEffectButton->addListener(this);
    addAndMakeVisible(*removeEffectButton);
}

void ProjectManagerUI::createSessionControls() {
    // Session controls would be implemented here
}

void ProjectManagerUI::createRecentProjectsList() {
    recentProjectsListModel = std::make_unique<RecentProjectsListModel>(recentProjects);
    recentProjectsListBox = std::make_unique<juce::ListBox>("Recent Projects");
    recentProjectsListBox->setModel(recentProjectsListModel.get());
    recentProjectsListBox->setRowHeight(25);
    recentProjectsListBox->onDoubleClick = [this](int row) {
        if (row >= 0 && row < recentProjects.size()) {
            loadProject(juce::File(recentProjects[row].path).getChildFile(recentProjects[row].name + ".zenith"));
        }
    };
    addAndMakeVisible(*recentProjectsListBox);
}

void ProjectManagerUI::createTemplatesList() {
    templatesListModel = std::make_unique<TemplatesListModel>(templateManager->getAllTemplates());
    templatesListBox = std::make_unique<juce::ListBox>("Templates");
    templatesListBox->setModel(templatesListModel.get());
    templatesListBox->setRowHeight(25);
    templatesListBox->onDoubleClick = [this](int row) {
        auto templates = templateManager->getAllTemplates();
        if (row >= 0 && row < templates.size()) {
            loadProjectTemplate(templates[row]);
        }
    };
    addAndMakeVisible(*templatesListBox);
}

void ProjectManagerUI::createSearchAndFilter() {
    // Search box
    searchBox = std::make_unique<juce::TextEditor>("Search");
    searchBox->addListener(this);
    addAndMakeVisible(*searchBox);
    
    // Filter combo
    filterComboBox = std::make_unique<juce::ComboBox>("Filter");
    filterComboBox->addItem("All", 1);
    filterComboBox->addItem("Audio", 2);
    filterComboBox->addItem("MIDI", 3);
    filterComboBox->addItem("Recent", 4);
    filterComboBox->setSelectedId(1);
    filterComboBox->addListener(this);
    addAndMakeVisible(*filterComboBox);
    
    // Sort combo
    sortByComboBox = std::make_unique<juce::ComboBox>("Sort By");
    sortByComboBox->addItem("Name", 1);
    sortByComboBox->addItem("Date", 2);
    sortByComboBox->addItem("Size", 3);
    sortByComboBox->addItem("Type", 4);
    sortByComboBox->setSelectedId(1);
    sortByComboBox->addListener(this);
    addAndMakeVisible(*sortByComboBox);
}

void ProjectManagerUI::createFileBrowser() {
    // File browser would be implemented here
}

void ProjectManagerUI::createStatusBar() {
    statusLabel = std::make_unique<juce::Label>("Status", "Ready");
    statusLabel->setFont(12.0f);
    statusLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*statusLabel);
    
    progressBar = std::make_unique<juce::ProgressBar>();
    addAndMakeVisible(*progressBar);
}

void ProjectManagerUI::updateProjectList() {
    // Filter and sort projects
    std::vector<Project> filteredProjects;
    
    for (const auto& project : allProjects) {
        // Apply search filter
        if (!searchQuery.isEmpty() && 
            !project.name.containsIgnoreCase(searchQuery)) {
            continue;
        }
        
        // Apply type filter
        if (!currentFilter.isEmpty() && currentFilter != "All") {
            // Add filter logic here
        }
        
        filteredProjects.push_back(project);
    }
    
    // Sort projects
    switch (currentSortMethod) {
        case SortMethod::ByName:
            std::sort(filteredProjects.begin(), filteredProjects.end(),
                     [](const Project& a, const Project& b) {
                         return a.name.compareIgnoreCase(b.name) < 0;
                     });
            break;
        case SortMethod::ByDate:
            std::sort(filteredProjects.begin(), filteredProjects.end(),
                     [](const Project& a, const Project& b) {
                         return a.modified > b.modified;
                     });
            break;
        case SortMethod::BySize:
            // Sort by track count
            std::sort(filteredProjects.begin(), filteredProjects.end(),
                     [](const Project& a, const Project& b) {
                         return a.tracks.size() > b.tracks.size();
                     });
            break;
        case SortMethod::ByType:
            // Sort by project type
            break;
    }
    
    // Update list model
    if (projectListModel) {
        projectListModel->setProjects(filteredProjects);
        projectListBox->updateContent();
    }
}

void ProjectManagerUI::updateTrackList() {
    if (trackListModel && currentProject) {
        trackListModel->setTracks(currentProject->tracks);
        trackListBox->updateContent();
    }
}

void ProjectManagerUI::updateEffectList() {
    if (effectListModel && currentProject) {
        effectListModel->setEffects(currentProject->effects);
        effectListBox->updateContent();
    }
}

void ProjectManagerUI::updateRecentProjects() {
    if (recentProjectsListModel) {
        recentProjectsListModel->setProjects(recentProjects);
        recentProjectsListBox->updateContent();
    }
}

void ProjectManagerUI::updateTemplatesList() {
    if (templatesListModel) {
        templatesListModel->setTemplates(templateManager->getAllTemplates());
        templatesListBox->updateContent();
    }
}

void ProjectManagerUI::updateStatusBar() {
    if (statusLabel) {
        if (currentProject) {
            juce::String text = currentProject->name;
            if (currentProject->isModified) {
                text += " *";
            }
            text += " - " + juce::String(currentProject->tracks.size()) + " tracks, ";
            text += juce::String(currentProject->effects.size()) + " effects";
            statusLabel->setText(text, juce::dontSendNotification);
        } else {
            statusLabel->setText("No project open", juce::dontSendNotification);
        }
    }
}

void ProjectManagerUI::applyTemplate(const ProjectTemplate& template_) {
    if (!currentProject) return;
    
    // Add template tracks
    for (const auto& track : template_.tracks) {
        Track newTrack = track;
        newTrack.trackId = juce::Uuid().toString(); // Generate new ID
        currentProject->tracks.push_back(newTrack);
    }
    
    // Add template effects
    for (const auto& effect : template_.effects) {
        Effect newEffect = effect;
        newEffect.effectId = juce::Uuid().toString(); // Generate new ID
        currentProject->effects.push_back(newEffect);
    }
}

void ProjectManagerUI::addToRecentProjects(const Project& project) {
    // Remove if already exists
    recentProjects.erase(std::remove_if(recentProjects.begin(), recentProjects.end(),
                                      [&project](const Project& p) { return p.projectId == project.projectId; }),
                       recentProjects.end());
    
    // Add to front
    recentProjects.insert(recentProjects.begin(), project);
    
    // Keep only last 10
    if (recentProjects.size() > 10) {
        recentProjects.resize(10);
    }
    
    // Save to file
    saveRecentProjects();
}

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

void ProjectManagerUI::notifyProjectCreated(const Project& project) {
    for (auto* listener : listeners) {
        listener->projectCreated(project);
    }
}

void ProjectManagerUI::notifyProjectOpened(const Project& project) {
    for (auto* listener : listeners) {
        listener->projectOpened(project);
    }
}

void ProjectManagerUI::notifyProjectClosed(const Project& project) {
    for (auto* listener : listeners) {
        listener->projectClosed(project);
    }
}

void ProjectManagerUI::notifyProjectSaved(const Project& project) {
    for (auto* listener : listeners) {
        listener->projectSaved(project);
    }
}

void ProjectManagerUI::notifyProjectDeleted(const Project& project) {
    for (auto* listener : listeners) {
        listener->projectDeleted(project);
    }
}

void ProjectManagerUI::notifyTrackAdded(const Track& track) {
    for (auto* listener : listeners) {
        listener->trackAdded(track);
    }
}

void ProjectManagerUI::notifyTrackRemoved(const juce::String& trackId) {
    for (auto* listener : listeners) {
        listener->trackRemoved(trackId);
    }
}

void ProjectManagerUI::notifyTrackUpdated(const Track& track) {
    for (auto* listener : listeners) {
        listener->trackUpdated(track);
    }
}

void ProjectManagerUI::notifyEffectAdded(const Effect& effect) {
    for (auto* listener : listeners) {
        listener->effectAdded(effect);
    }
}

void ProjectManagerUI::notifyEffectRemoved(const juce::String& effectId) {
    for (auto* listener : listeners) {
        listener->effectRemoved(effectId);
    }
}

void ProjectManagerUI::notifyEffectUpdated(const Effect& effect) {
    for (auto* listener : listeners) {
        listener->effectUpdated(effect);
    }
}

void ProjectManagerUI::notifyTemplateSaved(const ProjectTemplate& template_) {
    for (auto* listener : listeners) {
        listener->templateSaved(template_);
    }
}

// TrackListModel Implementation
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

// EffectListModel Implementation
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

// RecentProjectsListModel Implementation
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

// TemplatesListModel Implementation
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

// ProjectTemplateManager Implementation
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
