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

// MixerChannelComponent.h


#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../controls/SkiaButton.h"
#include "../controls/SkiaKnob.h"
#include "../controls/SkiaSlider.h"
#include "../controls/SkiaSpectrumComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../framework/SkiaComponent.h"

namespace zenith {
class Track;
class Engine;
class ProjectState;

/**
 * @class MixerChannelComponent
 * @brief A single channel strip in the mixer view
 *
 * Displays all controls for a single track: fader, pan, mute/solo/arm,
 * level meter, insert slots, and send levels. Uses Skia for GPU-accelerated
 * rendering with the Neon Noir glassmorphism design system.
 */
class MixerChannelComponent : public SkiaComponent,
                              public juce::ChangeListener {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  /**
   * @param isMaster If true, this is the master channel strip (wider, different
   * styling)
   */
  MixerChannelComponent(Track *track, ProjectState& state, Engine& engine, bool isMaster = false);
  ~MixerChannelComponent() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  //==========================================================================
  // ChangeListener interface
  //==========================================================================

  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

  //==========================================================================
  // Accessors
  //==========================================================================

  Engine& getEngine() { return engine_; }
  Track *getTrack() const { return track_; }
  bool isMasterChannel() const { return isMaster_; }

  /** Update UI controls from track state */
  void updateFromTrack();

  /** Get the spectrum analyzer's audio FIFO for feeding audio data */
  AudioFifo *getSpectrumFifo() const {
    if (spectrumAnalyzer_)
      return &spectrumAnalyzer_->getAudioFifo();
    return nullptr;
  }

  //==========================================================================
  // Selection State
  //==========================================================================

  void setSelected(bool selected);
  bool isSelected() const { return isSelected_; }

  /** Callback when the channel strip itself is clicked (for selection) */
  std::function<void()> onClick;

  void mouseDown(const juce::MouseEvent &e) override;

  //==========================================================================
  // Focus & Accessibility (WCAG 2.1)
  //==========================================================================

  void focusGained(juce::Component::FocusChangeType cause) override;
  void focusLost(juce::Component::FocusChangeType cause) override;
  bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
  
  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
  //==========================================================================
  // Timer callback for meter updates
  //==========================================================================

  void onAnimationTick(float deltaMs) override;

  //==========================================================================
  // Control callbacks
  //==========================================================================

  void onFaderChanged();
  void onPanChanged();
  void onMuteClicked();
  void onSoloClicked();
  void onArmClicked();

  //==========================================================================
  // Internal nested class: LevelMeter
  //==========================================================================

  class LevelMeter : public SkiaComponent {
  public:
    LevelMeter();
    ~LevelMeter() override;
    void drawSkia(SkCanvas *canvas) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void setLevel(float level);
    void onAnimationTick(float deltaMs) override;
    
    /** Set stereo mode for dual meters */
    void setStereo(bool stereo) { stereo_ = stereo; }
    void setLeftLevel(float level) {
      targetLevelL_.store(juce::jlimit(0.0f, 1.0f, level));
    }
    void setRightLevel(float level) {
      targetLevelR_.store(juce::jlimit(0.0f, 1.0f, level));
    }
    
  private:
    void drawMeterBar(SkCanvas *canvas, const SkRect &bounds, float level,
                      float peak);
                      
    std::atomic<float> targetLevel_{0.0f};
    std::atomic<float> targetLevelL_{0.0f};
    std::atomic<float> targetLevelR_{0.0f};
    float currentLevel_{0.0f};
    float currentLevelL_{0.0f};
    float currentLevelR_{0.0f};
    float peakLevel_{0.0f};
    float peakLevelL_{0.0f};
    float peakLevelR_{0.0f};
    float peakHoldMs_{0.0f};
    float peakHoldMsL_{0.0f};
    float peakHoldMsR_{0.0f};

    float velocity_{0.0f};
    float velocityL_{0.0f};
    float velocityR_{0.0f};
    bool stereo_{false};


  };

  //==========================================================================
  // Internal nested class: InsertSlotIndicator
  //==========================================================================

  class InsertSlotIndicator : public SkiaComponent {
  public:
    InsertSlotIndicator(MixerChannelComponent& owner, int slotIndex);
    void drawSkia(SkCanvas *canvas) override;
    void setOccupied(bool occupied, const juce::String &pluginName = "");
    bool isOccupied() const { return isOccupied_; }
    void mouseDown(const juce::MouseEvent& e) override;

  private:
    MixerChannelComponent& owner_;
    int slotIndex_;
    bool isOccupied_{false};
    juce::String pluginName_;
  };

  //==========================================================================
  // Internal nested class: SendIndicator
  //==========================================================================

  class SendIndicator : public SkiaComponent {
  public:
    SendIndicator(MixerChannelComponent& owner, int sendIndex);
    void drawSkia(SkCanvas *canvas) override;
    void setSendLevel(float level);
    void setDestination(const juce::String &destName);
    void mouseDown(const juce::MouseEvent& e) override;

  private:
    MixerChannelComponent& owner_;
    int sendIndex_;
    float sendLevel_{0.0f};
    juce::String destinationName_;
  };

  //==========================================================================
  // Member variables
  //==========================================================================

  Track *track_;
  ProjectState& projectState_;
  Engine& engine_;
  bool isMaster_{false};
  bool isSelected_{false};

  // Track name (editable label)
  juce::Label nameLabel_;

  // Main controls
  SkiaSlider faderSlider_;
  SkiaKnob panKnob_;
  SkiaButton muteButton_;
  SkiaButton soloButton_;
  SkiaButton armButton_; // Record arm

  // Spectrum analyzer
  std::unique_ptr<SkiaSpectrumComponent> spectrumAnalyzer_;

  // Level meter
  LevelMeter meter_;

  // Insert slots (8 total)
  std::vector<std::unique_ptr<InsertSlotIndicator>> insertSlots_;

  // Send indicators (4 total)
  std::vector<std::unique_ptr<SendIndicator>> sendIndicators_;

  // Layout bounds for headers
  SkRect insertHeaderBounds_;
  SkRect sendHeaderBounds_;

  // State
  bool updatingControls_{false};
  bool hasFocus_{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

} // namespace zenith
