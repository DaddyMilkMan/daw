/*
  ==============================================================================

    AiBridge.h
    Created: 2025-11-30
    Authors: Sarah Chen, Dr. Aris Vokos

    The Neural Bridge - COMPLETE IMPLEMENTATION
    All 10 features integrated.

  ==============================================================================
*/

#pragma once

#include "AiDataStructures.h"
#include "AiChordAnalyzer.h"
#include "AiAudioAnalyzer.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {
namespace ai {

class AiBridge : public juce::Thread {
public:
    AiBridge();
    ~AiBridge() override;

    // ========================================================================
    // FEATURE 1-4: Standard Generation with Full Context
    // ========================================================================

    void generateMidi(const AiProjectContext& context, 
                      std::function<void(AiGenerationResult)> callback);

    // ========================================================================
    // FEATURE 5: Multi-Track Generation
    // ========================================================================
    
    void generateMultiTrack(const AiProjectContext& context,
                           const std::vector<juce::String>& trackNames,
                           std::function<void(AiGenerationResult)> callback);

    // ========================================================================
    // FEATURE 7: Real-Time Streaming
    // ========================================================================
    
    void generateMidiStreaming(const AiProjectContext& context,
                              std::function<void(AiMidiNote)> onNoteGenerated,
                              std::function<void(bool, juce::String)> onComplete);

    // ========================================================================
    // FEATURE 10: Iterative Refinement
    // ========================================================================
    
    void refineGeneration(const AiGenerationResult& previousResult,
                         const juce::String& feedback,
                         std::function<void(AiGenerationResult)> callback);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void setApiKey(const juce::String& key);
    void setModelEndpoint(const juce::String& url);
    
    // Model selection
    enum class Model {
        Grok,
        GPT4,
        Claude,
        Gemini
    };
    void setModel(Model model);

private:
    void run() override;

    // Prompt construction
    juce::String constructSystemPrompt(bool isMultiTrack = false, bool isStreaming = false);
    juce::String constructUserPrompt(const AiProjectContext& context);
    juce::String constructRefinementPrompt(const AiGenerationResult& previous, const juce::String& feedback);
    
    // Network layer
    juce::var sendRequest(const juce::String& systemPrompt, const juce::String& userPrompt);
    juce::var sendStreamingRequest(const juce::String& systemPrompt, const juce::String& userPrompt,
                                   std::function<void(const juce::String&)> onChunk);
    
    // Response parsing
    AiGenerationResult parseResponse(const juce::var& jsonResponse);
    AiGenerationResult parseMultiTrackResponse(const juce::var& jsonResponse);
    AiMidiNote parseStreamingChunk(const juce::String& chunk);
    
    // Simulation mode (for testing without API key)
    AiGenerationResult simulateGeneration(const AiProjectContext& context);
    AiGenerationResult simulateMultiTrack(const AiProjectContext& context, const std::vector<juce::String>& tracks);

    // State
    enum class OperationMode {
        SingleTrack,
        MultiTrack,
        Streaming,
        Refinement
    };
    
    OperationMode currentMode_;
    AiProjectContext currentContext_;
    std::vector<juce::String> currentTrackNames_;
    AiGenerationResult previousResult_;
    juce::String currentFeedback_;
    
    std::function<void(AiGenerationResult)> currentCallback_;
    std::function<void(AiMidiNote)> streamingNoteCallback_;
    std::function<void(bool, juce::String)> streamingCompleteCallback_;
    
    juce::String apiKey_;
    juce::String endpointUrl_;
    Model currentModel_ = Model::Grok;
    
    bool isBusy_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiBridge)
};

} // namespace ai
} // namespace zenith
