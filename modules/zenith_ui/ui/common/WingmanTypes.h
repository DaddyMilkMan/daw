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
