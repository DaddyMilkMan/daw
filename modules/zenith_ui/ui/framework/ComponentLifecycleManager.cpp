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

#include "ComponentLifecycleManager.h"
#include "ZenithDesignSystem.h"

namespace zenith {
namespace lifecycle {

// ============================================================================
// ComponentLifecycleManager Implementation
// ============================================================================

ComponentLifecycleManager &ComponentLifecycleManager::getInstance() {
  static ComponentLifecycleManager instance;
  return instance;
}

void ComponentLifecycleManager::registerComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  juce::String componentId = "component_" + juce::String(nextComponentId_++);
  componentStates_.set(component, {ComponentState::Uninitialized, component});
  componentIds_.set(component, componentId);

  // Fire create event
  LifecycleEvent event;
  event.type = LifecycleEvent::Type::Create;
  event.oldState = ComponentState::Uninitialized;
  event.newState = ComponentState::Uninitialized;
  event.timestamp = juce::Time::getCurrentTime();
  event.componentId = componentId;
  event.message = "Component registered";

  fireLifecycleEvent(event);
}

void ComponentLifecycleManager::unregisterComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  // Fire destroy event if component still exists
  if (componentStates_.contains(component)) {
    LifecycleEvent event;
    event.type = LifecycleEvent::Type::Destroy;
    event.oldState = componentStates_[component].state;
    event.newState = ComponentState::Destroyed;
    event.timestamp = juce::Time::getCurrentTime();
    event.componentId = componentIds_[component];
    event.message = "Component unregistered";

    fireLifecycleEvent(event);
  }

  componentStates_.remove(component);
  componentIds_.remove(component);
}

void ComponentLifecycleManager::initializeComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (!componentStates_.contains(component)) {
    registerComponent(component);
  }

  ComponentState currentState = componentStates_[component].state;

  // Validate state transition
  if (currentState != ComponentState::Uninitialized &&
      currentState != ComponentState::Suspended) {
    reportError(component, "Invalid state transition to Initializing");
    return;
  }

  // Transition to initializing
  transitionComponent(component, ComponentState::Initializing);

  try {
    // Call component's initialization hook
    component->onInitialize();

    // Transition to ready state
    transitionComponent(component, ComponentState::Ready);

  } catch (const std::exception &e) {
    reportError(component, "Initialization failed: " + juce::String(e.what()));
    transitionComponent(component, ComponentState::Uninitialized);
  }
}

void ComponentLifecycleManager::updateComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (!componentStates_.contains(component)) {
    reportError(component, "Component not registered");
    return;
  }

  ComponentState currentState = componentStates_[component].state;

  if (currentState != ComponentState::Ready) {
    reportError(component, "Component must be in Ready state to update");
    return;
  }

  try {
    // Call component's update hook
    component->onUpdate();

    // Fire update event
    LifecycleEvent event;
    event.type = LifecycleEvent::Type::Update;
    event.oldState = currentState;
    event.newState = currentState;
    event.timestamp = juce::Time::getCurrentTime();
    event.componentId = componentIds_[component];
    event.message = "Component updated";

    fireLifecycleEvent(event);

  } catch (const std::exception &e) {
    reportError(component, "Update failed: " + juce::String(e.what()));
  }
}

void ComponentLifecycleManager::suspendComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (!componentStates_.contains(component)) {
    reportError(component, "Component not registered");
    return;
  }

  ComponentState currentState = componentStates_[component].state;

  if (currentState != ComponentState::Ready) {
    reportError(component, "Component must be in Ready state to suspend");
    return;
  }

  // Transition to suspending
  transitionComponent(component, ComponentState::Suspending);

  try {
    // Call component's suspend hook
    component->onSuspend();

    // Transition to suspended state
    transitionComponent(component, ComponentState::Suspended);

  } catch (const std::exception &e) {
    reportError(component, "Suspend failed: " + juce::String(e.what()));
    transitionComponent(component, ComponentState::Ready);
  }
}

void ComponentLifecycleManager::resumeComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (!componentStates_.contains(component)) {
    reportError(component, "Component not registered");
    return;
  }

  ComponentState currentState = componentStates_[component].state;

  if (currentState != ComponentState::Suspended) {
    reportError(component, "Component must be in Suspended state to resume");
    return;
  }

  // Transition to resuming
  transitionComponent(component, ComponentState::Resuming);

  try {
    // Call component's resume hook
    component->onResume();

    // Transition to ready state
    transitionComponent(component, ComponentState::Ready);

  } catch (const std::exception &e) {
    reportError(component, "Resume failed: " + juce::String(e.what()));
    transitionComponent(component, ComponentState::Suspended);
  }
}

