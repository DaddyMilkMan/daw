/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {
namespace lifecycle {

// ============================================================================
// Component State
// ============================================================================

enum class ComponentState {
  Uninitialized, // Component created but not initialized
  Initializing,  // Component is being initialized
  Ready,         // Component is ready for use
  Updating,      // Component is being updated
  Suspending,    // Component is being suspended
  Suspended,     // Component is suspended
  Resuming,      // Component is resuming from suspended state
  Destroying,    // Component is being destroyed
  Destroyed      // Component has been destroyed
};

// ============================================================================
// Lifecycle Events
// ============================================================================

class LifecycleComponent : public SkiaComponent, public LifecycleAware {
public:
  LifecycleComponent(const juce::String &componentId = {});
  ~LifecycleComponent() override;

  // SkiaComponent overrides
  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void focusGained(juce::Component::FocusChangeType cause) override;
  void focusLost(juce::Component::FocusChangeType cause) override;

  // LifecycleAware implementation
  ComponentState getCurrentState() const override { return currentState_; }
  juce::String getComponentId() const override { return componentId_; }

  // Lifecycle control methods
  void initialize();
  void update();
  void suspend();
  void resume();
  void destroy();

  // State validation
  bool isReady() const { return currentState_ == ComponentState::Ready; }
  bool isSuspended() const {
    return currentState_ == ComponentState::Suspended;
  }
  bool isDestroyed() const {
    return currentState_ == ComponentState::Destroyed;
  }

  // Error handling
  void setErrorHandler(std::function<void(const juce::String &)> handler);
  juce::String getLastError() const { return lastError_; }

protected:
  // Lifecycle hooks (override in subclasses)
  void onCreate() override;
  void onInitialize() override;
  void onUpdate() override;
  void onSuspend() override;
  void onResume() override;
  void onDestroy() override;
  void onStateChange(ComponentState oldState, ComponentState newState) override;

  // Error reporting
  void reportError(const juce::String &error);

private:
  ComponentState currentState_ = ComponentState::Uninitialized;
  juce::String componentId_;
  juce::String lastError_;
  std::function<void(const juce::String &)> errorHandler_;

  void validateCurrentState();
  void cleanupResources();
};

// ============================================================================
// Component Factory
// ============================================================================

} // namespace
