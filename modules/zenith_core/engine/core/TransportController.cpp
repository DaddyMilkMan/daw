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

 * @file TransportController.cpp
 * @brief Concrete transport control implementation
 */



TransportController::TransportController() {
    DBG("TransportController: Constructor");
    resetPlayhead();
}

TransportController::~TransportController() {
    DBG("TransportController: Destructor");
}

void TransportController::play() {
    if (playing_.load()) {
        DBG("TransportController: Already playing");
        return;
    }

    DBG("TransportController: Play");
    playing_.store(true);
    recording_.store(false);
    startTime_.store(juce::Time::getMillisecondCounterHiRes() / 1000.0);

    notifyTransportStateChanged();
}

void TransportController::stop() {
    if (!playing_.load()) {
        DBG("TransportController: Not playing");
        return;
    }

    DBG("TransportController: Stop");
    playing_.store(false);
    recording_.store(false);
    resetPlayhead();
    notifyTransportStateChanged();
    notifyPositionChanged();
}

bool TransportController::isPlaying() const {
    return playing_.load();
}

void TransportController::record() {
    if (!recordingArmed_.load()) {
        DBG("TransportController: Record armed");
        recordingArmed_.store(true);
    } else {
        DBG("TransportController: Start recording");
        playing_.store(true);
        recording_.store(true);
        startTime_.store(juce::Time::getMillisecondCounterHiRes() / 1000.0);
        recordingArmed_.store(false);
    }

    notifyTransportStateChanged();
}

void TransportController::stopRecording() {
    if (recording_.load()) {
        DBG("TransportController: Stop recording");
        recording_.store(false);
        notifyTransportStateChanged();
    }
}

bool TransportController::isRecording() const {
    return recording_.load();
}

void TransportController::toggleRecording() {
    if (recording_.load()) {
        stopRecording();
    } else if (isPlaying()) {
        record();
    } else {
        setRecordingArmed(true);
    }
}

void TransportController::panic() {
    DBG("TransportController: Panic - Stop all sound");

    playing_.store(false);
    recording_.store(false);
    recordingArmed_.store(false);
    overdubMode_.store(false);

    resetPlayhead();
    notifyTransportStateChanged();
    notifyPositionChanged();
}

juce::int64 TransportController::getPlayheadSamples() const {
    return playheadPosition_.load();
}

double TransportController::getPlaybackPositionBeats() const {
    // TODO: Convert samples to beats using tempo map
    return static_cast<double>(playheadPosition_.load()) / 44100.0; // Assuming 44.1kHz
}

void TransportController::setPlayheadSamples(juce::int64 position) {
    auto oldPosition = playheadPosition_.load();
    playheadPosition_.store(position);

    if (oldPosition != position) {
        notifyPositionChanged();
    }
}

void TransportController::seekToSamples(juce::int64 position) {
    setPlayheadSamples(position);
}

void TransportController::seekToBeats(double beats) {
    // TODO: Convert beats to samples using tempo map
    auto samples = static_cast<juce::int64>(beats * 44100.0);
    seekToSamples(samples);
}

void TransportController::setLooping(bool shouldLoop) {
    if (looping_.load() != shouldLoop) {
        DBG("TransportController: Looping " + juce::String(shouldLoop ? "enabled" : "disabled"));
        looping_.store(shouldLoop);
        notifyLoopChanged();
    }
}

bool TransportController::isLooping() const {
    return looping_.load();
}

void TransportController::setLoopRegion(juce::int64 start, juce::int64 end) {
    if (start < 0 || end <= start) {
        DBG("TransportController: Invalid loop region");
        return;
    }

    loopStart_.store(start);
    loopEnd_.store(end);

    DBG("TransportController: Loop region set to " + juce::String(start) + "-" + juce::String(end));
    notifyLoopChanged();
}

juce::int64 TransportController::getLoopStart() const {
    return loopStart_.load();
}

juce::int64 TransportController::getLoopEnd() const {
    return loopEnd_.load();
}

bool TransportController::isWithinLoop(juce::int64 position) const {
    if (!looping_.load()) return false;
    return position >= loopStart_.load() && position < loopEnd_.load();
}

void TransportController::setPlaybackRate(float rate) {
    if (rate <= 0.0f || rate > 8.0f) {
        DBG("TransportController: Invalid playback rate - " + juce::String(rate));
        return;
    }

    if (playbackRate_.load() != rate) {
        DBG("TransportController: Playback rate set to " + juce::String(rate));
        playbackRate_.store(rate);
    }
}

float TransportController::getPlaybackRate() const {
    return playbackRate_.load();
}

void TransportController::setPitchShift(float pitchSemi) {
    if (pitchSemi < -12.0f || pitchSemi > 12.0f) {
        DBG("TransportController: Invalid pitch shift - " + juce::String(pitchSemi));
        return;
    }

    if (pitchShift_.load() != pitchSemi) {
        DBG("TransportController: Pitch shift set to " + juce::String(pitchSemi));
        pitchShift_.store(pitchSemi);
    }
}

float TransportController::getPitchShift() const {
    return pitchShift_.load();
}

