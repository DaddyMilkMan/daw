/*
  ==============================================================================

    AIAssistantPanel.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of the AI Assistant Panel UI component.

  ==============================================================================
*/

#include "AIAssistantPanel.h"
#include "../../ai/SessionDebuggerAgent.h"
#include "../../ai/UXDirectorAgent.h"
#include "../design-system/ZenithDesignSystem.h"


namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

AIAssistantPanel::AIAssistantPanel() {
  setName("AIAssistantPanel");

  // Register as status listener
  ai::AIStatusManager::getInstance().addListener(this);

  // Start animation timer
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(50); // 20 FPS for pulse animation
}

AIAssistantPanel::~AIAssistantPanel() {
  ai::AIStatusManager::getInstance().removeListener(this);
  stopTimer();
}

//==============================================================================
// SkiaComponent
//==============================================================================

void AIAssistantPanel::resized() {
  SkiaComponent::resized();
  markDirty();
}

void AIAssistantPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Update cached data
  activeOps_ = ai::AIStatusManager::getInstance().getActiveOperations();
  recentOps_ = ai::AIStatusManager::getInstance().getRecentOperations();
  stats_ = ai::AIStatusManager::getInstance().getStats();

  // Background with glassmorphism
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::colors::SURFACE_ELEVATED);
  canvas->drawRoundRect(
      SkRect::MakeXYWH(0, 0, bounds.getWidth(), bounds.getHeight()), 12, 12,
      bgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1);
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawRoundRect(SkRect::MakeXYWH(0.5f, 0.5f, bounds.getWidth() - 1,
                                         bounds.getHeight() - 1),
                        12, 12, borderPaint);

  float yOffset = 16.0f;

  drawHeader(canvas);
  yOffset += 50.0f;

  drawActiveOperations(canvas, yOffset);
  drawRecentOperations(canvas, yOffset);
  drawActionButtons(canvas, yOffset);
  drawStats(canvas, yOffset);

  // Update pulse animation moved to timerCallback for consistency
  // pulsePhase_ += 0.1f;
  // if (pulsePhase_ > 6.28f)
  //   pulsePhase_ -= 6.28f;
}

void AIAssistantPanel::timerCallback() {
  pulsePhase_ += 0.1f;
  if (pulsePhase_ > 6.28f)
    pulsePhase_ -= 6.28f;

  if (!activeOps_.empty()) {
    markDirty();
  }
}


void AIAssistantPanel::drawHeader(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();

  // Title
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_PRIMARY);

  SkFont titleFont =
      design::typography::getSkFont(16.0f, design::FontWeight::SemiBold);
  canvas->drawString("[AI] Assistant", 16.0f, 30.0f, titleFont, textPaint);

  // Status indicator
  float indicatorX = bounds.getWidth() - 30.0f;
  float indicatorY = 22.0f;

  SkPaint indicatorPaint;
  indicatorPaint.setAntiAlias(true);

  if (!activeOps_.empty()) {
    // Pulsing green for active
    float pulse = (std::sin(pulsePhase_) + 1.0f) * 0.5f;
    indicatorPaint.setColor(
        SkColorSetARGB(static_cast<uint8_t>(128 + 127 * pulse), 0, 255, 157));
  } else {
    // Dim green for idle
    indicatorPaint.setColor(0x6600FF9D);
  }

  canvas->drawCircle(indicatorX, indicatorY, 6.0f, indicatorPaint);

  // Separator line
  SkPaint linePaint;
  linePaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawLine(16.0f, 45.0f, bounds.getWidth() - 16.0f, 45.0f, linePaint);
}

