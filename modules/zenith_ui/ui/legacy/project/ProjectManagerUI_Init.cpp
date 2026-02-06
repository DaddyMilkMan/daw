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

namespace ui {

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

} // namespace ui
} // namespace zenith
