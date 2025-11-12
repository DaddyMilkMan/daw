/**
 * @file AudioSettingsWindows.h
 * @brief Windows-specific audio device settings panel (WASAPI/ASIO tuning surface)
 *
 * STEP W3: UI stub for audio device configuration
 * - Device selection (output/input)
 * - Mode detection (Shared/Exclusive - automatic)
 * - Sample rate selection
 * - Buffer size selection
 * - ASIO control panel button (when applicable)
 * - Status line with latency info
 *
 * Thread Safety:
 * - All UI interactions on MESSAGE THREAD
 * - Device changes require engine stop/restart
 */

#pragma once

#ifdef _WIN32

#include <JuceHeader.h>
#include "ZenithLookAndFeel.h"

//==============================================================================
/**
 * @class AudioSettingsWindows
 * @brief Windows audio settings panel for WASAPI/ASIO configuration
 *
 * Layout:
 * ┌─────────────────────────────────────────┐
 * │  Audio Device Settings                  │
 * ├─────────────────────────────────────────┤
 * │  Device Type:  [WASAPI ▼]               │
 * │  Output Device: [Speakers (Realtek) ▼]  │
 * │  Input Device:  [Microphone (Realtek)▼] │
 * │  Mode:         Shared (automatic)       │
 * │  Sample Rate:  [48000 Hz ▼]             │
 * │  Buffer Size:  [256 samples ▼]          │
 * │  Latency:      ~5.3 ms                  │
 * │                                          │
 * │  [Open ASIO Panel...]  (if ASIO)        │
 * │                                          │
 * │  Status: Ready                          │
 * └─────────────────────────────────────────┘
 *
 * W3 Scope: Stub UI only - no engine mutation yet
 */
class AudioSettingsWindows : public juce::Component
{
public:
    //==========================================================================
    /**
     * @brief Callback when device settings changed (user action)
     *
     * Parameters: deviceTypeName, outputDevice, inputDevice, sampleRate, bufferSize
     *
     * Note: Actual device change requires engine stop/restart (implement in later step)
     */
    std::function<void(juce::String, juce::String, juce::String, double, int)> onSettingsChanged;

    /**
     * @brief Callback when ASIO control panel requested
     */
    std::function<void()> onOpenAsioPanel;

    //==========================================================================
    AudioSettingsWindows(juce::AudioDeviceManager& deviceManager);
    ~AudioSettingsWindows() override = default;

    //==========================================================================
    // Component interface
    //==========================================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
     * @brief Refresh device list and current settings
     */
    void refreshDevices();

    /**
     * @brief Update status line (e.g., "Applying settings...", "Ready", "Error: ...")
     */
    void setStatus(const juce::String& status);

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Populate device type dropdown (WASAPI, ASIO, DirectSound)
     */
    void populateDeviceTypes();

    /**
     * @brief Populate output/input device dropdowns for current device type
     */
    void populateDevices();

    /**
     * @brief Populate sample rate dropdown for current device
     */
    void populateSampleRates();

    /**
     * @brief Populate buffer size dropdown for current device
     */
    void populateBufferSizes();

    /**
     * @brief Update mode label (Shared/Exclusive detection heuristic)
     */
    void updateModeLabel();

    /**
     * @brief Update latency display (calculated from buffer size and sample rate)
     */
    void updateLatencyDisplay();

    /**
     * @brief Handle device type changed (e.g., WASAPI → ASIO)
     */
    void deviceTypeChanged();

    /**
     * @brief Handle output device changed
     */
    void outputDeviceChanged();

    /**
     * @brief Handle sample rate changed
     */
    void sampleRateChanged();

    /**
     * @brief Handle buffer size changed
     */
    void bufferSizeChanged();

    //==========================================================================
    // Member Variables
    //==========================================================================

    juce::AudioDeviceManager& deviceManager_;

    // Section labels
    juce::Label titleLabel;

    // Device Type
    juce::Label deviceTypeLabel;
    juce::ComboBox deviceTypeComboBox;

    // Output/Input Devices
    juce::Label outputDeviceLabel;
    juce::ComboBox outputDeviceComboBox;

    juce::Label inputDeviceLabel;
    juce::ComboBox inputDeviceComboBox;

    // Mode (read-only, automatic detection)
    juce::Label modeLabel;
    juce::Label modeValueLabel;

    // Sample Rate
    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateComboBox;

    // Buffer Size
    juce::Label bufferSizeLabel;
    juce::ComboBox bufferSizeComboBox;

    // Latency display (calculated)
    juce::Label latencyLabel;
    juce::Label latencyValueLabel;

    // ASIO panel button (visible only for ASIO devices)
    juce::TextButton asioControlPanelButton;

    // Status line
    juce::Label statusLabel;

    // Current settings cache
    juce::String currentDeviceTypeName;
    juce::String currentOutputDevice;
    juce::String currentInputDevice;
    double currentSampleRate = 48000.0;
    int currentBufferSize = 256;

    // Constants
    static constexpr int rowHeight = 32;
    static constexpr int labelWidth = 120;
    static constexpr int spacing = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsWindows)
};

#endif // _WIN32
