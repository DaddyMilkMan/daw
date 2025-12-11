/*
  ==============================================================================

    EngineEvent.h
    Created: 2025-12-02
    Author:  Zenith DAW

    Lock-free event structure for Engine communication.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h> // For MidiMessage, MidiBuffer, AbstractFifo
#include <juce_core/juce_core.h>
#include <vector>


namespace zenith {

struct EngineEvent {
  enum class Type {
    SetPluginParam,
    SetTrackVolume,
    SetTrackPan,
    SetTrackMute,
    SetTrackSolo,
    TransportPlay,
    TransportStop,
    TransportRecord,
    TransportRewind,
    None
  };

  Type type = Type::None;

  // Identifiers
  int trackIndex = -1;
  int pluginIndex = -1;
  int paramIndex = -1;

  // Data
  float value = 0.0f;
  bool boolValue = false;

  // For efficient copying
  EngineEvent() = default;
  EngineEvent(Type t) : type(t) {}
};

// Simple ring buffer for MIDI messages to avoid allocation in audio thread
class MidiFifo {
public:
  MidiFifo() : fifo(4096) { buffer.resize(4096); }

  void push(const juce::MidiMessage &msg) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
      buffer[start1] = msg;
      fifo.finishedWrite(1);
    }
  }

  bool pop(juce::MidiMessage &msg) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(1, start1, size1, start2, size2);

    if (size1 > 0) {
      msg = buffer[start1];
      fifo.finishedRead(1);
      return true;
    }
    return false;
  }

  // Helper to drain all messages into a MidiBuffer with offsets
  void drainTo(juce::MidiBuffer &destination, int numSamples) {
    juce::ignoreUnused(numSamples);
    int start1, size1, start2, size2;
    fifo.prepareToRead(fifo.getNumReady(), start1, size1, start2, size2);

    if (size1 > 0) {
      for (int i = 0; i < size1; ++i) {
        destination.addEvent(buffer[start1 + i],
                             0); // Add at start of block for now
      }
    }
    if (size2 > 0) {
      for (int i = 0; i < size2; ++i) {
        destination.addEvent(buffer[start2 + i], 0);
      }
    }
    fifo.finishedRead(size1 + size2);
  }

private:
  juce::AbstractFifo fifo;
  std::vector<juce::MidiMessage> buffer;
};

} // namespace zenith
