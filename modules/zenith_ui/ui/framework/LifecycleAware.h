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

class LifecycleAware {
public:
  virtual ~LifecycleAware() = default;

  // Lifecycle hooks
  virtual void onCreate() {}
  virtual void onInitialize() {}
  virtual void onUpdate() {}
  virtual void onSuspend() {}
  virtual void onResume() {}
  virtual void onDestroy() {}
  virtual void onStateChange(ComponentState oldState, ComponentState newState) {
    juce::ignoreUnused(oldState, newState);
  }

  // State queries
  virtual ComponentState getCurrentState() const = 0;
  virtual juce::String getComponentId() const = 0;
};

// ============================================================================
// Lifecycle Manager
// ============================================================================

} // namespace
