/*
  ==============================================================================

    AutomationStateManager.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Focused module for automation management within ProjectState.
    
    Extracted from ProjectState.cpp for better modularity.

    Thread Safety:
    - All methods are MESSAGE THREAD ONLY
    - Uses ValueTree for persistent state

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

class ProjectState;

/**
    Manages automation clips, lanes, and points within ProjectState.
*/
class AutomationStateManager {
public:
    explicit AutomationStateManager(ProjectState& projectState);
    ~AutomationStateManager() = default;

    //==========================================================================
    // Automation Access
    //==========================================================================

    /**
     * @brief Get or create an automation envelope for a parameter
     * @param trackId Track ID
     * @param paramId Parameter ID (e.g., "volume", "pan")
     * @return Envelope ValueTree
     */
    juce::ValueTree getOrCreateAutomationEnvelope(const juce::String& trackId,
                                                  const juce::String& paramId);

    /**
     * @brief Get existing automation envelope (returns invalid if not found)
     */
    juce::ValueTree getAutomationEnvelope(const juce::String& trackId,
                                          const juce::String& paramId) const;

    /**
     * @brief Check if automation exists for a parameter
     */
    bool hasAutomation(const juce::String& trackId, const juce::String& paramId) const;

    //==========================================================================
    // Point Management
    //==========================================================================

    /**
     * @brief Add a point to an automation curve
     * @return ID of the new point
     */
    juce::String addAutomationPoint(const juce::String& trackId,
                                    const juce::String& paramId,
                                    double timeBeats,
                                    double value,
                                    float tension,
                                    int curveType,
                                    const juce::String& actionName = "Add Point");

    /**
     * @brief Move an existing point
     */
    bool moveAutomationPoint(const juce::String& trackId,
                             const juce::String& paramId,
                             const juce::String& pointId,
                             double newTimeBeats,
                             double newValue,
                             const juce::String& actionName = "Move Point");

    /**
     * @brief Delete a specific point
     */
    bool deleteAutomationPoint(const juce::String& trackId,
                               const juce::String& paramId,
                               const juce::String& pointId,
                               const juce::String& actionName = "Delete Point");

    /**
     * @brief Clear all automation for a parameter
     */
    bool clearAutomation(const juce::String& trackId,
                         const juce::String& paramId,
                         const juce::String& actionName = "Clear Automation");

    //==========================================================================
    // Curve Properties
    //==========================================================================

    bool setAutomationTension(const juce::String& trackId,
                              const juce::String& paramId,
                              const juce::String& pointId,
                              float tension,
                              const juce::String& actionName = "Set Tension");

    bool setAutomationCurveType(const juce::String& trackId,
                                const juce::String& paramId,
                                const juce::String& pointId,
                                int curveType,
                                const juce::String& actionName = "Set Curve Type");

private:
    juce::ValueTree findAutomationPoint(const juce::ValueTree& envelope,
                                        const juce::String& pointId) const;
    
    juce::String generatePointId() const;

    ProjectState& projectState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationStateManager)
};

} // namespace zenith
