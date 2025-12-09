/**
 * @file AudioEngineCore.h
 * @brief Pure C++20 interface for audio engine, isolated from UI dependencies.
 *
 * This interface defines the core audio engine functionality without any UI
 * dependencies, enabling pure Skia UI integration and eventual removal of JUCE UI.
 *
 * Thread Safety:
 * - Methods are categorized by thread requirements (Message Thread or Audio Thread Safe).
 * - Real-time safety is noted where applicable.
 *
 * Design Principles:
 * - No JUCE UI headers (no juce_gui_basics, juce_graphics, etc.)
 * - Use standard C++ types and forward declarations where possible.
 * - Error reporting via callbacks, not UI dialogs.
 * - Transport, mixing, metering, plugin hosting, and project state management.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declarations for JUCE audio types (no UI)
namespace juce {
class AudioBuffer;
template <typename T> class AudioBuffer;
class AudioDeviceManager;
class AudioPluginFormatManager;
class MidiMessage;
class MidiBuffer;
class String;
class File;
} // namespace juce

namespace zenith {

// Forward declarations
class ProjectState;
class TempoMap;
class AudioFilePool;
class PluginHost;
class PluginEditorWindowManager;
class InstrumentRegistry;
class Track;
class AuxBus;
class EngineEvent;

/**
 * @class AudioEngineCore
 * @brief Pure abstract interface for the Zenith DAW audio engine.
 */
class AudioEngineCore {
public:
  virtual ~AudioEngineCore() = default;

  //==========================================================================
  // Error & Event Callbacks (UI‑agnostic)
  //==========================================================================
  using ErrorHandler = std::function<void(const std::string &errorMessage)>;
  using StatusHandler = std::function<void(const std::string &statusMessage)>;
  using RecordingCompleteHandler = std::function<void()>;

  /**
   * @brief Set handler for audio device errors (replaces juce::AlertWindow).
   * @note Message thread safe.
   */
  virtual void setErrorHandler(ErrorHandler handler) = 0;

  /**
   * @brief Set handler for general status updates (e.g., "Recording started").
   * @note Message thread safe.
   */
  virtual void setStatusHandler(StatusHandler handler) = 0;

  /**
   * @brief Set handler for recording completion (replaces async callbacks).
   * @note Message thread safe.
   */
  virtual void setRecordingCompleteHandler(RecordingCompleteHandler handler) = 0;

  //==========================================================================
  // Initialization & Shutdown (Message Thread)
  //==========================================================================

  /**
   * @brief Initialize audio engine and open default device.
   * @return true on success; errors are reported via error handler.
   * @note Message thread only.
   */
  virtual bool initialize() = 0;

  /**
   * @brief Shutdown engine, close audio device, release resources.
   * @note Message thread only.
   */
  virtual void shutdown() = 0;

  //==========================================================================
  // Project State Integration (Message Thread)
  //==========================================================================

  /**
   * @brief Attach project state for automation, tempo, and track sync.
   * @param state Pointer to ProjectState (non‑owning).
   * @note Message thread only.
   */
  virtual void setProjectState(ProjectState *state) = 0;

  /**
   * @brief Get attached project state.
   * @return Pointer to ProjectState (may be null).
   * @note Message thread safe.
   */
  virtual ProjectState *getProjectState() const = 0;

  /**
   * @brief Synchronize engine tracks with current project state.
   * @note Message thread only.
   */
  virtual void syncWithProjectState() = 0;

  //==========================================================================
  // Transport Control (Message Thread)
  //==========================================================================

  virtual void play() = 0;
  virtual void stop() = 0;
  virtual bool isPlaying() const = 0;

  virtual void record() = 0;
  virtual void stopRecording() = 0;
  virtual bool isRecording() const = 0;
  virtual void toggleRecording() = 0;

  //==========================================================================
  // Transport Position & Looping (Message Thread for setters)
  //==========================================================================

  virtual std::int64_t getPlayheadSamples() const = 0;
  virtual double getPlaybackPositionBeats() const = 0;
  virtual void setPlayheadSamples(std::int64_t position) = 0;

