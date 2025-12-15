/*
  ==============================================================================

    DeviceChainComponent.h
    Created: 2025-12-13
    Author:  Zenith AI

    Horizontal strip of device slots (Plugins/Instruments) for the selected
  track. Features:
    - Horizontal scrollable list
    - Device slots with name, bypass, and macros
    - Mini-Views for native plugins (EQ curve, etc)
    - Drag-and-drop reordering

  ==============================================================================
*/

#pragma once

#include "../../../include/Engine.h" // Corrected include path
#include "../../../include/ProjectState.h"
#include "../../engine/Track.h"
#include "SkiaComponent.h"
#include "SkiaKnob.h"
#include <JuceHeader.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <vector>


class Engine; // Forward declaration

namespace zenith {

class DeviceSlotComponent;

class DeviceChainComponent : public SkiaComponent,
                             public juce::ValueTree::Listener {
public:
  DeviceChainComponent(Engine &engine, ProjectState &state);
  ~DeviceChainComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // ValueTree::Listener overrides
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

  void setTrack(Track *track);

private:
  Engine &engine_;
  ProjectState &projectState_;
  Track *currentTrack_ = nullptr;

  // UI Resources
  ::SkPaint bgPaint_;
  ::SkFont labelFont_;

  juce::Viewport viewport_;
  std::unique_ptr<juce::Component> contentContainer_;
  std::vector<std::unique_ptr<DeviceSlotComponent>> deviceSlots_;

  void updateTrackFromSelection();
  void rebuildSlots();
  Track *findTrackById(const juce::String &trackId);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceChainComponent)
};

} // namespace zenith
