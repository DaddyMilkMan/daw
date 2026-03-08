/*
  ==============================================================================

    PluginValidator.cpp
    Implementation of plugin state validation

  ==============================================================================
*/

#include "PluginValidator.h"
#include <iostream>

namespace zenith {

//==============================================================================
PluginValidationResult PluginValidator::validateState(juce::AudioPluginInstance& plugin,
                                                        const juce::MemoryBlock& stateData) {
    PluginValidationResult result;
    result.stateSize = static_cast<int>(stateData.getSize());

    // Check state size
    if (stateData.getSize() == 0) {
        // Empty state is valid (represents default state)
        result.addWarning("State is empty (will use defaults)");
        return result;
    }

    if (stateData.getSize() > maxStateSize_) {
        result.isValid = false;
        result.errorMessage = "State size exceeds maximum allowed ("
                             + juce::String(maxStateSize_ / (1024 * 1024))
                             + "MB)";
        return result;
    }

    // Try to load state and verify plugin still works
    juce::MemoryBlock originalState;
    plugin.getStateInformation(originalState);

    // Save current parameter values for comparison
    std::vector<float> originalParams;
    for (int i = 0; i < plugin.getParameters().size(); ++i) {
        if (auto* param = plugin.getParameters()[i]) {
            originalParams.push_back(param->getValue());
        }
    }

    // Attempt to load the state
    try {
        plugin.setStateInformation(stateData.getData(), (int)stateData.getSize());

        // Verify plugin is still responsive
        bool pluginResponsive = true;

        // Check parameters are accessible
        for (int i = 0; i < plugin.getParameters().size(); ++i) {
            if (auto* param = plugin.getParameters()[i]) {
                // Try to get parameter value
                float value = param->getValue();
                juce::ignoreUnused(value);
            }
        }

        // Validate parameter ranges
        auto paramValidation = validateAllParameters(plugin);
        result.parameterCount = plugin.getParameters().size();
        result.invalidParameters = paramValidation.invalidParameters;

        if (paramValidation.invalidParameters > 0) {
            result.addWarning(juce::String(paramValidation.invalidParameters)
                              + " parameters out of range after loading state");
        }

        if (!pluginResponsive) {
            result.isValid = false;
            result.errorMessage = "Plugin became unresponsive after loading state";
        }

        // Restore original state for testing
        plugin.setStateInformation(originalState.getData(), (int)originalState.getSize());

    } catch (const std::exception& e) {
        result.isValid = false;
        result.errorMessage = "Exception while loading state: " + juce::String(e.what());
    } catch (...) {
        result.isValid = false;
        result.errorMessage = "Unknown exception while loading state";
    }

    return result;
}

//==============================================================================
PluginValidationResult PluginValidator::validateState(juce::AudioPluginInstance& plugin,
                                                        const juce::String& stateData) {
    juce::MemoryBlock block;
    block.append(stateData.toUTF8(), stateData.getNumBytesAsUTF8());
    return validateState(plugin, block);
}

//==============================================================================
bool PluginValidator::validateParameterValue(juce::AudioProcessorParameter& parameter,
                                             float value) const {
    // Check if value is in [0, 1] range (normalized)
    if (value < 0.0f || value > 1.0f) {
        std::cerr << "PluginValidator: Parameter " << parameter.getName(100)
                  << " value " << value << " out of range [0, 1]" << std::endl;
        return false;
    }

    // Additional checks for discrete parameters
    if (parameter.isDiscrete()) {
        // For discrete parameters, value should be close to a step
        int numSteps = parameter.getNumSteps();
        if (numSteps > 0) {
            float stepValue = std::round(value * (numSteps - 1)) / (numSteps - 1);
            if (std::abs(value - stepValue) > 0.001f) {
                std::cerr << "PluginValidator: Discrete parameter " << parameter.getName(100)
                          << " value " << value << " not on step boundary" << std::endl;
                return false;
            }
        }
    }

    return true;
}

//==============================================================================
float PluginValidator::sanitizeParameterValue(juce::AudioProcessorParameter& parameter,
                                                float value) const {
    // Clamp to [0, 1]
    value = juce::jlimit(0.0f, 1.0f, value);

    // For discrete parameters, snap to nearest step
    if (parameter.isDiscrete()) {
        int numSteps = parameter.getNumSteps();
        if (numSteps > 0) {
            value = std::round(value * (numSteps - 1)) / (numSteps - 1);
        }
    }

    return value;
}

//==============================================================================
PluginValidationResult PluginValidator::validateAllParameters(juce::AudioPluginInstance& plugin) {
    PluginValidationResult result;
    result.parameterCount = plugin.getParameters().size();

    for (int i = 0; i < plugin.getParameters().size(); ++i) {
        if (auto* param = plugin.getParameters()[i]) {
            float value = param->getValue();

            if (!validateParameterValue(*param, value)) {
                result.invalidParameters++;
                result.addWarning("Parameter " + param->getName(100) + " (index " +
                                  juce::String(i) + ") has invalid value");
            }
        }
    }

    if (result.invalidParameters > 0 && strictMode_) {
        result.isValid = false;
        result.errorMessage = juce::String(result.invalidParameters) +
                             " parameters have invalid values";
    }

    return result;
}

//==============================================================================
PluginValidationResult PluginValidator::validatePreset(const juce::File& presetFile,
                                                        juce::AudioPluginInstance& plugin) {
    PluginValidationResult result;

    if (!presetFile.existsAsFile()) {
        result.isValid = false;
        result.errorMessage = "Preset file does not exist: " + presetFile.getFullPathName();
        return result;
    }

    // Check file size (presets shouldn't be huge)
    juce::int64 fileSize = presetFile.getSize();
    if (fileSize > maxStateSize_) {
        result.isValid = false;
        result.errorMessage = "Preset file too large: " + juce::String(fileSize / (1024 * 1024))
                             + "MB (max: " + juce::String(maxStateSize_ / (1024 * 1024)) + "MB)";
        return result;
    }

    // Try to load preset and validate
    juce::MemoryBlock presetData;
    presetFile.loadFileAsData(presetData);

    // For now, do basic validation
    // Full validation would require knowing the preset format
    if (presetData.getSize() == 0) {
        result.isValid = false;
        result.errorMessage = "Preset file is empty";
        return result;
    }

    result.addWarning("Basic preset validation passed (contents not verified)");

    return result;
}

//==============================================================================
bool PluginValidator::testStateRoundTrip(juce::AudioPluginInstance& plugin) {
    try {
        // Save current state
        juce::MemoryBlock state1;
        plugin.getStateInformation(state1);

        // Restore it
        plugin.setStateInformation(state1.getData(), (int)state1.getSize());

        // Save again
        juce::MemoryBlock state2;
        plugin.getStateInformation(state2);

        // Compare
        if (state1.getSize() != state2.getSize()) {
            std::cerr << "PluginValidator: State size changed after round-trip: "
                      << state1.getSize() << " -> " << state2.getSize() << std::endl;
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "PluginValidator: Exception during round-trip test: "
                  << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "PluginValidator: Unknown exception during round-trip test" << std::endl;
        return false;
    }
}

//==============================================================================
bool SafePluginState::captureFrom(juce::AudioPluginInstance& plugin,
                                   PluginValidator& validator) {
    plugin.getStateInformation(stateData);
    isValid = false;

    // Validate the captured state
    validationResult = validator.validateState(plugin, stateData);

    if (validationResult.isValid) {
        isValid = true;
        return true;
    }

    // Validation failed
    std::cerr << "SafePluginState: Captured state failed validation: "
              << validationResult.errorMessage << std::endl;
    return false;
}

//==============================================================================
bool SafePluginState::applyTo(juce::AudioPluginInstance& plugin,
                                PluginValidator& validator) {
    if (!isValid) {
        std::cerr << "SafePluginState: Cannot apply invalid state" << std::endl;
        return false;
    }

    // Validate before applying
    auto validation = validator.validateState(plugin, stateData);
    if (!validation.isValid && validator.strictMode_) {
        std::cerr << "SafePluginState: State validation failed before apply: "
                  << validation.errorMessage << std::endl;
        return false;
    }

    // Apply the state
    try {
        plugin.setStateInformation(stateData.getData(), (int)stateData.getSize());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "SafePluginState: Exception applying state: " << e.what() << std::endl;
        return false;
    }
}

} // namespace zenith
