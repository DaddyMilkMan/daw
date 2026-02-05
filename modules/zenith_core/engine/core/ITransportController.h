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

 * @file ITransportController.h
 * @brief Transport control interface
 *
 * Manages playback state, position, and looping functionality.
 */


#include <juce_core/juce_core.h>

namespace zenith {

class ITransportController {
public:
    virtual ~ITransportController() = default;

    //==========================================================================
    // Transport State
    //==========================================================================

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual void record() = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
    virtual void toggleRecording() = 0;
    virtual void panic() = 0;

    //==========================================================================
    // Position Management
    //==========================================================================

    virtual juce::int64 getPlayheadSamples() const = 0;
    virtual double getPlaybackPositionBeats() const = 0;
    virtual void setPlayheadSamples(juce::int64 position) = 0;
    virtual void seekToSamples(juce::int64 position) = 0;
    virtual void seekToBeats(double beats) = 0;

    //==========================================================================
    // Looping
    //==========================================================================

    virtual void setLooping(bool shouldLoop) = 0;
    virtual bool isLooping() const = 0;
    virtual void setLoopRegion(juce::int64 start, juce::int64 end) = 0;
    virtual juce::int64 getLoopStart() const = 0;
    virtual juce::int64 getLoopEnd() const = 0;
    virtual bool isWithinLoop(juce::int64 position) const = 0;

    //==========================================================================
    // Speed Control
    //==========================================================================

    virtual void setPlaybackRate(float rate) = 0;
    virtual float getPlaybackRate() const = 0;
    virtual void setPitchShift(float pitchSemi) = 0;
    virtual float getPitchShift() const = 0;

    //==========================================================================
    // Recording
    //==========================================================================

    virtual void setRecordingArmed(bool armed) = 0;
    virtual bool isRecordingArmed() const = 0;
    virtual void setOverdubMode(bool overdub) = 0;
    virtual bool isOverdubMode() const = 0;

    //==========================================================================
    // Event Listeners
    //==========================================================================

    virtual void addTransportChangeListener(juce::ChangeListener* listener) = 0;
    virtual void removeTransportChangeListener(juce::ChangeListener* listener) = 0;

    //==========================================================================
    // State Serialization
    //==========================================================================

    virtual juce::ValueTree getState() const = 0;
    virtual void setState(const juce::ValueTree& state) = 0;

    //==========================================================================
    // Statistics
    //==========================================================================

    virtual double getCpuUsage() const = 0;
    virtual juce::int64 getTotalSamplesProcessed() const = 0;
    virtual double getElapsedTime() const = 0;
};

} // namespace zenith