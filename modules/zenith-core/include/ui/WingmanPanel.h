#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ai/OpenAIClient.h"
#include <vector>

namespace zenith {

// Forward declarations
class WingmanAPI;

/**
 * @brief AI assistant panel for Zenith DAW
 *
 * Provides natural language interaction with the DAW through Wingman AI.
 */
class WingmanPanel : public juce::Component
{
public:
    WingmanPanel(WingmanAPI& api, OpenAIClient& client);
    ~WingmanPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Set OpenAI API key
     */
    void setApiKey(const juce::String& apiKey);

private:
    WingmanAPI& api_;
    OpenAIClient& client_;

    // UI Components
    juce::TextEditor chatInput_;
    juce::TextEditor chatHistory_;
    juce::TextButton sendButton_;
    juce::Label titleLabel_;
    juce::ToggleButton thinkHarderToggle_;
    juce::Label statusLabel_;

    // Conversation history
    std::vector<ChatMessage> conversationHistory_;
    bool isWaitingForResponse_ = false;

    void onSendMessage();
    void appendToChatHistory(const juce::String& message, bool isUser);
    void onResponseReceived(const juce::String& response);
    void onErrorReceived(const juce::String& error);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