void AIAssistantPanel::drawActiveOperations(SkCanvas *canvas, float &yOffset) {
  if (activeOps_.empty()) {
    // Show "No active operations" message
    SkPaint dimText;
    dimText.setAntiAlias(true);
    dimText.setColor(design::colors::TEXT_SECONDARY);

    SkFont smallFont = design::typography::getSkFont(12.0f);
    canvas->drawString("No active operations", 16.0f, yOffset + 15.0f,
                       smallFont, dimText);
    yOffset += 30.0f;
    return;
  }

  SkPaint labelPaint;
  labelPaint.setAntiAlias(true);
  labelPaint.setColor(design::colors::TEXT_PRIMARY);

  SkFont labelFont =
      design::typography::getSkFont(11.0f, design::FontWeight::SemiBold);
  canvas->drawString("ACTIVE", 16.0f, yOffset + 12.0f, labelFont, labelPaint);
  yOffset += 20.0f;

  SkFont opFont = design::typography::getSkFont(12.0f);
  auto bounds = getLocalBounds().toFloat();

  for (const auto &op : activeOps_) {
    // Operation name
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);

    juce::String displayText = op.agentName + ": " + op.description;
    canvas->drawString(displayText.toStdString().c_str(), 16.0f,
                       yOffset + 12.0f, opFont, textPaint);

    // Progress bar
    float barY = yOffset + 18.0f;
    float barWidth = bounds.getWidth() - 32.0f;
    float barHeight = 4.0f;

    // Background
    SkPaint barBgPaint;
    barBgPaint.setAntiAlias(true);
    barBgPaint.setColor(design::colors::SURFACE_BASE);
    canvas->drawRoundRect(SkRect::MakeXYWH(16.0f, barY, barWidth, barHeight), 2,
                          2, barBgPaint);

    // Progress
    SkPaint barFgPaint;
    barFgPaint.setAntiAlias(true);
    barFgPaint.setColor(design::colors::NEON_GREEN);
    canvas->drawRoundRect(
        SkRect::MakeXYWH(16.0f, barY, barWidth * op.progress, barHeight), 2, 2,
        barFgPaint);

    yOffset += 30.0f;
  }
}

void AIAssistantPanel::drawRecentOperations(SkCanvas *canvas, float &yOffset) {
  if (recentOps_.empty())
    return;

  SkPaint labelPaint;
  labelPaint.setAntiAlias(true);
  labelPaint.setColor(design::colors::TEXT_SECONDARY);

  SkFont labelFont =
      design::typography::getSkFont(11.0f, design::FontWeight::SemiBold);
  canvas->drawString("RECENT", 16.0f, yOffset + 12.0f, labelFont, labelPaint);
  yOffset += 20.0f;

  SkFont opFont = design::typography::getSkFont(11.0f);

  // Show last 3 operations
  int shown = 0;
  for (const auto &op : recentOps_) {
    if (shown >= 3)
      break;

    SkPaint textPaint;
    textPaint.setAntiAlias(true);

    // Color based on status
    switch (op.status) {
    case ai::AIOperationStatus::Success:
      textPaint.setColor(design::colors::SUCCESS);
      break;
    case ai::AIOperationStatus::Warning:
      textPaint.setColor(design::colors::AMBER);
      break;
    case ai::AIOperationStatus::Error:
      textPaint.setColor(design::colors::DANGER);
      break;
    default:
      textPaint.setColor(design::colors::TEXT_SECONDARY);
    }

    juce::String statusIcon =
        op.status == ai::AIOperationStatus::Success   ? "[OK] "
        : op.status == ai::AIOperationStatus::Warning ? "[!] "
        : op.status == ai::AIOperationStatus::Error   ? "[X] "
                                                      : "[ ] ";

    juce::String displayText = statusIcon + op.agentName;
    canvas->drawString(displayText.toStdString().c_str(), 16.0f,
                       yOffset + 12.0f, opFont, textPaint);

    yOffset += 18.0f;
    shown++;
  }
}

