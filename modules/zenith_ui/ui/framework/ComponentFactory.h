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

class ComponentFactory {
public:
  static ComponentFactory &getInstance();

  // Registration
  using ComponentCreator =
      std::function<std::unique_ptr<LifecycleComponent>(const juce::String &)>;

  void registerComponentType(const juce::String &typeName,
                             ComponentCreator creator);
  void unregisterComponentType(const juce::String &typeName);

  // Creation
  std::unique_ptr<LifecycleComponent>
  createComponent(const juce::String &typeName,
                  const juce::String &componentId = {});

  // Query
  juce::StringArray getRegisteredComponentTypes() const;
  bool isComponentTypeRegistered(const juce::String &typeName) const;

private:
  ComponentFactory() = default;
  ~ComponentFactory() = default;

  juce::HashMap<juce::String, ComponentCreator> componentCreators_;
  juce::CriticalSection lock_;
};

// ============================================================================
// Memory Leak Detector
// ============================================================================

} // namespace
