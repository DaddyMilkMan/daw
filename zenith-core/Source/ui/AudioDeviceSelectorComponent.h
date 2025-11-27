/**
 * @file AudioDeviceSelectorComponent.h
 * @brief Audio device selection with input/output routing
 *
 * Features:
 * - Dropdown device selector with smooth animations
 * - Sample rate selector (44.1kHz, 48kHz, 96kHz, 192kHz)
 * - Buffer size selector for latency control
 * - Real-time device status indicator
 * - Audio I/O channel count display
 * - Driver information display
 * - Device latency monitoring
 * - Auto-refresh device list
 * - Apple-style compact design
 *
 * @phase Phase 1: Audio Setup
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

class Engine;

namespace zenith {

/**
 * @class AudioDeviceSelectorComponent
 * @brief Audio device and settings selector
 */
class AudioDeviceSelectorComponent : public juce::Component,
                                     public juce::ComboBox::Listener,
                                     public juce::Timer
{
public:
    //==========================================================================
    explicit AudioDeviceSelectorComponent(Engine& eng);
    ~AudioDeviceSelectorComponent() override;

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // ComboBox::Listener
    //==========================================================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    //==========================================================================
    // Methods
    //==========================================================================

    /**
     * @brief Refresh available devices list
     */
    void refreshDeviceList();

    /**
     * @brief Get currently selected device name
     */
    juce::String getSelectedDevice() const;

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    /**
     * @brief Paint device status indicator (green/red circle)
     */
    void paintStatusIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    /**
     * @brief Paint settings row
     */
    void paintSettingRow(juce::Graphics& g, const juce::String& label,
                        const juce::Rectangle<int>& bounds);

    //==========================================================================
    // Members
    //==========================================================================

    Engine& engine_;

    // Device selector
    juce::ComboBox deviceSelector_;

    // Sample rate selector
    juce::ComboBox sampleRateSelector_;

    // Buffer size selector
    juce::ComboBox bufferSizeSelector_;

    // Status labels
    juce::Label statusLabel_;
    juce::Label latencyLabel_;
    juce::Label channelLabel_;

    // Device status (green if connected, red if error)
    bool deviceConnected_ = true;

    // Layout constants
    static constexpr int ROW_HEIGHT = 32;
    static constexpr int LABEL_WIDTH = 100;
    static constexpr int SPACING = 8;
    static constexpr int PADDING = 12;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceSelectorComponent)
};

}  // namespace zenith