void TransportController::setRecordingArmed(bool armed) {
    if (recordingArmed_.load() != armed) {
        DBG("TransportController: Recording armed " + juce::String(armed ? "enabled" : "disabled"));
        recordingArmed_.store(armed);
        notifyTransportStateChanged();
    }
}

bool TransportController::isRecordingArmed() const {
    return recordingArmed_.load();
}

void TransportController::setOverdubMode(bool overdub) {
    if (overdubMode_.load() != overdub) {
        DBG("TransportController: Overdub mode " + juce::String(overdub ? "enabled" : "disabled"));
        overdubMode_.store(overdub);
        notifyTransportStateChanged();
    }
}

bool TransportController::isOverdubMode() const {
    return overdubMode_.load();
}

void TransportController::addTransportChangeListener(juce::ChangeListener* listener) {
    changeListeners_.addListener(listener);
}

void TransportController::removeTransportChangeListener(juce::ChangeListener* listener) {
    changeListeners_.removeListener(listener);
}

juce::ValueTree TransportController::getState() const {
    juce::ValueTree state("TransportState");

    state.setProperty("playing", isPlaying(), nullptr);
    state.setProperty("recording", isRecording(), nullptr);
    state.setProperty("recordingArmed", isRecordingArmed(), nullptr);
    state.setProperty("overdubMode", isOverdubMode(), nullptr);
    state.setProperty("looping", isLooping(), nullptr);
    state.setProperty("playheadPosition", getPlayheadSamples(), nullptr);
    state.setProperty("loopStart", getLoopStart(), nullptr);
    state.setProperty("loopEnd", getLoopEnd(), nullptr);
    state.setProperty("playbackRate", getPlaybackRate(), nullptr);
    state.setProperty("pitchShift", getPitchShift(), nullptr);

    return state;
}

void TransportController::setState(const juce::ValueTree& state) {
    if (!state.hasType("TransportState")) {
        DBG("TransportController: Invalid state type");
        return;
    }

    DBG("TransportController: Setting state");

    playing_.store(state.getProperty("playing", false));
    recording_.store(state.getProperty("recording", false));
    recordingArmed_.store(state.getProperty("recordingArmed", false));
    overdubMode_.store(state.getProperty("overdubMode", false));
    looping_.store(state.getProperty("looping", false));
    playheadPosition_.store(state.getProperty("playheadPosition", 0));
    loopStart_.store(state.getProperty("loopStart", 0));
    loopEnd_.store(state.getProperty("loopEnd", 0));
    playbackRate_.store(state.getProperty("playbackRate", 1.0f));
    pitchShift_.store(state.getProperty("pitchShift", 0.0f));

    notifyTransportStateChanged();
    notifyPositionChanged();
    notifyLoopChanged();
}

double TransportController::getCpuUsage() const {
    return cpuUsage_.load();
}

juce::int64 TransportController::getTotalSamplesProcessed() const {
    return totalSamplesProcessed_.load();
}

double TransportController::getElapsedTime() const {
    if (!playing_.load()) {
        return 0.0;
    }

    auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    return now - startTime_.load();
}

void TransportController::processAudioBlock(int numSamples) {
    if (!playing_.load()) {
        return;
    }

    // Update CPU usage (simplified)
    auto now = juce::Time::getMillisecondCounterHiRes();
    auto lastCheck = totalSamplesProcessed_.load();

    if (lastCheck > 0) {
        auto timeDiff = now - lastCheck;
        if (timeDiff > 0) {
            double usage = std::min(100.0, numSamples / timeDiff * 0.1);
            cpuUsage_.store(usage);
        }
    }

    // Update playhead position
    updatePlayhead(numSamples);
    totalSamplesProcessed_.store(totalSamplesProcessed_.load() + numSamples);
}

void TransportController::updatePlayhead(int numSamples) {
    auto newPosition = playheadPosition_.load() + static_cast<juce::int64>(numSamples * playbackRate_.load());

    // Handle looping
    if (looping_.load() && newPosition >= loopEnd_.load()) {
        auto loopLength = loopEnd_.load() - loopStart_.load();
        newPosition = loopStart_.load() + ((newPosition - loopStart_.load()) % loopLength);
    }

    auto oldPosition = playheadPosition_.load();
    playheadPosition_.store(newPosition);

    if (oldPosition != newPosition) {
        notifyPositionChanged();
    }
}

void TransportController::notifyTransportStateChanged() {
    juce::ChangeBroadcaster::sendChangeMessage();
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

void TransportController::notifyPositionChanged() {
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

void TransportController::notifyLoopChanged() {
    changeListeners_.call(&juce::ChangeListener::changeListenerCallback, nullptr);
}

juce::int64 TransportController::clampToLoop(juce::int64 position) const {
    if (!looping_.load()) {
        return position;
    }

    if (position < loopStart_.load()) {
        return loopStart_.load();
    } else if (position >= loopEnd_.load()) {
        return loopStart_.load();
    } else {
        return position;
    }
}

void TransportController::resetPlayhead() {
    playheadPosition_.store(0);
    startTime_.store(0.0);
    notifyPositionChanged();
}

} // namespace zenith