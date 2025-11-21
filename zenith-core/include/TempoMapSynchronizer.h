/**
 * @file TempoMapSynchronizer.h
 * @brief Synchronizes tempo map from ProjectState to Engine
 *
 * Phase 15: Tempo Map & Global Markers MVP
 *
 * This class bridges the message-thread ProjectState (ValueTree) with the
 * Engine's TempoMap. It:
 * - Listens to ProjectState tempo map changes
 * - Updates Engine's TempoMap in an RT-safe manner
 *
 * Thread Safety:
 * - Listens to ValueTree on MESSAGE THREAD
 * - Updates TempoMap from MESSAGE THREAD
 * - Engine/audio thread reads TempoMap (lock-free, safe)
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "TempoMap.h"
#include <memory>

// Forward declarations
class Engine;

//==============================================================================
/**
 * @class TempoMapSynchronizer
 * @brief Syncs tempo map from ProjectState to Engine
 *
 * This class listens to ProjectState changes and updates the Engine's TempoMap.
 * Unlike TrackAutomationSynchronizer, this doesn't need a timer because tempo
 * changes are instantaneous (not sampled over time).
 */
class TempoMapSynchronizer : private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param tempoMap Reference to engine's tempo map (must outlive this object)
     */
    TempoMapSynchronizer(ProjectState& projectState, TempoMap& tempoMap);

    /**
     * @brief Destructor
     */
    ~TempoMapSynchronizer() override;

    //==========================================================================
    /**
     * @brief Start synchronization
     */
    void start();

    /**
     * @brief Stop synchronization
     */
    void stop();

    /**
     * @brief Force immediate update from current ProjectState
     * @note Call this after loading a project or when ProjectState changes externally
     */
    void forceUpdate();

private:
    //==========================================================================
    // ValueTree::Listener (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Update tempo map from current ProjectState
     * @note MESSAGE THREAD ONLY
     */
    void updateTempoMap();

    /**
     * @brief Check if a ValueTree node is part of the tempo map
     */
    bool isTempoMapNode(const juce::ValueTree& tree) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    TempoMap& tempoMap;

    bool isActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMapSynchronizer)
};