void ComponentLifecycleManager::destroyComponent(LifecycleAware *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (!componentStates_.contains(component)) {
    return; // Already destroyed or never registered
  }

  ComponentState currentState = componentStates_[component].state;

  // Transition to destroying
  transitionComponent(component, ComponentState::Destroying);

  try {
    // Call component's destroy hook
    component->onDestroy();

    // Transition to destroyed state
    transitionComponent(component, ComponentState::Destroyed);

  } catch (const std::exception &e) {
    reportError(component, "Destroy failed: " + juce::String(e.what()));
  }

  // Remove from tracking
  componentStates_.remove(component);
  componentIds_.remove(component);
}

ComponentState ComponentLifecycleManager::getComponentState(
    const LifecycleAware *component) const {
  juce::ScopedLock lock(lock_);
  if (componentStates_.contains(component)) {
    return componentStates_[component].state;
  }
  return ComponentState::Uninitialized;
}

juce::Array<LifecycleAware *>
ComponentLifecycleManager::getComponentsInState(ComponentState state) const {
  juce::ScopedLock lock(lock_);

  juce::Array<LifecycleAware *> result;
  for (juce::HashMap<const LifecycleAware *, ComponentEntry>::Iterator it(
           componentStates_);
       it.next();) {
    if (it.getValue().state == state) {
      result.add(it.getValue().component);
    }
  }
  return result;
}

void ComponentLifecycleManager::addLifecycleListener(
    LifecycleListener *listener) {
  juce::ScopedLock lock(lock_);
  lifecycleListeners_.addIfNotAlreadyThere(listener);
}

void ComponentLifecycleManager::removeLifecycleListener(
    LifecycleListener *listener) {
  juce::ScopedLock lock(lock_);
  lifecycleListeners_.removeAllInstancesOf(listener);
}

void ComponentLifecycleManager::suspendAllComponents() {
  juce::ScopedLock lock(lock_);

  auto components = getComponentsInState(ComponentState::Ready);
  for (auto *component : components) {
    suspendComponent(component);
  }
}

void ComponentLifecycleManager::resumeAllComponents() {
  juce::ScopedLock lock(lock_);

  auto components = getComponentsInState(ComponentState::Suspended);
  for (auto *component : components) {
    resumeComponent(component);
  }
}

void ComponentLifecycleManager::destroyAllComponents() {
  juce::ScopedLock lock(lock_);

  // Destroy components in reverse order of registration
  juce::Array<LifecycleAware *> components;
  for (juce::HashMap<const LifecycleAware *, ComponentEntry>::Iterator it(
           componentStates_);
       it.next();) {
    components.add(it.getValue().component);
  }

  for (int i = components.size() - 1; i >= 0; --i) {
    destroyComponent(components[i]);
  }
}

int ComponentLifecycleManager::getTotalComponentCount() const {
  juce::ScopedLock lock(lock_);
  return componentStates_.size();
}

int ComponentLifecycleManager::getComponentCountInState(
    ComponentState state) const {
  juce::ScopedLock lock(lock_);

  int count = 0;
  for (juce::HashMap<const LifecycleAware *, ComponentEntry>::Iterator it(
           componentStates_);
       it.next();) {
    if (it.getValue().state == state) {
      ++count;
    }
  }
  return count;
}

juce::String ComponentLifecycleManager::getLifecycleStatistics() const {
  juce::ScopedLock lock(lock_);

  juce::String stats;
  stats << "=== Component Lifecycle Statistics ===\n";
  stats << "Total Components: " << getTotalComponentCount() << "\n";
  stats << "Uninitialized: "
        << getComponentCountInState(ComponentState::Uninitialized) << "\n";
  stats << "Initializing: "
        << getComponentCountInState(ComponentState::Initializing) << "\n";
  stats << "Ready: " << getComponentCountInState(ComponentState::Ready) << "\n";
  stats << "Suspended: " << getComponentCountInState(ComponentState::Suspended)
        << "\n";
  stats << "Destroying: "
        << getComponentCountInState(ComponentState::Destroying) << "\n";
  stats << "Destroyed: " << getComponentCountInState(ComponentState::Destroyed)
        << "\n";

  return stats;
}

