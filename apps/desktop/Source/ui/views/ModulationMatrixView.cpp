/*
  ==============================================================================

    ModulationMatrixView.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Universal Modulation Matrix UI Implementation.

  ==============================================================================
*/

#include "ModulationMatrixView.h"
#include "../../../include/Engine.h"
#include "../../engine/Track.h"

namespace zenith {

//==============================================================================
ModulationMatrixView::ModulationMatrixView() {
  // Create viewport for scrolling
  viewport_ = std::make_unique<juce::Viewport>();
  matrixContent_ = std::make_unique<juce::Component>();

  viewport_->setViewedComponent(matrixContent_.get(), false);
  viewport_->setScrollBarsShown(true, true);
  addAndMakeVisible(*viewport_);

  // Start timer for live updates
  startTimerHz(30);
}

ModulationMatrixView::~ModulationMatrixView() { stopTimer(); }

void ModulationMatrixView::setEngine(Engine *engine) {
  engine_ = engine;
  refreshMatrix();
}

void ModulationMatrixView::paint(juce::Graphics &g) {
  // Background
  g.fillAll(juce::Colour(0xff1a1a1a));

  // Title
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font(18.0f).boldened());
  g.drawText("Modulation Matrix", 10, 10, getWidth() - 20, 30,
             juce::Justification::centredLeft);

  // Header separator
  g.setColour(juce::Colour(0xff333333));
  g.drawHorizontalLine(44, 0, static_cast<float>(getWidth()));
}

void ModulationMatrixView::resized() {
  auto bounds = getLocalBounds();

  // Leave space for title
  bounds.removeFromTop(50);

  viewport_->setBounds(bounds);

  // Size the matrix content
  if (matrixContent_) {
    int contentWidth =
        kLabelWidth + static_cast<int>(destLabels_.size()) * kCellWidth + 20;
    int contentHeight = kHeaderHeight +
                        static_cast<int>(sourceLabels_.size()) * kCellHeight +
                        20;
    matrixContent_->setSize(juce::jmax(contentWidth, bounds.getWidth()),
                            juce::jmax(contentHeight, bounds.getHeight()));
  }
}

void ModulationMatrixView::timerCallback() {
  // TODO: Update visualization of active modulation values
  // For now, we just repaint active cells slightly brighter?
  // In a real implementation, we'd query the Engine for current modulation
  // values.
  repaint();
}

void ModulationMatrixView::refreshMatrix() {
  buildSourceLabels();
  buildDestLabels();
  createCells();
  resized();
  repaint();
}

void ModulationMatrixView::buildSourceLabels() {
  sourceLabels_.clear();
  sourceIds_.clear();

  // Global LFOs
  for (int i = 0; i < zenith::kNumGlobalLFOs; ++i) {
    sourceLabels_.add("LFO " + juce::String(i + 1));
    sourceIds_.add("sys:lfo:" + juce::String(i));
  }

  // Macros
  for (int i = 0; i < Engine::getNumMacros(); ++i) {
    if (engine_) {
      sourceLabels_.add(engine_->getMacro(i).getName());
    } else {
      sourceLabels_.add("Macro " + juce::String(i + 1));
    }
    sourceIds_.add("sys:macro:" + juce::String(i));
  }

  // Track Envelopes (if engine available)
  if (engine_) {
    const auto &tracks = engine_->tracks();
    for (size_t i = 0; i < tracks.size(); ++i) {
      if (tracks[i]) {
        sourceLabels_.add(tracks[i]->getName() + " Env");
        // Use Track ID for AudioEnvelope source
        sourceIds_.add(tracks[i]->getTrackId());
      }
    }
  }
}

