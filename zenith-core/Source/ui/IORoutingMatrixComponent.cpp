#include "../../include/IORoutingMatrixComponent.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"

IORoutingMatrixComponent::IORoutingMatrixComponent(Engine &engine,
                                                   ProjectState &state)
    : engine_(engine), projectState_(state) {
  // Initialize routing data
  refreshRoutingData();

  // Set up UI
  setSize(800, 600);
}

IORoutingMatrixComponent::~IORoutingMatrixComponent() {}

void IORoutingMatrixComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey);

  // Title
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font(24.0f, juce::Font::bold));
  g.drawText("I/O Routing Matrix", getLocalBounds().removeFromTop(40),
             juce::Justification::centred);

  // Grid layout
  const int rowHeight = 30;
  const int colWidth = 150;
  const int headerHeight = 30;

  auto bounds = getLocalBounds().reduced(10);
  bounds.removeFromTop(40); // Remove title area

  // Draw column headers
  auto headerBounds = bounds.removeFromTop(headerHeight);
  int x = 0;

  g.setFont(juce::Font(14.0f, juce::Font::bold));

  // Track name column
  g.draw Text("Track", headerBounds.removeFromLeft(colWidth),
              juce::Justification::centredLeft);

  // Input columns
  g.drawText("Audio In", headerBounds.removeFromLeft(colWidth),
             juce::Justification::centred);
  g.drawText("MIDI In", headerBounds.removeFromLeft(colWidth),
             juce::Justification::centred);

  // Output columns
  g.drawText("Output", headerBounds.removeFromLeft(colWidth),
             juce::Justification::centred);

  // Send columns (up to 4)
  for (int i = 0; i < 4; ++i) {
    g.drawText("Send " + juce::String(i + 1),
               headerBounds.removeFromLeft(colWidth),
               juce::Justification::centred);
  }

  // Draw track rows
  g.setFont(juce::Font(13.0f));

  for (size_t i = 0; i < trackRoutings_.size(); ++i) {
    auto &routing = trackRoutings_[i];
    auto rowBounds = bounds.removeFromTop(rowHeight);

    // Alternate row colors
    if (i % 2 == 0) {
      g.setColour(juce::Colours::black.withAlpha(0.3f));
      g.fillRect(rowBounds);
    }

    g.setColour(juce::Colours::white);

    // Track name
    g.drawText(routing.trackName, rowBounds.removeFromLeft(colWidth),
               juce::Justification::centredLeft);

    // Audio input
    g.setColour(juce::Colours::lightblue);
    auto audioInBounds = rowBounds.removeFromLeft(colWidth).reduced(5);
    g.drawText(routing.audioInput.isEmpty() ? "None" : routing.audioInput,
               audioInBounds, juce::Justification::centred);

    // MIDI input
    g.setColour(juce::Colours::lightgreen);
    auto midiInBounds = rowBounds.removeFromLeft(colWidth).reduced(5);
    g.drawText(routing.midiInput.isEmpty() ? "None" : routing.midiInput,
               midiInBounds, juce::Justification::centred);

    // Output
    g.setColour(juce::Colours::lightyellow);
    auto outputBounds = rowBounds.removeFromLeft(colWidth).reduced(5);
    g.drawText(routing.audioOutput, outputBounds, juce::Justification::centred);

    // Sends
    for (int sendIdx = 0; sendIdx < 4; ++sendIdx) {
      auto sendBounds = rowBounds.removeFromLeft(colWidth).reduced(10);

      if (sendIdx < routing.sendLevels.size()) {
        float level = routing.sendLevels[sendIdx];

        // Draw level bar
        if (level > 0.001f) {
          g.setColour(juce::Colours::orange.withAlpha(0.7f));
          auto levelRect = sendBounds.reduced(0, 5);
          levelRect.setWidth(static_cast<int>(levelRect.getWidth() * level));
          g.fillRoundedRectangle(levelRect.toFloat(), 2.0f);

          // Draw level text
          g.setColour(juce::Colours::white);
          g.setFont(juce::Font(11.0f));
          g.drawText(juce::String(static_cast<int>(level * 100)) + "%",
                     sendBounds, juce::Justification::centred);
        } else {
          g.setColour(juce::Colours::grey.withAlpha(0.3f));
          g.drawText("-", sendBounds, juce::Justification::centred);
        }
      }
    }
  }

  // Draw grid lines
  g.setColour(juce::Colours::grey.withAlpha(0.5f));
  bounds = getLocalBounds().reduced(10);
  bounds.removeFromTop(40 + headerHeight);

  // Vertical lines
  int xPos = colWidth;
  for (int i = 0; i < 7; ++i) // 7 columns total
  {
    g.drawLine(static_cast<float>(xPos), 40.0f + headerHeight,
               static_cast<float>(xPos), static_cast<float>(getHeight() - 10),
               1.0f);
    xPos += colWidth;
  }
}

void IORoutingMatrixComponent::resized() {
  // Layout is handled in paint()
}