void ComponentLifecycleManager::checkForMemoryLeaks() {
  juce::ScopedLock lock(lock_);

  auto readyComponents = getComponentsInState(ComponentState::Ready);
  auto suspendedComponents = getComponentsInState(ComponentState::Suspended);

  if (!readyComponents.isEmpty() || !suspendedComponents.isEmpty()) {
    DBG("Potential memory leak detected: "
        << readyComponents.size() + suspendedComponents.size()
        << " components not properly destroyed");
  }
}

void ComponentLifecycleManager::forceGarbageCollection() {
  // In a real implementation, this would trigger garbage collection
  // For now, just log the request
  DBG("Garbage collection requested");
}

void ComponentLifecycleManager::transitionComponent(LifecycleAware *component,
                                                    ComponentState newState) {
  ComponentState oldState = componentStates_[component].state;

  // Validate state transition
  validateStateTransition(oldState, newState);

  // Update state
  componentStates_.set(component, {newState, component});

  // Fire state change event
  LifecycleEvent event;
  event.type = LifecycleEvent::Type::StateChange;
  event.oldState = oldState;
  event.newState = newState;
  event.timestamp = juce::Time::getCurrentTime();
  event.componentId = componentIds_[component];
  event.message = "State changed from " + getStateName(oldState) + " to " +
                  getStateName(newState);

  fireLifecycleEvent(event);

  // Call component's state change hook
  component->onStateChange(oldState, newState);
}

void ComponentLifecycleManager::fireLifecycleEvent(
    const LifecycleEvent &event) {
  // Add to event history
  eventHistory_.add(event);
  if (eventHistory_.size() > MAX_EVENT_HISTORY) {
    eventHistory_.removeRange(0, eventHistory_.size() - MAX_EVENT_HISTORY);
  }

  // Notify listeners
  for (auto *listener : lifecycleListeners_) {
    listener->onLifecycleEvent(event);
  }
}

void ComponentLifecycleManager::validateStateTransition(
    ComponentState oldState, ComponentState newState) {
  // Define valid state transitions
  switch (oldState) {
  case ComponentState::Uninitialized:
    if (newState != ComponentState::Initializing &&
        newState != ComponentState::Destroyed) {
      throw std::runtime_error("Invalid transition from Uninitialized");
    }
    break;

  case ComponentState::Initializing:
    if (newState != ComponentState::Ready &&
        newState != ComponentState::Uninitialized) {
      throw std::runtime_error("Invalid transition from Initializing");
    }
    break;

  case ComponentState::Ready:
    if (newState != ComponentState::Updating &&
        newState != ComponentState::Suspending &&
        newState != ComponentState::Destroying) {
      throw std::runtime_error("Invalid transition from Ready");
    }
    break;

  case ComponentState::Suspended:
    if (newState != ComponentState::Resuming &&
        newState != ComponentState::Destroying) {
      throw std::runtime_error("Invalid transition from Suspended");
    }
    break;

  case ComponentState::Destroying:
    if (newState != ComponentState::Destroyed) {
      throw std::runtime_error("Invalid transition from Destroying");
    }
    break;

  case ComponentState::Destroyed:
    throw std::runtime_error("Cannot transition from Destroyed state");

  default:
    break;
  }
}

void ComponentLifecycleManager::reportError(LifecycleAware *component,
                                            const juce::String &error) {
  DBG("Lifecycle error for component " << componentIds_[component] << ": "
                                       << error);

  // Fire error event
  LifecycleEvent event;
  event.type = LifecycleEvent::Type::StateChange;
  event.oldState = componentStates_[component].state;
  event.newState = ComponentState::Uninitialized;
  event.timestamp = juce::Time::getCurrentTime();
  event.componentId = componentIds_[component];
  event.message = "ERROR: " + error;

  fireLifecycleEvent(event);
}

juce::String ComponentLifecycleManager::getStateName(ComponentState state) {
  switch (state) {
  case ComponentState::Uninitialized:
    return "Uninitialized";
  case ComponentState::Initializing:
    return "Initializing";
  case ComponentState::Ready:
    return "Ready";
  case ComponentState::Updating:
    return "Updating";
  case ComponentState::Suspending:
    return "Suspending";
  case ComponentState::Suspended:
    return "Suspended";
  case ComponentState::Resuming:
    return "Resuming";
  case ComponentState::Destroying:
    return "Destroying";
  case ComponentState::Destroyed:
    return "Destroyed";
  default:
    return "Unknown";
  }
}

