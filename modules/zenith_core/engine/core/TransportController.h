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

//==============================================================================

#include "ITransportController.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

namespace zenith {

class TransportController : public ITransportController {
public:
    TransportController();
    ~TransportController() override;

    //==========================================================================
    // ITransportController Implementation
    //==========================================================================

    void play() override;
    void stop() override;
    bool isPlaying() const override;
    void record() override;
    void stopRecording() override;
    bool isRecording() const override;
    void toggleRecording() override;
    void panic() override;

    juce::int64 getPlayheadSamples() const override;
    double getPlaybackPositionBeats() const override;
    void setPlayheadSamples(juce::int64 position) override;
    void seekToSamples(juce::int64 position) override;
    void seekToBeats(double beats) override;

    void setLooping(bool shouldLoop) override;
    bool isLooping() const override;
    void setLoopRegion(juce::int64 start, juce::int64 end) override;
    juce::int64 getLoopStart() const override;
    juce::int64 getLoopEnd() const override;
    bool isWithinLoop(juce::int64 position) const override;

    void setPlaybackRate(float rate) override;
    float getPlaybackRate() const override;
    void setPitchShift(float pitchSemi) override;
    float getPitchShift() const override;

    void setRecordingArmed(bool armed) override;
    bool isRecordingArmed() const override;
    void setOverdubMode(bool overdub) override;
    bool isOverdubMode() const override;

    void addTransportChangeListener(juce::ChangeListener* listener) override;
    void removeTransportChangeListener(juce::ChangeListener* listener) override;

    juce::ValueTree getState() const override;
    void setState(const juce::ValueTree& state) override;

    double getCpuUsage() const override;
    juce::int64 getTotalSamplesProcessed() const override;
    double getElapsedTime() const override;

    //==========================================================================
    // Transport Processing
    //==========================================================================

    void processAudioBlock(int numSamples);
    void updatePlayhead(int numSamples);

private:
    //==========================================================================
    // Transport State
    //==========================================================================

    std::atomic<bool> playing_{false};
    std::atomic<bool> recording_{false};
    std::atomic<bool> recordingArmed_{false};
    std::atomic<bool> overdubMode_{false};
    std::atomic<bool> looping_{false};

    //==========================================================================
    // Position State
    //==========================================================================

    std::atomic<juce::int64> playheadPosition_{0};
    std::atomic<juce::int64> loopStart_{0};
    std::atomic<juce::int64> loopEnd_{0};
    std::atomic<float> playbackRate_{1.0f};
    std::atomic<float> pitchShift_{0.0f};

    //==========================================================================
    // Statistics
    //==========================================================================

    mutable std::atomic<double> cpuUsage_{0.0};
    std::atomic<juce::int64> totalSamplesProcessed_{0};
    std::atomic<double> startTime_{0.0};

    //==========================================================================
    // Event System
    //==========================================================================

    juce::ListenerList<juce::ChangeListener> changeListeners_;

    //==========================================================================
    // Helpers
    //==========================================================================

    void notifyTransportStateChanged();
    void notifyPositionChanged();
    void notifyLoopChanged();
    juce::int64 clampToLoop(juce::int64 position) const;
    void resetPlayhead();
};

} // namespace zenith