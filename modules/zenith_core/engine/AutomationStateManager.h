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

    //==========================================================================
    // Plugin Parameter Automation
    //==========================================================================

    /**
     * @brief Create or get automation envelope for a plugin parameter
     * @param trackId Track ID
     * @param pluginIndex Index of the plugin in the track's chain
     * @param parameterIndex Index of the parameter in the plugin
     * @param parameterName Human-readable parameter name (for display)
     * @return Parameter ID string (format: "plugin_X_Y")
     */
    juce::String createPluginParameterAutomation(const juce::String& trackId,
                                                  int pluginIndex,
                                                  int parameterIndex,
                                                  const juce::String& parameterName);

    /**
     * @brief Check if a parameter ID refers to a plugin parameter
     * @param paramId Parameter ID to check
     * @return true if it's a plugin parameter (starts with "plugin_")
     */
    static bool isPluginParameter(const juce::String& paramId) {
        return paramId.startsWith("plugin_");
    }

    /**
     * @brief Parse plugin parameter ID
     * @param paramId Parameter ID in format "plugin_X_Y"
     * @param outPluginIndex Output: plugin index
     * @param outParamIndex Output: parameter index
     * @return true if successfully parsed
     */
    static bool parsePluginParameterId(const juce::String& paramId,
                                        int& outPluginIndex,
                                        int& outParamIndex);


private:
    juce::ValueTree findAutomationPoint(const juce::ValueTree& envelope,
                                        const juce::String& pointId) const;
    
    juce::String generatePointId() const;

    ProjectState& projectState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationStateManager)
};

} // namespace zenith
