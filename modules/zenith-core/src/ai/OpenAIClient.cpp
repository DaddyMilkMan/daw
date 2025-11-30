/**
 * @file OpenAIClient.cpp
 * @brief Implementation of OpenAI API client
 */

#include "../../include/ai/OpenAIClient.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
// OpenAIClient Implementation
//==============================================================================

OpenAIClient::OpenAIClient()
    : systemPrompt_("You are Wingman, an AI assistant integrated into Zenith DAW. "
                   "You help musicians with music production, mixing, sound design, "
                   "composition, arrangement, and technical questions about the DAW. "
                   "Be concise, practical, and helpful.")
{
}

void OpenAIClient::setApiKey(const juce::String& apiKey)
{
    apiKey_ = apiKey;
}

juce::String OpenAIClient::getApiKeyRedacted() const
{
    if (apiKey_.length() < 8)
        return "***";

    return apiKey_.substring(0, 8) + "...";
}

juce::String OpenAIClient::getModelName(OpenAIModel model)
{
    switch (model)
    {
        case OpenAIModel::Grok41Fast:
            return "grok-4-1-fast-non-reasoning";
        case OpenAIModel::Grok41Reasoning:
            return "grok-4-1-fast-reasoning";
        default:
            return "grok-4-1-fast-non-reasoning";
    }
}

void OpenAIClient::setSystemPrompt(const juce::String& prompt)
{
    systemPrompt_ = prompt;
}

void OpenAIClient::sendChatRequest(
    const std::vector<ChatMessage>& messages,
    OpenAIModel model,
    std::function<void(const juce::String&)> callback,
    std::function<void(const juce::String&)> errorCallback)
{
    // Validate API key
    if (apiKey_.isEmpty())
    {
        if (errorCallback)
        {
            errorCallback("xAI API key not set. Get your free key at console.x.ai");
        }
        return;
    }

    // TODO: Implement full xAI Grok API HTTP integration
    // For now, provide a stub response showing model selection is working
    if (callback)
    {
        juce::String modelDesc = (model == OpenAIModel::Grok41Reasoning)
            ? "Grok 4.1 Reasoning (Think Harder mode - deep analysis)"
            : "Grok 4.1 Fast (Quick responses)";

        juce::String response = "✅ Grok 4.1 integration ready!\n\n"
                                 "**Current Settings:**\n"
                                 "- Model: " + modelDesc + "\n"
                                 "- API: xAI (api.x.ai)\n"
                                 "- Context: 2M tokens\n"
                                 "- Pricing: $0.20/1M input, $0.50/1M output\n"
                                 "- API Key: " + getApiKeyRedacted() + "\n\n"
                                 "**I can help with:**\n"
                                 "- Music production & composition\n"
                                 "- Mixing & mastering techniques\n"
                                 "- Sound design & synthesis\n"
                                 "- DAW workflow optimization\n"
                                 "- Plugin recommendations\n"
                                 "- Audio engineering questions\n\n"
                                 "Full HTTP integration coming soon!";

        callback(response);
    }
}

void OpenAIClient::cancelAllRequests()
{
    juce::ScopedLock lock(requestLock_);

    for (auto& request : pendingRequests_)
    {
        if (request && request->task)
            request->task.reset();
    }

    pendingRequests_.clear();
}

juce::var OpenAIClient::buildRequestBody(const std::vector<ChatMessage>& messages, OpenAIModel model)
{
    auto* obj = new juce::DynamicObject();

    // Model
    obj->setProperty("model", getModelName(model));

    // Messages array
    juce::Array<juce::var> messagesArray;

    // Add system prompt if not already present
    bool hasSystemMessage = false;
    for (const auto& msg : messages)
    {
        if (msg.role == "system")
        {
            hasSystemMessage = true;
            break;
        }
    }

    if (!hasSystemMessage && systemPrompt_.isNotEmpty())
    {
        auto* systemMsg = new juce::DynamicObject();
        systemMsg->setProperty("role", "system");
        systemMsg->setProperty("content", systemPrompt_);
        messagesArray.add(juce::var(systemMsg));
    }

    // Add user messages
    for (const auto& msg : messages)
    {
        auto* msgObj = new juce::DynamicObject();
        msgObj->setProperty("role", msg.role);
        msgObj->setProperty("content", msg.content);
        messagesArray.add(juce::var(msgObj));
    }

    obj->setProperty("messages", messagesArray);

    // Additional parameters
    obj->setProperty("temperature", 0.7);
    obj->setProperty("max_tokens", 1000);

    return juce::var(obj);
}

bool OpenAIClient::parseResponse(const juce::var& json, juce::String& outResponse, juce::String& outError)
{
    if (!json.isObject())
    {
        outError = "Response is not a JSON object";
        return false;
    }

    auto* obj = json.getDynamicObject();
    if (!obj)
    {
        outError = "Failed to get JSON object";
        return false;
    }

    // Get choices array
    if (!obj->hasProperty("choices"))
    {
        outError = "Response missing 'choices' field";
        return false;
    }

    auto choicesVar = obj->getProperty("choices");
    if (!choicesVar.isArray())
    {
        outError = "'choices' is not an array";
        return false;
    }

    auto* choicesArray = choicesVar.getArray();
    if (choicesArray->size() == 0)
    {
        outError = "'choices' array is empty";
        return false;
    }

    // Get first choice
    auto choice = choicesArray->getReference(0);
    if (!choice.isObject())
    {
        outError = "Choice is not an object";
        return false;
    }

    auto* choiceObj = choice.getDynamicObject();
    if (!choiceObj || !choiceObj->hasProperty("message"))
    {
        outError = "Choice missing 'message' field";
        return false;
    }

    // Get message content
    auto messageVar = choiceObj->getProperty("message");
    if (!messageVar.isObject())
    {
        outError = "Message is not an object";
        return false;
    }

    auto* messageObj = messageVar.getDynamicObject();
    if (!messageObj || !messageObj->hasProperty("content"))
    {
        outError = "Message missing 'content' field";
        return false;
    }

    outResponse = messageObj->getProperty("content").toString();
    return true;
}

} // namespace zenith
