/**
 * @file TransportProtocolAgent.cpp
 * @brief Implementation of TransportProtocolAgent
 */

#include "TransportProtocolAgent.h"
#include <cmath>

namespace zenith {

// MIDI Clock sends 24 pulses per quarter note (PPQN)
static constexpr int kMidiClockPPQN = 24;

TransportProtocolAgent::TransportProtocolAgent() {
}

TransportProtocolAgent::~TransportProtocolAgent() {
}

void TransportProtocolAgent::initialize(double sampleRate, double initialTempo) {
    sampleRate_ = sampleRate;
    tempo_ = initialTempo;
    
    calculateClockTiming();
    
    nextClockSample_ = 0;
}

void TransportProtocolAgent::setProtocolEnabled(SyncProtocol protocol, bool enabled) {
    // MESSAGE THREAD ONLY
    
    switch (protocol) {
        case SyncProtocol::MidiClock:
            midiClockEnabled_.store(enabled);
            break;
        case SyncProtocol::MTC:
            mtcEnabled_.store(enabled);
            break;
        case SyncProtocol::MMC:
            mmcEnabled_.store(enabled);
            break;
        default:
            break;
    }
}

int TransportProtocolAgent::generateMidiClock(int bufferSize, 
                                             int64_t currentSamplePosition,
                                             std::vector<MidiClockEvent>& outEvents) {
    // AUDIO THREAD SAFE - RT-safe operations only
    
    if (!midiClockEnabled_.load()) {
        return 0;
    }
    
    int clockCount = 0;
    int64_t bufferEnd = currentSamplePosition + bufferSize;
    
    // Generate clock events within this buffer
    while (nextClockSample_ < bufferEnd) {
        if (nextClockSample_ >= currentSamplePosition) {
            MidiClockEvent event;
            event.status = 0xF8;  // MIDI Clock status byte
            event.samplePosition = nextClockSample_;
            
            outEvents.push_back(event);
            clockCount++;
        }
        
        nextClockSample_ += static_cast<int64_t>(samplesPerClock_);
    }
    
    return clockCount;
}

void TransportProtocolAgent::setTempo(double bpm) {
    // MESSAGE THREAD ONLY
    tempo_ = bpm;
    calculateClockTiming();
}

bool TransportProtocolAgent::isProtocolEnabled(SyncProtocol protocol) const {
    switch (protocol) {
        case SyncProtocol::MidiClock:
            return midiClockEnabled_.load();
        case SyncProtocol::MTC:
            return mtcEnabled_.load();
        case SyncProtocol::MMC:
            return mmcEnabled_.load();
        default:
            return false;
    }
}

void TransportProtocolAgent::calculateClockTiming() {
    // Calculate samples between MIDI clock messages
    // MIDI clock: 24 pulses per quarter note
    double beatsPerSecond = tempo_ / 60.0;
    double pulsesPerSecond = beatsPerSecond * kMidiClockPPQN;
    samplesPerClock_ = sampleRate_ / pulsesPerSecond;
}

} // namespace zenith
