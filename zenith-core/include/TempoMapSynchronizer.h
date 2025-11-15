/**
 * @file TempoMapSynchronizer.h
 * @brief Synchronizes tempo map and markers from ProjectState to Engine
 *
 * Phase 15: Tempo Map + Markers MVP
 *
 * This class bridges the message-thread ProjectState (ValueTree) with the
 * audio-thread Engine tempo map runtime. It:
 * - Listens to ProjectState tempo map and marker changes
 * - Updates Engine's tempo map segments when changes occur
 * - Runs purely on the message thread
 *
 * Thread Safety:
 * - Listens to ValueTree on MESSAGE THREAD
 * - Calls Engine::setTempoMap from MESSAGE THREAD
 * - Engine handles RT-safe access to tempo map
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"

//==============================================================================
/**
 * @class TempoMapSynchronizer
 * @brief Syncs tempo map from ProjectState to Engine
 *
 * This class listens for changes to the tempo map in ProjectState and
 * updates the Engine's tempo map runtime accordingly.
 */
class TempoMapSynchronizer : private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param engine Reference to audio engine (must outlive this object)
     */
    TempoMapSynchronizer(ProjectState& projectState, Engine& engine);

    /**
     * @brief Destructor
     */
    ~TempoMapSynchronizer() override;

    //==========================================================================
    /**
     * @brief Initialize tempo map synchronization
     * @note Call this after construction to start listening
     */
    void initialize();

    /**
     * @brief Force update of tempo map from ProjectState to Engine
     * @note Useful when first setting up or after major changes
     */
    void forceUpdate();

private:
    //==========================================================================
    // ValueTree::Listener (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Update Engine tempo map from ProjectState
     * @note Called on message thread when tempo map changes
     */
    void updateEngineTempoMap();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMapSynchronizer)
};
