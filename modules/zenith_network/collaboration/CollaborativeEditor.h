/*
  ==============================================================================
    CollaborativeSession.h
    Real-time collaborative editing system
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace zenith {
namespace collaboration {

// User information
class CollaborativeEditor : public juce::Component,
                          public CollaborativeSession::Listener {
public:
    CollaborativeEditor();
    ~CollaborativeEditor() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Session management
    void setSession(CollaborativeSession* session);
    CollaborativeSession* getSession() const { return session; }

    // User interface
    void showUserList(bool show);
    void showChat(bool show);
    void showOperationHistory(bool show);

    // Collaboration features
    void highlightUserActions(const juce::String& userId, bool highlight);
    void showUserCursors(bool show);
    void enableRealTimeUpdates(bool enabled);

    // CollaborativeSession::Listener overrides
    void userConnected(const UserInfo& user) override;
    void userDisconnected(const UserInfo& user) override;
    void operationReceived(const Operation& operation) override;
    void operationApplied(const Operation& operation) override;
    void chatMessageReceived(const ChatMessage& message) override;

private:
    CollaborativeSession* session = nullptr;

    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;

    // User list
    std::unique_ptr<juce::ListBox> userListBox;
    std::unique_ptr<juce::TextButton> inviteButton;

    // Chat
    std::unique_ptr<juce::Viewport> chatViewport;
    std::unique_ptr<juce::Component> chatComponent;
    std::unique_ptr<juce::ListBox> chatListBox;
    std::unique_ptr<juce::TextEditor> messageEditor;
    std::unique_ptr<juce::TextButton> sendButton;

    // Operation history
    std::unique_ptr<juce::ListBox> operationListBox;
    std::unique_ptr<juce::TextButton> clearHistoryButton;

    // Settings
    std::unique_ptr<juce::ToggleButton> realTimeToggle;
    std::unique_ptr<juce::ToggleButton> highlightToggle;
    std::unique_ptr<juce::ToggleButton> cursorsToggle;

    // UI creation
    void createUserInterface();
    void createChatInterface();
    void createOperationHistory();
    void createSettingsInterface();

    // Updates
    void updateUserList();
    void updateChatDisplay();
    void updateOperationHistory();

    // User list model

} // namespace
