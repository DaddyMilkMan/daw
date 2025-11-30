#include "../../include/ui/WingmanPanel.h"

namespace zenith {

WingmanPanel::WingmanPanel(WingmanAPI& api, OpenAIClient& client)
    : api_(api)
    , client_(client)
{
    // Title
    addAndMakeVisible(titleLabel_);
    titleLabel_.setText("Wingman AI", juce::dontSendNotification);
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));

    // Chat history
    addAndMakeVisible(chatHistory_);
    chatHistory_.setMultiLine(true);
    chatHistory_.setReadOnly(true);
    chatHistory_.setScrollbarsShown(true);
    chatHistory_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1e1e1e));
    chatHistory_.setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);

    // Chat input
    addAndMakeVisible(chatInput_);
    chatInput_.setMultiLine(false);
    chatInput_.setReturnKeyStartsNewLine(false);
    chatInput_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    chatInput_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    chatInput_.setTextToShowWhenEmpty("Ask Wingman anything...", juce::Colours::grey);
    chatInput_.setInputRestrictions(10000); // Max 10,000 characters to prevent DoS

    chatInput_.onReturnKey = [this]() { onSendMessage(); };

    // Send button
    addAndMakeVisible(sendButton_);
    sendButton_.setButtonText("Send");
    sendButton_.onClick = [this]() { onSendMessage(); };

    // Think Harder toggle
    addAndMakeVisible(thinkHarderToggle_);
    thinkHarderToggle_.setButtonText("Think Harder (GPT-5)");
    thinkHarderToggle_.setTooltip("Enable GPT-5 for complex reasoning (slower but more capable). Disabled uses GPT-5 Mini for fast responses.");
    thinkHarderToggle_.setToggleState(false, juce::dontSendNotification);

    // Status label
    addAndMakeVisible(statusLabel_);
    statusLabel_.setJustificationType(juce::Justification::centredLeft);
    statusLabel_.setFont(juce::FontOptions(11.0f));
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
    statusLabel_.setText("Ready - Using GPT-5 Mini", juce::dontSendNotification);

    thinkHarderToggle_.onClick = [this]()
    {
        if (thinkHarderToggle_.getToggleState())
            statusLabel_.setText("Ready - Using GPT-5 (Think Harder mode)", juce::dontSendNotification);
        else
            statusLabel_.setText("Ready - Using GPT-5 Mini", juce::dontSendNotification);
    };

    // Initial message
    appendToChatHistory("Welcome to Wingman AI! How can I help with your music production today?", false);
    appendToChatHistory("Tip: Enable 'Think Harder' for complex questions, or use GPT-5 Mini for quick answers.", false);
}

void WingmanPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void WingmanPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel_.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(5);

    // Status label
    statusLabel_.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);

    // Think Harder toggle
    thinkHarderToggle_.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);

    // Input area at bottom
    auto inputArea = bounds.removeFromBottom(40);
    sendButton_.setBounds(inputArea.removeFromRight(80));
    inputArea.removeFromRight(5);
    chatInput_.setBounds(inputArea);

    bounds.removeFromBottom(10);

    // Chat history takes remaining space
    chatHistory_.setBounds(bounds);
}

void WingmanPanel::onSendMessage()
{
    juce::String message = chatInput_.getText().trim();

    // Input validation
    if (message.isEmpty())
        return;

    if (message.length() > 10000)
    {
        appendToChatHistory("Error: Message too long (max 10,000 characters)", false);
        return;
    }

    // Check if already waiting for response
    if (isWaitingForResponse_)
    {
        appendToChatHistory("Please wait for the current response to complete...", false);
        return;
    }

    // Display user message
    appendToChatHistory("You: " + message, true);

    // Clear input
    chatInput_.clear();

    // Update status
    isWaitingForResponse_ = true;
    statusLabel_.setText("Thinking...", juce::dontSendNotification);
    sendButton_.setEnabled(false);

    // Add message to conversation history
    ChatMessage userMsg;
    userMsg.role = "user";
    userMsg.content = message;
    conversationHistory_.push_back(userMsg);

    // Determine model based on Think Harder toggle
    OpenAIModel model = thinkHarderToggle_.getToggleState() ? OpenAIModel::Grok41Reasoning : OpenAIModel::Grok41Fast;

    // Send request to OpenAI
    client_.sendChatRequest(
        conversationHistory_,
        model,
        [this](const juce::String& response)
        {
            onResponseReceived(response);
        },
        [this](const juce::String& error)
        {
            onErrorReceived(error);
        }
    );
}

void WingmanPanel::onResponseReceived(const juce::String& response)
{
    // Display assistant response
    appendToChatHistory("Wingman: " + response, false);

    // Add to conversation history
    ChatMessage assistantMsg;
    assistantMsg.role = "assistant";
    assistantMsg.content = response;
    conversationHistory_.push_back(assistantMsg);

    // Update status
    isWaitingForResponse_ = false;
    if (thinkHarderToggle_.getToggleState())
        statusLabel_.setText("Ready - Using GPT-5 (Think Harder mode)", juce::dontSendNotification);
    else
        statusLabel_.setText("Ready - Using GPT-5 Mini", juce::dontSendNotification);
    sendButton_.setEnabled(true);
}

void WingmanPanel::onErrorReceived(const juce::String& error)
{
    // Display error
    appendToChatHistory("Error: " + error, false);

    // Update status
    isWaitingForResponse_ = false;
    statusLabel_.setText("Error - Ready to retry", juce::dontSendNotification);
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
    sendButton_.setEnabled(true);

    // Reset status color after 3 seconds
    juce::Timer::callAfterDelay(3000, [this]()
    {
        statusLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        if (thinkHarderToggle_.getToggleState())
            statusLabel_.setText("Ready - Using GPT-5 (Think Harder mode)", juce::dontSendNotification);
        else
            statusLabel_.setText("Ready - Using GPT-5 Mini", juce::dontSendNotification);
    });
}

void WingmanPanel::setApiKey(const juce::String& apiKey)
{
    client_.setApiKey(apiKey);

    if (!apiKey.isEmpty())
    {
        appendToChatHistory("OpenAI API key configured successfully. You can now chat with Wingman!", false);
    }
}

void WingmanPanel::appendToChatHistory(const juce::String& message, bool isUser)
{
    juce::String currentText = chatHistory_.getText();

    if (currentText.isNotEmpty())
        currentText += "\n\n";

    currentText += message;

    chatHistory_.setText(currentText);

    // Scroll to bottom
    chatHistory_.moveCaretToEnd();
}

} // namespace zenith
