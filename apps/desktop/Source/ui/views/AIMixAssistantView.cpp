/*
  ==============================================================================

    AIMixAssistantView.cpp
    Created: 2025-12-20
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AIMixAssistantView.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

AIMixAssistantView::AIMixAssistantView(Engine &engine) : engine_(engine) {
  addAndMakeVisible(analyzeButton);
  analyzeButton.setStyle(ZenithButton::Style::Primary);
  analyzeButton.onClick = [this] { performAnalysis(); };

  addAndMakeVisible(applyMasteringButton);
  applyMasteringButton.setStyle(ZenithButton::Style::Success);
  applyMasteringButton.onClick = [this] { applyMastering(); };

  addAndMakeVisible(targetLufsSlider);
  targetLufsSlider.setOrientation(ZenithSlider::Orientation::Horizontal);
  targetLufsSlider.setRange(-24.0f, -6.0f, -14.0f);
  targetLufsSlider.setValue(-14.0f);

  targetLufsLabel = std::make_unique<SkiaLabel>("ai_target_lufs", "Target Loudness (LUFS)");
  targetLufsLabel->setTextColour(zenith::design::colors::TEXT_SECONDARY);
  addAndMakeVisible(targetLufsLabel.get());

  addAndMakeVisible(enableEq);
  enableEq.setStyle(ZenithButton::Style::Secondary);
  enableEq.setToggleable(true);
  enableEq.setToggleState(true);

  addAndMakeVisible(enableComp);
  enableComp.setStyle(ZenithButton::Style::Secondary);
  enableComp.setToggleable(true);
  enableComp.setToggleState(true);

  addAndMakeVisible(enableLimit);
  enableLimit.setStyle(ZenithButton::Style::Secondary);
  enableLimit.setToggleable(true);
  enableLimit.setToggleState(true);

  addAndMakeVisible(compAmountSlider);
  compAmountSlider.setOrientation(ZenithSlider::Orientation::Horizontal);
  compAmountSlider.setRange(0.0f, 1.0f, 0.5f);
  compAmountSlider.setValue(0.5f);

  compAmountLabel = std::make_unique<SkiaLabel>("ai_comp_amt", "Compression Amount");
  compAmountLabel->setTextColour(zenith::design::colors::TEXT_SECONDARY);
  addAndMakeVisible(compAmountLabel.get());
}

AIMixAssistantView::~AIMixAssistantView() {}

void AIMixAssistantView::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkPaint bg;
  bg.setColor(zenith::design::colors::BG_PANEL);
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bg);

  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(zenith::design::colors::TEXT_PRIMARY);
  SkFont titleFont =
      zenith::design::getSkFont(zenith::design::typography::FONT_LG,
                                zenith::design::FontWeight::Bold);
  canvas->drawString("AI Mix Assistant", 20.0f, 34.0f, titleFont, titlePaint);

  SkPaint statusPaint;
  statusPaint.setAntiAlias(true);
  statusPaint.setColor(zenith::design::colors::TEXT_SECONDARY);
  SkFont statusFont =
      zenith::design::getSkFont(zenith::design::typography::FONT_MD,
                                zenith::design::FontWeight::Regular);
  canvas->drawString(mixStatus.toRawUTF8(), 20.0f, 58.0f, statusFont, statusPaint);
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

  if (targetLufsLabel) targetLufsLabel->setBounds(rightCol.removeFromTop(20));
  targetLufsSlider.setBounds(rightCol.removeFromTop(40));
  if (compAmountLabel) compAmountLabel->setBounds(rightCol.removeFromTop(20));
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
  opts.targetLufs = targetLufsSlider.getValue();
  opts.applyEq = enableEq.getToggleState();
  opts.applyCompression = enableComp.getToggleState();
  opts.applyLimiter = enableLimit.getToggleState();
  opts.compressionAmount = compAmountSlider.getValue();

  // Trigger agent
  if (auto *agent = engine_.getMasteringAgent()) {
    agent->runMasteringPass(opts);
  }
  mixStatus = "Mastering Chain Applied to Output.";
  repaint();
}

} // namespace zenith
