/*
  ==============================================================================

    AutomationStateManager.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Automation management implementation.

  ==============================================================================
*/

#include "AutomationStateManager.h"
#include "ProjectState.h"

namespace zenith {

AutomationStateManager::AutomationStateManager(ProjectState& projectState)
    : projectState_(projectState)
{
}

//==============================================================================
// Automation Access
//==============================================================================

juce::ValueTree AutomationStateManager::getOrCreateAutomationEnvelope(const juce::String& trackId,
                                                                      const juce::String& paramId)
{
    auto track = projectState_.getTrack(trackId);
    if (!track.isValid())
        return {};

    auto automation = track.getChildWithName(ProjectState::ID_AUTOMATION);
    if (!automation.isValid())
    {
        automation = juce::ValueTree(ProjectState::ID_AUTOMATION);
        track.addChild(automation, -1, &projectState_.getUndoManager());
    }

    // Look for existing envelope
    for (auto envelope : automation)
    {
        if (envelope.getProperty(ProjectState::PROP_PARAM_ID).toString() == paramId)
            return envelope;
    }

    // Create new
    juce::ValueTree envelope(ProjectState::ID_ENVELOPE);
    envelope.setProperty(ProjectState::PROP_PARAM_ID, paramId, nullptr);
    envelope.setProperty(ProjectState::PROP_ID, "env_" + juce::Uuid().toString().substring(0, 8), nullptr);
    
    // Create points container
    juce::ValueTree points(ProjectState::ID_POINTS);
    envelope.addChild(points, -1, nullptr);

    automation.addChild(envelope, -1, &projectState_.getUndoManager());
    return envelope;
}

juce::ValueTree AutomationStateManager::getAutomationEnvelope(const juce::String& trackId,
                                                              const juce::String& paramId) const
{
    auto track = projectState_.getTrack(trackId);
    if (!track.isValid())
        return {};

    auto automation = track.getChildWithName(ProjectState::ID_AUTOMATION);
    if (!automation.isValid())
        return {};

    for (auto envelope : automation)
    {
        if (envelope.getProperty(ProjectState::PROP_PARAM_ID).toString() == paramId)
            return envelope;
    }
    return {};
}

bool AutomationStateManager::hasAutomation(const juce::String& trackId, const juce::String& paramId) const
{
    return getAutomationEnvelope(trackId, paramId).isValid();
}

//==============================================================================
// Point Management
//==============================================================================

juce::String AutomationStateManager::addAutomationPoint(const juce::String& trackId,
                                                        const juce::String& paramId,
                                                        double timeBeats,
                                                        double value,
                                                        float tension,
                                                        int curveType,
                                                        const juce::String& actionName)
{
    auto envelope = getOrCreateAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return {};

    auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
    if (!points.isValid())
    {
        points = juce::ValueTree(ProjectState::ID_POINTS);
        envelope.addChild(points, -1, &projectState_.getUndoManager());
    }

    projectState_.getUndoManager().beginNewTransaction(actionName);

    juce::String pointId = generatePointId();
    juce::ValueTree point(ProjectState::ID_POINT);
    point.setProperty(ProjectState::PROP_ID, pointId, nullptr);
    point.setProperty(ProjectState::PROP_TIME_BEATS, timeBeats, nullptr);
    point.setProperty(ProjectState::PROP_VALUE, value, nullptr);
    point.setProperty(ProjectState::PROP_TENSION, tension, nullptr);
    point.setProperty(ProjectState::PROP_CURVE_TYPE, curveType, nullptr);

    // Insert sorted
    int insertIndex = 0;
    while (insertIndex < points.getNumChildren())
    {
        double t = points.getChild(insertIndex)[ProjectState::PROP_TIME_BEATS];
        if (t > timeBeats)
            break;
        insertIndex++;
    }

    points.addChild(point, insertIndex, &projectState_.getUndoManager());
    return pointId;
}

bool AutomationStateManager::moveAutomationPoint(const juce::String& trackId,
                                                 const juce::String& paramId,
                                                 const juce::String& pointId,
                                                 double newTimeBeats,
                                                 double newValue,
                                                 const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);

    // If time changed, we might need to reorder. 
    // Simplest approach: update properties, then sort.
    // But ValueTree doesn't auto-sort. We should remove and re-insert if order changes.
    // For now, let's just set properties and assume caller handles complexity or we rely on robust rendering.
    // Actually, let's re-insert to keep sorted.
    
    double oldTime = point[ProjectState::PROP_TIME_BEATS];
    point.setProperty(ProjectState::PROP_VALUE, newValue, &projectState_.getUndoManager());

    if (std::abs(oldTime - newTimeBeats) > 0.0001)
    {
        // Re-insert
        auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
        points.removeChild(point, &projectState_.getUndoManager());
        point.setProperty(ProjectState::PROP_TIME_BEATS, newTimeBeats, nullptr); // no undo for internal state change during re-insert

        int insertIndex = 0;
        int numPoints = points.getNumChildren();
        while (insertIndex < numPoints)
        {
            double t = points.getChild(insertIndex)[ProjectState::PROP_TIME_BEATS];
            if (t > newTimeBeats)
                break;
            insertIndex++;
        }
        points.addChild(point, insertIndex, &projectState_.getUndoManager());
    }

    return true;
}

bool AutomationStateManager::deleteAutomationPoint(const juce::String& trackId,
                                                   const juce::String& paramId,
                                                   const juce::String& pointId,
                                                   const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    auto points = point.getParent();
    points.removeChild(point, &projectState_.getUndoManager());
    return true;
}

bool AutomationStateManager::clearAutomation(const juce::String& trackId,
                                             const juce::String& paramId,
                                             const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid())
        return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    
    // Remove the envelope entirely
    auto automation = envelope.getParent();
    automation.removeChild(envelope, &projectState_.getUndoManager());
    
    return true;
}

//==============================================================================
// Curve Properties
//==============================================================================

bool AutomationStateManager::setAutomationTension(const juce::String& trackId,
                                                  const juce::String& paramId,
                                                  const juce::String& pointId,
                                                  float tension,
                                                  const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid()) return false;
    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid()) return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    point.setProperty(ProjectState::PROP_TENSION, tension, &projectState_.getUndoManager());
    return true;
}

bool AutomationStateManager::setAutomationCurveType(const juce::String& trackId,
                                                    const juce::String& paramId,
                                                    const juce::String& pointId,
                                                    int curveType,
                                                    const juce::String& actionName)
{
    auto envelope = getAutomationEnvelope(trackId, paramId);
    if (!envelope.isValid()) return false;
    auto point = findAutomationPoint(envelope, pointId);
    if (!point.isValid()) return false;

    projectState_.getUndoManager().beginNewTransaction(actionName);
    point.setProperty(ProjectState::PROP_CURVE_TYPE, curveType, &projectState_.getUndoManager());
    return true;
}

//==============================================================================
// Internal Helpers
//==============================================================================

juce::ValueTree AutomationStateManager::findAutomationPoint(const juce::ValueTree& envelope,
                                                            const juce::String& pointId) const
{
    auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
    for (auto p : points)
    {
        if (p.getProperty(ProjectState::PROP_ID).toString() == pointId)
            return p;
    }
    return {};
}

juce::String AutomationStateManager::generatePointId() const
{
    return "pt_" + juce::Uuid().toString().substring(0, 8);
}

} // namespace zenith
