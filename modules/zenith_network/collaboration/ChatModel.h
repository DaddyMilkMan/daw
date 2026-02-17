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
    class ChatModel : public juce::ListBoxModel {
    public:
        ChatModel(const std::vector<ChatMessage>& messages);
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;

        void setMessages(const std::vector<ChatMessage>& messages);

    private:
        std::vector<ChatMessage> messages;
    };

    std::unique_ptr<UserListModel> userListModel;
    std::unique_ptr<ChatModel> chatModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollaborativeEditor)
};

// Operational transform implementation

} // namespace
