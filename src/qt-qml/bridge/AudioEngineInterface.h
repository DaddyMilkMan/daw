/**
 * Audio Engine Interface
 *
 * This class bridges between the Qt/QML UI and the JUCE audio engine.
 * It exposes audio engine functionality to QML via Qt properties, signals, and slots.
 */

#pragma once

#include <QObject>
#include <QVector>
#include <QVariantMap>
#include <QTimer>
#include <QString>
#include <QUrl>

// Forward declare JUCE audio engine (will be implemented when JUCE is integrated)
// #include "AudioEngine.h"

class AudioEngineInterface : public QObject
{
    Q_OBJECT

    // Audio state properties - automatically bound to QML
    Q_PROPERTY(bool isPlaying READ isPlaying WRITE setIsPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isRecording READ isRecording WRITE setIsRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(double tempo READ tempo WRITE setTempo NOTIFY tempoChanged)
    Q_PROPERTY(int numerator READ numerator WRITE setNumerator NOTIFY timeSignatureChanged)
    Q_PROPERTY(int denominator READ denominator WRITE setDenominator NOTIFY timeSignatureChanged)
    Q_PROPERTY(double currentBar READ currentBar NOTIFY currentBarChanged)
    Q_PROPERTY(bool loopEnabled READ loopEnabled WRITE setLoopEnabled NOTIFY loopEnabledChanged)
    Q_PROPERTY(double loopStart READ loopStart WRITE setLoopStart NOTIFY loopRegionChanged)
    Q_PROPERTY(double loopEnd READ loopEnd WRITE setLoopEnd NOTIFY loopRegionChanged)
    Q_PROPERTY(bool metronomeEnabled READ metronomeEnabled WRITE setMetronomeEnabled NOTIFY metronomeEnabledChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)
    Q_PROPERTY(QVector<float> currentWaveform READ currentWaveform NOTIFY waveformChanged)
    Q_PROPERTY(float masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)

public:
    explicit AudioEngineInterface(QObject* parent = nullptr);
    ~AudioEngineInterface() override;

    // Initialize audio engine
    bool initialize();
    void shutdown();

    // Property getters
    bool isPlaying() const { return m_isPlaying; }
    bool isRecording() const { return m_isRecording; }
    double tempo() const { return m_tempo; }
    int numerator() const { return m_numerator; }
    int denominator() const { return m_denominator; }
    double currentBar() const { return m_currentBar; }
    bool loopEnabled() const { return m_loopEnabled; }
    double loopStart() const { return m_loopStart; }
    double loopEnd() const { return m_loopEnd; }
    bool metronomeEnabled() const { return m_metronomeEnabled; }
    QVariantList tracks() const { return m_tracks; }
    QVector<float> currentWaveform() const { return m_waveform; }
    float masterVolume() const { return m_masterVolume; }

    // Property setters
    void setIsPlaying(bool playing);
    void setIsRecording(bool recording);
    void setTempo(double bpm);
    void setNumerator(int numerator);
    void setDenominator(int denominator);
    void setLoopEnabled(bool enabled);
    void setLoopStart(double bar);
    void setLoopEnd(double bar);
    void setMetronomeEnabled(bool enabled);
    void setMasterVolume(float volume);

signals:
    // Property change notifications
    void isPlayingChanged(bool playing);
    void isRecordingChanged(bool recording);
    void tempoChanged(double bpm);
    void timeSignatureChanged(int numerator, int denominator);
    void currentBarChanged(double bar);
    void loopEnabledChanged(bool enabled);
    void loopRegionChanged(double start, double end);
    void metronomeEnabledChanged(bool enabled);
    void tracksChanged();
    void waveformChanged();
    void masterVolumeChanged(float volume);

    // Real-time updates
    void levelMeterUpdate(int trackId, float leftLevel, float rightLevel);
    void midiNoteReceived(int trackId, int pitch, int velocity);
    void errorOccurred(const QString& message);
    void warningOccurred(const QString& message);

public slots:
    // Transport controls
    void play();
    void stop();
    void record();
    void pause();
    void rewind();
    void fastForward();
    void gotoBar(double bar);

    // Track management
    void addTrack(const QString& name, bool isAudio);
    void removeTrack(int trackId);
    void duplicateTrack(int trackId);
    void renameTrack(int trackId, const QString& name);
    void setTrackVolume(int trackId, float volume);
    void setTrackPan(int trackId, float pan);
    void setTrackMute(int trackId, bool muted);
    void setTrackSolo(int trackId, bool solo);
    void setTrackRecordArm(int trackId, bool armed);
    void setTrackColor(int trackId, const QString& color);
    void moveTrack(int trackId, int newPosition);

    // Audio file operations
    void importAudioFile(int trackId, const QUrl& fileUrl, double position);
    void exportAudio(const QUrl& fileUrl, double startBar, double endBar);
    void exportWav(const QUrl& fileUrl);
    void exportMp3(const QUrl& fileUrl, int bitrate);

    // Plugin management
    void scanForPlugins();
    QVariantList getAvailablePlugins() const;
    void addPlugin(int trackId, const QString& pluginId);
    void removePlugin(int trackId, int pluginIndex);
    void togglePluginBypass(int trackId, int pluginIndex);
    void setPluginParameter(int trackId, int pluginIndex, int paramIndex, float value);
    void showPluginEditor(int trackId, int pluginIndex);

    // MIDI operations
    void addMidiNote(int trackId, int pitch, double time, double duration, int velocity);
    void removeMidiNote(int trackId, int noteId);
    void updateMidiNote(int trackId, int noteId, int pitch, double time, double duration, int velocity);
    void clearMidiNotes(int trackId);

    // Automation
    void addAutomationPoint(int trackId, const QString& parameter, double time, float value);
    void removeAutomationPoint(int trackId, const QString& parameter, int pointId);
    void clearAutomation(int trackId, const QString& parameter);

    // Session/arrangement view
    void createClip(int trackId, int sceneIndex, double length);
    void deleteClip(int clipId);
    void launchClip(int clipId);
    void stopClip(int trackId);
    void launchScene(int sceneIndex);

    // Project management
    void newProject();
    void saveProject(const QUrl& fileUrl);
    void loadProject(const QUrl& fileUrl);
    void saveProjectAs(const QUrl& fileUrl);

    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    // Audio device settings
    void setAudioDevice(const QString& deviceName);
    void setBufferSize(int samples);
    void setSampleRate(int rate);
    QVariantList getAvailableAudioDevices() const;

private slots:
    void updateLevelMeters();
    void updateWaveform();
    void updatePosition();

private:
    // JUCE audio engine instance (placeholder for now)
    // std::unique_ptr<AudioEngine> m_audioEngine;

    // Update timers
    QTimer* m_levelMeterTimer;
    QTimer* m_waveformTimer;
    QTimer* m_positionTimer;

    // Audio state
    bool m_isPlaying = false;
    bool m_isRecording = false;
    double m_tempo = 120.0;
    int m_numerator = 4;
    int m_denominator = 4;
    double m_currentBar = 0.0;
    bool m_loopEnabled = false;
    double m_loopStart = 0.0;
    double m_loopEnd = 4.0;
    bool m_metronomeEnabled = false;
    float m_masterVolume = 0.75f;

    // Tracks data
    QVariantList m_tracks;

    // Waveform data for visualization
    QVector<float> m_waveform;

    // Helper methods
    void syncTracksFromEngine();
    void createDemoTracks(); // Temporary for testing
};
