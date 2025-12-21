/*
  ==============================================================================

    AIBridgeClient.cpp
    Created: 2025-12-19

  ==============================================================================
*/

#include "AIBridgeClient.h"
#include "SecureKeyStore.h"
#include "AITools.h"
#include "AIPrompts.h"

namespace zenith {

AIBridgeClient::AIBridgeClient(CommandAPI& api) : commandAPI_(api) {
    commandFunctions_ = AITools::getAvailableFunctions();
}

AIBridgeClient::~AIBridgeClient() {
    if (grokClient_)
        grokClient_->cancelRequest();
}

void AIBridgeClient::processNaturalLanguage(const juce::String& naturalLanguage, GrokMode mode) {
    if (isProcessing_)
        return;

    if (!grokClient_) {
        grokClient_ = std::make_unique<GrokAPIClient>();
        juce::String key = getAPIKeyFromSettings();
        if (key.isNotEmpty()) {
             grokClient_->setAPIKey(key);
        }
    }

    if (!grokClient_->hasAPIKey()) {
        handleGrokError("API Key not configured. Please set it in Settings.");
        return;
    }

    isProcessing_ = true;
    status_ = "Connecting...";
    listeners_.call(&Listener::statusChanged, status_);

    grokClient_->sendChat(
        naturalLanguage,
        mode,
        commandFunctions_,
        "You are Wingman, a DAW assistant. Help the user create music by executing commands.",
        [this](juce::String response) { handleGrokResponse(response); },
        [this](GrokFunctionCall call) { handleFunctionCall(call); },
        [this](juce::String error) { handleGrokError(error); }
    );
}

void AIBridgeClient::handleGrokResponse(const juce::String& response) {
    isProcessing_ = false;
    status_ = "Ready";
    listeners_.call(&Listener::responseReceived, response);
    listeners_.call(&Listener::statusChanged, status_);
}

void AIBridgeClient::handleFunctionCall(const GrokFunctionCall& call) {
    status_ = "Executing command: " + call.functionName;
    listeners_.call(&Listener::statusChanged, status_);

    // Execute via CommandAPI
    juce::DynamicObject* requestObj = new juce::DynamicObject();
    requestObj->setProperty("id", call.functionName);
    requestObj->setProperty("parameters", call.arguments);
    
    juce::var result = commandAPI_.executeCommand(juce::var(requestObj));
    
    // Submit result back to Grok
    grokClient_->submitFunctionResult(
        call,
        result,
        [this](juce::String response) { handleGrokResponse(response); },
        [this](juce::String error) { handleGrokError(error); }
    );
}

void AIBridgeClient::handleGrokError(const juce::String& error) {
    isProcessing_ = false;
    status_ = "Error";
    listeners_.call(&Listener::errorReceived, error);
    listeners_.call(&Listener::statusChanged, status_);
}

juce::String AIBridgeClient::getAPIKeyFromSettings() {
    juce::String key;
    if (SecureKeyStore::retrieveKey(SecureKeyStore::GrokAPIKey, key)) {
        return key;
    }
    return "";
}

} // namespace zenith
