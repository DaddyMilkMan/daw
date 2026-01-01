/*
  ==============================================================================

    RefactorPanel.h
    Created: 2025-12-07
    Author:  Zenith DAW

    UI Component for the Project Refactorer feature.
    Shows a "Refactor" button that analyzes and cleans up the project.

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "../ai/ProjectRefactorerAgent.h"
#include "SkiaComponent.h"
#include "ZenithDesignSystem.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
/**
    Floating panel that shows refactoring analysis results and allows execution
*/
class RefactorResultsPanel : public SkiaComponent {
public:
  RefactorResultsPanel(ai::ProjectRefactorerAgent &agent) : agent_(agent) {
    setSize(400, 500);
  }

  void setPlan(const ai::RefactorPlan &plan) {
    plan_ = plan;
    repaint();
  }

  void drawSkia(SkCanvas *canvas) override {
    using namespace design;

    auto bounds = getLocalBounds().toFloat();

    // Background with glassmorphism
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors::BG_DARKER);

    SkRRect bgRRect;
    bgRRect.setRectXY(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                      spacing::RADIUS_MD, spacing::RADIUS_MD);
    canvas->drawRRect(bgRRect, bgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setColor(colors::BORDER_DEFAULT);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRRect(bgRRect, borderPaint);

    // Header
    SkPaint headerPaint;
    headerPaint.setAntiAlias(true);
    headerPaint.setColor(colors::CYAN);

    SkFont headerFont = design::getDisplayFont(18.0f);

    canvas->drawSimpleText("Project Refactoring Plan", 24,
                           SkTextEncoding::kUTF8, 20.0f, 35.0f, headerFont,
                           headerPaint);

    // Divider
    SkPaint dividerPaint;
    dividerPaint.setColor(colors::BORDER_SUBTLE);
    canvas->drawRect(
        SkRect::MakeXYWH(15.0f, 50.0f, bounds.getWidth() - 30.0f, 1.0f),
        dividerPaint);

    // Content
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors::TEXT_SECONDARY);

    SkFont bodyFont = design::getSkFont(13.0f);

    float y = 75.0f;
    float lineHeight = 22.0f;

    // Summary
    if (plan_.isEmpty()) {
      textPaint.setColor(colors::NEON_GREEN);
      canvas->drawSimpleText("[OK] Project is clean - no refactoring needed!", 46,
                             SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                             textPaint);
    } else {
      // Issues summary
      textPaint.setColor(colors::TEXT_PRIMARY);
      juce::String summary =
          "Found " + juce::String(plan_.totalIssuesFound) + " issues to fix:";
      canvas->drawSimpleText(summary.toRawUTF8(), summary.length(),
                             SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                             textPaint);
      y += lineHeight * 1.5f;

      // Track renames
      if (!plan_.trackRenames.empty()) {
        textPaint.setColor(colors::CYAN);
        juce::String renameTitle =
            "Track Renames (" + juce::String(plan_.trackRenames.size()) + "):";
        canvas->drawSimpleText(renameTitle.toRawUTF8(), renameTitle.length(),
                               SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                               textPaint);
        y += lineHeight;

        textPaint.setColor(colors::TEXT_SECONDARY);
        for (size_t i = 0; i < std::min((size_t)5, plan_.trackRenames.size());
             ++i) {
          const auto &rename = plan_.trackRenames[i];
          juce::String line = juce::String::fromUTF8("  • ") + rename.oldName + juce::String::fromUTF8(" → ") + rename.newName;
          canvas->drawSimpleText(line.toRawUTF8(), line.length(),
                                 SkTextEncoding::kUTF8, 25.0f, y, bodyFont,
                                 textPaint);
          y += lineHeight;
        }

        if (plan_.trackRenames.size() > 5) {
          juce::String more = "  ... and " +
                              juce::String(plan_.trackRenames.size() - 5) +
                              " more";
          canvas->drawSimpleText(more.toRawUTF8(), more.length(),
                                 SkTextEncoding::kUTF8, 25.0f, y, bodyFont,
                                 textPaint);
          y += lineHeight;
        }
        y += lineHeight * 0.5f;
      }

      // Color assignments
      if (!plan_.colorChanges.empty()) {
        textPaint.setColor(colors::MAGENTA);
        juce::String colorTitle =
            "Color Assignments: " + juce::String(plan_.colorChanges.size()) +
            " tracks";
        canvas->drawSimpleText(colorTitle.toRawUTF8(), colorTitle.length(),
                               SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                               textPaint);
        y += lineHeight * 1.5f;
      }

      // Dead clips
      if (!plan_.clipsToDelete.empty()) {
        textPaint.setColor(colors::RED);
        juce::String clipTitle =
            "Dead Clips to Remove: " + juce::String(plan_.clipsToDelete.size());
        canvas->drawSimpleText(clipTitle.toRawUTF8(), clipTitle.length(),
                               SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                               textPaint);
        y += lineHeight * 1.5f;
      }

      // Sample consolidation
      if (!plan_.samplesToConsolidate.empty()) {
        textPaint.setColor(colors::AMBER);
        juce::String sampleTitle =
            "Samples to Consolidate: " +
            juce::String(plan_.samplesToConsolidate.size());
        canvas->drawSimpleText(sampleTitle.toRawUTF8(), sampleTitle.length(),
                               SkTextEncoding::kUTF8, 20.0f, y, bodyFont,
                               textPaint);
        y += lineHeight * 1.5f;
      }
    }
  }

  // Callbacks
  std::function<void()> onExecute;
  std::function<void()> onCancel;

