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

class ComponentStatePersistence {
public:
  static ComponentStatePersistence &getInstance();

  // Save/Load component state
  void saveComponentState(const LifecycleComponent *component,
                          const juce::File &file);
  void loadComponentState(LifecycleComponent *component,
                          const juce::File &file);

  // Batch operations
  void saveAllComponentStates(const juce::File &directory);
  void loadAllComponentStates(const juce::File &directory);

  // State format
  enum class Format { JSON, XML, Binary };

  void setFormat(Format format) { format_ = format; }
  Format getFormat() const { return format_; }

private:
  ComponentStatePersistence() = default;
  ~ComponentStatePersistence() = default;

  Format format_ = Format::JSON;
  juce::CriticalSection lock_;

  juce::String serializeComponentState(const LifecycleComponent *component);
  void deserializeComponentState(LifecycleComponent *component,
                                 const juce::String &data);
};

} // namespace lifecycle
} // namespace zenith