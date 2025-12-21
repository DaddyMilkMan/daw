// DEPRECATED: This JUCE-based popup has been replaced with a Skia-based
// version. Use zenith::CollabPanel from ui/skia/CollabPanel.h instead.
#pragma once
#include "../network/CollaborationManager.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

class CollabPopup : public juce::Component, public juce::ChangeListener {
public:
  CollabPopup() {
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Real-Time Collaboration", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    // Host Section
    addAndMakeVisible(hostGroup);
    hostGroup.setText("Host Session (P2P)");

    addAndMakeVisible(hostBtn);
    hostBtn.setButtonText("Start Hosting");
    hostBtn.onClick = [this] {
      CollaborationManager::getInstance().startHosting();
      // UI updates via ChangeListener
    };

    addAndMakeVisible(codeDisplay);
    codeDisplay.setReadOnly(true);
    codeDisplay.setText("----");
    codeDisplay.setJustification(juce::Justification::centred);
    codeDisplay.setFont(juce::Font(30.0f, juce::Font::bold));

    addAndMakeVisible(copyBtn);
    copyBtn.setButtonText("Copy");
    copyBtn.setVisible(false);
    copyBtn.onClick = [this] {
      juce::SystemClipboard::copyTextToClipboard(codeDisplay.getText());
      copyBtn.setButtonText("Copied!");
    };

    // Join Section
    addAndMakeVisible(joinGroup);
    joinGroup.setText("Join Session");

    addAndMakeVisible(codeEntry);
    codeEntry.setTextToShowWhenEmpty("Enter 4-digit code", juce::Colours::grey);
    codeEntry.setJustification(juce::Justification::centred);
    codeEntry.setInputRestrictions(4, "0123456789");

    addAndMakeVisible(joinBtn);
    joinBtn.setButtonText("Join");
    joinBtn.onClick = [this] {
      CollaborationManager::getInstance().joinSession(codeEntry.getText());
    };

    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);

    CollaborationManager::getInstance().addChangeListener(this);
    setSize(300, 400);
  }

  ~CollabPopup() override {
    CollaborationManager::getInstance().removeChangeListener(this);
  }

  void changeListenerCallback(juce::ChangeBroadcaster *) override {
    auto &mgr = CollaborationManager::getInstance();
    auto state = mgr.getState();

    if (state == CollaborationManager::ConnectionState::Connected) {
      statusLabel.setText("Connected!", juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    } else if (state == CollaborationManager::ConnectionState::Connecting) {
      statusLabel.setText("Connecting...", juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    } else if (state == CollaborationManager::ConnectionState::Error) {
      statusLabel.setText("Connection / Server Error",
                          juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    } else if (state == CollaborationManager::ConnectionState::Hosting) {
      statusLabel.setText("Hosting Active", juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
      codeDisplay.setText(mgr.getCurrentCode(), juce::dontSendNotification);
      copyBtn.setVisible(true);
    } else if (state == CollaborationManager::ConnectionState::Registering) {
      statusLabel.setText("Signaling Server...", juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    } else if (state == CollaborationManager::ConnectionState::Punching) {
      statusLabel.setText("Punching Firewall...", juce::dontSendNotification);
      statusLabel.setColour(juce::Label::textColourId, juce::Colours::pink);
    }
  }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colours::darkgrey.darker(0.2f));
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
  }

  void resized() override {
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(40));

    area.removeFromTop(20);
    hostGroup.setBounds(area.removeFromTop(120));
    auto hostArea = hostGroup.getBounds().reduced(15, 20);
    hostBtn.setBounds(hostArea.removeFromTop(30));
    hostArea.removeFromTop(10);
    auto codeRow = hostArea.removeFromTop(40);
    codeDisplay.setBounds(codeRow.removeFromLeft(150));
    codeRow.removeFromLeft(10);
    copyBtn.setBounds(codeRow);

    area.removeFromTop(20);
    joinGroup.setBounds(area.removeFromTop(120));
    auto joinArea = joinGroup.getBounds().reduced(15, 20);
    codeEntry.setBounds(joinArea.removeFromTop(30));
    joinArea.removeFromTop(10);
    joinBtn.setBounds(joinArea.removeFromTop(30));

    statusLabel.setBounds(area.removeFromBottom(30));
  }

private:
  juce::Label titleLabel, codeDisplay, statusLabel;
  juce::TextButton hostBtn, copyBtn, joinBtn;
  juce::TextEditor codeEntry;
  juce::GroupComponent hostGroup, joinGroup;
};
