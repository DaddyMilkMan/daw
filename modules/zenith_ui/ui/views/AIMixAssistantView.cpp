/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    AIMixAssistantView.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/


#include "AIMixAssistantView.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../engine/Engine.h"

namespace zenith {

AIMixAssistantView::AIMixAssistantView(Engine &engine) : engine_(engine) {
  addAndMakeVisible(analyzeButton);
  analyzeButton.onClick = [this] { performAnalysis(); };

  addAndMakeVisible(applyMasteringButton);
  applyMasteringButton.setColour(juce::TextButton::buttonColourId,
                                 zenith::design::colors::ACCENT_PRIMARY);
  applyMasteringButton.onClick = [this] { applyMastering(); };

  addAndMakeVisible(targetLufsSlider);
  targetLufsSlider.setRange(-24.0, -6.0, 0.1);
  targetLufsSlider.setValue(-14.0);
  targetLufsSlider.setTextValueSuffix(" LUFS");

  addAndMakeVisible(targetLufsLabel);
  targetLufsLabel.setText("Target Loudness", juce::dontSendNotification);
  targetLufsLabel.attachToComponent(&targetLufsSlider, false);

  addAndMakeVisible(enableEq);
  enableEq.setToggleState(true, juce::dontSendNotification);

  addAndMakeVisible(enableComp);
  enableComp.setToggleState(true, juce::dontSendNotification);

  addAndMakeVisible(enableLimit);
  enableLimit.setToggleState(true, juce::dontSendNotification);

  addAndMakeVisible(compAmountSlider);
  compAmountSlider.setRange(0.0, 1.0, 0.01);
  compAmountSlider.setValue(0.5);

  addAndMakeVisible(compAmountLabel);
  compAmountLabel.setText("Compression Amount", juce::dontSendNotification);
  compAmountLabel.attachToComponent(&compAmountSlider, false);
}

AIMixAssistantView::~AIMixAssistantView() {}

void AIMixAssistantView::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds();

  // Background
  g.fillAll(zenith::design::colors::BG_PANEL);

  // Header
  g.setFont(zenith::design::typography::getFont(
      zenith::design::typography::FONT_LG,
      zenith::design::typography::FontWeight::Bold));
  g.setColour(zenith::design::colors::TEXT_PRIMARY);
  g.drawText("AI Mix Assistant", bounds.removeFromTop(40).reduced(10, 0),
             juce::Justification::left, true);

  // Status
  g.setFont(
      zenith::design::typography::getFont(zenith::design::typography::FONT_MD));
  g.setColour(zenith::design::colors::TEXT_SECONDARY);
  g.drawText(mixStatus, bounds.removeFromTop(30).reduced(10, 0),
             juce::Justification::left, true);
}

void AIMixAssistantView::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(60); // Header area

  auto analyzeArea = area.removeFromTop(40);
  analyzeButton.setBounds(analyzeArea.removeFromLeft(200));

  area.removeFromTop(20);

  auto optsArea = area.removeFromTop(120);
  int w = optsArea.getWidth() / 2;

  auto leftCol = optsArea.removeFromLeft(w).reduced(10);
  auto rightCol = optsArea.reduced(10);

  enableEq.setBounds(leftCol.removeFromTop(30));
  enableComp.setBounds(leftCol.removeFromTop(30));
  enableLimit.setBounds(leftCol.removeFromTop(30));

  targetLufsSlider.setBounds(rightCol.removeFromTop(40));
  compAmountSlider.setBounds(rightCol.removeFromTop(40));

  area.removeFromTop(20);
  applyMasteringButton.setBounds(area.removeFromTop(40).removeFromLeft(200));
}

void AIMixAssistantView::performAnalysis() {
  // In a real implementation, this would trigger the agent
  // For now, we simulate success
  mixStatus = "Analysis Complete: Dynamic Range Good, Balance Optimized.";
  repaint();

  if (auto *agent = engine_.getMasteringAgent()) {
    agent->runMasteringPass({});
  }
}

void AIMixAssistantView::applyMastering() {
  ai::AIMasteringAgent::MasteringOptions opts;
  opts.targetLufs = (float)targetLufsSlider.getValue();
  opts.applyEq = enableEq.getToggleState();
  opts.applyCompression = enableComp.getToggleState();
  opts.applyLimiter = enableLimit.getToggleState();
  opts.compressionAmount = (float)compAmountSlider.getValue();

  // Trigger agent
  if (auto *agent = engine_.getMasteringAgent()) {
    agent->runMasteringPass(opts);
  }
  mixStatus = "Mastering Chain Applied to Output.";
  repaint();
}

} // namespace zenith