  virtual void setLooping(bool shouldLoop) = 0;
  virtual bool isLooping() const = 0;
  virtual void setLoopRegion(std::int64_t start, std::int64_t end) = 0;
  virtual std::int64_t getLoopStart() const = 0;
  virtual std::int64_t getLoopEnd() const = 0;

  //==========================================================================
  // Audio Device Info (Thread Safe)
  //==========================================================================

  virtual std::string getAudioDeviceInfo() const = 0;
  virtual double getSampleRate() const = 0;
  virtual int getBufferSize() const = 0;
  virtual double getCpuUsage() const = 0;

  //==========================================================================
  // Track Management (Message Thread)
  //==========================================================================

  virtual int getNumTracks() const = 0;
  virtual std::string createTrack(const std::string &name,
                                  const std::string &type) = 0;
  virtual void removeTrack(int index) = 0;

  //==========================================================================
  // Mixer Control (Message Thread)
  //==========================================================================

  virtual void setTrackVolume(int trackIndex, float volume) = 0;
  virtual void setTrackPan(int trackIndex, float pan) = 0;
  virtual void setTrackMute(int trackIndex, bool muted) = 0;
  virtual void setTrackSolo(int trackIndex, bool solo) = 0;
  virtual void setTrackArmed(int trackIndex, bool armed) = 0;
  virtual void setTrackInputChannel(int trackIndex, int channelIndex) = 0;

  //==========================================================================
  // Metering (Audio Thread Safe)
  //==========================================================================

  virtual float getTrackLevel(int trackIndex) const = 0;
  virtual float getTrackPeakLevel(int trackIndex) const = 0;
  virtual float getMasterLevel() const = 0;
  virtual float getMasterPeakLevel() const = 0;
  virtual void resetPeakMeters() = 0;

  //==========================================================================
  // Plugin Delay Compensation (Message Thread)
  //==========================================================================

  virtual int getTrackLatency(int trackIndex) const = 0;
  virtual int getMasterLatency() const = 0;
  virtual void recalculatePDC() = 0;
  virtual bool isPDCEnabled() const = 0;
  virtual void setPDCEnabled(bool enabled) = 0;
  virtual int getMaxTrackLatency() const = 0;

  //==========================================================================
  // Aux Bus Management (Message Thread)
  //==========================================================================

  virtual int createAuxBus(const std::string &name) = 0;
  virtual void removeAuxBus(int auxIndex) = 0;
  virtual int getNumAuxBuses() const = 0;
  virtual float getAuxBusLevel(int auxIndex) const = 0;
  virtual float getAuxBusPeakLevel(int auxIndex) const = 0;

  //==========================================================================
  // Plugin Hosting (Message Thread)
  //==========================================================================

  virtual PluginHost &getPluginHost() = 0;
  virtual int scanForPlugins() = 0;
  virtual PluginEditorWindowManager &getPluginEditorWindowManager() = 0;

  //==========================================================================
  // Core Services Access (Thread Safe where noted)
  //==========================================================================

  virtual AudioFilePool &getAudioFilePool() = 0;
  virtual const TempoMap &getTempoMap() const = 0;
  virtual InstrumentRegistry &getInstrumentRegistry() = 0;
  virtual const InstrumentRegistry &getInstrumentRegistry() const = 0;

  //==========================================================================
  // Real‑time Event Queue (Lock‑Free)
  //==========================================================================

  virtual bool queueEvent(const EngineEvent &e) = 0;

  //==========================================================================
  // Project Export (Message Thread)
  //==========================================================================

  enum class ExportFormat { WAV, FLAC, OGG };

  struct ExportOptions {
    juce::File outputFile;
    double sampleRate = 44100.0;
    int bitDepth = 24;
    ExportFormat format = ExportFormat::WAV;
    bool enableDither = true;
    bool normalize = false;
    double normalizeDb = -0.1;
    double duration = 0.0;
  };

  virtual bool exportProjectToWav(const juce::File &outputFile,
                                  double sampleRate, int bitDepth,
                                  double durationInSeconds) = 0;
  virtual bool exportProject(const ExportOptions &options) = 0;

  //==========================================================================
  // Factory Function
  //==========================================================================

  static std::unique_ptr<AudioEngineCore> create();
};

} // namespace zenith