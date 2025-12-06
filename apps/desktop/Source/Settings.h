/*
  ==============================================================================

    Settings.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Global application settings.
    Acts as a persistent store and data model for preferences.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "rendering/SkiaRenderer.h"

namespace zenith {

class Settings {
public:
    static Settings& getInstance() {
        static Settings instance;
        return instance;
    }

    //==============================================================================
    // Persistence
    //==============================================================================
    void load() {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = ".settings";
        options.folderName = "ZenithAudio";
        options.osxLibrarySubFolder = "Application Support";
        
        juce::ApplicationProperties props;
        props.setStorageParameters(options);
        
        if (auto* userSettings = props.getUserSettings()) {
            renderBackend_ = (SkiaRenderer::Backend)userSettings->getIntValue("renderBackend", (int)SkiaRenderer::Backend::Auto);
            targetFPS_ = userSettings->getIntValue("targetFPS", 60);
            globalScale_ = (float)userSettings->getDoubleValue("globalScale", 1.0);
            glowIntensity_ = (float)userSettings->getDoubleValue("glowIntensity", 1.0);
        }
    }

    void save() {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = ".settings";
        options.folderName = "ZenithAudio";
        options.osxLibrarySubFolder = "Application Support";
        
        juce::ApplicationProperties props;
        props.setStorageParameters(options);
        
        if (auto* userSettings = props.getUserSettings()) {
            userSettings->setValue("renderBackend", (int)renderBackend_);
            userSettings->setValue("targetFPS", targetFPS_);
            userSettings->setValue("globalScale", globalScale_);
            userSettings->setValue("glowIntensity", glowIntensity_);
            userSettings->saveIfNeeded();
        }
    }

    //==============================================================================
    // Display Settings
    //==============================================================================
    void setRenderBackend(SkiaRenderer::Backend backend) {
        if (renderBackend_ != backend) {
            renderBackend_ = backend;
            save();
            sendChangeMessage();
        }
    }

    SkiaRenderer::Backend getRenderBackend() const { return renderBackend_; }

    void setTargetFPS(int fps) {
        if (targetFPS_ != fps) {
            targetFPS_ = fps;
            save();
            sendChangeMessage();
        }
    }
    int getTargetFPS() const { return targetFPS_; }

    void setGlobalScale(float scale) {
        if (globalScale_ != scale) {
            globalScale_ = scale;
            save();
            sendChangeMessage();
        }
    }
    float getGlobalScale() const { return globalScale_; }

    void setGlowIntensity(float intensity) {
        if (glowIntensity_ != intensity) {
            glowIntensity_ = intensity;
            save();
            sendChangeMessage();
        }
    }
    float getGlowIntensity() const { return glowIntensity_; }

    //==============================================================================
    // Plugin Settings
    //==============================================================================
    // ...

    //==============================================================================
    // Listeners
    //==============================================================================
    void addChangeListener(juce::ChangeListener* listener) { broadcaster_.addChangeListener(listener); }
    void removeChangeListener(juce::ChangeListener* listener) { broadcaster_.removeChangeListener(listener); }
    
private:
    Settings() = default;
    
    void sendChangeMessage() {
        broadcaster_.sendChangeMessage();
    }

    // Data
    SkiaRenderer::Backend renderBackend_ = SkiaRenderer::Backend::Auto;
    int targetFPS_ = 60;
    float globalScale_ = 1.0f;
    float glowIntensity_ = 1.0f;

    juce::ChangeBroadcaster broadcaster_;
};

} // namespace zenith