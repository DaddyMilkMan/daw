/*
  ==============================================================================
    MixingAssistant.h
    AI-powered mixing assistant for intelligent audio processing
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ai/GrokAPIClient.h"
#include "../audio/RealTimeAudioBuffer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace zenith {
namespace ai {

// Mix analysis results
struct MixAnalysis {
    // Overall mix metrics
    float overallLoudness = -23.0f;  // LUFS
    float dynamicRange = 12.0f;      // dB
    float stereoWidth = 1.0f;
    float clarity = 0.7f;
    float punch = 0.6f;
    float warmth = 0.5f;
    float brightness = 0.5f;
    
    // Frequency balance
    float subBassLevel = 0.0f;   // 20-60 Hz
    float bassLevel = 0.0f;      // 60-250 Hz
    float lowMidLevel = 0.0f;    // 250-500 Hz
    float midLevel = 0.0f;       // 500-2000 Hz
    float highMidLevel = 0.0f;   // 2000-4000 Hz
    float presenceLevel = 0.0f;  // 4000-6000 Hz
    float brillianceLevel = 0.0f; // 6000-20000 Hz
    
    // Track-specific analysis
    struct TrackAnalysis {
        juce::String trackId;
        juce::String trackName;
        juce::String trackType;  // "kick", "snare", "bass", "vocal", etc.
        float loudness = -23.0f;
        float pan = 0.0f;
        float frequencyCenter = 1000.0f;
        float maskingIndex = 0.0f;  // How much this track masks others
        float clarityIndex = 0.0f;  // How clear this track is in the mix
        bool hasClipping = false;
        float peakLevel = -1.0f;
        juce::String suggestedAction;
    };
    
    std::vector<TrackAnalysis> trackAnalyses;
    
    // Issues and suggestions
    struct MixIssue {
        juce::String description;
        juce::String severity;  // "low", "medium", "high", "critical"
        juce::String category;  // "loudness", "frequency", "dynamic", "stereo", "clipping"
        juce::String suggestion;
        juce::String affectedTrack;
        float confidence = 0.0f;
    };
    
    std::vector<MixIssue> issues;
    
    // Genre-specific targets
    juce::String detectedGenre;
    juce::String targetGenre;
    std::unordered_map<juce::String, float> genreTargets;
};

// Mix suggestion
struct MixSuggestion {
    juce::String id;
    juce::String type;  // "eq", "compression", "reverb", "delay", "volume", "pan"
    juce::String targetTrack;
    juce::String description;
    juce::var parameters;
    float confidence = 0.0f;
    float priority = 0.0f;  // 0.0 to 1.0
    bool isApplied = false;
    juce::Time timestamp;
};

// Mixing assistant configuration
struct MixingAssistantConfig {
    // Analysis settings
    bool enableRealTimeAnalysis = true;
    int analysisInterval = 1000;  // ms
    float analysisWindow = 10.0f;  // seconds
    
    // AI settings
    bool enableAISuggestions = true;
    float suggestionThreshold = 0.6f;
    int maxSuggestionsPerTrack = 5;
    bool learnFromUserActions = true;
    
    // Genre settings
    bool autoDetectGenre = true;
    juce::String targetGenre = "";
    bool useGenreSpecificTargets = true;
    
    // Processing settings
    bool enableAutoCorrection = false;
    float correctionStrength = 0.5f;
    bool preserveDynamics = true;
    bool preserveStereoImage = true;
    
    // UI settings
    bool showRealTimeFeedback = true;
    bool showSuggestions = true;
    bool showAnalysis = true;
    bool enableUndoRedo = true;
};

// AI-powered mixing assistant
class MixingAssistant : public juce::Timer,
                       public juce::AsyncUpdater {
public:
    MixingAssistant();
    ~MixingAssistant() override;
    
    // Configuration
    void setConfig(const MixingAssistantConfig& config);
    MixingAssistantConfig getConfig() const;
    
    // Audio input
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void addTrackAudio(const juce::String& trackId, const juce::AudioBuffer<float>& buffer, double sampleRate);
    void removeTrack(const juce::String& trackId);
    
    // Analysis
    void analyzeMix();
    void analyzeTrack(const juce::String& trackId);
    MixAnalysis getCurrentAnalysis() const;
    MixAnalysis getTrackAnalysis(const juce::String& trackId) const;
    
    // Suggestions
    void generateSuggestions();
    std::vector<MixSuggestion> getSuggestions() const;
    std::vector<MixSuggestion> getTrackSuggestions(const juce::String& trackId) const;
    void applySuggestion(const juce::String& suggestionId);
    void rejectSuggestion(const juce::String& suggestionId);
    void clearSuggestions();
    
    // AI integration
    void setGrokClient(std::shared_ptr<GrokAPIClient> client);
    void requestAISuggestion(const juce::String& trackId, const juce::String& type);
    void learnFromUserAction(const juce::String& trackId, const juce::String& action, const juce::var& parameters);
    
    // Auto-mixing
    void startAutoMix();
    void stopAutoMix();
    bool isAutoMixActive() const;
    void setAutoMixStrength(float strength);
    
    // Genre detection
    juce::String detectGenre();
    void setTargetGenre(const juce::String& genre);
    std::vector<juce::String> getSupportedGenres() const;
    
    // Reference tracks
    void loadReferenceTrack(const juce::File& file);
    void analyzeReferenceTrack();
    void matchToReference();
    void clearReferenceTrack();
    
    // Timer callback for real-time analysis
    void timerCallback() override;
    void handleAsyncUpdate() override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void analysisUpdated(const MixAnalysis& analysis) {}
        virtual void suggestionGenerated(const MixSuggestion& suggestion) {}
        virtual void suggestionApplied(const MixSuggestion& suggestion) {}
        virtual void suggestionRejected(const MixSuggestion& suggestion) {}
        virtual void genreDetected(const juce::String& genre) {}
        virtual void autoMixStarted() {}
        virtual void autoMixStopped() {}
        virtual void referenceTrackLoaded() {}
        virtual void referenceTrackMatched() {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Configuration
    MixingAssistantConfig config;
    
    // Audio data
    juce::AudioBuffer<float> mainAudioBuffer;
    double mainSampleRate = 44100.0;
    std::unordered_map<juce::String, juce::AudioBuffer<float>> trackBuffers;
    std::unordered_map<juce::String, double> trackSampleRates;
    std::mutex audioMutex;
    
    // Analysis
    MixAnalysis currentAnalysis;
    std::unordered_map<juce::String, MixAnalysis> trackAnalyses;
    std::mutex analysisMutex;
    
    // Suggestions
    std::vector<MixSuggestion> suggestions;
    std::mutex suggestionsMutex;
    
    // AI
    std::shared_ptr<GrokAPIClient> grokClient;
    
    // Auto-mix
    std::atomic<bool> autoMixActive{false};
    float autoMixStrength = 0.5f;
    
    // Genre
    juce::String detectedGenre;
    juce::String targetGenre;
    
    // Reference track
    juce::AudioBuffer<float> referenceBuffer;
    double referenceSampleRate = 44100.0;
    MixAnalysis referenceAnalysis;
    bool hasReferenceTrack = false;
    
    // State
    std::atomic<bool> isAnalyzing{false};
    std::atomic<bool> needsAnalysis{false};
    std::atomic<bool> needsSuggestions{false};
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Analysis methods
    void analyzeLoudness(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeFrequencyBalance(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeStereoImage(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeDynamics(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeClipping(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void detectIssues(MixAnalysis& analysis);
    
    // Track analysis
    void analyzeTrackLevel(const juce::String& trackId, MixAnalysis::TrackAnalysis& trackAnalysis);
    void analyzeTrackFrequency(const juce::String& trackId, MixAnalysis::TrackAnalysis& trackAnalysis);
    void analyzeTrackMasking(const juce::String& trackId, MixAnalysis::TrackAnalysis& trackAnalysis);
    
    // Suggestion generation
    void generateVolumeSuggestions();
    void generateEQSuggestions();
    void generateCompressionSuggestions();
    void generateReverbSuggestions();
    void generateStereoSuggestions();
    void generateClippingSuggestions();
    
    // AI suggestion methods
    void requestVolumeAISuggestion(const juce::String& trackId);
    void requestEQAISuggestion(const juce::String& trackId);
    void requestCompressionAISuggestion(const juce::String& trackId);
    void requestReverbAISuggestion(const juce::String& trackId);
    
    // Auto-mix methods
    void performAutoMix();
    void autoMixLevels();
    void autoMixEQ();
    void autoMixCompression();
    void autoMixStereo();
    
    // Genre methods
    std::unordered_map<juce::String, float> getGenreTargets(const juce::String& genre);
    void applyGenreTargets();
    
    // Reference track methods
    void matchLoudnessToReference();
    void matchFrequencyToReference();
    void matchStereoToReference();
    
    // Utility methods
    float calculateLUFS(const juce::AudioBuffer<float>& buffer);
    float calculateRMS(const juce::AudioBuffer<float>& buffer);
    float calculatePeak(const juce::AudioBuffer<float>& buffer);
    float calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    std::vector<float> calculateSpectrum(const juce::AudioBuffer<float>& buffer);
    float calculateFrequencyCenter(const juce::AudioBuffer<float>& buffer);
    float calculateMaskingIndex(const juce::AudioBuffer<float>& track1, const juce::AudioBuffer<float>& track2);
    
    // Notification
    void notifyAnalysisUpdated(const MixAnalysis& analysis);
    void notifySuggestionGenerated(const MixSuggestion& suggestion);
    void notifySuggestionApplied(const MixSuggestion& suggestion);
    void notifySuggestionRejected(const MixSuggestion& suggestion);
    void notifyGenreDetected(const juce::String& genre);
    void notifyAutoMixStarted();
    void notifyAutoMixStopped();
    void notifyReferenceTrackLoaded();
    void notifyReferenceTrackMatched();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixingAssistant)
};

// Mix suggestion UI component
class MixSuggestionComponent : public juce::Component,
                              public juce::Button::Listener {
public:
    MixSuggestionComponent(const MixSuggestion& suggestion);
    ~MixSuggestionComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Button::Listener
    void buttonClicked(juce::Button* button) override;
    
    // Access methods
    const MixSuggestion& getSuggestion() const { return suggestion; }
    void setApplied(bool applied);
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void suggestionApplied(const MixSuggestion& suggestion) {}
        virtual void suggestionRejected(const MixSuggestion& suggestion) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    MixSuggestion suggestion;
    
    // UI components
    std::unique_ptr<juce::Label> descriptionLabel;
    std::unique_ptr<juce::Label> confidenceLabel;
    std::unique_ptr<juce::Label> priorityLabel;
    std::unique_ptr<juce::TextButton> applyButton;
    std::unique_ptr<juce::TextButton> rejectButton;
    std::unique_ptr<juce::ProgressBar> progressBar;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixSuggestionComponent)
};

// Mixing assistant UI
class MixingAssistantUI : public juce::Component,
                         public MixingAssistant::Listener {
public:
    MixingAssistantUI();
    ~MixingAssistantUI() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Assistant access
    void setAssistant(MixingAssistant* assistant);
    MixingAssistant* getAssistant() const { return assistant; }
    
    // UI controls
    void showAnalysis(bool show);
    void showSuggestions(bool show);
    void showAutoMix(bool show);
    
    // MixingAssistant::Listener
    void analysisUpdated(const MixAnalysis& analysis) override;
    void suggestionGenerated(const MixSuggestion& suggestion) override;
    void suggestionApplied(const MixSuggestion& suggestion) override;
    void genreDetected(const juce::String& genre) override;
    void autoMixStarted() override;
    void autoMixStopped() override;
    
private:
    MixingAssistant* assistant = nullptr;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Analysis panel
    std::unique_ptr<juce::Component> analysisPanel;
    std::unique_ptr<juce::Label> loudnessLabel;
    std::unique_ptr<juce::Label> dynamicRangeLabel;
    std::unique_ptr<juce::Label> stereoWidthLabel;
    std::unique_ptr<juce::Label> clarityLabel;
    std::unique_ptr<juce::Label> genreLabel;
    
    // Frequency spectrum
    std::unique_ptr<juce::Component> spectrumComponent;
    
    // Suggestions panel
    std::unique_ptr<juce::Viewport> suggestionsViewport;
    std::unique_ptr<juce::Component> suggestionsComponent;
    std::vector<std::unique_ptr<MixSuggestionComponent>> suggestionComponents;
    
    // Auto-mix controls
    std::unique_ptr<juce::ToggleButton> autoMixToggle;
    std::unique_ptr<juce::Slider> autoMixStrengthSlider;
    std::unique_ptr<juce::TextButton> applyAllButton;
    std::unique_ptr<juce::TextButton> clearAllButton;
    
    // Settings
    std::unique_ptr<juce::TextButton> settingsButton;
    std::unique_ptr<juce::ToggleButton> realTimeToggle;
    std::unique_ptr<juce::ToggleButton> aiToggle;
    
    // UI creation
    void createAnalysisPanel();
    void createSuggestionsPanel();
    void createAutoMixControls();
    void createSettingsControls();
    
    // Updates
    void updateAnalysisDisplay();
    void updateSuggestionsDisplay();
    void updateAutoMixControls();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixingAssistantUI)
};

// Mixing assistant factory
class MixingAssistantFactory {
public:
    static std::unique_ptr<MixingAssistant> createDefaultAssistant();
    static std::unique_ptr<MixingAssistant> createProfessionalAssistant();
    static std::unique_ptr<MixingAssistant> createBeginnerAssistant();
    
    // Default configurations
    static MixingAssistantConfig getDefaultConfig();
    static MixingAssistantConfig getProfessionalConfig();
    static MixingAssistantConfig getBeginnerConfig();
    
    // Genre presets
    static std::unordered_map<juce::String, float> getRockTargets();
    static std::unordered_map<juce::String, float> getPopTargets();
    static std::unordered_map<juce::String, float> getJazzTargets();
    static std::unordered_map<juce::String, float> getElectronicTargets();
    static std::unordered_map<juce::String, float> getOrchestralTargets();
    
private:
    static void setupDefaultAnalysis(MixingAssistant& assistant);
    static void setupProfessionalAnalysis(MixingAssistant& assistant);
    static void setupBeginnerAnalysis(MixingAssistant& assistant);
};

} // namespace ai
} // namespace zenith
