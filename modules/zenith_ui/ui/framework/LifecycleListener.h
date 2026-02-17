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

class LifecycleListener {
public:
  virtual ~LifecycleListener() = default;
  virtual void onLifecycleEvent(const LifecycleEvent &event) = 0;
};

// ============================================================================
// Component Lifecycle Interface
// ============================================================================

} // namespace
