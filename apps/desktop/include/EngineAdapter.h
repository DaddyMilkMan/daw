/**
 * @file EngineAdapter.h
 * @brief Adapter wrapper that implements AudioEngineCore by delegating to Engine.
 *
 * This class provides a pure C++20 interface to the existing Engine,
 * enabling UI-agnostic access and paving the way for pure Skia UI.
 */

#pragma once

#include "AudioEngineCore.h"
#include "Engine.h"

namespace zenith {

/**
 * @class EngineAdapter
 * @brief Concrete implementation of AudioEngineCore that wraps an Engine instance.
 *
 * Thread Safety:
 * - Forwards all calls to the underlying Engine, preserving its thread-safety guarantees.
 * - Callback handlers are set via message thread and invoked from appropriate contexts.
 */
class EngineAdapter : public AudioEngineCore {
public:
  /**
   * @brief Construct an adapter wrapping an existing Engine.
   * @param engine Reference to Engine (must outlive this adapter)
   */
  explicit EngineAdapter(Engine& engine);

  ~EngineAdapter() override = default;

  //==========================================================================
  // Error & Event Callbacks (UI‑agnostic)
  //==========================================================================
  void setErrorHandler(ErrorHandler handler) override;
  void setStatusHandler(StatusHandler handler) override;
  void setRecordingCompleteHandler(RecordingCompleteHandler handler) override;

  //==========================================================================
  // Initialization & Shutdown (Message Thread)
  //==========================================================================
  bool initialize() override;
  void shutdown() override;

  //==========================================================================
  // Project State Integration (Message Thread)
  //==========================================================================
  void setProjectState(ProjectState* state) override;
  ProjectState* getProjectState() const override;
  void syncWithProjectState() override;

  //==========================================================================
  // Transport Control (Message Thread)
  //==========================================================================
  void play() override;
  void stop() override;
  bool isPlaying() const override;

  void record() override;
  void stopRecording() override;
  bool isRecording() const override;
  void toggleRecording() override;

  //==========================================================================
  // Transport Position & Looping (Message Thread for setters)
  //==========================================================================
  std::int64_t getPlayheadSamples() const override;
  double getPlaybackPositionBeats() const override;
  void setPlayheadSamples(std::int64_t position) override;

  void setLooping(bool shouldLoop) override;
  bool isLooping() const override;
  void setLoopRegion(std::int64_t start, std::int64_t end) override;
  std::int64_t getLoopStart() const override;
  std::int64_t getLoopEnd() const override;

  //==========================================================================
  // Audio Device Info (Thread Safe)
  //==========================================================================
  std::string getAudioDeviceInfo() const override;
  double getSampleRate() const override;
  int getBufferSize() const override;
  double getCpuUsage() const override;

  //==========================================================================
  // Track Management (Message Thread)
  //==========================================================================
  int getNumTracks() const override;
  std::string createTrack(const std::string& name, const std::string& type) override;
  void removeTrack(int index) override;

  //==========================================================================
  // Mixer Control (Message Thread)
  //==========================================================================
  void setTrackVolume(int trackIndex, float volume) override;
  void setTrackPan(int trackIndex, float pan) override;
  void setTrackMute(int trackIndex, bool muted) override;
  void setTrackSolo(int trackIndex, bool solo) override;
  void setTrackArmed(int trackIndex, bool armed) override;
  void setTrackInputChannel(int trackIndex, int channelIndex) override;

  //==========================================================================
  // Metering (Audio Thread Safe)
  //==========================================================================
  float getTrackLevel(int trackIndex) const override;
  float getTrackPeakLevel(int trackIndex) const override;
  float getMasterLevel() const override;
  float getMasterPeakLevel() const override;
  void resetPeakMeters() override;

  //==========================================================================
  // Plugin Delay Compensation (Message Thread)
  //==========================================================================
  int getTrackLatency(int trackIndex) const override;
  int getMasterLatency() const override;
  void recalculatePDC() override;
  bool isPDCEnabled() const override;
  void setPDCEnabled(bool enabled) override;
  int getMaxTrackLatency() const override;

  //==========================================================================
  // Aux Bus Management (Message Thread)
  //==========================================================================
  int createAuxBus(const std::string& name) override;
  void removeAuxBus(int auxIndex) override;
  int getNumAuxBuses() const override;
  float getAuxBusLevel(int auxIndex) const override;
  float getAuxBusPeakLevel(int auxIndex) const override;

  //==========================================================================
  // Plugin Hosting (Message Thread)
  //==========================================================================
  PluginHost& getPluginHost() override;
  int scanForPlugins() override;
  PluginEditorWindowManager& getPluginEditorWindowManager() override;

  //==========================================================================
  // Core Services Access (Thread Safe where noted)
  //==========================================================================
  AudioFilePool& getAudioFilePool() override;
  const TempoMap& getTempoMap() const override;
  InstrumentRegistry& getInstrumentRegistry() override;
  const InstrumentRegistry& getInstrumentRegistry() const override;

  //==========================================================================
  // Real‑time Event Queue (Lock‑Free)
  //==========================================================================
  bool queueEvent(const EngineEvent& e) override;

  //==========================================================================
  // Project Export (Message Thread)
  //==========================================================================
  bool exportProjectToWav(const juce::File& outputFile,
                          double sampleRate, int bitDepth,
                          double durationInSeconds) override;
  bool exportProject(const ExportOptions& options) override;

  //==========================================================================
  // Factory Function Implementation (from AudioEngineCore)
  //==========================================================================
  static std::unique_ptr<AudioEngineCore> create();

private:
  Engine& engine_;
  ErrorHandler errorHandler_;
  StatusHandler statusHandler_;
  RecordingCompleteHandler recordingCompleteHandler_;

  // Helper to convert juce::String to std::string
  static std::string toString(const juce::String& s);
  // Helper to convert std::string to juce::String
  static juce::String toJuceString(const std::string& s);
};

} // namespace zenith