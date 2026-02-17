/**
 * @file MixerChannelComponent.h
 * @brief Full-featured mixer channel strip UI component
 *
 * Features:
 * - Vertical fader with metallic handle
 * - Pan knob with center detent
 * - Solo/Mute/Record arm toggle buttons
 * - Peak meter with gradient (green→yellow→red)
 * - Track name label (editable)
 * - 8 insert slot indicators
 * - Send level indicators
 * - Glassmorphic panel design
 * - Accent glow when selected
 * - Smooth animations via spring physics
 */

#pragma once

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
                              public juce::ChangeListener,
                              public juce::DragAndDropTarget {
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
  bool isInterestedInDragSource(
      const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragEnter(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragMove(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragExit(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDropped(const juce::DragAndDropTarget::SourceDetails &details) override;

private:
  //==========================================================================
  // Timer callback for meter updates
  //==========================================================================

  void timerCallback() override;

  //==========================================================================
  // Control callbacks
  //==========================================================================

  void onFaderChanged();
  void onPanChanged();
  void onMuteClicked();
  void onSoloClicked();
  void onArmClicked();

  //==========================================================================
  // Drawing helpers
  //==========================================================================

  void drawInsertSlots(SkCanvas *canvas, const SkRect &bounds);
  void drawSendIndicators(SkCanvas *canvas, const SkRect &bounds);
  int getInsertSlotIndexAt(juce::Point<int> localPosition) const;
  bool tryInsertPluginFromDragDescription(const juce::String &description,
                                          int slotIndex);

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
    void timerCallback() override;

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
    int peakHoldCounter_{0};
    int peakHoldCounterL_{0};
    int peakHoldCounterR_{0};

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
  bool isDropTargetActive_{false};
  int dragHoverInsertSlot_{-1};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

} // namespace zenith
