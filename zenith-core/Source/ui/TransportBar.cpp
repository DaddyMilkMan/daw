/**
 * @file TransportBar.cpp
 * @brief Implementation of transport bar component
 */

#include "TransportBar.h"

//==============================================================================
TransportBar::TransportBar(Engine& eng)
    : engine(eng)
{
    // Play button
    playButton.setButtonText("Play");
    playButton.onClick = [this]() { playButtonClicked(); };
    addAndMakeVisible(playButton);

    // Stop button
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this]() { stopButtonClicked(); };
    addAndMakeVisible(stopButton);

    // Record button
    recordButton.setButtonText("Record");
    recordButton.onClick = [this]() { recordButtonClicked(); };
    addAndMakeVisible(recordButton);

    // Loop button
    loopButton.setButtonText("Loop");
    loopButton.setClickingTogglesState(true);
    loopButton.onClick = [this]() { loopButtonClicked(); };
    addAndMakeVisible(loopButton);

    // Metronome button
    metronomeButton.setButtonText("Metro");
    metronomeButton.setClickingTogglesState(true);
    metronomeButton.onClick = [this]() { metronomeButtonClicked(); };
    addAndMakeVisible(metronomeButton);

    // Tap tempo button
    tapTempoButton.setButtonText("Tap");
    tapTempoButton.onClick = [this]() { tapTempoButtonClicked(); };
    addAndMakeVisible(tapTempoButton);

    // BPM label
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    bpmLabel.setFont(bpmFont);  // W4: Use cached font
    addAndMakeVisible(bpmLabel);

    // BPM slider
    bpmSlider.setRange(20.0, 300.0, 0.1);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    bpmSlider.onValueChange = [this]()
    {
        currentBPM = bpmSlider.getValue();
        if (onBPMChanged)
            onBPMChanged(currentBPM);
    };
    addAndMakeVisible(bpmSlider);

    // Position label
    positionLabel.setText("1.1.1", juce::dontSendNotification);
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(positionFont);  // W4: Use cached font
    positionLabel.setColour(juce::Label::backgroundColourId, ZenithColours::backgroundLight);
    addAndMakeVisible(positionLabel);

    // Timecode label
    timecodeLabel.setText("00:00:00:00", juce::dontSendNotification);
    timecodeLabel.setJustificationType(juce::Justification::centred);
    timecodeLabel.setFont(timecodeFont);  // W4: Use cached font
    timecodeLabel.setColour(juce::Label::textColourId, ZenithColours::textSecondary);
    addAndMakeVisible(timecodeLabel);

    // CPU label
    cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    cpuLabel.setFont(cpuFont);  // W4: Use cached font
    addAndMakeVisible(cpuLabel);

    // W4: Initialize cached strings to avoid first-frame allocation
    lastCpuText = "CPU: 0%";
    lastPositionText = "1.1.1";
    lastTimecodeText = "00:00:00:00";

    // Start timer (30 Hz)
    startTimer(33);
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(ZenithColours::backgroundMedium);

    // Top border
    g.setColour(ZenithColours::border);
    g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 1.0f);
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Transport buttons (left)
    auto transportSection = bounds.removeFromLeft(310);
    int buttonWidth = 90;
    int spacing = 10;

    playButton.setBounds(transportSection.removeFromLeft(buttonWidth));
    transportSection.removeFromLeft(spacing);
    stopButton.setBounds(transportSection.removeFromLeft(buttonWidth));
    transportSection.removeFromLeft(spacing);
    recordButton.setBounds(transportSection.removeFromLeft(buttonWidth));

    bounds.removeFromLeft(20);

    // Toggle buttons
    auto toggleSection = bounds.removeFromLeft(210);
    buttonWidth = 60;

    loopButton.setBounds(toggleSection.removeFromLeft(buttonWidth));
    toggleSection.removeFromLeft(spacing);
    metronomeButton.setBounds(toggleSection.removeFromLeft(buttonWidth));
    toggleSection.removeFromLeft(spacing);
    tapTempoButton.setBounds(toggleSection);

    bounds.removeFromLeft(20);

    // BPM section
    auto bpmSection = bounds.removeFromLeft(180);
    bpmLabel.setBounds(bpmSection.removeFromLeft(40));
    bpmSection.removeFromLeft(5);
    bpmSlider.setBounds(bpmSection);

    // Position displays (center)
    auto positionSection = bounds.removeFromLeft(juce::jmax(150, bounds.getWidth() - 140));
    positionSection = positionSection.withSizeKeepingCentre(130, bounds.getHeight());

    auto posY = positionSection.getY();
    auto posHeight = positionSection.getHeight();

    positionLabel.setBounds(positionSection.getX(), posY, 130, posHeight * 0.6f);
    timecodeLabel.setBounds(positionSection.getX(), posY + posHeight * 0.5f, 130, posHeight * 0.4f);

    // CPU meter (right)
    bounds.removeFromLeft(20);
    cpuLabel.setBounds(bounds);
}

void TransportBar::timerCallback()
{
    // W4: Update CPU with dirty-check (epsilon 0.05% to avoid jitter)
    double cpuUsage = engine.getCpuUsage();
    if (std::abs(cpuUsage - lastDisplayedCpuUsage) > 0.05)
    {
        lastDisplayedCpuUsage = cpuUsage;
        juce::String newCpuText = "CPU: " + juce::String(cpuUsage, 1) + "%";

        if (newCpuText != lastCpuText)
        {
            lastCpuText = newCpuText;
            cpuLabel.setText(newCpuText, juce::dontSendNotification);
        }
    }

    // W4: Update position if playing with dirty-check
    if (isPlaying)
    {
        currentPosition += 0.1; // Simulated

        // Only format and update if position actually changed
        if (std::abs(currentPosition - lastDisplayedPosition) > 0.01)
        {
            lastDisplayedPosition = currentPosition;

            juce::String newPositionText = formatPosition(currentPosition);
            if (newPositionText != lastPositionText)
            {
                lastPositionText = newPositionText;
                positionLabel.setText(newPositionText, juce::dontSendNotification);
            }

            double positionInSeconds = currentPosition / (currentBPM / 60.0);
            juce::String newTimecodeText = formatTimecode(positionInSeconds);
            if (newTimecodeText != lastTimecodeText)
            {
                lastTimecodeText = newTimecodeText;
                timecodeLabel.setText(newTimecodeText, juce::dontSendNotification);
            }
        }
    }
}

