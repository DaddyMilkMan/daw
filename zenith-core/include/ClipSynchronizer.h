/**
 * @file ClipSynchronizer.h
 * @brief Synchronizes clip data between Engine and ProjectState
 *
 * Worker E: ClipSynchronizer Bridge
 *
 * This class bridges the audio-thread Engine clips with the message-thread
 * ProjectState (ValueTree). It:
 * - Polls engine tracks for new recorded clips
 * - Creates corresponding CLIP nodes in ProjectState
 * - Listens to ProjectState clip changes
 * - Updates Engine clip positions when clips are moved in the arranger
 *
 * Thread Safety:
 * - Polls engine from MESSAGE THREAD via timer
 * - Listens to ValueTree on MESSAGE THREAD
 * - Never accesses audio thread from here
 *
 * Pattern: Similar to TrackAutomationSynchronizer
 */

#pragma once

#include <JuceHeader.h>
#include "ProjectState.h"
#include "Engine.h"
#include <map>
#include <set>
#include <memory>

//==============================================================================
/**
 * @class ClipSynchronizer
 * @brief Syncs clips between Engine and ProjectState
 *
 * This class runs a timer on the message thread that:
 * 1. Polls engine tracks for new clips (created by recording)
 * 2. Creates corresponding CLIP nodes in ProjectState
 * 3. Listens for CLIP node changes in ProjectState
 * 4. Updates engine clip positions accordingly
 *
 * Clip ID mapping:
 * - Each engine clip gets a unique clipId string
 * - The same clipId is stored in both engine clip and ProjectState CLIP node
 * - This allows bidirectional synchronization
 */
class ClipSynchronizer : public juce::Timer,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     * @brief Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param engine Reference to audio engine (must outlive this object)
     */
    ClipSynchronizer(ProjectState& projectState, Engine& engine);

    /**
     * @brief Destructor
     */
    ~ClipSynchronizer() override;

    //==========================================================================
    /**
     * @brief Start clip synchronization
     * @param updateRateHz Update rate in Hz (default 10 - clips don't need rapid updates)
     */
    void start(int updateRateHz = 10);

    /**
     * @brief Stop clip synchronization
     */
    void stop();

    /**
     * @brief Check if synchronizer is running
     */
    bool isRunning() const { return isTimerRunning(); }

private:
    //==========================================================================
    // Timer callback (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Timer callback - polls engine for new clips
     * @note Called on MESSAGE THREAD at regular intervals
     */
    void timerCallback() override;

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
     * @brief Poll engine for new clips and create ProjectState nodes
     * @note MESSAGE THREAD only
     */
    void syncNewRecordedClipsFromEngine();

    /**
     * @brief Apply clip position change from ProjectState to engine
     * @param clipNode CLIP ValueTree node
     * @note MESSAGE THREAD only
     */
    void applyClipMoveToEngine(const juce::ValueTree& clipNode);

    /**
     * @brief Convert samples to beats using current tempo
     * @param samples Sample position
     * @return Position in beats
     */
    double samplesToBeats(int64_t samples) const;

    /**
     * @brief Convert beats to samples using current tempo
     * @param beats Beat position
     * @return Position in samples
     */
    int64_t beatsToSamples(double beats) const;

    /**
     * @brief Rebuild internal state
     * @note Call when tracks change or on initialization
     */
    void rebuildState();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ProjectState& projectState;
    Engine& engine;

    // Track known clip IDs to detect new clips
    std::set<juce::String> knownClipIds;

    // Map ProjectState track IDs to engine track indices
    std::map<juce::String, int> trackIdToIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSynchronizer)
};
