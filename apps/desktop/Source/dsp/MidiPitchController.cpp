/*
  ==============================================================================

    MidiPitchController.cpp
    Created: 2026-01-29
    Author:  Zenith DAW

    MIDI-controlled pitch target implementation.

  ==============================================================================
*/

#include "MidiPitchController.h"
#include <cmath>

namespace zenith {
namespace dsp {

//==============================================================================
MidiPitchController::MidiPitchController()
{
    activeNotes_.fill(false);
    noteVelocities_.fill(0);
    notesHeldBySustain_.fill(false);
}

MidiPitchController::~MidiPitchController()
{
}

//==============================================================================
void MidiPitchController::reset()
{
    activeNotes_.fill(false);
    noteVelocities_.fill(0);
    notesHeldBySustain_.fill(false);
    
    currentNote_.store(-1);
    targetPitchHz_.store(0.0f);
    smoothedPitchHz_.store(0.0f);
    isActive_.store(false);
    pitchBendAmount_.store(0.0f);
    sustainPedalDown_.store(false);
    lowestActiveNote_ = -1;
    highestActiveNote_ = -1;
    lastNotePlayed_ = -1;
}

//==============================================================================
void MidiPitchController::processMidi(const juce::MidiBuffer& midiBuffer, double sampleRate)
{
    sampleRate_ = sampleRate;
    
    for (const auto metadata : midiBuffer)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            noteOn(message.getNoteNumber(), message.getVelocity());
        }
        else if (message.isNoteOff())
        {
            noteOff(message.getNoteNumber());
        }
        else if (message.isPitchWheel())
        {
            pitchBend(message.getPitchWheelValue());
        }
        else if (message.isControllerOfType(64))  // Sustain pedal
        {
            sustainPedal(message.getControllerValue() >= 64);
        }
    }
    
    // Update target pitch based on current state
    updateTargetPitch();
}

//==============================================================================
void MidiPitchController::noteOn(int note, int velocity)
{
    if (note < 0 || note >= kMaxNotes)
        return;
    
    activeNotes_[note] = true;
    noteVelocities_[note] = velocity;
    lastNotePlayed_ = note;
    
    // Calculate correction amount from velocity
    float velNorm = velocity / 127.0f;
    float sens = velocitySensitivity_.load();
    float newAmount = 0.3f + velNorm * sens * 0.7f;  // Minimum 30% correction
    correctionAmount_.store(newAmount);
    
    updateTargetPitch();
}

void MidiPitchController::noteOff(int note)
{
    if (note < 0 || note >= kMaxNotes)
        return;
    
    if (sustainPedalDown_.load())
    {
        // Sustain pedal is down - hold the note
        notesHeldBySustain_[note] = true;
    }
    else
    {
        activeNotes_[note] = false;
        noteVelocities_[note] = 0;
    }
    
    updateTargetPitch();
}

void MidiPitchController::pitchBend(int value)
{
    // Convert 0-16383 to -1 to +1
    float normalized = (value - 8192) / 8192.0f;
    pitchBendAmount_.store(normalized);
}

void MidiPitchController::sustainPedal(bool down)
{
    sustainPedalDown_.store(down);
    
    if (!down)
    {
        // Pedal released - clear all sustained notes
        for (int i = 0; i < kMaxNotes; ++i)
        {
            if (notesHeldBySustain_[i])
            {
                notesHeldBySustain_[i] = false;
                activeNotes_[i] = false;
                noteVelocities_[i] = 0;
            }
        }
        
        updateTargetPitch();
    }
}

//==============================================================================
void MidiPitchController::updateTargetPitch()
{
    // Find active notes
    lowestActiveNote_ = -1;
    highestActiveNote_ = -1;
    int numActive = 0;
    
    for (int i = 0; i < kMaxNotes; ++i)
    {
        if (activeNotes_[i] || notesHeldBySustain_[i])
        {
            if (lowestActiveNote_ == -1)
                lowestActiveNote_ = i;
            highestActiveNote_ = i;
            numActive++;
        }
    }
    
    if (numActive == 0)
    {
        // No active notes - MIDI not controlling pitch
        isActive_.store(false);
        currentNote_.store(-1);
        return;
    }
    
    isActive_.store(true);
    
    // Determine target note
    int targetNote;
    
    if (chordMode_.load() && numActive > 1)
    {
        // Chord mode - use lowest note (bass) as primary target
        targetNote = lowestActiveNote_;
    }
    else
    {
        // Single note mode - use last played note
        if (activeNotes_[lastNotePlayed_] || notesHeldBySustain_[lastNotePlayed_])
        {
            targetNote = lastNotePlayed_;
        }
        else
        {
            // Last note released, find highest active (most recent usually)
            targetNote = highestActiveNote_;
        }
    }
    
    currentNote_.store(targetNote);
    
    // Calculate target frequency with pitch bend
    float pitchBendSemitones = pitchBendAmount_.load() * pitchBendRange_.load();
    float targetHz = midiNoteToHz(targetNote + pitchBendSemitones);
    targetPitchHz_.store(targetHz);
    
    // Apply glide/portamento
    if (glideEnabled_.load() && smoothedPitchHz_.load() > 0.0f)
    {
        float currentSmoothed = smoothedPitchHz_.load();
        float glideSamples = glideTime_.load() * sampleRate_;
        
        if (glideSamples > 0.0f)
        {
            float glideFactor = 1.0f / glideSamples;
            float newSmoothed = currentSmoothed + (targetHz - currentSmoothed) * glideFactor;
            smoothedPitchHz_.store(newSmoothed);
        }
        else
        {
            smoothedPitchHz_.store(targetHz);
        }
    }
    else
    {
        smoothedPitchHz_.store(targetHz);
    }
}

float MidiPitchController::midiNoteToHz(int note) const
{
    // A4 = 69 = 440Hz
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

} // namespace dsp
} // namespace zenith
