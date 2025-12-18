/*
  ==============================================================================
    Example: Integrating Grok API with AI Mastering Agent
  ==============================================================================
*/

#include "AIMasteringAgent.h"
#include <curl/curl.h>  // Or whatever HTTP library you're using

namespace zenith {
namespace ai {

//==============================================================================
/**
    Example Grok API Client
    
    Replace this with your actual Grok API implementation.
    This shows the interface the AIMasteringAgent expects.
*/
class GrokAPIClient {
public:
    GrokAPIClient(const juce::String& apiKey) 
        : apiKey_(apiKey)
    {
    }
    
    /**
     * Make a synchronous API call to Grok
     * 
     * @param prompt The user prompt
     * @param systemMessage The system message for context
     * @return Grok's response as a string
     */
    juce::String callGrok(const juce::String& prompt, const juce::String& systemMessage) {
        // Build JSON request
        juce::DynamicObject::Ptr request = new juce::DynamicObject();
        request->setProperty("model", "grok-4.1");
        request->setProperty("temperature", 0.7);
        request->setProperty("max_tokens", 1000);
        
        // Messages array
        juce::Array<juce::var> messages;
        
        juce::DynamicObject::Ptr sysMsg = new juce::DynamicObject();
        sysMsg->setProperty("role", "system");
        sysMsg->setProperty("content", systemMessage);
        messages.add(sysMsg.get());
        
        juce::DynamicObject::Ptr userMsg = new juce::DynamicObject();
        userMsg->setProperty("role", "user");
        userMsg->setProperty("content", prompt);
        messages.add(userMsg.get());
        
        request->setProperty("messages", messages);
        
        juce::String requestJson = juce::JSON::toString(request.get());
        
        // Make HTTP POST request
        juce::String response = makeHttpPost(
            "https://api.x.ai/v1/chat/completions",  // Grok API endpoint
            requestJson,
            {
                {"Authorization", "Bearer " + apiKey_},
                {"Content-Type", "application/json"}
            }
        );
        
        // Parse response
        return extractContentFromResponse(response);
    }
    
private:
    juce::String apiKey_;
    
    juce::String makeHttpPost(const juce::String& url, 
                              const juce::String& data,
                              const std::map<juce::String, juce::String>& headers) {
        // TODO: Implement actual HTTP request
        // Use JUCE's URL class or curl or whatever you prefer
        
        // Example pseudocode:
        /*
        juce::URL apiUrl(url);
        juce::StringPairArray headerArray;
        for (const auto& [key, value] : headers) {
            headerArray.set(key, value);
        }
        
        auto stream = apiUrl.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                .withExtraHeaders(headerArray)
                .withConnectionTimeoutMs(30000)
                .withPostData(data)
        );
        
        if (stream != nullptr) {
            return stream->readEntireStreamAsString();
        }
        */
        
        return "{}";  // Placeholder
    }
    
    juce::String extractContentFromResponse(const juce::String& responseJson) {
        auto json = juce::JSON::parse(responseJson);
        if (json.isVoid()) return "";
        
        auto* obj = json.getDynamicObject();
        if (!obj) return "";
        
        // Grok response structure: response.choices[0].message.content
        auto choices = obj->getProperty("choices");
        if (!choices.isArray()) return "";
        
        auto choicesArray = *choices.getArray();
        if (choicesArray.isEmpty()) return "";
        
        auto* firstChoice = choicesArray[0].getDynamicObject();
        if (!firstChoice) return "";
        
        auto* message = firstChoice->getProperty("message").getDynamicObject();
        if (!message) return "";
        
        return message->getProperty("content").toString();
    }
};

//==============================================================================
/**
    Example usage of AIMasteringAgent with Grok
*/
class MasteringController {
public:
    MasteringController(Engine& engine, const juce::String& grokApiKey)
        : masteringAgent_(engine),
          grokClient_(grokApiKey)
    {
        // Set up the Grok callback
        masteringAgent_.setGrokCallback(
            [this](const juce::String& prompt, const juce::String& system) {
                return grokClient_.callGrok(prompt, system);
            }
        );
        
        DBG("Mastering Controller initialized with Grok AI");
    }
    
    void masterProject(juce::AudioBuffer<float>& mixBuffer) {
        // Configure options
        AIMasteringAgent::Options options;
        options.useAI = true;
        options.targetLoudness = -14.0f;  // Streaming standard
        options.userIntent = "professional, balanced master for streaming";
        
        // Analyze and get AI decisions (background thread!)
        juce::Thread::launch([this, &mixBuffer, options]() {
            masteringAgent_.analyzeAndConfigure(mixBuffer, options);
            
            // Show AI decision in UI
            auto decision = masteringAgent_.getLastDecision();
            if (decision.valid) {
                DBG("AI Mastering Decision:");
                DBG("  " + decision.reasoning);
            }
        });
    }
    
    void processAudioBlock(juce::AudioBuffer<float>& buffer) {
        // Real-time processing (audio thread safe)
        masteringAgent_.processBlock(buffer);
    }
    
private:
    AIMasteringAgent masteringAgent_;
    GrokAPIClient grokClient_;
};

//==============================================================================
/**
    Example: How to use in your audio callback
*/
void exampleAudioCallback(juce::AudioBuffer<float>& buffer, 
                          MasteringController& mastering) {
    // Just call processBlock - it's thread-safe
    mastering.processAudioBlock(buffer);
}

//==============================================================================
/**
    Example: How to trigger AI mastering analysis
*/
void exampleMasterButton(Engine& engine, 
                         MasteringController& mastering,
                         juce::AudioBuffer<float>& mixdownBuffer) {
    // User clicked "AI Master" button
    
    // Launch on background thread (CRITICAL - don't block audio thread!)
    juce::Thread::launch([&mastering, &mixdownBuffer]() {
        mastering.masterProject(mixdownBuffer);
    });
    
    // UI can show "Analyzing..." spinner while AI thinks
}

} // namespace ai
} // namespace zenith