private:
  ai::ProjectRefactorerAgent &agent_;
  ai::RefactorPlan plan_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefactorResultsPanel)
};

//==============================================================================
/**
    Main Refactor Button component

    This is a toolbar button that triggers project analysis and cleanup.
    Click → Analyze → Review Plan → Execute
*/
class RefactorButton : public SkiaComponent {
public:
  RefactorButton(Engine &engine, ProjectState &projectState)
      : engine_(engine), projectState_(projectState),
        agent_(std::make_unique<ai::ProjectRefactorerAgent>(engine,
                                                            projectState)) {
    setSize(100, 32);
    setTooltip(
        "Refactor Project: Clean up track names, colors, and organize files");
  }

  ~RefactorButton() override = default;

  //==========================================================================
  void drawSkia(SkCanvas *canvas) override {
    using namespace design;

    auto bounds = getLocalBounds().toFloat();

    // Button background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    if (isAnalyzing_) {
      // Pulsing cyan while analyzing
      float pulse = 0.5f + 0.5f * std::sin(animationPhase_ * 3.14159f * 2.0f);
      bgPaint.setColor(
          SkColorSetARGB(static_cast<uint8_t>(100 + 50 * pulse), 0, 255, 255));
    } else if (isHovered_) {
      bgPaint.setColor(colors::BG_MEDIUM);
    } else {
      bgPaint.setColor(colors::BG_DARK);
    }

    SkRRect buttonRRect;
    buttonRRect.setRectXY(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                          spacing::RADIUS_SM, spacing::RADIUS_SM);
    canvas->drawRRect(buttonRRect, bgPaint);

