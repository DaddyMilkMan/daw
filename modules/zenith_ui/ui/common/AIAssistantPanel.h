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

    AIAssistantPanel.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    UI panel showing all agent states and actions.
    Provides visibility into AI operations and manual control.


  ==============================================================================
*/

#pragma once

#include "../ai/AIEventBus.h"
#include "../ai/AIStatusManager.h"
#include "../ui/framework/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {
namespace ai {
class SessionDebuggerAgent;
class UXDirectorAgent;
} // namespace ai

//==============================================================================
/**
    UI panel displaying AI agent states, operations, and controls.

    Features:
    - Active operation progress display
    - Recent operations history
    - Agent health scores
    - Manual trigger buttons
*/
class AIAssistantPanel : public SkiaComponent, 
                         public ai::AIStatusListener {
public:
  AIAssistantPanel();
  ~AIAssistantPanel() override;

  void timerCallback() override;


  //============================================================================
  // SkiaComponent
  //============================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  //============================================================================
  // AIStatusListener
  //============================================================================

  void onOperationStarted(const ai::AIOperation &operation) override;
  void onOperationProgress(const ai::AIOperation &operation) override;
  void onOperationCompleted(const ai::AIOperation &operation) override;
  void onOperationError(const ai::AIOperation &operation) override;

  //============================================================================
  // Agent References (optional, for direct control)
  //============================================================================

  void setSessionDebugger(ai::SessionDebuggerAgent *agent) {
    sessionDebugger_ = agent;
  }
  void setUXDirector(ai::UXDirectorAgent *agent) { uxDirector_ = agent; }

  //============================================================================
  // Actions
  //============================================================================

  void runAllAnalysis();
  void applyAllFixes();
  void stopAllOperations();

private:
  void drawHeader(SkCanvas *canvas);
  void drawActiveOperations(SkCanvas *canvas, float &yOffset);
  void drawRecentOperations(SkCanvas *canvas, float &yOffset);
  void drawActionButtons(SkCanvas *canvas, float &yOffset);
  void drawStats(SkCanvas *canvas, float &yOffset);

  void handleMouseClick(const juce::MouseEvent &e);
  void mouseDown(const juce::MouseEvent &e) override;

  // Agent references (weak, not owned)
  ai::SessionDebuggerAgent *sessionDebugger_ = nullptr;
  ai::UXDirectorAgent *uxDirector_ = nullptr;

  // Button hit areas
  juce::Rectangle<float> runAnalysisButton_;
  juce::Rectangle<float> applyFixesButton_;
  juce::Rectangle<float> stopButton_;

  // Cached state for rendering
  std::vector<ai::AIOperation> activeOps_;
  std::vector<ai::AIOperation> recentOps_;
  ai::AIStatusManager::Stats stats_;

  // Animation
  float pulsePhase_ = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIAssistantPanel)
};

} // namespace zenith
