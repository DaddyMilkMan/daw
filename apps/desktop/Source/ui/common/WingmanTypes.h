/*
  ==============================================================================

    WingmanTypes.h
    Created: 2026-02-02
    Author:  Zenith DAW

    Data structures for the Wingman AI assistant panel.
    Separated from WingmanPanel.h for better code organization.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include "WingmanTextLayout.h"  // For CachedTextLayout

namespace zenith {

//==============================================================================
// Wingman Data Structures
//==============================================================================

/**
 * @brief Suggestion chip for quick actions in Wingman
 */
struct Suggestion {
    juce::String id;
    juce::String label;
    juce::String intent;
    bool isUserCreated = false;
    bool isEditable = true;
};

/**
 * @brief A single message in the Wingman chat
 */
struct WingmanMessage {
    juce::String id;
    juce::String sender; // "User" or "Wingman"
    juce::String content;
    juce::String reasoning;
    int64_t timestamp;
    bool isReasoning = false; // Legacy flag (keep for compat or repurpose)
    bool hasReasoning = false; // TRUE if reasoning field is populated

    // Layout Cache
    CachedTextLayout layout;
    float cachedHeight = 0.0f;
    float cachedWidth = 0.0f;
};

/**
 * @brief A chat session with the Wingman AI
 */
struct WingmanSession {
    juce::String id;
    juce::String title;
    juce::String dateLabel;
    bool isActive = false;
    bool reasoningEnabled = false; // Default OFF for new chats
    std::vector<WingmanMessage> messages;
    std::vector<Suggestion> suggestions;
};

} // namespace zenith