// ============================================================================
// LifecycleComponent Implementation
// ============================================================================

LifecycleComponent::LifecycleComponent(const juce::String &componentId)
    : componentId_(componentId.isEmpty()
                       ? "component_" + juce::String((juce::int64)this)
                       : componentId) {

  // Register with lifecycle manager
  ComponentLifecycleManager::getInstance().registerComponent(this);

  // Call create hook
  onCreate();
}

LifecycleComponent::~LifecycleComponent() {
  // Ensure component is properly destroyed
  if (currentState_ != ComponentState::Destroyed) {
    destroy();
  }

  // Unregister from lifecycle manager
  ComponentLifecycleManager::getInstance().unregisterComponent(this);
}

void LifecycleComponent::paint(juce::Graphics &g) {
  juce::ignoreUnused(g);
  // Skia components handle their own painting
}

void LifecycleComponent::resized() {
  // Notify lifecycle manager of update
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::mouseDown(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  // Notify lifecycle manager of interaction
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::mouseDrag(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::focusGained(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::focusLost(juce::Component::FocusChangeType cause) {
  juce::ignoreUnused(cause);
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::initialize() {
  ComponentLifecycleManager::getInstance().initializeComponent(this);
}

void LifecycleComponent::update() {
  ComponentLifecycleManager::getInstance().updateComponent(this);
}

void LifecycleComponent::suspend() {
  ComponentLifecycleManager::getInstance().suspendComponent(this);
}

void LifecycleComponent::resume() {
  ComponentLifecycleManager::getInstance().resumeComponent(this);
}

void LifecycleComponent::destroy() {
  ComponentLifecycleManager::getInstance().destroyComponent(this);
}

void LifecycleComponent::onCreate() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onInitialize() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onUpdate() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onSuspend() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onResume() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onDestroy() {
  // Default implementation - override in subclasses
}

void LifecycleComponent::onStateChange(ComponentState oldState,
                                       ComponentState newState) {
  juce::ignoreUnused(oldState, newState);
  // Default implementation - override in subclasses
}

void LifecycleComponent::setErrorHandler(
    std::function<void(const juce::String &)> handler) {
  errorHandler_ = handler;
}

void LifecycleComponent::reportError(const juce::String &error) {
  lastError_ = error;

  if (errorHandler_) {
    errorHandler_(error);
  }

  DBG("LifecycleComponent " << componentId_ << " error: " << error);
}

void LifecycleComponent::validateCurrentState() {
  ComponentState expectedState =
      ComponentLifecycleManager::getInstance().getComponentState(this);

  if (currentState_ != expectedState) {
    reportError(
        "State mismatch: expected " +
        ComponentLifecycleManager::getInstance().getStateName(expectedState) +
        ", actual " +
        ComponentLifecycleManager::getInstance().getStateName(currentState_));
  }
}

void LifecycleComponent::cleanupResources() {
  // Clean up any allocated resources
  // This would be customized in subclasses
}

// ============================================================================
// ComponentFactory Implementation
// ============================================================================

ComponentFactory &ComponentFactory::getInstance() {
  static ComponentFactory instance;
  return instance;
}

void ComponentFactory::registerComponentType(const juce::String &typeName,
                                             ComponentCreator creator) {
  juce::ScopedLock lock(lock_);
  componentCreators_.set(typeName, creator);
}

void ComponentFactory::unregisterComponentType(const juce::String &typeName) {
  juce::ScopedLock lock(lock_);
  componentCreators_.remove(typeName);
}

std::unique_ptr<LifecycleComponent>
ComponentFactory::createComponent(const juce::String &typeName,
                                  const juce::String &componentId) {
  juce::ScopedLock lock(lock_);

  if (componentCreators_.contains(typeName)) {
    return componentCreators_[typeName](componentId);
  }

  return nullptr;
}

juce::StringArray ComponentFactory::getRegisteredComponentTypes() const {
  juce::ScopedLock lock(lock_);

  juce::StringArray types;
  for (juce::HashMap<juce::String, ComponentCreator>::Iterator it(
           componentCreators_);
       it.next();) {
    types.add(it.getKey());
  }
  return types;
}

bool ComponentFactory::isComponentTypeRegistered(
    const juce::String &typeName) const {
  juce::ScopedLock lock(lock_);
  return componentCreators_.contains(typeName);
}

// ============================================================================
// MemoryLeakDetector Implementation
// ============================================================================

MemoryLeakDetector &MemoryLeakDetector::getInstance() {
  static MemoryLeakDetector instance;
  return instance;
}

void MemoryLeakDetector::trackComponent(const LifecycleComponent *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  juce::String componentId = component->getComponentId();
  activeComponents_.set(component, componentId);

  stats_.totalComponentsCreated++;
  stats_.currentComponentCount++;

  // Update type counts
  juce::String typeName = typeid(*component).name();
  stats_.componentTypeCounts[typeName]++;
}

void MemoryLeakDetector::untrackComponent(const LifecycleComponent *component) {
  if (!component)
    return;

  juce::ScopedLock lock(lock_);

  if (activeComponents_.contains(component)) {
    activeComponents_.remove(component);
    stats_.totalComponentsDestroyed++;
    stats_.currentComponentCount--;
  }
}

void MemoryLeakDetector::checkForLeaks() {
  juce::ScopedLock lock(lock_);

  if (activeComponents_.size() > 0) {
    DBG("=== MEMORY LEAK DETECTED ===");
    DBG("Active components: " << activeComponents_.size());

    for (juce::HashMap<const LifecycleComponent *, juce::String>::Iterator it(
             activeComponents_);
         it.next();) {
      DBG("  - " << it.getValue());
    }
  } else {
    DBG("No memory leaks detected");
  }
}

juce::String MemoryLeakDetector::getLeakReport() const {
  juce::ScopedLock lock(lock_);

  juce::String report;
  report << "=== Memory Leak Report ===\n";
  report << "Active Components: " << activeComponents_.size() << "\n";

  for (juce::HashMap<const LifecycleComponent *, juce::String>::Iterator it(
           activeComponents_);
       it.next();) {
    report << "  - " << it.getValue() << "\n";
  }

  return report;
}

MemoryLeakDetector::MemoryStats MemoryLeakDetector::getMemoryStats() const {
  juce::ScopedLock lock(lock_);
  return stats_;
}

// ============================================================================
// ComponentStatePersistence Implementation
// ============================================================================

ComponentStatePersistence &ComponentStatePersistence::getInstance() {
  static ComponentStatePersistence instance;
  return instance;
}

void ComponentStatePersistence::saveComponentState(
    const LifecycleComponent *component, const juce::File &file) {
  if (!component || !file.hasWriteAccess())
    return;

  juce::ScopedLock lock(lock_);

  juce::String stateData = serializeComponentState(component);

  if (format_ == Format::JSON) {
    file.replaceWithText(stateData);
  } else if (format_ == Format::XML) {
    // XML implementation would go here
    file.replaceWithText(stateData);
  } else if (format_ == Format::Binary) {
    // Binary implementation would go here
    file.replaceWithData(stateData.toRawUTF8(), stateData.getNumBytesAsUTF8());
  }
}

void ComponentStatePersistence::loadComponentState(
    LifecycleComponent *component, const juce::File &file) {
  if (!component || !file.existsAsFile())
    return;

  juce::ScopedLock lock(lock_);

  juce::String stateData;

  if (format_ == Format::JSON || format_ == Format::XML) {
    stateData = file.loadFileAsString();
  } else if (format_ == Format::Binary) {
    // Binary implementation would go here
    stateData = file.loadFileAsString();
  }

  deserializeComponentState(component, stateData);
}

juce::String ComponentStatePersistence::serializeComponentState(
    const LifecycleComponent *component) {
  // Basic JSON serialization
  juce::DynamicObject::Ptr state = new juce::DynamicObject();

  state->setProperty("componentId", component->getComponentId());
  state->setProperty("componentState", (int)component->getCurrentState());
  state->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));

  // Component-specific state would be added here
  // Subclasses should override this method to save their state
  // DBG("WARNING: Base serializeComponentState called for " << component->getComponentId() << " - specific state may be missing");

  juce::String json = juce::JSON::toString(juce::var(state.get()));
  return json;
}

void ComponentStatePersistence::deserializeComponentState(
    LifecycleComponent *component, const juce::String &data) {
  auto parsed = juce::JSON::parse(data);

  if (auto *object = parsed.getDynamicObject()) {
    // Restore component state
    // Subclasses should override this method to load their state
    
    // DBG("Component state loaded for: " << component->getComponentId());
  }
}

} // namespace lifecycle
} // namespace zenith