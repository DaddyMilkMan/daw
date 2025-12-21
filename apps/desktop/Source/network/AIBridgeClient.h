/*
  ==============================================================================

    AIBridgeClient.h
    Created: 2025-12-19
    Author:  Wingman Agent

    Bridge between WingmanPanel and GrokAPIClient.
    Acts as the main AI integration point for the DAW.

  ==============================================================================
*/

#pragma once

#include "GrokAPIClient.h"
#include "../commands/CommandAPI.h"
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

class AIBridgeClient {
public:
    AIBridgeClient(CommandAPI& api);
    ~AIBridgeClient();

    /**
        Sends a natural language request to Grok.
    */
    void processNaturalLanguage(const juce::String& naturalLanguage, GrokMode mode);

    /**
        Callbacks for Grok responses.
    */
    void handleGrokResponse(const juce::String& response);
    void handleFunctionCall(const GrokFunctionCall& call);
    void handleGrokError(const juce::String& error);

    /**
        Status and state tracking.
    */
    juce::String getStatus() const { return status_; }
    bool isProcessing() const { return isProcessing_; }

    /**
        UI Listeners can register here.
    */
    struct Listener {
        virtual ~Listener() = default;
        virtual void responseReceived(const juce::String& response) = 0;
        virtual void errorReceived(const juce::String& error) = 0;
        virtual void statusChanged(const juce::String& status) = 0;
    };

    void addListener(Listener* l) { listeners_.add(l); }
    void removeListener(Listener* l) { listeners_.remove(l); }

private:
    CommandAPI& commandAPI_;
    std::unique_ptr<GrokAPIClient> grokClient_;
    juce::Array<GrokFunction> commandFunctions_;
    
    juce::ListenerList<Listener> listeners_;
    juce::String status_ = "Ready";
    bool isProcessing_ = false;

    juce::String getAPIKeyFromSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIBridgeClient)
};

} // namespace zenith