void IORoutingMatrixComponent::mouseDown(const juce::MouseEvent &event) {
  // Calculate which cell was clicked
  const int rowHeight = 30;
  const int colWidth = 150;
  const int headerHeight = 30;

  int mouseY = event.y - 40 - headerHeight; // Remove title and header
  int mouseX = event.x - 10;                // Remove margin

  if (mouseY < 0 || mouseX < 0)
    return;

  int row = mouseY / rowHeight;
  int col = mouseX / colWidth;

  if (row < 0 || row >= static_cast<int>(trackRoutings_.size()))
    return;

  auto &routing = trackRoutings_[row];

  // Column 0: Track name (no action)
  // Column 1: Audio input
  if (col == 1) {
    showAudioInputMenu(row);
  }
  // Column 2: MIDI input
  else if (col == 2) {
    showMidiInputMenu(row);
  }
  // Column 3: Output
  else if (col == 3) {
    showOutputMenu(row);
  }
  // Columns 4-7: Sends
  else if (col >= 4 && col < 8) {
    int sendIndex = col - 4;
    showSendLevelSlider(row, sendIndex);
  }
}

void IORoutingMatrixComponent::refresh Routing Data() {
  trackRoutings_.clear();

  // Get track data from Engine
  const auto &tracks = engine_.tracks();

  for (size_t i = 0; i < tracks.size(); ++i) {
    auto *track = tracks[i].get();
    if (track == nullptr)
      continue;

    TrackRouting routing;
    routing.trackId = juce::String(i);
    routing.trackName = track->getName();

    // Get audio input channel
    int inputCh = track->getInputChannel();
    routing.audioInput = "Input " + juce::String(inputCh + 1);

    // MIDI input (hardcoded for now)
    routing.midiInput = "All MIDI";

    // Output (always master for now)
    routing.audioOutput = "Master";

    // Get send levels from mixer channel
    routing.sendLevels.clear();
    routing.sendPrePost.clear();

    for (int s = 0; s < 4; ++s) {
      float level = track->getMixerChannel().getSendLevel(s);
      routing.sendLevels.add(level);
      routing.sendPrePost.add(false); // Post-fader default
    }

    routing.monitoring = track->isArmed();
    routing.inputGain = 1.0f;

    trackRoutings_.add(routing);
  }

  repaint();
}

void IORoutingMatrixComponent::showAudioInputMenu(int trackIndex) {
  juce::PopupMenu menu;

  // Get audio input devices
  // For now, show simple numeric inputs
  for (int i = 0; i < 8; ++i) {
    menu.addItem(i + 1, "Input " + juce::String(i + 1));
  }

  menu.showMenuAsync(
      juce::PopupMenu::Options(), [this, trackIndex](int result) {
        if (result > 0) {
          // Get track and set input channel
          const auto &tracks = engine_.tracks();
          if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size())) {
            tracks[trackIndex]->setInputChannel(result - 1);
            refreshRoutingData();
          }
        }
      });
}

void IORoutingMatrixComponent::showMidiInputMenu(int trackIndex) {
  juce::PopupMenu menu;

  menu.addItem(1, "All MIDI Inputs", true, true);
  menu.addSeparator();

  // Get MIDI input devices
  auto midiInputs = juce::MidiInput::getAvailableDevices();
  for (int i = 0; i < midiInputs.size(); ++i) {
    menu.addItem(i + 10, midiInputs[i].name);
  }

  menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
    // TODO: Implement per-track MIDI input routing
    // For now, all tracks receive from all MIDI inputs
  });
}

void IORoutingMatrixComponent::showOutputMenu(int trackIndex) {
  juce::PopupMenu menu;

  menu.addItem(1, "Master", true, true);
  menu.addSeparator();

  // Add aux buses as output options
  for (int i = 0; i < engine_.getNumAuxBuses(); ++i) {
    auto *auxBus = engine_.getAuxBus(i);
    if (auxBus != nullptr) {
      menu.addItem(i + 10, auxBus->getName());
    }
  }

  menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
    // TODO: Implement track output routing to aux buses
    // Currently all tracks route to master
  });
}

void IORoutingMatrixComponent::showSendLevelSlider(int trackIndex,
                                                   int sendIndex) {
  const auto &tracks = engine_.tracks();
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size()))
    return;

  auto *track = tracks[trackIndex].get();
  if (track == nullptr)
    return;

  // Create a simple slider popup
  auto *slider = new juce::Slider(juce::Slider::LinearHorizontal,
                                  juce::Slider::TextBoxRight);
  slider->setRange(0.0, 1.0, 0.01);
  slider->setValue(track->getMixerChannel().getSendLevel(sendIndex));
  slider->setSize(200, 30);

  // Show as popup
  juce::CallOutBox::launchAsynchronously(
      std::unique_ptr<juce::Component>(slider), getScreenBounds(), nullptr);

  slider->onValueChange = [this, track, sendIndex, slider]() {
    track->getMixerChannel().setSendLevel(
        sendIndex, static_cast<float>(slider->getValue()));
    refreshRoutingData();
  };
}
