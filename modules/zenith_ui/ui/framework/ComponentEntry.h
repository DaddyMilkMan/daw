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

  struct ComponentEntry {
    ComponentState state;
    LifecycleAware *component; // Mutable pointer stored in value
  };

  juce::HashMap<const LifecycleAware *, ComponentEntry> componentStates_;
  juce::Array<LifecycleListener *> lifecycleListeners_;
  juce::CriticalSection lock_;
  juce::int64 nextComponentId_ = 1;

  juce::HashMap<const LifecycleAware *, juce::String> componentIds_;
  juce::Array<LifecycleEvent> eventHistory_;
  static constexpr int MAX_EVENT_HISTORY = 1000;
};

// ============================================================================
// Base Lifecycle Component
// ============================================================================

} // namespace