    // Border with glow when active
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);

    if (isAnalyzing_) {
      borderPaint.setColor(colors::CYAN);
    } else if (hasResults_ && !lastPlan_.isEmpty()) {
      borderPaint.setColor(colors::AMBER); // Has changes available
    } else {
      borderPaint.setColor(colors::BORDER_DEFAULT);
    }

    canvas->drawRRect(buttonRRect, borderPaint);

    // Icon (wrench/magic wand)
    SkPaint iconPaint;
    iconPaint.setAntiAlias(true);
    iconPaint.setColor(isAnalyzing_ ? colors::CYAN : colors::TEXT_PRIMARY);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(1.5f);
    iconPaint.setStrokeCap(SkPaint::kRound_Cap);

    float iconX = 12.0f;
    float iconY = bounds.getHeight() / 2.0f;

    // Draw a simple magic wand / refactor icon
    SkPath iconPath;
    iconPath.moveTo(iconX, iconY + 5);
    iconPath.lineTo(iconX + 10, iconY - 5);
    iconPath.moveTo(iconX + 2, iconY - 6);
    iconPath.lineTo(iconX + 4, iconY - 8);
    iconPath.moveTo(iconX + 6, iconY - 8);
    iconPath.lineTo(iconX + 8, iconY - 10);
    canvas->drawPath(iconPath, iconPaint);

    // Text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors::TEXT_PRIMARY);

    SkFont font = design::getSkFont(12.0f, design::FontWeight::Bold);

    juce::String label = isAnalyzing_ ? "Analyzing..." : "Refactor";
    canvas->drawSimpleText(label.toRawUTF8(), label.length(),
                           SkTextEncoding::kUTF8, 30.0f,
                           bounds.getHeight() / 2.0f + 4.0f, font, textPaint);
  }

  //==========================================================================
  void mouseEnter(const juce::MouseEvent &) override {
    isHovered_ = true;
    repaint();
  }

  void mouseExit(const juce::MouseEvent &) override {
    isHovered_ = false;
    repaint();
  }

  void mouseDown(const juce::MouseEvent &) override {
    if (!isAnalyzing_) {
      startAnalysis();
    }
  }

  //==========================================================================
  void startAnalysis() {
    if (isAnalyzing_)
      return;

    isAnalyzing_ = true;
    hasResults_ = false;
    animationPhase_ = 0.0f;

    // Start animation timer
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(30);

    // Run analysis
    agent_->analyze(
        [this](const ai::RefactorProgress &progress) {
          // Update progress (could show in tooltip or status bar)
          juce::ignoreUnused(progress);
          repaint();
        },
        [this](const ai::RefactorPlan &plan) {
          lastPlan_ = plan;
          isAnalyzing_ = false;
          hasResults_ = true;
          stopTimer();
          repaint();

          // Show results dialog
          showResultsDialog(plan);
        });

    repaint();
  }

  void showResultsDialog(const ai::RefactorPlan &plan) {
    // Create dialog content
    juce::String message = plan.getSummary();

    if (plan.isEmpty()) {
      juce::AlertWindow::showMessageBoxAsync(
          juce::MessageBoxIconType::InfoIcon, "Project Refactoring",
          "Your project is already clean - no refactoring needed!", "OK");
    } else {
      // Show confirmation dialog
      auto options =
          juce::MessageBoxOptions()
              .withTitle("Project Refactoring")
              .withMessage(message +
                           "\n\nWould you like to apply these changes?")
              .withButton("Apply Changes")
              .withButton("Cancel")
              .withIconType(juce::MessageBoxIconType::QuestionIcon);

      juce::AlertWindow::showAsync(options, [this, plan](int result) {
        if (result == 1) // Apply Changes
        {
          executeRefactoring(plan);
        }
      });
    }
  }

  void executeRefactoring(const ai::RefactorPlan &plan) {
    agent_->execute(
        plan,
        [](const ai::RefactorProgress &progress) {
          // Could show progress bar
          juce::ignoreUnused(progress);
        },
        [](bool success, const juce::String &message) {
          if (success) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon, "Refactoring Complete",
                message, "OK");
          } else {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon, "Refactoring Failed",
                message, "OK");
          }
        });
  }

  //==========================================================================
  void timerCallback() override {
    animationPhase_ += 0.03f;
    if (animationPhase_ > 1.0f)
      animationPhase_ -= 1.0f;
    repaint();
  }

private:
  Engine &engine_;
  ProjectState &projectState_;
  std::unique_ptr<ai::ProjectRefactorerAgent> agent_;

  bool isHovered_ = false;
  bool isAnalyzing_ = false;
  bool hasResults_ = false;
  float animationPhase_ = 0.0f;
  ai::RefactorPlan lastPlan_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RefactorButton)
};

} // namespace zenith
