/*
  ==============================================================================
    RealTimeCollaboration.h
    Real-time collaboration system for Phase 3: Autonomous Intelligence (10/10)
    Live suggestions, instant feedback, and collaborative mastering
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PredictiveMastering.h"
#include "CreativePartner.h"
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <queue>

namespace zenith {
namespace ai {

struct CollaborationEvent {
    enum class Type {
        Suggestion,
        Feedback,
        Question,
        Analysis,
        Warning,
        Success
    };
    
    Type type;
    juce::String message;
    juce::String details;
    juce::var data;
    juce::Time timestamp;
    float urgency;  // 0.0 to 1.0
    bool requiresAction;
};

struct LiveSuggestion {
    juce::String id;
    CollaborationEvent event;
    std::function<void()> applyCallback;
    std::function<void()> dismissCallback;
    float confidence;
    juce::String reasoning;
    bool isActive;
};

struct RealTimeAnalysis {
    float currentLoudness = 0.0f;
    float currentDynamics = 0.0f;
    float currentStereoWidth = 0.0f;
    float currentFrequencyBalance = 0.0f;
    juce::String currentPhase;
    std::vector<juce::String> activeIssues;
    std::vector<juce::String> opportunities;
};

class RealTimeCollaboration {
public:
    RealTimeCollaboration();
    ~RealTimeCollaboration();

    // Real-time monitoring
    void startRealTimeMonitoring(double sampleRate = 44100.0);
    void stopRealTimeMonitoring();
    void processAudioBuffer(const juce::AudioBuffer<float>& buffer);
    
    // Live suggestions
    void generateLiveSuggestions(const juce::AudioBuffer<float>& currentAudio,
                                const juce::var& currentSettings);
    
    // Event handling
    void subscribeToEvents(std::function<void(const CollaborationEvent&)> callback);
    void broadcastEvent(const CollaborationEvent& event);
    
    // Interactive collaboration
    juce::String askQuestion(const juce::String& question);
    void provideFeedback(const juce::String& suggestionId, bool wasHelpful, const juce::String& comment = "");
    
    // Real-time analysis
    RealTimeAnalysis getCurrentAnalysis() const;
    bool hasIssues() const;
    std::vector<juce::String> getActiveSuggestions() const;
    
    // Configuration
    void setSensitivity(float sensitivity = 0.5f);  // How sensitive to changes
    void setResponseTime(float milliseconds = 100.0f);  // How quickly to respond
    void setCollaborationMode(const juce::String& mode);  // "passive", "active", "collaborative"

private:
    // Real-time processing
    std::unique_ptr<std::thread> monitoringThread;
    std::atomic<bool> shouldMonitor{false};
    std::queue<juce::AudioBuffer<float>> audioQueue;
    std::mutex audioQueueMutex;
    
    // Analysis components
    std::unique_ptr<PredictiveMastering> predictiveEngine;
    std::unique_ptr<CreativePartner> creativePartner;
    
    // State tracking
    RealTimeAnalysis currentAnalysis;
    std::vector<LiveSuggestion> activeSuggestions;
    juce::var lastSettings;
    
    // Event system
    std::vector<std::function<void(const CollaborationEvent&)>> eventCallbacks;
    std::mutex callbacksMutex;
    
    // Configuration
    float sensitivity = 0.5f;
    float responseTime = 100.0f;
    juce::String collaborationMode = "active";
    
    // Analysis methods
    void analyzeAudioChunk(const juce::AudioBuffer<float>& buffer);
    void detectChanges(const RealTimeAnalysis& newAnalysis);
    void generateSuggestions(const RealTimeAnalysis& analysis);
    
    // Suggestion management
    void addSuggestion(const LiveSuggestion& suggestion);
    void removeSuggestion(const juce::String& id);
    void updateSuggestions();
    
    // Real-time metrics
    float calculateLoudness(const juce::AudioBuffer<float>& buffer);
    float calculateDynamics(const juce::AudioBuffer<float>& buffer);
    float calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    float calculateFrequencyBalance(const juce::AudioBuffer<float>& buffer);
    
    // Monitoring loop
    void monitoringWorker();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeCollaboration)
};

// Collaborative mastering session
class CollaborativeSession {
public:
    struct Participant {
        juce::String id;
        juce::String name;
        juce::String role;  // "engineer", "producer", "artist", "ai"
        bool isActive;
        juce::var preferences;
    };
    
    struct SessionEvent {
        juce::String participantId;
        juce::String action;
        juce::var parameters;
        juce::Time timestamp;
        juce::String comment;
    };
    
    CollaborativeSession();
    ~CollaborativeSession();
    
    // Session management
    void startSession(const juce::String& sessionId);
    void endSession();
    bool isActive() const { return sessionActive; }
    
    // Participant management
    void addParticipant(const Participant& participant);
    void removeParticipant(const juce::String& participantId);
    std::vector<Participant> getParticipants() const;
    
    // Collaboration
    void broadcastAction(const juce::String& participantId, const juce::String& action, const juce::var& parameters);
    void proposeChange(const juce::String& participantId, const juce::String& change, const juce::var& settings);
    void voteOnChange(const juce::String& changeId, const juce::String& participantId, bool approve);
    
    // AI collaboration
    void enableAICollaboration();
    void disableAICollaboration();
    juce::String getAIRecommendation() const;
    
    // Session history
    std::vector<SessionEvent> getSessionHistory() const;
    void exportSession(const juce::File& filePath) const;
    
private:
    juce::String sessionId;
    bool sessionActive = false;
    std::vector<Participant> participants;
    std::vector<SessionEvent> eventHistory;
    
    // AI participant
    Participant aiParticipant;
    bool aiEnabled = false;
    
    // Voting system
    std::unordered_map<juce::String, std::vector<std::pair<juce::String, bool>>> pendingVotes;
    
    // Session management
    void recordEvent(const SessionEvent& event);
    void notifyParticipants(const SessionEvent& event);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollaborativeSession)
};

// Instant feedback system
class InstantFeedback {
public:
    struct FeedbackRequest {
        juce::String id;
        juce::String question;
        juce::var context;
        std::function<void(const juce::String&)> onResponse;
        juce::Time timeout;
    };
    
    InstantFeedback();
    ~InstantFeedback();
    
    // Feedback requests
    void requestFeedback(const FeedbackRequest& request);
    void cancelRequest(const juce::String& id);
    
    // Quick responses
    void provideQuickResponse(const juce::String& requestId, const juce::String& response);
    void provideDetailedResponse(const juce::String& requestId, const juce::String& response, const juce::var& details);
    
    // Auto-responses
    void enableAutoResponses();
    void disableAutoResponses();
    
    // Feedback analysis
    std::vector<juce::String> analyzeFeedbackPatterns() const;
    float getAverageResponseTime() const;
    
private:
    std::queue<FeedbackRequest> pendingRequests;
    std::mutex requestsMutex;
    
    bool autoResponsesEnabled = true;
    std::unordered_map<juce::String, juce::Time> requestStartTimes;
    
    // Auto-response generation
    juce::String generateAutoResponse(const FeedbackRequest& request);
    bool shouldAutoRespond(const FeedbackRequest& request) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstantFeedback)
};

} // namespace ai
} // namespace zenith
