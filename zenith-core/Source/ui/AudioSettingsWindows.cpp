/**
 * @file AudioSettingsWindows.cpp
 * @brief Implementation of Windows audio settings panel
 */

#ifdef _WIN32

#include "AudioSettingsWindows.h"

//==============================================================================
AudioSettingsWindows::AudioSettingsWindows(juce::AudioDeviceManager& deviceManager)
    : deviceManager_(deviceManager)
{
    // Title
    titleLabel.setText("Audio Device Settings", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Device Type
    deviceTypeLabel.setText("Device Type:", juce::dontSendNotification);
    deviceTypeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(deviceTypeLabel);

    deviceTypeComboBox.onChange = [this]() { deviceTypeChanged(); };
    addAndMakeVisible(deviceTypeComboBox);

    // Output Device
    outputDeviceLabel.setText("Output Device:", juce::dontSendNotification);
    outputDeviceLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(outputDeviceLabel);

    outputDeviceComboBox.onChange = [this]() { outputDeviceChanged(); };
    addAndMakeVisible(outputDeviceComboBox);

    // Input Device
    inputDeviceLabel.setText("Input Device:", juce::dontSendNotification);
    inputDeviceLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(inputDeviceLabel);

    inputDeviceComboBox.onChange = [this]() {
        // W3 stub: no engine mutation yet
        DBG("Input device changed: " + inputDeviceComboBox.getText());
    };
    addAndMakeVisible(inputDeviceComboBox);

    // Mode (read-only label)
    modeLabel.setText("Mode:", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(modeLabel);

    modeValueLabel.setText("Shared (automatic)", juce::dontSendNotification);
    modeValueLabel.setJustificationType(juce::Justification::centredLeft);
    modeValueLabel.setColour(juce::Label::textColourId, ZenithColours::textSecondary);
    addAndMakeVisible(modeValueLabel);

    // Sample Rate
    sampleRateLabel.setText("Sample Rate:", juce::dontSendNotification);
    sampleRateLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(sampleRateLabel);

    sampleRateComboBox.onChange = [this]() { sampleRateChanged(); };
    addAndMakeVisible(sampleRateComboBox);

    // Buffer Size
    bufferSizeLabel.setText("Buffer Size:", juce::dontSendNotification);
    bufferSizeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bufferSizeLabel);

    bufferSizeComboBox.onChange = [this]() { bufferSizeChanged(); };
    addAndMakeVisible(bufferSizeComboBox);

    // Latency display
    latencyLabel.setText("Latency:", juce::dontSendNotification);
    latencyLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(latencyLabel);

    latencyValueLabel.setText("~5.3 ms", juce::dontSendNotification);
    latencyValueLabel.setJustificationType(juce::Justification::centredLeft);
    latencyValueLabel.setColour(juce::Label::textColourId, ZenithColours::accent);
    addAndMakeVisible(latencyValueLabel);

    // ASIO Control Panel button
    asioControlPanelButton.setButtonText("Open ASIO Panel...");
    asioControlPanelButton.onClick = [this]()
    {
        if (onOpenAsioPanel)
            onOpenAsioPanel();
    };
    asioControlPanelButton.setEnabled(false); // Disabled by default (enable for ASIO devices)
    addAndMakeVisible(asioControlPanelButton);

    // Status label
    statusLabel.setText("Status: Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, ZenithColours::textSecondary);
    addAndMakeVisible(statusLabel);

    // Populate initial values
    refreshDevices();
}

//==============================================================================
void AudioSettingsWindows::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(ZenithColours::backgroundDark);

    // Border
    g.setColour(ZenithColours::border);
    g.drawRect(getLocalBounds(), 1);
}

void AudioSettingsWindows::resized()
{
    auto bounds = getLocalBounds().reduced(spacing);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    // Helper lambda for label + combobox rows
    auto layoutRow = [&](juce::Label& label, juce::ComboBox& comboBox)
    {
        auto row = bounds.removeFromTop(rowHeight);
        label.setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(spacing);
        comboBox.setBounds(row);
        bounds.removeFromTop(spacing / 2);
    };

    // Helper lambda for label + label rows (read-only)
    auto layoutLabelRow = [&](juce::Label& label, juce::Label& valueLabel)
    {
        auto row = bounds.removeFromTop(rowHeight);
        label.setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(spacing);
        valueLabel.setBounds(row);
        bounds.removeFromTop(spacing / 2);
    };

    // Device Type
    layoutRow(deviceTypeLabel, deviceTypeComboBox);

    // Output Device
    layoutRow(outputDeviceLabel, outputDeviceComboBox);

    // Input Device
    layoutRow(inputDeviceLabel, inputDeviceComboBox);

    // Mode (read-only)
    layoutLabelRow(modeLabel, modeValueLabel);

    // Sample Rate
    layoutRow(sampleRateLabel, sampleRateComboBox);

    // Buffer Size
    layoutRow(bufferSizeLabel, bufferSizeComboBox);

    // Latency
    layoutLabelRow(latencyLabel, latencyValueLabel);

    bounds.removeFromTop(spacing);

    // ASIO Control Panel button
    asioControlPanelButton.setBounds(bounds.removeFromTop(rowHeight).reduced(labelWidth, 0));
    bounds.removeFromTop(spacing);

    // Status label
    statusLabel.setBounds(bounds.removeFromTop(rowHeight));
}

void AudioSettingsWindows::visibilityChanged()
{
    if (isVisible())
    {
        // Refresh devices when panel becomes visible
        refreshDevices();
    }
}

//==============================================================================
void AudioSettingsWindows::refreshDevices()
{
    DBG("AudioSettingsWindows: Refreshing devices...");

    populateDeviceTypes();
    populateDevices();
    populateSampleRates();
    populateBufferSizes();
    updateModeLabel();
    updateLatencyDisplay();
}

void AudioSettingsWindows::setStatus(const juce::String& status)
{
    statusLabel.setText("Status: " + status, juce::dontSendNotification);
}

//==============================================================================
void AudioSettingsWindows::populateDeviceTypes()
{
    deviceTypeComboBox.clear();

    int itemId = 1;
    int currentTypeIndex = -1;

    // Get available device types from AudioDeviceManager
    auto& deviceTypes = deviceManager_.getAvailableDeviceTypes();

    for (int i = 0; i < deviceTypes.size(); ++i)
    {
        auto* deviceType = deviceTypes[i];
        auto typeName = deviceType->getTypeName();

        deviceTypeComboBox.addItem(typeName, itemId++);

        // Check if this is the current device type
        if (auto* currentDevice = deviceManager_.getCurrentAudioDevice())
        {
            if (currentDevice->getTypeName() == typeName)
                currentTypeIndex = i + 1; // ComboBox is 1-indexed
        }
    }

    // Select current device type or default to first
    if (currentTypeIndex >= 0)
        deviceTypeComboBox.setSelectedId(currentTypeIndex, juce::dontSendNotification);
    else if (deviceTypeComboBox.getNumItems() > 0)
        deviceTypeComboBox.setSelectedId(1, juce::dontSendNotification);

    DBG("Device types populated: " + juce::String(deviceTypeComboBox.getNumItems()) + " types");
}

void AudioSettingsWindows::populateDevices()
{
    outputDeviceComboBox.clear();
    inputDeviceComboBox.clear();

    int selectedTypeIndex = deviceTypeComboBox.getSelectedId() - 1;
    if (selectedTypeIndex < 0)
        return;

    auto& deviceTypes = deviceManager_.getAvailableDeviceTypes();
    if (selectedTypeIndex >= deviceTypes.size())
        return;

    auto* deviceType = deviceTypes[selectedTypeIndex];
    currentDeviceTypeName = deviceType->getTypeName();

    // Scan for devices
    deviceType->scanForDevices();

    // Output devices
    auto outputNames = deviceType->getDeviceNames(true); // true = output
    int itemId = 1;
    for (auto& name : outputNames)
    {
        outputDeviceComboBox.addItem(name, itemId++);
    }

    // Input devices
    auto inputNames = deviceType->getDeviceNames(false); // false = input
    itemId = 1;
    for (auto& name : inputNames)
    {
        inputDeviceComboBox.addItem(name, itemId++);
    }

    // Select current device or first available
    if (auto* currentDevice = deviceManager_.getCurrentAudioDevice())
    {
        auto currentOutputName = currentDevice->getName();

        for (int i = 0; i < outputDeviceComboBox.getNumItems(); ++i)
        {
            if (outputDeviceComboBox.getItemText(i) == currentOutputName)
            {
                outputDeviceComboBox.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
    else
    {
        if (outputDeviceComboBox.getNumItems() > 0)
            outputDeviceComboBox.setSelectedId(1, juce::dontSendNotification);
        if (inputDeviceComboBox.getNumItems() > 0)
            inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification);
    }

    DBG("Devices populated - Output: " + juce::String(outputDeviceComboBox.getNumItems()) +
        ", Input: " + juce::String(inputDeviceComboBox.getNumItems()));
}

void AudioSettingsWindows::populateSampleRates()
{
    sampleRateComboBox.clear();

    auto* currentDevice = deviceManager_.getCurrentAudioDevice();
    if (currentDevice == nullptr)
    {
        // Default sample rates if no device
        sampleRateComboBox.addItem("44100 Hz", 1);
        sampleRateComboBox.addItem("48000 Hz", 2);
        sampleRateComboBox.setSelectedId(2, juce::dontSendNotification);
        return;
    }

    // Get available sample rates from current device
    auto availableRates = currentDevice->getAvailableSampleRates();

    int itemId = 1;
    int currentRateIndex = -1;
    double currentRate = currentDevice->getCurrentSampleRate();

    for (auto rate : availableRates)
    {
        juce::String rateText = juce::String(rate, 0) + " Hz";
        sampleRateComboBox.addItem(rateText, itemId);

        if (std::abs(rate - currentRate) < 1.0)
            currentRateIndex = itemId;

        itemId++;
    }

    // Select current sample rate or default to 48 kHz
    if (currentRateIndex >= 0)
        sampleRateComboBox.setSelectedId(currentRateIndex, juce::dontSendNotification);
    else
    {
        // Try to find 48 kHz
        for (int i = 0; i < sampleRateComboBox.getNumItems(); ++i)
        {
            if (sampleRateComboBox.getItemText(i).contains("48000"))
            {
                sampleRateComboBox.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }

    DBG("Sample rates populated: " + juce::String(sampleRateComboBox.getNumItems()) + " rates");
}

void AudioSettingsWindows::populateBufferSizes()
{
    bufferSizeComboBox.clear();

    auto* currentDevice = deviceManager_.getCurrentAudioDevice();
    if (currentDevice == nullptr)
    {
        // Default buffer sizes if no device
        bufferSizeComboBox.addItem("128 samples", 1);
        bufferSizeComboBox.addItem("256 samples", 2);
        bufferSizeComboBox.addItem("512 samples", 3);
        bufferSizeComboBox.setSelectedId(2, juce::dontSendNotification);
        return;
    }

    // Get available buffer sizes from current device
    auto availableSizes = currentDevice->getAvailableBufferSizes();

    int itemId = 1;
    int currentSizeIndex = -1;
    int currentSize = currentDevice->getCurrentBufferSizeSamples();

    for (auto size : availableSizes)
    {
        juce::String sizeText = juce::String(size) + " samples";
        bufferSizeComboBox.addItem(sizeText, itemId);

        if (size == currentSize)
            currentSizeIndex = itemId;

        itemId++;
    }

    // Select current buffer size or default to 256
    if (currentSizeIndex >= 0)
        bufferSizeComboBox.setSelectedId(currentSizeIndex, juce::dontSendNotification);
    else
    {
        // Try to find 256 samples
        for (int i = 0; i < bufferSizeComboBox.getNumItems(); ++i)
        {
            if (bufferSizeComboBox.getItemText(i).contains("256"))
            {
                bufferSizeComboBox.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }

    DBG("Buffer sizes populated: " + juce::String(bufferSizeComboBox.getNumItems()) + " sizes");
}

void AudioSettingsWindows::updateModeLabel()
{
    // Heuristic for exclusive mode detection (not foolproof)
    auto* currentDevice = deviceManager_.getCurrentAudioDevice();
    if (currentDevice == nullptr)
    {
        modeValueLabel.setText("No device", juce::dontSendNotification);
        return;
    }

    auto typeName = currentDevice->getTypeName();

    // ASIO is always exclusive
    if (typeName == "ASIO")
    {
        modeValueLabel.setText("Exclusive (ASIO)", juce::dontSendNotification);
        asioControlPanelButton.setEnabled(true);
        return;
    }

    // WASAPI: check if very low buffer sizes are available (indicates exclusive mode capable)
    auto availableSizes = currentDevice->getAvailableBufferSizes();
    bool hasVeryLowLatency = false;

    for (auto size : availableSizes)
    {
        if (size <= 64) // 64 samples or less suggests exclusive mode
        {
            hasVeryLowLatency = true;
            break;
        }
    }

    if (hasVeryLowLatency)
        modeValueLabel.setText("Shared/Exclusive (automatic)", juce::dontSendNotification);
    else
        modeValueLabel.setText("Shared (automatic)", juce::dontSendNotification);

    asioControlPanelButton.setEnabled(false); // Only ASIO has control panel
}

void AudioSettingsWindows::updateLatencyDisplay()
{
    // Extract buffer size from combo box text
    auto bufferSizeText = bufferSizeComboBox.getText();
    int bufferSize = bufferSizeText.upToFirstOccurrenceOf(" ", false, false).getIntValue();

    // Extract sample rate from combo box text
    auto sampleRateText = sampleRateComboBox.getText();
    double sampleRate = sampleRateText.upToFirstOccurrenceOf(" ", false, false).getDoubleValue();

    if (bufferSize > 0 && sampleRate > 0)
    {
        // Calculate one-way latency in milliseconds
        double latencyMs = (bufferSize / sampleRate) * 1000.0;
        latencyValueLabel.setText(juce::String(latencyMs, 2) + " ms", juce::dontSendNotification);
    }
    else
    {
        latencyValueLabel.setText("Unknown", juce::dontSendNotification);
    }
}

//==============================================================================
void AudioSettingsWindows::deviceTypeChanged()
{
    DBG("Device type changed: " + deviceTypeComboBox.getText());

    // Refresh devices for new device type
    populateDevices();
    populateSampleRates();
    populateBufferSizes();
    updateModeLabel();
    updateLatencyDisplay();

    setStatus("Device type changed (restart required to apply)");
}

void AudioSettingsWindows::outputDeviceChanged()
{
    DBG("Output device changed: " + outputDeviceComboBox.getText());

    currentOutputDevice = outputDeviceComboBox.getText();

    // W3 stub: no engine mutation yet
    // In later step, this will trigger:
    // - deviceManager.closeAudioDevice()
    // - deviceManager.setAudioDeviceSetup(newSetup)

    populateSampleRates();
    populateBufferSizes();
    updateModeLabel();
    updateLatencyDisplay();

    setStatus("Output device changed (restart required to apply)");

    // Placeholder callback
    if (onSettingsChanged)
    {
        onSettingsChanged(
            currentDeviceTypeName,
            currentOutputDevice,
            inputDeviceComboBox.getText(),
            currentSampleRate,
            currentBufferSize
        );
    }
}

void AudioSettingsWindows::sampleRateChanged()
{
    auto rateText = sampleRateComboBox.getText();
    currentSampleRate = rateText.upToFirstOccurrenceOf(" ", false, false).getDoubleValue();

    DBG("Sample rate changed: " + juce::String(currentSampleRate) + " Hz");

    updateLatencyDisplay();
    setStatus("Sample rate changed (restart required to apply)");
}

void AudioSettingsWindows::bufferSizeChanged()
{
    auto sizeText = bufferSizeComboBox.getText();
    currentBufferSize = sizeText.upToFirstOccurrenceOf(" ", false, false).getIntValue();

    DBG("Buffer size changed: " + juce::String(currentBufferSize) + " samples");

    updateLatencyDisplay();
    updateModeLabel(); // Mode might change based on buffer size
    setStatus("Buffer size changed (restart required to apply)");
}

#endif // _WIN32
