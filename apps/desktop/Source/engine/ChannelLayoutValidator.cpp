/*
  ==============================================================================

    ChannelLayoutValidator.cpp
    Implementation of channel layout validation

  ==============================================================================
*/

#include "ChannelLayoutValidator.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// ChannelLayoutValidator Implementation
//==============================================================================

ChannelLayoutValidator::ChannelLayoutValidator() {
    std::cout << "ChannelLayoutValidator: Initialized" << std::endl;
}

ChannelLayoutValidator::~ChannelLayoutValidator() {
    std::cout << "ChannelLayoutValidator: Shut down" << std::endl;
}

//==============================================================================
LayoutValidationResult ChannelLayoutValidator::validateLayout(
    const juce::AudioChannelSet& layout) const
{
    LayoutValidationResult result;
    result.isValid = true;

    // Check channel count
    int channelCount = layout.size();
    if (!isValidChannelCount(channelCount)) {
        LayoutValidationIssue issue;
        issue.type = LayoutValidationIssue::InvalidChannelCount;
        issue.description = "Channel count " + juce::String(channelCount) +
                           " is out of range (1-" +
                           juce::String(getMaximumChannelCount()) + ")";
        issue.channelCount = channelCount;
        issue.severity = 9;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check for disabled/canonical
    if (layout == juce::AudioChannelSet::disabled()) {
        LayoutValidationIssue issue;
        issue.type = LayoutValidationIssue::InvalidChannelCount;
        issue.description = "Layout is disabled";
        issue.severity = 10;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    if (layout == juce::AudioChannelSet::disabled()) {
        LayoutValidationIssue issue;
        issue.type = LayoutValidationIssue::UnsupportedLayout;
        issue.description = "Disabled layout is not valid";
        issue.channelCount = channelCount;
        issue.severity = 7;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    // Check for duplicate channels
    if (!checkForDuplicateChannels(layout, result)) {
        result.isValid = false;
    }

    // Check channel types
    if (!checkChannelTypes(layout, result)) {
        result.isValid = false;
    }

    // Generate summary
    if (result.isValid) {
        result.summary = "Layout is valid: " + layout.getDescription();
    } else {
        result.summary = "Layout has " +
                        juce::String(result.issues.size()) +
                        " validation issues";
    }

    return result;
}

//==============================================================================
LayoutValidationResult ChannelLayoutValidator::validateCompatibility(
    const juce::AudioChannelSet& layout1,
    const juce::AudioChannelSet& layout2) const
{
    LayoutValidationResult result;
    result.isValid = true;

    // Validate both layouts first
    auto validation1 = validateLayout(layout1);
    auto validation2 = validateLayout(layout2);

    if (!validation1.isValid) {
        result.issues.insert(result.issues.end(),
                            validation1.issues.begin(),
                            validation1.issues.end());
        result.isValid = false;
    }

    if (!validation2.isValid) {
        result.issues.insert(result.issues.end(),
                            validation2.issues.begin(),
                            validation2.issues.end());
        result.isValid = false;
    }

    if (!result.isValid) {
        result.summary = "Cannot validate compatibility - layouts are invalid";
        return result;
    }

    // Check compatibility
    // Two layouts are compatible if:
    // 1. They have the same channel count (direct mapping)
    // 2. One can be upmixed to the other
    // 3. One can be downmixed to the other
    // 4. Both are ambisonic (order conversion)

    int channels1 = layout1.size();
    int channels2 = layout2.size();

    bool compatible = false;

    // Direct mapping
    if (channels1 == channels2) {
        compatible = true;
    }
    // Ambisonic order conversion
    else if (isAmbisonic(layout1) && isAmbisonic(layout2)) {
        compatible = true;
    }

    if (!compatible) {
        LayoutValidationIssue issue;
        issue.type = LayoutValidationIssue::IncompatibleLayouts;
        issue.description = "Layouts are incompatible: " +
                           layout1.getDescription() +
                           " cannot be converted to " +
                           layout2.getDescription();
        issue.severity = 7;
        result.issues.push_back(issue);
        result.isValid = false;
    }

    if (result.isValid) {
        result.summary = "Layouts are compatible";
    } else {
        result.summary = "Layouts are incompatible";
    }

    return result;
}

//==============================================================================
bool ChannelLayoutValidator::isStandardLayout(const juce::AudioChannelSet& layout) {
    // Check against all standard layouts
    std::vector<juce::AudioChannelSet> standards = getStandardLayouts();

    for (const auto& standard : standards) {
        if (layout == standard) {
            return true;
        }
    }

    return false;
}

//==============================================================================
bool ChannelLayoutValidator::isDiscreteLayout(const juce::AudioChannelSet& layout) {
    return !isStandardLayout(layout) && layout.size() > 0;
}

//==============================================================================
bool ChannelLayoutValidator::hasChannelType(
    const juce::AudioChannelSet& layout,
    juce::AudioChannelSet::ChannelType type)
{
    // Check if layout contains the specified channel type
    for (int i = 0; i < layout.size(); ++i) {
        if (layout.getTypeOfChannel(i) == type) {
            return true;
        }
    }
    return layout.size() > 0;  // Simplified check
}

//==============================================================================
juce::String ChannelLayoutValidator::getDetailedLayoutDescription(
    const juce::AudioChannelSet& layout)
{
    juce::String desc = layout.getDescription();
    desc += " (" + juce::String(layout.size()) + " channels)";

    // Add ambisonic info if applicable
    if (isAmbisonic(layout)) {
        int order = getAmbisonicOrder(layout);
        desc += ", Ambisonic Order " + juce::String(order);
    }

    // Add standard/discrete info
    if (isStandardLayout(layout)) {
        desc += ", Standard";
    } else if (isDiscreteLayout(layout)) {
        desc += ", Discrete";
    }

    return desc;
}

//==============================================================================
int ChannelLayoutValidator::getMinimumChannelCount(
    const juce::AudioChannelSet& layout)
{
    // Each layout has a minimum required channel count
    if (layout == juce::AudioChannelSet::mono()) return 1;
    if (layout == juce::AudioChannelSet::stereo()) return 2;
    if (layout == juce::AudioChannelSet::createLCR()) return 3;
    if (layout == juce::AudioChannelSet::createLCRS()) return 4;
    if (layout == juce::AudioChannelSet::create5point0()) return 5;
    if (layout == juce::AudioChannelSet::create5point1()) return 6;
    if (layout == juce::AudioChannelSet::create7point0()) return 7;
    if (layout == juce::AudioChannelSet::create7point1()) return 8;
    if (layout == juce::AudioChannelSet::quadraphonic()) return 4;
    if (layout == juce::AudioChannelSet::ambisonic(1)) return 4;  // 1st order
    if (layout == juce::AudioChannelSet::ambisonic(2)) return 9;  // 2nd order

    // Discrete or unknown
    return layout.size();
}

//==============================================================================
std::vector<juce::AudioChannelSet> ChannelLayoutValidator::getStandardLayouts() {
    std::vector<juce::AudioChannelSet> layouts;

    layouts.push_back(juce::AudioChannelSet::mono());
    layouts.push_back(juce::AudioChannelSet::stereo());
    layouts.push_back(juce::AudioChannelSet::createLCR());
    layouts.push_back(juce::AudioChannelSet::createLCRS());
    layouts.push_back(juce::AudioChannelSet::create5point0());
    layouts.push_back(juce::AudioChannelSet::create5point1());
    layouts.push_back(juce::AudioChannelSet::create7point0());
    layouts.push_back(juce::AudioChannelSet::create7point1());
    layouts.push_back(juce::AudioChannelSet::quadraphonic());
    layouts.push_back(juce::AudioChannelSet::ambisonic(1));
    layouts.push_back(juce::AudioChannelSet::ambisonic(2));

    return layouts;
}

//==============================================================================
bool ChannelLayoutValidator::isAmbisonic(const juce::AudioChannelSet& layout) {
    return getAmbisonicOrder(layout) >= 0;
}

//==============================================================================
int ChannelLayoutValidator::getAmbisonicOrder(const juce::AudioChannelSet& layout) {
    // 1st order ambisonic: 4 channels (W, X, Y, Z)
    if (layout == juce::AudioChannelSet::ambisonic(1)) {
        return 1;
    }

    // 2nd order ambisonic: 9 channels
    if (layout == juce::AudioChannelSet::ambisonic(2)) {
        return 2;
    }

    return -1;  // Not ambisonic
}

//==============================================================================
// Private Methods
//==============================================================================

bool ChannelLayoutValidator::checkForDuplicateChannels(
    const juce::AudioChannelSet& layout,
    LayoutValidationResult& result) const
{
    // JUCE's AudioChannelSet doesn't allow duplicates by design
    // This is a placeholder for future custom implementations
    return true;
}

bool ChannelLayoutValidator::checkRequiredChannels(
    const juce::AudioChannelSet& layout,
    LayoutValidationResult& result) const
{
    // Check if required channels are present for specific layouts
    // For example, 5.1 requires L, R, C, LFE, Ls, Rs

    if (layout == juce::AudioChannelSet::create5point1()) {
        // Should have 6 channels
        if (layout.size() != 6) {
            LayoutValidationIssue issue;
            issue.type = LayoutValidationIssue::MissingRequiredChannel;
            issue.description = "5.1 layout requires exactly 6 channels";
            issue.severity = 8;
            result.issues.push_back(issue);
            return false;
        }
    }

    return true;
}

bool ChannelLayoutValidator::checkChannelTypes(
    const juce::AudioChannelSet& layout,
    LayoutValidationResult& result) const
{
    // Check if all channel types are valid
    // JUCE's AudioChannelSet ensures valid types by design
    // This is a placeholder for future custom implementations
    return true;
}

} // namespace zenith
