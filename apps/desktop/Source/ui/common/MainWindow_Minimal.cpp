/*
    MainWindow_Minimal.cpp - Main application window for Zenith DAW

    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "MainWindow.h"
#include "zenith_commands/commands/CommandAPI.h"
#include "zenith_core/engine/Engine.h"
#include "zenith_core/engine/ProjectState.h"
#include "zenith_ui/ui/controls/SkiaAlertWindow.h"
#include "zenith_ui/ui/controls/SkiaButton.h"
#include "zenith_ui/ui/controls/SkiaFileChooser.h"
#include "zenith_ui/ui/controls/SkiaLabel.h"
#include "zenith_ui/ui/framework/SkiaComponent.h"
#include <core/SkColor.h>

namespace zenith {

namespace {

class BasicMainComponent final : public SkiaComponent, private juce::Timer {
public:
    BasicMainComponent(Engine& engine, ProjectState& state)
        : engine_(engine), state_(state) {
        title_ = std::make_unique<SkiaLabel>("title", "Zenith DAW (minimal)");
        title_->setFont(18.0f);
        title_->setJustification(SkiaLabel::Justification::Left);
        addAndMakeVisible(title_.get());

        info_ = std::make_unique<SkiaLabel>("info", "Skia minimal shell. Engine is running.");
        info_->setFont(13.0f);
        info_->setJustification(SkiaLabel::Justification::Left);
        addAndMakeVisible(info_.get());

        play_ = std::make_unique<SkiaButton>("Play");
        play_->setStyle(SkiaButton::Style::Primary);
        play_->onClick = [this] { engine_.play(); };
        addAndMakeVisible(play_.get());

        stop_ = std::make_unique<SkiaButton>("Stop");
        stop_->setStyle(SkiaButton::Style::Secondary);
        stop_->onClick = [this] { engine_.stop(); };
        addAndMakeVisible(stop_.get());

        record_ = std::make_unique<SkiaButton>("Record");
        record_->setStyle(SkiaButton::Style::Warning);
        record_->onClick = [this] { engine_.toggleRecording(); };
        addAndMakeVisible(record_.get());

        panic_ = std::make_unique<SkiaButton>("Panic");
        panic_->setStyle(SkiaButton::Style::Danger);
        panic_->onClick = [this] { engine_.panic(); };
        addAndMakeVisible(panic_.get());

        dirty_ = std::make_unique<SkiaLabel>();
        dirty_->setFont(12.0f);
        dirty_->setJustification(SkiaLabel::Justification::Left);
        addAndMakeVisible(dirty_.get());

        updateDirtyLabel();
        startTimerHz(4);
    }

    ~BasicMainComponent() override { stopTimer(); }

    void drawSkia(SkCanvas* canvas) override {
        SkPaint bg;
        bg.setColor(SkColorSetRGB(10, 12, 16));
        canvas->drawRect(SkRect::MakeWH((float) getWidth(), (float) getHeight()), bg);

        SkPaint border;
        border.setColor(SkColorSetRGB(50, 56, 64));
        border.setStyle(SkPaint::kStroke_Style);
        border.setStrokeWidth(1.0f);
        border.setAntiAlias(true);
        canvas->drawRect(SkRect::MakeWH((float) getWidth(), (float) getHeight()), border);

        drawChildren(canvas);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(16);

        title_->setBounds(r.removeFromTop(28));
        r.removeFromTop(8);
        info_->setBounds(r.removeFromTop(22));
        r.removeFromTop(12);

        auto buttons = r.removeFromTop(36);
        play_->setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        stop_->setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        record_->setBounds(buttons.removeFromLeft(100));
        buttons.removeFromLeft(8);
        panic_->setBounds(buttons.removeFromLeft(100));

        r.removeFromTop(12);
        dirty_->setBounds(r.removeFromTop(22));
    }

private:
    void timerCallback() override { updateDirtyLabel(); }

    void updateDirtyLabel() {
        const bool dirty = state_.hasUnsavedChanges();
        dirty_->setText(dirty ? "Project: UNSAVED CHANGES" : "Project: saved", juce::dontSendNotification);
        dirty_->setTextColour(dirty ? SkColorSetRGB(255, 180, 0) : SkColorSetRGB(80, 220, 120));
    }

    Engine& engine_;
    ProjectState& state_;

    std::unique_ptr<SkiaLabel> title_;
    std::unique_ptr<SkiaLabel> info_;
    std::unique_ptr<SkiaButton> play_;
    std::unique_ptr<SkiaButton> stop_;
    std::unique_ptr<SkiaButton> record_;
    std::unique_ptr<SkiaButton> panic_;
    std::unique_ptr<SkiaLabel> dirty_;
};

} // namespace

MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name, juce::Colours::lightgrey, DocumentWindow::allButtons, true) {
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setBounds(100, 100, 1200, 800);

    engine_ = std::make_unique<zenith::Engine>();
    projectState_ = std::make_unique<zenith::ProjectState>();
    commandAPI_ = std::make_unique<zenith::CommandAPI>(*projectState_, *engine_);

    engine_->setProjectState(projectState_.get());
    const bool engineReady = engine_->initialize();

    setContentOwned(new BasicMainComponent(*engine_, *projectState_), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);

    if (!engineReady) {
        showAlert("Engine Initialization Failed",
                  "Audio engine failed to initialize. The UI will still open, but audio may not work.");
    }
}

MainWindow::~MainWindow() {
    setContentOwned(nullptr, true);

    if (engine_) {
        engine_->shutdown();
    }

    activeChooser_.reset();
    activeAlert_.reset();
    commandAPI_.reset();
    projectState_.reset();
    engine_.reset();
}

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::resized() {
    DocumentWindow::resized();
    if (auto* content = getContentComponent()) {
        if (activeAlert_) {
            activeAlert_->setBounds(content->getLocalBounds().reduced(240, 170));
        }
        if (activeChooser_) {
            activeChooser_->setBounds(content->getLocalBounds().reduced(170, 110));
        }
    }
}

zenith::ProjectState* MainWindow::getProjectState() {
    return projectState_.get();
}

void MainWindow::checkUnsavedAndQuit() {
    if (!projectState_ || !projectState_->hasUnsavedChanges()) {
        juce::JUCEApplication::getInstance()->quit();
        return;
    }

    promptSaveAndQuit();
}

void MainWindow::showAlert(const juce::String& title, const juce::String& message) {
    activeAlert_ = std::make_unique<SkiaAlertWindow>(title, message, SkiaAlertWindow::IconType::WarningIcon);
    activeAlert_->addButton("OK", SkiaAlertWindow::Result::Button1, SkiaButton::Style::Primary);

    juce::Component::SafePointer<MainWindow> safeThis(this);
    activeAlert_->showAsync([safeThis](SkiaAlertWindow::Result) {
        if (safeThis) {
            safeThis->activeAlert_.reset();
        }
    });

    if (auto* content = getContentComponent()) {
        content->addAndMakeVisible(activeAlert_.get());
        activeAlert_->setBounds(content->getLocalBounds().reduced(240, 170));
        activeAlert_->toFront(true);
    }
}

void MainWindow::promptSaveAndQuit() {
    activeAlert_ = std::make_unique<SkiaAlertWindow>(
        "Unsaved Changes",
        "Save changes before quitting?",
        SkiaAlertWindow::IconType::WarningIcon);
    activeAlert_->addButton("Save", SkiaAlertWindow::Result::Button1, SkiaButton::Style::Primary);
    activeAlert_->addButton("Discard", SkiaAlertWindow::Result::Button2, SkiaButton::Style::Danger);
    activeAlert_->addButton("Cancel", SkiaAlertWindow::Result::Cancelled, SkiaButton::Style::Secondary);

    juce::Component::SafePointer<MainWindow> safeThis(this);
    activeAlert_->showAsync([safeThis](SkiaAlertWindow::Result result) {
        if (!safeThis) {
            return;
        }

        safeThis->activeAlert_.reset();

        if (result == SkiaAlertWindow::Result::Button1) {
            auto target = safeThis->projectState_->getProjectFile();
            if (!target.existsAsFile()) {
                safeThis->launchSaveChooserAndQuit();
            } else {
                safeThis->saveToFileAndQuit(target);
            }
            return;
        }

        if (result == SkiaAlertWindow::Result::Button2) {
            juce::JUCEApplication::getInstance()->quit();
        }
    });

    if (auto* content = getContentComponent()) {
        content->addAndMakeVisible(activeAlert_.get());
        activeAlert_->setBounds(content->getLocalBounds().reduced(240, 170));
        activeAlert_->toFront(true);
    }
}

void MainWindow::saveToFileAndQuit(const juce::File& target) {
    if (!projectState_ || !projectState_->saveToFile(target)) {
        showAlert("Save Failed", "Could not save the project.");
        return;
    }

    juce::JUCEApplication::getInstance()->quit();
}

void MainWindow::launchSaveChooserAndQuit() {
    activeChooser_ = std::make_unique<SkiaFileChooser>(
        "Save Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zenith",
        SkiaFileChooser::Mode::SaveFile);

    juce::Component::SafePointer<MainWindow> safeThis(this);
    activeChooser_->showAsync([safeThis](SkiaFileChooser::Result result, const juce::File& selected) {
        if (!safeThis) {
            return;
        }

        if (result != SkiaFileChooser::Result::Approved || selected == juce::File()) {
            safeThis->activeChooser_.reset();
            return;
        }

        juce::File target = selected;
        if (!target.hasFileExtension(".zenith")) {
            target = target.withFileExtension(".zenith");
        }

        safeThis->projectState_->setProjectFile(target);
        safeThis->activeChooser_.reset();
        safeThis->saveToFileAndQuit(target);
    });

    if (auto* content = getContentComponent()) {
        content->addAndMakeVisible(activeChooser_.get());
        activeChooser_->setBounds(content->getLocalBounds().reduced(170, 110));
        activeChooser_->toFront(true);
    }
}

} // namespace zenith
