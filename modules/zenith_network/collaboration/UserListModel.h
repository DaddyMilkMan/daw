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
    class UserListModel : public juce::ListBoxModel {
    public:
        UserListModel(const std::vector<UserInfo>& users);
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
        juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

        void setUsers(const std::vector<UserInfo>& users);

    private:
        std::vector<UserInfo> users;
    };

    // Chat model

} // namespace
