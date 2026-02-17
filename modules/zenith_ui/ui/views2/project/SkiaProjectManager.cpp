/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "SkiaProjectManager.h"
#include "../../design-system/ZenithTheme.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith::ui {

SkiaProjectManager::SkiaProjectManager(Engine& engine, ProjectState& projectState)
    : engine_(engine), projectState_(projectState) {

    // Load recent projects from engine
    // TODO: Implement recent project loading
}

SkiaProjectManager::~SkiaProjectManager() {
}

void SkiaProjectManager::paint(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(SK_ColorBLACK);
    canvas->drawPaint(bgPaint);

    // Draw header
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont font;
    font.setSize(24.0f);
    canvas->drawString("Project Manager", 20.0f, 40.0f, font, textPaint);
}

void SkiaProjectManager::resized() {
    // Layout child components
}

bool SkiaProjectManager::keyPressed(const juce::KeyPress& key) {
    if (key.getKeyCode() == juce::KeyPress::escapeKey) {
        // Close project manager
        return true;
    }
    return false;
}

bool SkiaProjectManager::loadProject(const juce::File& file) {
    if (!file.existsAsFile()) {
        return false;
    }

    // Try loading as JSON
    juce::var json;
    if (auto jsonStream = file.createInputStream()) {
        juce::JSON::parse(*jsonStream);
    }

    // Try loading as XML
    juce::XmlDocument doc(file);
    if (auto* xml = doc.getDocumentElement().get()) {
        projectState_.fromValueTree(juce::ValueTree::fromXml(*xml));
    }

    hasUnsavedChanges_ = false;
    repaint();
    return true;
}

bool SkiaProjectManager::saveProject() {
    auto& currentProject = projectState_.getProject();

    if (currentProject.filePath.isEmpty()) {
        return false;
    }

    juce::File file(currentProject.filePath);

    // Save as JSON
    juce::var json = projectState_.toVar();
    if (auto outputStream = file.createOutputStream()) {
        outputStream->writeString(juce::JSON::toString(json, true));
    }

    // Alternatively save as XML
    juce::ValueTree vt = projectState_.toValueTree();
    if (auto xml = vt.toXmlString(juce::XmlElement::TextFormat().singleLine())) {
        file.replaceWithText(xml);
    }

    hasUnsavedChanges_ = false;
    repaint();
    return true;
}

void SkiaProjectManager::createNewProject() {
    projectState_.clear();
    hasUnsavedChanges_ = false;
    repaint();
}

} // namespace zenith::ui
