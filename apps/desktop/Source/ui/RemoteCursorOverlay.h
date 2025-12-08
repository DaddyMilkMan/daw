#pragma once
#include "../network/CollaborationManager.h"
#include <JuceHeader.h>


class RemoteCursorOverlay : public juce::Component,
                            public juce::ChangeListener,
                            public juce::Timer {
public:
  RemoteCursorOverlay() {
    setInterceptsMouseClicks(false, false); // Pass clicks through to DAW
    CollaborationManager::getInstance().addChangeListener(this);
    startTimerHz(60); // Smooth animation
  }

  ~RemoteCursorOverlay() override {
    CollaborationManager::getInstance().removeChangeListener(this);
  }

  void changeListenerCallback(juce::ChangeBroadcaster *) override { repaint(); }

  void timerCallback() override {
    // Send MY mouse position to others
    auto mouse =
        juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
    auto relative = getLocalPoint(nullptr, mouse);

    float normX = (float)relative.getX() / (float)getWidth();
    float normY = (float)relative.getY() / (float)getHeight();

    CollaborationManager::getInstance().updateLocalCursor(normX, normY);
  }

  void paint(juce::Graphics &g) override {
    auto &mgr = CollaborationManager::getInstance();
    if (mgr.getState() != CollaborationManager::ConnectionState::Connected &&
        mgr.getState() != CollaborationManager::ConnectionState::Hosting)
      return;

    for (const auto &user : mgr.getRemoteUsers()) {
      float x = user.mousePosition.x * getWidth();
      float y = user.mousePosition.y * getHeight();

      g.setColour(user.color);

      // Draw Cursor Arrow
      juce::Path p;
      p.startNewSubPath(x, y);
      p.lineTo(x + 15, y + 5);
      p.lineTo(x + 5, y + 15);
      p.closeSubPath();
      g.fillPath(p);
      g.strokePath(p, juce::PathStrokeType(1.0f));

      // Draw Name Tag
      g.setFont(12.0f);
      g.drawText(user.name, (int)x + 10, (int)y + 10, 100, 20,
                 juce::Justification::left);
    }
  }
};
