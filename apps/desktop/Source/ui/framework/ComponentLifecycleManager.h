/*
  ==============================================================================

    ComponentLifecycleManager.h
    Created: 2025-12-07
    Author:  AI Assistant

    Unified component lifecycle management system

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include <juce_core/juce_core.h>
#include <map>


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

struct LifecycleEvent {
  enum class Type {
    Create,
    Initialize,
    Update,
    Suspend,
    Resume,
    Destroy,
    StateChange
  };

  Type type;
  ComponentState oldState;
  ComponentState newState;
  juce::Time timestamp;
  juce::String componentId;
  juce::String message;
};

// ============================================================================
// Component Lifecycle Interface
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

class ComponentLifecycleManager {
public:
  static ComponentLifecycleManager &getInstance();

  // Component registration
  void registerComponent(LifecycleAware *component);
  void unregisterComponent(LifecycleAware *component);

  // Lifecycle management
  void initializeComponent(LifecycleAware *component);
  void updateComponent(LifecycleAware *component);
  void suspendComponent(LifecycleAware *component);
  void resumeComponent(LifecycleAware *component);
  void destroyComponent(LifecycleAware *component);

  // State queries
  ComponentState getComponentState(const LifecycleAware *component) const;
  juce::Array<LifecycleAware *>
  getComponentsInState(ComponentState state) const;

  // Event handling
  using LifecycleCallback = std::function<void(const LifecycleEvent &)>;
  void addLifecycleListener(LifecycleCallback callback);
  void removeLifecycleListener(LifecycleCallback callback);

  // Batch operations
  void suspendAllComponents();
  void resumeAllComponents();
  void destroyAllComponents();

  // Statistics
  int getTotalComponentCount() const;
  int getComponentCountInState(ComponentState state) const;
  juce::String getLifecycleStatistics() const;

  // Memory management
  void checkForMemoryLeaks();
  void forceGarbageCollection();

  void reportError(LifecycleAware *component, const juce::String &error);
  static juce::String getStateName(ComponentState state);

private:
  ComponentLifecycleManager() = default;
  ~ComponentLifecycleManager() = default;

  ComponentLifecycleManager(const ComponentLifecycleManager &) = delete;
  ComponentLifecycleManager &
  operator=(const ComponentLifecycleManager &) = delete;

  // Internal methods
  void transitionComponent(LifecycleAware *component, ComponentState newState);
  void fireLifecycleEvent(const LifecycleEvent &event);
  void validateStateTransition(ComponentState oldState,
                               ComponentState newState);

  // Data members
  juce::HashMap<LifecycleAware *, ComponentState> componentStates_;
  juce::Array<LifecycleCallback> lifecycleListeners_;
  juce::CriticalSection lock_;
  juce::int64 nextComponentId_ = 1;

  juce::HashMap<LifecycleAware *, juce::String> componentIds_;
  juce::Array<LifecycleEvent> eventHistory_;
  static constexpr int MAX_EVENT_HISTORY = 1000;
};

// ============================================================================
// Base Lifecycle Component
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

class MemoryLeakDetector {
public:
  static MemoryLeakDetector &getInstance();

  void trackComponent(const LifecycleComponent *component);
  void untrackComponent(const LifecycleComponent *component);

  void checkForLeaks();
  juce::String getLeakReport() const;

  struct MemoryStats {
    int totalComponentsCreated = 0;
    int totalComponentsDestroyed = 0;
    int currentComponentCount = 0;
    std::map<juce::String, int> componentTypeCounts;

    MemoryStats() = default;
    MemoryStats(const MemoryStats &other) {
      totalComponentsCreated = other.totalComponentsCreated;
      totalComponentsDestroyed = other.totalComponentsDestroyed;
      currentComponentCount = other.currentComponentCount;
      componentTypeCounts = other.componentTypeCounts;
    }
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