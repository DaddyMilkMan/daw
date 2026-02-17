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

  struct MemoryStats {
    int totalComponentsCreated = 0;
    int totalComponentsDestroyed = 0;
    int currentComponentCount = 0;
    std::map<juce::String, int> componentTypeCounts;
  };

  MemoryStats getMemoryStats() const;

private:
  MemoryLeakDetector() = default;
  ~MemoryLeakDetector() = default;

  juce::HashMap<const LifecycleComponent *, juce::String> activeComponents_;
  MemoryStats stats_;
  juce::CriticalSection lock_;
};

// ============================================================================
// State Persistence
// ============================================================================

} // namespace