void AIAssistantPanel::drawActionButtons(SkCanvas *canvas, float &yOffset) {
  auto bounds = getLocalBounds().toFloat();
  float buttonWidth = (bounds.getWidth() - 48.0f) / 3.0f;
  float buttonHeight = 28.0f;

  yOffset += 10.0f;

  SkFont buttonFont =
      design::typography::getSkFont(11.0f, design::FontWeight::Medium);

  // Run Analysis button
  runAnalysisButton_ =
      juce::Rectangle<float>(16.0f, yOffset, buttonWidth, buttonHeight);

  SkPaint buttonPaint;
  buttonPaint.setAntiAlias(true);
  buttonPaint.setColor(design::colors::SURFACE_BASE);
  canvas->drawRoundRect(SkRect::MakeXYWH(runAnalysisButton_.getX(),
                                         runAnalysisButton_.getY(),
                                         runAnalysisButton_.getWidth(),
                                         runAnalysisButton_.getHeight()),
                        6, 6, buttonPaint);

  SkPaint buttonTextPaint;
  buttonTextPaint.setAntiAlias(true);
  buttonTextPaint.setColor(design::colors::TEXT_PRIMARY);
  canvas->drawString("Analyze", runAnalysisButton_.getX() + 8.0f,
                     runAnalysisButton_.getCentreY() + 4.0f, buttonFont,
                     buttonTextPaint);

  // Apply Fixes button
  applyFixesButton_ = juce::Rectangle<float>(
      16.0f + buttonWidth + 8.0f, yOffset, buttonWidth, buttonHeight);

  buttonPaint.setColor(design::colors::NEON_GREEN);
  canvas->drawRoundRect(SkRect::MakeXYWH(applyFixesButton_.getX(),
                                         applyFixesButton_.getY(),
                                         applyFixesButton_.getWidth(),
                                         applyFixesButton_.getHeight()),
                        6, 6, buttonPaint);

  buttonTextPaint.setColor(design::colors::SURFACE_BASE);
  canvas->drawString("Fix All", applyFixesButton_.getX() + 8.0f,
                     applyFixesButton_.getCentreY() + 4.0f, buttonFont,
                     buttonTextPaint);

  // Stop button
  stopButton_ = juce::Rectangle<float>(16.0f + (buttonWidth + 8.0f) * 2,
                                       yOffset, buttonWidth, buttonHeight);

  buttonPaint.setColor(design::colors::DANGER);
  buttonPaint.setAlphaf(activeOps_.empty() ? 0.3f : 1.0f);
  canvas->drawRoundRect(SkRect::MakeXYWH(stopButton_.getX(), stopButton_.getY(),
                                         stopButton_.getWidth(),
                                         stopButton_.getHeight()),
                        6, 6, buttonPaint);

  buttonTextPaint.setColor(design::colors::TEXT_PRIMARY);
  buttonTextPaint.setAlphaf(activeOps_.empty() ? 0.3f : 1.0f);
  canvas->drawString("Stop", stopButton_.getX() + 8.0f,
                     stopButton_.getCentreY() + 4.0f, buttonFont,
                     buttonTextPaint);

  yOffset += buttonHeight + 10.0f;
}

void AIAssistantPanel::drawStats(SkCanvas *canvas, float &yOffset) {
  SkPaint statsPaint;
  statsPaint.setAntiAlias(true);
  statsPaint.setColor(design::colors::TEXT_SECONDARY);

  SkFont statsFont = design::typography::getSkFont(10.0f);

  juce::String statsText = juce::String::formatted(
      "Total: %d | Success: %d | Failed: %d", stats_.totalOperations,
      stats_.successfulOperations, stats_.failedOperations);

  canvas->drawString(statsText.toStdString().c_str(), 16.0f, yOffset + 12.0f,
                     statsFont, statsPaint);
}

//==============================================================================
// Mouse Handling
//==============================================================================

void AIAssistantPanel::mouseDown(const juce::MouseEvent &e) {
  handleMouseClick(e);
}

void AIAssistantPanel::handleMouseClick(const juce::MouseEvent &e) {
  auto pos = e.position;

  if (runAnalysisButton_.contains(pos.x, pos.y)) {
    runAllAnalysis();
  } else if (applyFixesButton_.contains(pos.x, pos.y)) {
    applyAllFixes();
  } else if (stopButton_.contains(pos.x, pos.y) && !activeOps_.empty()) {
    stopAllOperations();
  }
}

//==============================================================================
// Actions
//==============================================================================

void AIAssistantPanel::runAllAnalysis() {
  DBG("AIAssistantPanel: Running all analysis");

  if (sessionDebugger_) {
    sessionDebugger_->runAnalysis();
  }

  if (uxDirector_) {
    uxDirector_->runAnalysis();
  }

  markDirty();
}

void AIAssistantPanel::applyAllFixes() {
  DBG("AIAssistantPanel: Applying all fixes");

  if (uxDirector_) {
    uxDirector_->applyAllFixes();
  }

  markDirty();
}

void AIAssistantPanel::stopAllOperations() {
  DBG("AIAssistantPanel: Stopping all operations");
  // In a full implementation, we'd signal agents to stop
  markDirty();
}

//==============================================================================
// AIStatusListener
//==============================================================================

void AIAssistantPanel::onOperationStarted(const ai::AIOperation &operation) {
  juce::ignoreUnused(operation);
  markDirty();
}

void AIAssistantPanel::onOperationProgress(const ai::AIOperation &operation) {
  juce::ignoreUnused(operation);
  markDirty();
}

void AIAssistantPanel::onOperationCompleted(const ai::AIOperation &operation) {
  juce::ignoreUnused(operation);
  markDirty();
}

void AIAssistantPanel::onOperationError(const ai::AIOperation &operation) {
  juce::ignoreUnused(operation);
  markDirty();
}

} // namespace zenith
