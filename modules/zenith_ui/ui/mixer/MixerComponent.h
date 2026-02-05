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

/*
    ==============================================================================
    Original file header:
*/

 * @file MixerComponent.h
 * @brief Full-featured Mixer panel component for Zenith DAW
 *
 * Features:
 * - Horizontal scrolling viewport for many tracks
 * - Master channel strip (wider, prominent)
 * - Track channel strips with full controls
 * - Glassmorphic panel design
 * - Selection glow on active channel

 * - Smooth animations
 *
 * This component acts as a container for MixerChannelComponents and
 * manages the overall mixer layout including the master channel.
 */

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include "ZenithDesignSystem.h"
#include "MixerChannelComponent.h"


#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @class MixerComponent
 * @brief The main mixer view containing all channel strips
 *
 * Displays a horizontally scrollable array of channel strips for each track,
 * plus a master channel strip on the right side. Uses Skia for GPU-accelerated
 * rendering with the Neon Noir glassmorphism design system.
 */
class MixerComponent : public SkiaComponent, public juce::ValueTree::Listener {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  /**
   * @brief Constructor
   * @param engine Reference to the audio engine (for Track access)
   * @param state Reference to project state (for listeners/order)
   */
  MixerComponent(Engine &engine, ProjectState &state);
  ~MixerComponent() override;

  //==========================================================================
  // Component interface (Pure Skia - no JUCE paint override)
  //==========================================================================

  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

  //==========================================================================
  // ValueTree::Listener interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

  //==========================================================================
  // Selection Management
  //==========================================================================

  /** Select a channel by track ID */
  void selectChannel(const juce::String &trackId);

  /** Get the currently selected track ID */
  juce::String getSelectedTrackId() const { return selectedTrackId_; }

  /** Callback when selection changes */
  std::function<void(const juce::String &)> onSelectionChanged;

  //==========================================================================
  // Keyboard Navigation (WCAG 2.1.1 Keyboard)
  //==========================================================================

  bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
  
  /** Navigate to next channel (right arrow) */
  void selectNextChannel();
  
  /** Navigate to previous channel (left arrow) */
  void selectPreviousChannel();
  
  /** Get index of currently selected channel (-1 if none, numChannels for master) */
  int getSelectedChannelIndex() const;

private:
  //==========================================================================
  // Internal methods
  //==========================================================================

  void rebuildChannels();
  Track *findTrackById(const juce::String &trackId);
  void updateSelection();

  //==========================================================================
  // Inner class: ChannelContainer
  //==========================================================================

  /**
   * @class ChannelContainer
   * @brief Container for track channel strips (used inside viewport)
   */
  class ChannelContainer : public SkiaComponent {
  public:
    ChannelContainer();
    void drawSkia(SkCanvas *canvas) override;
    void resized() override;

    void addChannel(std::unique_ptr<MixerChannelComponent> channel);
    void clearChannels();
    int getChannelCount() const { return static_cast<int>(channels_.size()); }
    MixerChannelComponent *getChannel(int index);

    void layoutChannels(int stripWidth, int stripSpacing, int topMargin,
                        int bottomMargin);
    int getTotalWidth(int stripWidth, int stripSpacing, int sideMargin) const;

  private:
    std::vector<std::unique_ptr<MixerChannelComponent>> channels_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelContainer)
  };

  //==========================================================================
  // Member variables
  //==========================================================================

  Engine &engine_;
  ProjectState &projectState_;

  // Horizontal scrolling viewport for track channels
  juce::Viewport trackViewport_;
  std::unique_ptr<ChannelContainer> trackContainer_;

  // Master channel strip (outside viewport, always visible on right)
  std::unique_ptr<MixerChannelComponent> masterChannel_;

  // Selection state
  juce::String selectedTrackId_;

  // Layout constants - Use design system tokens (no magic numbers!)
  // Anti-Corner-Cutting: These must reference ZenithDesignSystem values
  static constexpr int stripWidth = design::dimensions::MIXER_CHANNEL_WIDTH;
  static constexpr int masterStripWidth = design::dimensions::MIXER_MASTER_WIDTH;
  static constexpr int stripSpacing = design::dimensions::MIXER_CHANNEL_SPACING;
  static constexpr int topMargin = static_cast<int>(design::spacing::SM);
  static constexpr int bottomMargin = static_cast<int>(design::spacing::SM);
  static constexpr int sideMargin = static_cast<int>(design::spacing::SM);
  static constexpr int dividerWidth = design::dimensions::DIVIDER_WIDTH;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};

} // namespace zenith
