/*
  ==============================================================================
    MixingAssistant.h
    AI-powered mixing assistant for intelligent audio processing
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "GrokAPIClient.h"
#include <zenith_core/audio/RealTimeAudioBuffer.h>
#include <zenith_ui/ui/framework/SkiaComponent.h>
#include <zenith_ui/ui/controls/SkiaListBox.h>
#include <zenith_ui/ui/controls/ZenithButton.h>
#include <zenith_ui/ui/controls/ZenithToggle.h>
#include <zenith_ui/ui/controls/ZenithSlider.h>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace zenith {
namespace ai {

// Mix analysis results
struct MixAnalysis {
    // Overall mix metrics
    float overallLoudness = -23.0f;  // LUFS
    float momentaryLUFS = -23.0f;
    float shortTermLUFS = -23.0f;
    float loudnessRange = 0.0f;
    float truePeak = -100.0f;
    float samplePeak = -100.0f;
    float crestFactor = 0.0f;
    float dynamicRange = 12.0f;      // dB
    float stereoWidth = 1.0f;
    float phaseCorrelation = 1.0f;
    float midSideRatio = 0.0f;
    
    float clarity = 0.7f;
    float punch = 0.6f;
    float warmth = 0.5f;
    float brightness = 0.5f;
    float presence = 0.5f;
    float stereoImage = 0.5f;
    float overallScore = 0.0f;
    
    // Frequency balance
    float subBassLevel = 0.0f;   // 20-60 Hz
    float bassLevel = 0.0f;      // 60-250 Hz
    float lowMidLevel = 0.0f;    // 250-500 Hz
    float midLevel = 0.0f;       // 500-2000 Hz
    float highMidLevel = 0.0f;   // 2000-4000 Hz
    float presenceLevel = 0.0f;  // 4000-6000 Hz
    float brillianceLevel = 0.0f; // 6000-20000 Hz
    
    // Spectral features
    float spectralCentroid = 1000.0f;
    
    // Dynamics history
    std::vector<float> rmsHistory;
    float averageRMS = -23.0f;
    float rmsVariation = 0.0f;
    float peakToRMSRatio = 0.0f;
    
    // Pitch and Key
    float fundamentalFrequency = 0.0f;
    float fundamentalConfidence = 0.0f;
    juce::String detectedKey = "C";
    juce::String detectedMode = "major";
    float keyConfidence = 0.0f;
    
    // Rhythm
    float tempo = 120.0f;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    float timeSignatureConfidence = 0.0f;
    std::vector<float> beatPositions;
    std::vector<float> beatStrengths;
    std::vector<float> onsets;
    
    // Timbre
    std::vector<float> mfccCoefficients;
    float roughness = 0.0f;
    juce::String primaryInstrument;
    float instrumentConfidence = 0.0f;
    std::vector<std::pair<juce::String, float>> instrumentProbabilities;
    
    // Channel-specific metrics
    std::vector<float> channelLUFS;
    std::vector<float> channelPeaks;
    
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
    std::vector<juce::String> recommendations;
    
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
    int fftOrder = 11;             // 2048 samples
    juce::dsp::WindowingFunction<float>::WindowingMethod windowMethod = juce::dsp::WindowingFunction<float>::hann;
    int updateInterval = 500;
    
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
    MixAnalysis::TrackAnalysis getTrackAnalysis(const juce::String& trackId) const;
    
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
    mutable std::mutex audioMutex;
    
    // Analysis
    MixAnalysis currentAnalysis;
    std::unordered_map<juce::String, MixAnalysis::TrackAnalysis> trackAnalyses;
    mutable std::mutex analysisMutex;
    
    // Suggestions
    std::vector<MixSuggestion> suggestions;
    mutable std::mutex suggestionsMutex;
    
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
    
    // DSP
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> windowFunction_;
    std::vector<std::complex<float>> fftBuffer;
    std::vector<float> magnitudeBuffer;
    std::vector<float> phaseBuffer;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Analysis methods
    void analyzeMainMix();
    void analyzeAllTracks();
    void analyzeLoudness(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeFrequencyBalance(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeStereoImage(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeDynamics(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzePitch(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeRhythm(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeTimbre(MixAnalysis& analysis, const juce::AudioBuffer<float>& buffer);
    void analyzeQuality(MixAnalysis& analysis);
    void analyzeHarmony(MixAnalysis& analysis);
    void detectKey(MixAnalysis& analysis);
    float detectTempo();
    void detectTimeSignature(MixAnalysis& analysis);
    void trackBeats(MixAnalysis& analysis);
    void detectOnsets(MixAnalysis& analysis);
    void calculateMFCC(MixAnalysis& analysis);
    void calculateTimbreDescriptors(MixAnalysis& analysis);
    void classifyInstrument(MixAnalysis& analysis);
    void assessClarity(MixAnalysis& analysis);
    void assessPunch(MixAnalysis& analysis);
    void assessWarmth(MixAnalysis& analysis);
    void detectIssues(MixAnalysis& analysis);
    void detectMixIssues();
    void generateRecommendations(MixAnalysis& analysis);
    void calculateOverallScore(MixAnalysis& analysis);
    void calculateSpectralFeatures(MixAnalysis& analysis);
    float detectFundamental(const std::vector<float>& magnitude);
    float calculatePitchConfidence(const juce::AudioBuffer<float>& buffer);
    
    // Track analysis
    void analyzeTrackFrequency(const juce::String& trackId, MixAnalysis::TrackAnalysis& trackAnalysis);
    void analyzeTrackMasking(const juce::String& trackId, MixAnalysis::TrackAnalysis& trackAnalysis);
    juce::String detectTrackType(const MixAnalysis::TrackAnalysis& trackAnalysis);
    juce::String generateTrackSuggestion(const MixAnalysis::TrackAnalysis& trackAnalysis);
    
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
    float calculateMomentaryLUFS(const juce::AudioBuffer<float>& buffer);
    float calculateShortTermLUFS(const juce::AudioBuffer<float>& buffer);
    float calculateLRA(const juce::AudioBuffer<float>& buffer);
    float calculateDynamicRange(const juce::AudioBuffer<float>& buffer);
    float calculateTruePeak(const juce::AudioBuffer<float>& buffer);
    float calculateRMS(const juce::AudioBuffer<float>& buffer);
    float calculatePeak(const juce::AudioBuffer<float>& buffer);
    void performFFT(const juce::AudioBuffer<float>& buffer);
    float calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    float calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer);
    float calculateFrequencyCenter(const juce::AudioBuffer<float>& buffer);
    float calculateSpectralSpread();
    float calculateBandLevel(const std::vector<float>& spectrum, int startBin, int endBin);
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
class MixSuggestionComponent : public zenith::SkiaComponent {
public:
    MixSuggestionComponent(const MixSuggestion& suggestion);
    ~MixSuggestionComponent() override;
    
    // Component overrides
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    // Access methods
    const MixSuggestion& getSuggestion() const { return suggestion_; }
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
    MixSuggestion suggestion_;
    
    // UI components
    std::unique_ptr<zenith::ZenithButton> applyButton_;
    std::unique_ptr<zenith::ZenithButton> rejectButton_;
    
    // Listeners
    std::vector<Listener*> listeners_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixSuggestionComponent)
};

// Mixing assistant UI
class MixingAssistantUI : public zenith::SkiaComponent,
                         public MixingAssistant::Listener,
                         public zenith::SkiaListBox::Model {
public:
    MixingAssistantUI();
    ~MixingAssistantUI() override;
    
    // Component overrides
    void drawSkia(SkCanvas* canvas) override;
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

    // SkiaListBox::Model
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas& canvas, int width,
                          int height, bool rowIsSelected) override;
    void listBoxItemClicked(int rowNumber, const juce::MouseEvent& e) override;
    juce::String getTooltipForRow(int rowNumber) override;
    
private:
    MixingAssistant* assistant = nullptr;
    
    // UI components
    std::unique_ptr<zenith::SkiaListBox> suggestionsList_;
    std::unique_ptr<zenith::ZenithToggle> autoMixToggle_;
    std::unique_ptr<zenith::ZenithSlider> autoMixStrengthSlider_;
    std::unique_ptr<zenith::ZenithButton> applySelectedButton_;
    std::unique_ptr<zenith::ZenithButton> rejectSelectedButton_;
    std::unique_ptr<zenith::ZenithButton> applyAllButton_;
    std::unique_ptr<zenith::ZenithButton> clearAllButton_;
    std::unique_ptr<zenith::ZenithButton> settingsButton_;
    std::unique_ptr<zenith::ZenithToggle> realTimeToggle_;
    std::unique_ptr<zenith::ZenithToggle> aiToggle_;
    
    // Cached data
    MixAnalysis analysisSnapshot_;
    std::vector<MixSuggestion> suggestionsCache_;
    int selectedSuggestionIndex_ = -1;
    juce::String detectedGenre_;
    
    // Layout
    juce::Rectangle<int> analysisBounds_;
    juce::Rectangle<int> suggestionsBounds_;
    juce::Rectangle<int> settingsBounds_;
    juce::Rectangle<int> suggestionsHeaderBounds_;
    juce::Rectangle<int> suggestionsListBounds_;
    juce::Rectangle<int> suggestionsFooterBounds_;
    juce::Rectangle<int> settingsHeaderBounds_;
    
    // UI creation
    void createControls();
    void wireControlCallbacks();
    
    // Updates
    void updateAnalysisSnapshot(const MixAnalysis& analysis);
    void refreshSuggestions();
    void updateAutoMixControls();
    void updateConfig(const std::function<void(MixingAssistantConfig&)>& mutator);
    void updateVisibility();
    
    // Actions
    void applySelectedSuggestion();
    void rejectSelectedSuggestion();
    void applyAllSuggestions();
    void clearAllSuggestions();
    
    // Drawing
    void drawPanelBackground(SkCanvas* canvas, const juce::Rectangle<int>& bounds);
    void drawAnalysisPanel(SkCanvas* canvas);
    void drawSuggestionsHeader(SkCanvas* canvas);
    void drawSettingsHeader(SkCanvas* canvas);
    
    bool showAnalysis_ = true;
    bool showSuggestions_ = true;
    bool showAutoMix_ = true;
    
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