void ModulationMatrixView::buildDestLabels() {
  destLabels_.clear();
  destMappings_.clear();

  // Per-track destinations (if engine available)
  if (engine_) {
    const auto &tracks = engine_->tracks();
    for (size_t i = 0; i < tracks.size() && i < 8; ++i) { // Limit for UI sanity
      if (tracks[i]) {
        juce::String prefix = tracks[i]->getName();
        juce::String trackId = tracks[i]->getTrackId();

        // Iterate ALL plugins on the track
        int numPlugins = tracks[i]->getNumPlugins();
        for (int pl = 0; pl < numPlugins; ++pl) {
          auto *plugin = tracks[i]->getPlugin(pl);
          if (plugin) {
            auto params = plugin->getParameters();
            for (int p = 0; p < params.size(); ++p) {
              auto *param = params[p];
              if (!param)
                continue;

              juce::String paramName = param->getName(32);
              if (paramName.isEmpty())
                paramName = "P" + juce::String(p + 1);

              // Label: "TrackName FX1 ParamName"
              destLabels_.add(prefix + " FX" + juce::String(pl + 1) + " " +
                              paramName);
              destMappings_.push_back({trackId, pl, p});
            }
          }
        }
      }
    }
  }
}

void ModulationMatrixView::createCells() {
  // Clear managed components
  cells_.clear();
  labels_.clear();

  if (!matrixContent_)
    return;

  // Remove all children from content component
  matrixContent_->removeAllChildren();

  int numRows = sourceLabels_.size();
  int numCols = destLabels_.size();

  // Create source labels (row headers)
  for (int row = 0; row < numRows; ++row) {
    auto *label = new juce::Label();
    label->setText(sourceLabels_[row], juce::dontSendNotification);
    label->setColour(juce::Label::textColourId, juce::Colours::white);
    label->setFont(juce::Font(12.0f));
    label->setBounds(5, kHeaderHeight + row * kCellHeight, kLabelWidth - 10,
                     kCellHeight);
    matrixContent_->addAndMakeVisible(label);
    labels_.add(label); // Manage memory
  }

  // Create destination labels (column headers) - rotated
  for (int col = 0; col < numCols; ++col) {
    auto *label = new juce::Label();
    label->setText(destLabels_[col], juce::dontSendNotification);
    label->setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    label->setFont(juce::Font(10.0f));
    label->setJustificationType(juce::Justification::bottomLeft);

    // Position for rotated text effect
    int x = kLabelWidth + col * kCellWidth;
    label->setBounds(x, 5, kCellWidth, kHeaderHeight - 10);
    label->setTransform(juce::AffineTransform::rotation(
        -0.5f, static_cast<float>(x + kCellWidth / 2),
        static_cast<float>(kHeaderHeight / 2)));
    matrixContent_->addAndMakeVisible(label);
    labels_.add(label); // Manage memory
  }

  // Create matrix cells
  for (int row = 0; row < numRows; ++row) {
    for (int col = 0; col < numCols; ++col) {
      auto *cell = new ModulationMatrixCell();

      int x = kLabelWidth + col * kCellWidth;
      int y = kHeaderHeight + row * kCellHeight;
      cell->setBounds(x, y, kCellWidth - 2, kCellHeight - 2);

      // Capture IDs
      juce::String srcId = sourceIds_[row];
      DestMapping dest = destMappings_[static_cast<size_t>(col)];

      // Initial state check (read from RoutingGraph)
      float initialAmount = 0.0f;
      if (engine_) {
        auto connections =
            engine_->getRoutingGraph().getConnectionsTo(dest.nodeId);
        for (const auto &conn : connections) {
          // Check if this connection matches our cell (Source -> Dest Param)
          if (conn.type == RoutingGraph::Connection::Type::Modulation &&
              conn.sourceId == srcId &&
              conn.targetPluginIndex == dest.pluginIndex &&
              conn.targetParamIndex == dest.paramIndex) {
            initialAmount = conn.gain;
            break;
          }
        }
      }
      cell->setAmount(initialAmount);

      cell->onAmountChanged = [this, srcId, dest](float amount) {
        if (engine_) {
          // Update RoutingGraph
          // Note: connectModulation is thread-safe (uses lock)
          engine_->getRoutingGraph().connectModulation(
              srcId, dest.nodeId, dest.pluginIndex, dest.paramIndex, amount);
        }
      };

      cells_.add(cell);
      matrixContent_->addAndMakeVisible(cell);
    }
  }

  // Update content size
  int contentWidth = kLabelWidth + numCols * kCellWidth + 20;
  int contentHeight = kHeaderHeight + numRows * kCellHeight + 20;
  matrixContent_->setSize(contentWidth, contentHeight);
}

} // namespace zenith
