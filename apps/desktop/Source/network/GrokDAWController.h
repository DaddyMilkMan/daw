/*
  ==============================================================================

    GrokDAWController.h
    Created: 2025-11-29


    High-level controller that connects Grok AI to DAW functions
    Handles natural language commands, function calling, and preset generation

  ==============================================================================
*/

#pragma once

#include "GrokDAWClient.h"
#include "AudioAnalysisService.h"
#include "../commands/CommandAPI.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

namespace zenith {

/**
    High-level controller for Grok AI integration
    
    This class sits between the user interface (WingmanPanel) and the
    low-level Grok API client. It:
    - Converts natural language to DAW commands
    - Provides DAW context to Grok
    - Handles function calling workflow
    - Manage preset generation
*/

class GrokDAWController
{
public:
    //==========================================================================
    GrokDAWController(CommandAPI& commandAPI);
    ~GrokDAWController();
    
    //==========================================================================
    /**
        Initialize with API key
        
        @param apiKey Grok API key (if empty, retrieves from SecureKeyStore)
        @return true if initialized successfully
    */
    bool initialize(const juce::String& apiKey = juce::String());
    
    /**
        Check if controller is ready
    */
    bool isReady() const;
    
    //==========================================================================
    /**
        Execute a natural language command
        
        @param userCommand Natural language command from user
        @param mode Fast or Thinking mode
        @param onResponse Callback when command completes
        @param onError Callback on error
        @param onProgress Optional progress callback for multi-step operations
    */
    void executeCommand(
        const juce::String& userCommand,
        GrokMode mode,
        std::function<void(juce::String response)> onResponse,
        std::function<void(juce::String error)> onError,
        std::function<void(juce::String status)> onProgress = nullptr
    );
    
    /**
        Generate a synth preset from description
        
        @param instrumentId Instrument type (e.g., "zenith_poly_synth")
        @param description Natural language description
        @param genre Optional genre for context
        @param onComplete Callback with generated preset
        @param onError Callback on error
    */
    void generatePreset(
        const juce::String& instrumentId,
        const juce::String& description,
        const juce::String& genre,
        std::function<void(juce::String presetName, juce::var parameters)> onComplete,
        std::function<void(juce::String error)> onError
    );
    
    /**
        Cancel ongoing operation
    */
    void cancel();
    
    /**
        Clear conversation history
    */
    void clearHistory();
    
    /**
        Get conversation history
    */
    juce::Array<juce::var> getHistory() const;
    
    //==========================================================================
    /**
        Set DAW context provider
        
        This function is called to get current DAW state for Grok
    */
    void setContextProvider(std::function<juce::var()> provider);
    
private:
    //==========================================================================
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokDAWController)
};

} // namespace zenith
