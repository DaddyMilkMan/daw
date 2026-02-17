/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
class SkiaVerticalLayout;
} // namespace layout
} // namespace zenith

namespace zenith {
namespace config {

// ============================================================================
// Configuration Keys
// ============================================================================

namespace keys {
// UI State
const juce::String WINDOW_X = "window.x";
const juce::String WINDOW_Y = "window.y";
const juce::String WINDOW_WIDTH = "window.width";
const juce::String WINDOW_HEIGHT = "window.height";
const juce::String WINDOW_MAXIMIZED = "window.maximized";

// Theme
const juce::String THEME_NAME = "theme.name";
const juce::String THEME_ACCENT_COLOR = "theme.accentColor";
const juce::String THEME_BACKGROUND_COLOR = "theme.backgroundColor";
const juce::String THEME_OLED_MODE = "theme.oledMode";
const juce::String THEME_GLOW_INTENSITY = "theme.glowIntensity";
const juce::String THEME_UI_SCALE = "theme.uiScale";

// Layout
const juce::String LAYOUT_BROWSER_VISIBLE = "layout.browser.visible";
const juce::String LAYOUT_BROWSER_WIDTH = "layout.browser.width";
const juce::String LAYOUT_RIGHT_PANEL_VISIBLE = "layout.rightPanel.visible";
const juce::String LAYOUT_RIGHT_PANEL_WIDTH = "layout.rightPanel.width";
const juce::String LAYOUT_BOTTOM_PANEL_VISIBLE = "layout.bottomPanel.visible";
const juce::String LAYOUT_BOTTOM_PANEL_HEIGHT = "layout.bottomPanel.height";
const juce::String LAYOUT_MIXER_VISIBLE = "layout.mixer.visible";
const juce::String LAYOUT_MIXER_HEIGHT = "layout.mixer.height";

// Arranger
const juce::String ARRANGER_ZOOM = "arranger.zoom";
const juce::String ARRANGER_SCROLL_X = "arranger.scrollX";
const juce::String ARRANGER_SCROLL_Y = "arranger.scrollY";
const juce::String ARRANGER_FOLLOW_PLAYHEAD = "arranger.followPlayhead";

// Audio
const juce::String AUDIO_DEVICE_TYPE = "audio.deviceType";
const juce::String AUDIO_OUTPUT_DEVICE = "audio.outputDevice";
const juce::String AUDIO_INPUT_DEVICE = "audio.inputDevice";
const juce::String AUDIO_SAMPLE_RATE = "audio.sampleRate";
const juce::String AUDIO_BUFFER_SIZE = "audio.bufferSize";

// MIDI
const juce::String MIDI_INPUT_DEVICE = "midi.inputDevice";
const juce::String MIDI_OUTPUT_DEVICE = "midi.outputDevice";

// User Preferences
const juce::String PREF_AUTO_SAVE_ENABLED = "preferences.autoSave.enabled";
const juce::String PREF_AUTO_SAVE_INTERVAL = "preferences.autoSave.interval";
const juce::String PREF_TOOLTIPS_ENABLED = "preferences.tooltips.enabled";
const juce::String PREF_TOOLTIPS_DELAY = "preferences.tooltips.delay";
const juce::String PREF_WELCOME_SCREEN = "preferences.welcomeScreen.enabled";
const juce::String PREF_RECENT_FILES_MAX = "preferences.recentFiles.maxCount";
const juce::String PREF_UNDO_LIMIT = "preferences.undo.limit";

// AI Integration
const juce::String AI_API_KEY = "ai.apiKey";
const juce::String AI_PROVIDER = "ai.provider";
const juce::String AI_MODEL = "ai.model";
const juce::String AI_AUTO_SUGGESTIONS = "ai.autoSuggestions.enabled";

// Performance
const juce::String PERF_FPS_LIMIT = "performance.fpsLimit";
const juce::String PERF_VSYNC = "performance.vsync.enabled";
const juce::String PERF_GPU_ACCELERATION =
    "performance.gpuAcceleration.enabled";
const juce::String PERF_METER_REFRESH_RATE = "performance.meterRefreshRate";

// Project Defaults
const juce::String PROJECT_DEFAULT_TEMPO = "project.defaultTempo";
const juce::String PROJECT_DEFAULT_TIMESIG_NUM = "project.defaultTimeSigNum";
const juce::String PROJECT_DEFAULT_TIMESIG_DEN = "project.defaultTimeSigDen";
const juce::String PROJECT_DEFAULT_SAMPLE_RATE = "project.defaultSampleRate";
const juce::String PROJECT_DEFAULT_BUFFER_SIZE = "project.defaultBufferSize";

// Collaboration
const juce::String COLLAB_SALT = "collab.salt";
const juce::String COLLAB_SERVER_IP = "collab.serverIP";
} // namespace keys

// ============================================================================
// Configuration Value Wrapper
// ============================================================================

} // namespace
