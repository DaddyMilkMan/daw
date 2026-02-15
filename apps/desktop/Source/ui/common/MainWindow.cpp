/*
    MainWindow.cpp - Main application window for Zenith DAW

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

#include "MainWindow.h"
#include "zenith_commands/commands/CommandAPI.h"
#include "zenith_core/engine/Engine.h"
#include "zenith_core/engine/ProjectState.h"

namespace zenith {

namespace {

class BasicMainComponent final : public juce::Component {
public:
    BasicMainComponent(Engine& engine, ProjectState& state)
        : engine_(engine), state_(state) {
        addAndMakeVisible(title_);
        title_.setText("Zenith DAW (alpha)", juce::dontSendNotification);
        title_.setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(info_);
        info_.setText("This is a minimal UI shell. Engine is running.", juce::dontSendNotification);
        info_.setJustificationType(juce::Justification::centredLeft);

        addAndMakeVisible(play_);
        play_.setButtonText("Play");
        play_.setTooltip("Start Playback (Space)");
        play_.onClick = [this] { engine_.play(); };

        addAndMakeVisible(stop_);
        stop_.setButtonText("Stop");
        stop_.setTooltip("Stop Playback (Space)");
        stop_.onClick = [this] { engine_.stop(); };

        addAndMakeVisible(record_);
        record_.setButtonText("Record");
        record_.setTooltip("Record (R)");
        record_.onClick = [this] { engine_.toggleRecording(); };

        addAndMakeVisible(panic_);
        panic_.setButtonText("Panic");
        panic_.setTooltip("Stop All Sound");
        panic_.onClick = [this] { engine_.panic(); };

        addAndMakeVisible(dirty_);
        dirty_.setJustificationType(juce::Justification::centredLeft);
        updateDirtyLabel();

        startTimerHz(4);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds(), 1);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(16);

        title_.setBounds(r.removeFromTop(28));
        r.removeFromTop(8);
        info_.setBounds(r.removeFromTop(22));
        r.removeFromTop(12);

        auto buttons = r.removeFromTop(32);
        play_.setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        stop_.setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        record_.setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        panic_.setBounds(buttons.removeFromLeft(100));

        r.removeFromTop(12);
        dirty_.setBounds(r.removeFromTop(22));
    }

private:
    struct DirtyTimer final : public juce::Timer {
        DirtyTimer(BasicMainComponent& owner) : owner_(owner) {}
        void timerCallback() override { owner_.updateDirtyLabel(); }
        BasicMainComponent& owner_;
    };

    void startTimerHz(int hz) { timer_ = std::make_unique<DirtyTimer>(*this); timer_->startTimerHz(hz); }

    void updateDirtyLabel() {
        dirty_.setText(state_.hasUnsavedChanges() ? "Project: UNSAVED CHANGES" : "Project: saved",
                       juce::dontSendNotification);
        dirty_.setColour(juce::Label::textColourId,
                         state_.hasUnsavedChanges() ? juce::Colours::orange : juce::Colours::lightgreen);
    }

    Engine& engine_;
    ProjectState& state_;

    juce::Label title_;
    juce::Label info_;
    juce::TextButton play_;
    juce::TextButton stop_;
    juce::TextButton record_;
    juce::TextButton panic_;
    juce::Label dirty_;

    std::unique_ptr<DirtyTimer> timer_;
};

} // namespace

MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name,
                     juce::Colours::lightgrey,
                     DocumentWindow::allButtons,
                     true) // Allow close button
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setBounds(100, 100, 1200, 800);

    engine_ = std::make_unique<zenith::Engine>();
    projectState_ = std::make_unique<zenith::ProjectState>();
    commandAPI_ = std::make_unique<zenith::CommandAPI>(*projectState_, *engine_);

    engine_->setProjectState(projectState_.get());
    if (!engine_->initialize()) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Engine Initialization Failed",
            "Audio engine failed to initialize. The UI will still open, but audio may not work.",
            "OK");
    }

    setContentOwned(new BasicMainComponent(*engine_, *projectState_), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

MainWindow::~MainWindow() {
    setContentOwned(nullptr, true);

    if (engine_) {
        engine_->shutdown();
    }
    commandAPI_.reset();
    projectState_.reset();
    engine_.reset();
}

void MainWindow::closeButtonPressed() {
    // Handle close button press
    // This will trigger systemRequestedQuit through the application
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

zenith::ProjectState* MainWindow::getProjectState() {
    return projectState_.get();
}

void MainWindow::checkUnsavedAndQuit() {
    if (!projectState_ || !projectState_->hasUnsavedChanges()) {
        juce::JUCEApplication::getInstance()->quit();
        return;
    }

    const int result = juce::NativeMessageBox::showYesNoCancelBox(
        juce::AlertWindow::WarningIcon,
        "Unsaved Changes",
        "Save changes before quitting?",
        this,
        nullptr);

    if (result == 0) { // Cancel
        return;
    }

    if (result == 1) { // Yes
        juce::File target = projectState_->getProjectFile();
        if (!target.existsAsFile()) {
            // JUCE disables synchronous modal file choosers when
            // JUCE_MODAL_LOOPS_PERMITTED=0, so use async mode.
            auto chooser = std::make_shared<juce::FileChooser>(
                "Save Project",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.zenith");

            auto flags = juce::FileBrowserComponent::saveMode
                       | juce::FileBrowserComponent::canSelectFiles;

            juce::Component::SafePointer<MainWindow> safeThis(this);
            chooser->launchAsync(flags, [safeThis, chooser](const juce::FileChooser& fc) {
                if (!safeThis) return;
                if (!safeThis->projectState_) return;

                auto targetFile = fc.getResult();
                if (targetFile == juce::File()) {
                    return; // user canceled
                }

                safeThis->projectState_->setProjectFile(targetFile);
                if (!safeThis->projectState_->saveToFile(targetFile)) {
                    juce::NativeMessageBox::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Save Failed",
                        "Could not save the project.",
                        safeThis.getComponent());
                    return;
                }

                juce::JUCEApplication::getInstance()->quit();
            });

            return; // quit happens in async callback on successful save
        }

        if (!projectState_->saveToFile(target)) {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Could not save the project.",
                this);
            return;
        }
    }

    // No (discard) or successful save -> quit.
    juce::JUCEApplication::getInstance()->quit();
}

} // namespace zenith