void TransportBar::playButtonClicked()
{
    isPlaying = !isPlaying;
    engine.play();
    setPlaying(isPlaying);

    if (onPlay)
        onPlay();
}

void TransportBar::stopButtonClicked()
{
    isPlaying = false;
    isRecording = false;
    currentPosition = 0.0;
    engine.stop();
    setPlaying(false);
    setRecording(false);
    positionLabel.setText("1.1.1", juce::dontSendNotification);
    timecodeLabel.setText("00:00:00:00", juce::dontSendNotification);

    if (onStop)
        onStop();
}

void TransportBar::recordButtonClicked()
{
    isRecording = !isRecording;
    if (isRecording && !isPlaying)
    {
        isPlaying = true;
        engine.play();
    }
    setRecording(isRecording);

    if (onRecord)
        onRecord();
}

void TransportBar::loopButtonClicked()
{
    loopEnabled = !loopEnabled;

    if (onLoopToggle)
        onLoopToggle(loopEnabled);
}

void TrackView::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is in timeline
    if (event.y < timelineHeight)
    {
        double clickTime = xToTime((float)event.x);
        if (onPlayheadClicked)
            onPlayheadClicked(clickTime);
        return;
    }

    // Check which track was clicked
    int trackY = event.y - timelineHeight;
    int trackIndex = (int)(((float)trackY + scrollY) / ((float)defaultTrackHeight * verticalZoom));

    if (juce::isPositiveAndBelow(trackIndex, trackNames.size()))
    {
        if (onTrackSelected)
            onTrackSelected(trackIndex);
    }
}

void TransportBar::metronomeButtonClicked()
{
    metronomeEnabled = !metronomeEnabled;

    if (onMetronomeToggle)
        onMetronomeToggle(metronomeEnabled);
}

void TransportBar::tapTempoButtonClicked()
{
    auto currentTime = juce::Time::currentTimeMillis();

    // Remove old taps
    tapTimes.erase(
        std::remove_if(tapTimes.begin(), tapTimes.end(),
                      [currentTime](juce::int64 time) {
                          return currentTime - time > tapTimeoutMs;
                      }),
        tapTimes.end()
    );

    tapTimes.push_back(currentTime);

    if (tapTimes.size() >= 2)
    {
        juce::int64 totalInterval = 0;
        for (size_t i = 1; i < tapTimes.size(); ++i)
            totalInterval += tapTimes[i] - tapTimes[i - 1];

        double avgInterval = (double)totalInterval / (double)(tapTimes.size() - 1);
        double bpm = 60000.0 / avgInterval;

        bpm = juce::jlimit(20.0, 300.0, bpm);
        setBPM(bpm);
        bpmSlider.setValue(bpm, juce::dontSendNotification);

        if (onBPMChanged)
            onBPMChanged(bpm);
    }

    if (tapTimes.size() > (size_t)maxTaps)
        tapTimes.erase(tapTimes.begin());
}

void TransportBar::setPosition(double positionInQuarterNotes)
{
    currentPosition = positionInQuarterNotes;
    positionLabel.setText(formatPosition(positionInQuarterNotes), juce::dontSendNotification);

    double positionInSeconds = positionInQuarterNotes / (currentBPM / 60.0);
    timecodeLabel.setText(formatTimecode(positionInSeconds), juce::dontSendNotification);
}

void TransportBar::setBPM(double bpm)
{
    currentBPM = bpm;
    bpmSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportBar::setPlaying(bool playing)
{
    isPlaying = playing;
    playButton.setButtonText(isPlaying ? "Pause" : "Play");
}

void TransportBar::setRecording(bool recording)
{
    isRecording = recording;
    recordButton.setToggleState(isRecording, juce::dontSendNotification);
}

void TransportBar::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    loopButton.setToggleState(enabled, juce::dontSendNotification);
}

void TransportBar::setMetronomeEnabled(bool enabled)
{
    metronomeEnabled = enabled;
    metronomeButton.setToggleState(enabled, juce::dontSendNotification);
}

juce::String TransportBar::formatPosition(double positionInQuarterNotes)
{
    int totalSixteenths = (int)(positionInQuarterNotes * 4);
    int bar = totalSixteenths / 16 + 1;
    int beat = (totalSixteenths % 16) / 4 + 1;
    int sixteenth = (totalSixteenths % 4) + 1;

    return juce::String(bar) + "." + juce::String(beat) + "." + juce::String(sixteenth);
}

juce::String TransportBar::formatTimecode(double positionInSeconds)
{
    // W4: Use pre-allocated buffer + snprintf to avoid String::formatted allocation
    int totalFrames = (int)(positionInSeconds * 30.0);
    int hours = totalFrames / (30 * 60 * 60);
    int minutes = (totalFrames / (30 * 60)) % 60;
    int seconds = (totalFrames / 30) % 60;
    int frames = totalFrames % 30;

    std::snprintf(timecodeBuffer, sizeof(timecodeBuffer), "%02d:%02d:%02d:%02d",
                 hours, minutes, seconds, frames);

    return juce::String(timecodeBuffer);
}
