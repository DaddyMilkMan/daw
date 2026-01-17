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
    push(msg, 0);
  }

  void push(const juce::MidiMessage &msg, int sampleOffset) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
      buffer[start1] = {msg, sampleOffset};
      fifo.finishedWrite(1);
    }
  }

  bool pop(juce::MidiMessage &msg) {
    int ignoredOffset = 0;
    return pop(msg, ignoredOffset);
  }

  bool pop(juce::MidiMessage &msg, int &sampleOffset) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(1, start1, size1, start2, size2);

    if (size1 > 0) {
      msg = buffer[start1].message;
      sampleOffset = buffer[start1].sampleOffset;
      fifo.finishedRead(1);
      return true;
    }
    return false;
  }

  template <typename Fn>
  void drain(Fn &&fn) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(fifo.getNumReady(), start1, size1, start2, size2);

    if (size1 > 0) {
      for (int i = 0; i < size1; ++i) {
        fn(buffer[start1 + i].message);
      }
    }
    if (size2 > 0) {
      for (int i = 0; i < size2; ++i) {
        fn(buffer[start2 + i].message);
      }
    }
    fifo.finishedRead(size1 + size2);
  }

  template <typename Fn>
  void drainWithOffsets(Fn &&fn) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(fifo.getNumReady(), start1, size1, start2, size2);

    if (size1 > 0) {
      for (int i = 0; i < size1; ++i) {
        const auto &entry = buffer[start1 + i];
        fn(entry.message, entry.sampleOffset);
      }
    }
    if (size2 > 0) {
      for (int i = 0; i < size2; ++i) {
        const auto &entry = buffer[start2 + i];
        fn(entry.message, entry.sampleOffset);
      }
    }
    fifo.finishedRead(size1 + size2);
  }

  // Helper to drain all messages into a MidiBuffer with offsets
  void drainTo(juce::MidiBuffer &destination, int numSamples) {
    const int maxOffset = juce::jmax(0, numSamples - 1);
    drainWithOffsets([&destination, maxOffset](const juce::MidiMessage &msg,
                                               int sampleOffset) {
      destination.addEvent(msg, juce::jlimit(0, maxOffset, sampleOffset));
    });
  }

private:
  struct Entry {
    juce::MidiMessage message;
    int sampleOffset = 0;
  };

  juce::AbstractFifo fifo;
  std::vector<Entry> buffer;
};

} // namespace zenith
