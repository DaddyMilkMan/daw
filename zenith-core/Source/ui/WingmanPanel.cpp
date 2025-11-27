/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    In-DAW command console implementation with AI mode

  ==============================================================================
*/

// POLISH: spacing normalized to 8px grid (titleBar 32, margins 8, buttons
// 24/32, badge 48x16) POLISH: typography now uses SkiaTheme::Typography
// (body/small)

#include "WingmanPanel.h"
#include "../Source/commands/CommandAPI.h"
#include "../Source/commands/SessionGraph.h"
#include "../Source/network/AIBridgeClient.h"
#include "AudioFeedback.h"
#include "ZenithLookAndFeel.h"
#include "skia/SkiaTheme.h"

//==============================================================================
WingmanPanel::WingmanPanel(zenith::CommandAPI &api,
                           zenith::AIBridgeClient &aiClient)
    : commandAPI(api), aiBridgeClient(aiClient) {
  using namespace zenith;

  // Start 60Hz animation timer
  startTimerHz(60);

  // Register as listener for AI responses
  aiBridgeClient.addChangeListener(this);

#ifdef ZENITH_USE_SKIA
  // ===== SKIA VERSION =====

  // Create history display (read-only, multi-line)
  historyDisplay = std::make_unique<zenith::SkiaTextInput>("");
  historyDisplay->setReadOnly(true);
  addAndMakeVisible(*historyDisplay);

  // Create command input
  commandInput = std::make_unique<zenith::SkiaTextInput>("");
  commandInput->setPlaceholder("Enter command (JSON or shorthand)...");
  commandInput->onReturnKey = [this]() {
    juce::String input = commandInput->getText().trim();
    if (input.isNotEmpty()) {
      executeCommand(input);
      commandInput->setText("");
    }
  };
  addAndMakeVisible(*commandInput);

  // Mode buttons
  commandModeButton = std::make_unique<zenith::SkiaButtonNative>(
      "Command", zenith::SkiaButtonNative::Style::Primary);
  commandModeButton->onClick = [this]() {
    setMode(Mode::Command);
    commandModeButton->setStyle(zenith::SkiaButtonNative::Style::Primary);
    aiModeButton->setStyle(zenith::SkiaButtonNative::Style::Secondary);
  };
  addAndMakeVisible(*commandModeButton);

  aiModeButton = std::make_unique<zenith::SkiaButtonNative>(
      "AI", zenith::SkiaButtonNative::Style::Secondary);
  aiModeButton->onClick = [this]() {
    setMode(Mode::AI);
    aiModeButton->setStyle(zenith::SkiaButtonNative::Style::Primary);
    commandModeButton->setStyle(zenith::SkiaButtonNative::Style::Secondary);
  };
  addAndMakeVisible(*aiModeButton);

  clearButton = std::make_unique<zenith::SkiaButtonNative>(
      "Clear", zenith::SkiaButtonNative::Style::Secondary);
  clearButton->onClick = [this]() { clearHistory(); };
  addAndMakeVisible(*clearButton);

#else
  // ===== JUCE FALLBACK VERSION =====

  // Create history display (read-only, multi-line)
  historyDisplay = std::make_unique<juce::TextEditor>();
  historyDisplay->setMultiLine(true);
  historyDisplay->setReadOnly(true);
  historyDisplay->setFont(ZenithLookAndFeel::getFontSmall());
  historyDisplay->setColour(
      juce::TextEditor::backgroundColourId,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundDark));
  historyDisplay->setColour(
      juce::TextEditor::textColourId,
      juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
  historyDisplay->setColour(juce::TextEditor::outlineColourId,
                            juce::Colour(ZenithLookAndFeel::Colors::border));
  addAndMakeVisible(*historyDisplay);

  // Create command input with Zenith styling
  commandInput = std::make_unique<juce::TextEditor>();
  commandInput->setMultiLine(false);
  commandInput->setReturnKeyStartsNewLine(false);
  commandInput->setFont(ZenithLookAndFeel::getFontBody());
  commandInput->setColour(
      juce::TextEditor::backgroundColourId,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel));
  commandInput->setColour(juce::TextEditor::textColourId,
                          juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
  commandInput->setColour(juce::TextEditor::outlineColourId,
                          juce::Colour(ZenithLookAndFeel::Colors::border));
  commandInput->setColour(
      juce::TextEditor::focusedOutlineColourId,
      juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
  commandInput->addListener(this);
  commandInput->setTextToShowWhenEmpty(
      "Enter command (JSON or shorthand)...",
      juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
  addAndMakeVisible(*commandInput);

  // JUCE buttons with Zenith styling
  commandModeButton = std::make_unique<juce::TextButton>("Command");
  commandModeButton->setClickingTogglesState(true);
  commandModeButton->setRadioGroupId(1);
  commandModeButton->setToggleState(true, juce::dontSendNotification);
  commandModeButton->setColour(
      juce::TextButton::buttonColourId,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
  commandModeButton->setColour(
      juce::TextButton::buttonOnColourId,
      juce::Colour(ZenithLookAndFeel::Colors::accentPrimary));
  commandModeButton->setColour(
      juce::TextButton::textColourOffId,
      juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
  commandModeButton->setColour(
      juce::TextButton::textColourOnId,
      juce::Colour(ZenithLookAndFeel::Colors::textOnAccent));
  commandModeButton->onClick = [this]() { setMode(Mode::Command); };
  addAndMakeVisible(*commandModeButton);

  aiModeButton = std::make_unique<juce::TextButton>("AI");
  aiModeButton->setClickingTogglesState(true);
  aiModeButton->setRadioGroupId(1);
  aiModeButton->setColour(
      juce::TextButton::buttonColourId,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
  aiModeButton->setColour(
      juce::TextButton::buttonOnColourId,
      juce::Colour(ZenithLookAndFeel::Colors::accentSecondary));
  aiModeButton->setColour(
      juce::TextButton::textColourOffId,
      juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
  aiModeButton->setColour(
      juce::TextButton::textColourOnId,
      juce::Colour(ZenithLookAndFeel::Colors::textOnAccent));
  aiModeButton->onClick = [this]() { setMode(Mode::AI); };
  addAndMakeVisible(*aiModeButton);

  clearButton = std::make_unique<juce::TextButton>("Clear");
  clearButton->setColour(
      juce::TextButton::buttonColourId,
      juce::Colour(ZenithLookAndFeel::Colors::backgroundLight));
  clearButton->setColour(
      juce::TextButton::textColourOffId,
      juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
  clearButton->onClick = [this]() { clearHistory(); };
  addAndMakeVisible(*clearButton);
#endif

  // Welcome message
  addMessage("╔════════════════════════════════════════════════════════════╗",
             false);
  addMessage("║           Wingman AI v1 - Command Console                 ║",
             false);
  addMessage("╚════════════════════════════════════════════════════════════╝",
             false);
  addMessage("", false);
  addMessage("Mode: [Command] | AI", false);
  addMessage("", false);
  addMessage("Available commands:", false);
  addMessage("  tracks                   → list all tracks", false);
  addMessage("  create audio <name>      → create audio track", false);
  addMessage("  create midi <name>       → create MIDI track", false);
  addMessage("  delete <trackId>         → delete track", false);
  addMessage("  rename <trackId> <name>  → rename track", false);
  addMessage("  clips <trackId>          → list clips on track", false);
  addMessage("  volume <trackId> <db>    → set track volume", false);
  addMessage("  pan <trackId> <value>    → set track pan (-1.0 to 1.0)", false);
  addMessage("  graph                    → show session graph", false);
  addMessage("", false);
  addMessage("Or use JSON format:", false);
  addMessage("  {\"command\":\"list_tracks\",\"params\":{}}", false);
  addMessage("", false);
  addMessage("Ready.", false);
  addMessage("", false);

  DBG("WingmanPanel: Initialized");
}

WingmanPanel::~WingmanPanel() {
  aiBridgeClient.removeChangeListener(this);
#ifndef ZENITH_USE_SKIA
  commandInput->removeListener(this);
#endif
}
#ifndef ZENITH_USE_SKIA
void WingmanPanel::paint(juce::Graphics &g) {
  using namespace zenith;
  auto bounds = getLocalBounds();

  // Background
  g.setColour(juce::Colour(ZenithLookAndFeel::Colors::backgroundPanel));
  g.fillRoundedRectangle(bounds.toFloat(), ZenithLookAndFeel::Radius::l);
  // Border
  g.setColour(juce::Colour(ZenithLookAndFeel::Colors::border));
  g.drawRoundedRectangle(bounds.toFloat(), ZenithLookAndFeel::Radius::l, 1.0f);

  // Draw title bar
  auto titleBar = bounds.removeFromTop(30);
  g.setColour(juce::Colour(ZenithLookAndFeel::Colors::backgroundDark));
  g.fillRoundedRectangle(titleBar.toFloat(),
                         ZenithLookAndFeel::Radius::l); // Top corners rounded

  // Title text
  g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textPrimary));
  g.setFont(
      ZenithLookAndFeel::Typography::getSmall().withStyle(juce::Font::bold));
  g.drawText("Wingman Console", titleBar.reduced(10, 0),
             juce::Justification::centredLeft);

  // Status indicator (mode badge)
  juce::String modeText = (currentMode == Mode::AI) ? "AI" : "CMD";

  juce::Colour modeColor =
      (currentMode == Mode::AI)
          ? juce::Colour(ZenithLookAndFeel::Colors::accentSecondary)
          : juce::Colour(ZenithLookAndFeel::Colors::accentPrimary);

  int badgeX = getWidth() - 60;
  int badgeY = titleBar.getY() + (titleBar.getHeight() - 18) / 2;
  juce::Rectangle<int> badgeBounds(badgeX, badgeY, 45, 18);

  g.setColour(modeColor.withAlpha(0.2f));
  g.fillRoundedRectangle(badgeBounds.toFloat(), ZenithLookAndFeel::Radius::s);

  g.setColour(modeColor);
  g.drawRoundedRectangle(badgeBounds.toFloat(), ZenithLookAndFeel::Radius::s,
                         1.0f);

  g.setColour(modeColor);
  g.setFont(ZenithLookAndFeel::Typography::getTiny());
  g.drawText(modeText, badgeBounds, juce::Justification::centred);

  // Draw input focus glow
  if (inputFocusAnim > 0.01f) {
    auto inputBounds = commandInput->getBounds().toFloat().expanded(2.0f);
    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary)
                    .withAlpha(inputFocusAnim * 0.3f));
    g.drawRoundedRectangle(inputBounds, ZenithLookAndFeel::Radius::s, 2.0f);
  }

  // Draw typing indicator when waiting for AI
  if (isWaitingForAI) {
    int dotSize = 6;
    int spacing = 10;
    int startX = 20;
    int dotY = getHeight() - 55;

    for (int i = 0; i < 3; ++i) {
      float phase = typingIndicatorPhase + (i * 0.33f);
      float bounce =
          std::sin(phase * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f;
      float alpha = 0.3f + (bounce * 0.6f);

      g.setColour(juce::Colour(ZenithLookAndFeel::Colors::accentPrimary)
                      .withAlpha(alpha));
      g.fillEllipse(static_cast<float>(startX + i * spacing),
                    static_cast<float>(dotY - bounce * 4),
                    static_cast<float>(dotSize), static_cast<float>(dotSize));
    }

    g.setColour(juce::Colour(ZenithLookAndFeel::Colors::textSecondary));
    g.setFont(ZenithLookAndFeel::Typography::getTiny());
    g.drawText("AI is thinking...", startX + 40, dotY - 5, 150, 15,
               juce::Justification::centredLeft);
  }
}
#endif

#ifdef ZENITH_USE_SKIA
#include <include/core/SkFont.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/core/SkTextBlob.h>

void WingmanPanel::paintSkia(SkCanvas &canvas,
                             const juce::Rectangle<int> &bounds) {
  // Convert bounds to SkRect
  SkRect rect = SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(),
                                 (float)bounds.getHeight());

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(0xFF1E1E1E); // backgroundPanel approx
  bgPaint.setAntiAlias(true);

  // Rounded corners
  float radius = 8.0f; // radiusL
  SkRRect rrect;
  rrect.setRectXY(rect, radius, radius);
  canvas.drawRRect(rrect, bgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setColor(0xFF333333); // border approx
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);
  canvas.drawRRect(rrect, borderPaint);

  // Title bar
  SkRect titleRect = SkRect::MakeXYWH(rect.fLeft, rect.fTop, rect.width(),
                                      32.0f); // 8px grid: 30→32
  SkPaint titleBgPaint;
  titleBgPaint.setColor(0xFF121212); // backgroundDark approx
  titleBgPaint.setAntiAlias(true);

  // Clip to rounded top corners
  canvas.save();
  canvas.clipRRect(rrect);
  canvas.drawRect(titleRect, titleBgPaint);
  canvas.restore();

  // Title text
  auto &typo = zenith::SkiaTheme::getInstance().getTypography();
  SkFont font;
  font.setSize(typo.body.size);
  font.setEmbolden(true);
  font.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint textPaint;
  textPaint.setColor(0xFFE0E0E0); // textPrimary
  textPaint.setAntiAlias(true);

  canvas.drawString("Wingman Console", rect.fLeft + 8, rect.fTop + 20,
                    font, // 8px grid: 10→8
                    textPaint);

  // Status Badge
  bool isAI = (currentMode == Mode::AI);
  SkColor modeColor = isAI ? 0xFF00D4AA : 0xFF00AAFF; // Teal vs Blue

  float badgeW = 48.0f;                    // 8px grid: 45→48
  float badgeH = 16.0f;                    // 8px grid: 18→16
  float badgeX = rect.fRight - badgeW - 8; // 8px grid: 10→8
  float badgeY = rect.fTop + (32 - badgeH) / 2;
  SkRect badgeRect = SkRect::MakeXYWH(badgeX, badgeY, badgeW, badgeH);

  SkPaint badgeBgPaint;
  badgeBgPaint.setColor(modeColor);
  badgeBgPaint.setAlpha(50); // 0.2 alpha
  badgeBgPaint.setAntiAlias(true);

  SkRRect badgeRRect;
  badgeRRect.setRectXY(badgeRect, 4.0f, 4.0f); // radiusS
  canvas.drawRRect(badgeRRect, badgeBgPaint);

  SkPaint badgeBorderPaint;
  badgeBorderPaint.setColor(modeColor);
  badgeBorderPaint.setStyle(SkPaint::kStroke_Style);
  badgeBorderPaint.setStrokeWidth(1.0f);
  badgeBorderPaint.setAntiAlias(true);
  canvas.drawRRect(badgeRRect, badgeBorderPaint);

  SkFont badgeFont;
  badgeFont.setSize(typo.body.size);
  badgeFont.setEmbolden(true);
  badgeFont.setEdging(SkFont::Edging::kAntiAlias);

  SkPaint badgeTextPaint;
  badgeTextPaint.setColor(modeColor);
  badgeTextPaint.setAntiAlias(true);

  const char *modeStr = isAI ? "AI" : "CMD";
  // Center text (approx)
  float textW =
      badgeFont.measureText(modeStr, strlen(modeStr), SkTextEncoding::kUTF8);
  canvas.drawString(modeStr, badgeX + (badgeW - textW) / 2,
                    badgeY + 12, // 8px grid: 13→12
                    badgeFont, badgeTextPaint);

  // Typing indicator
  if (isWaitingForAI) {
    int dotSize = 8;               // 8px grid: 6→8
    int spacing = 8;               // 8px grid: 10→8
    int startX = 16;               // 8px grid: 20→16
    int dotY = rect.height() - 56; // 8px grid: 55→56

    for (int i = 0; i < 3; ++i) {
      float phase = typingIndicatorPhase + (i * 0.33f);
      float bounce = std::sin(phase * 6.28f) * 0.5f + 0.5f;
      float alpha = 0.3f + (bounce * 0.6f);

      SkPaint dotPaint;
      dotPaint.setColor(0xFF00D4AA); // accentPrimary
      dotPaint.setAlpha((int)(alpha * 255));
      dotPaint.setAntiAlias(true);

      canvas.drawCircle(static_cast<float>(startX + i * spacing) +
                            dotSize / 2.0f,
                        static_cast<float>(dotY) - bounce * 4 + dotSize / 2.0f,
                        dotSize / 2.0f, dotPaint);
    }

    SkFont tinyFont;
    tinyFont.setSize(typo.small.size);
    tinyFont.setEdging(SkFont::Edging::kAntiAlias);

    SkPaint tinyTextPaint;
    tinyTextPaint.setColor(0xFFAAAAAA); // textSecondary
    tinyTextPaint.setAntiAlias(true);

    canvas.drawString("AI is thinking...", startX + 40, dotY - 4 + 12,
                      tinyFont, // 8px grid: -5→-4
                      tinyTextPaint);
  }
}
#endif

void WingmanPanel::resized() {
  auto bounds = getLocalBounds();

  // Title bar (32px)  // 8px grid: 30→32
  bounds.removeFromTop(32);

  // Margin  // 8px grid: 10→8
  bounds.reduce(8, 8);

  // Top row: Mode toggle buttons + Clear button  // 8px grid: 25→24
  auto topRow = bounds.removeFromTop(24);

  // Mode toggle buttons (left side, 160px total)
  auto modeBounds = topRow.removeFromLeft(160);
  commandModeButton->setBounds(modeBounds.removeFromLeft(80));
  aiModeButton->setBounds(modeBounds);

  // Clear button (right side, 80px)
  clearButton->setBounds(topRow.removeFromRight(80));

  // Space below buttons  // 8px grid: 10→8
  bounds.removeFromTop(8);

  // Command input (bottom, 32px)  // 8px grid: 30→32
  auto inputBounds = bounds.removeFromBottom(32);
  commandInput->setBounds(inputBounds);

  // Space above input  // 8px grid: 10→8
  bounds.removeFromBottom(8);

  // History display (remaining space)
  historyDisplay->setBounds(bounds);
}

void WingmanPanel::timerCallback() {
  // Smooth focus animations (60Hz, 0.15 speed)
  bool needsRepaint = false;

  // Input focus animation
  float inputTarget = inputHasFocus ? 1.0f : 0.0f;
  if (std::abs(inputFocusAnim - inputTarget) > 0.01f) {
    inputFocusAnim += (inputTarget - inputFocusAnim) * 0.15f;
    needsRepaint = true;
  }

  // Typing indicator animation
  if (isWaitingForAI) {
    typingIndicatorPhase += 0.03f;
    if (typingIndicatorPhase > 1.0f)
      typingIndicatorPhase -= 1.0f;
    needsRepaint = true;
  }

  if (needsRepaint)
    repaint();
}

//==============================================================================
// Command Execution
//==============================================================================

void WingmanPanel::executeCommand(const juce::String &input) {
  if (input.trim().isEmpty())
    return;

  // Check if we're in pending batch state
  if (hasPendingBatch) {
    juce::String lowerInput = input.trim().toLowerCase();

    if (lowerInput == "yes" || lowerInput == "y") {
      addMessage("> yes", true);
      executePendingBatch();
      return;
    } else if (lowerInput == "no" || lowerInput == "n") {
      addMessage("> no", true);
      cancelPendingBatch();
      return;
    } else {
      addMessage("> " + input, true);
      addMessage("Please type 'yes' to apply the batch or 'no' to cancel.",
                 false);
      addMessage("", false);
      return;
    }
  }

  // Add user input to history
  addMessage("> " + input, true);

  // Route based on mode
  if (currentMode == Mode::AI) {
    // Play whoosh sound for AI command
    zenith::AudioFeedback::getInstance().playSound(
        zenith::AudioFeedback::Whoosh, 0.25f);

    // AI mode: send natural language to AI bridge
    executeAIRequest(input);
  } else {
    // Command mode: parse and execute locally
    juce::String jsonCommand;

    if (input.trimStart().startsWith("{")) {
      // Already JSON
      jsonCommand = input;
    } else {
      // Try shorthand parser
      jsonCommand = parseShorthand(input);

      if (jsonCommand.isEmpty()) {
        addMessage("ERROR: Could not parse command. Use 'help' for syntax.",
                   false);
        addMessage("", false);
        return;
      }
    }

    // Execute command
    juce::String response = commandAPI.executeCommandString(jsonCommand);

    // Parse response to check for success/error
    juce::var responseVar;
    auto parseResult = juce::JSON::parse(response, responseVar);

    if (parseResult.failed()) {
      addMessage("ERROR: Failed to parse response: " +
                     parseResult.getErrorMessage(),
                 false);
      addMessage(response, false);
    } else {
      bool success = responseVar.getProperty("success", false);

      if (success) {
        // Pretty-print result
        juce::var result = responseVar.getProperty("result", juce::var());
        juce::String resultString = juce::JSON::toString(result, true, 2);
        addMessage("✓ Success:", false);
        addMessage(resultString, false);
      } else {
        // Show error
        juce::String error =
            responseVar.getProperty("error", "Unknown error").toString();
        addMessage("✗ Error: " + error, false);
      }
    }

    addMessage("", false); // Blank line for spacing
  }
}

void WingmanPanel::clearHistory() {
  messageHistory.clear();
#ifdef ZENITH_USE_SKIA
  historyDisplay->setText("");
#else
  historyDisplay->clear();
#endif

  addMessage("History cleared.", false);
  addMessage("", false);
}

//==============================================================================
// TextEditor::Listener (JUCE only)
//==============================================================================

#ifndef ZENITH_USE_SKIA
void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor &editor) {
  if (&editor == commandInput.get()) {
    juce::String input = commandInput->getText();
    commandInput->clear();

    executeCommand(input);
  }
}
#endif

//==============================================================================
// Shorthand Parser
//==============================================================================

juce::String WingmanPanel::parseShorthand(const juce::String &input) {
  juce::StringArray tokens;
  tokens.addTokens(input, true); // true = ignore empty tokens

  if (tokens.isEmpty())
    return {};

  juce::String command = tokens[0].toLowerCase();

  // Simple shorthand commands
  if (command == "tracks") {
    return "{\"command\":\"list_tracks\",\"params\":{}}";
  } else if (command == "graph") {
    return "{\"command\":\"get_session_graph\",\"params\":{}}";
  } else if (command == "undo" || command == "u") {
    return "{\"command\":\"undo\",\"params\":{}}";
  } else if (command == "redo" || command == "r") {
    return "{\"command\":\"redo\",\"params\":{}}";
  } else if (command == "history" || command == "h") {
    return "{\"command\":\"history\",\"params\":{}}";
  } else if (command == "help") {
    // Special case: show help (no actual command)
    addMessage("Available shorthand commands:", false);
    addMessage("  tracks                     → list all tracks", false);
    addMessage("  create <audio|midi> <name> → create track", false);
    addMessage("  delete <trackId>           → delete track", false);
    addMessage("  rename <trackId> <name>    → rename track", false);
    addMessage("  clips <trackId>            → list clips", false);
    addMessage("  volume <trackId> <db>      → set volume", false);
    addMessage("  pan <trackId> <value>      → set pan", false);
    addMessage("  graph                      → session graph", false);
    addMessage("  undo (or u)                → undo last action", false);
    addMessage("  redo (or r)                → redo last action", false);
    addMessage("  history (or h)             → show undo/redo status", false);
    return {}; // Don't execute
  } else if (command == "create" && tokens.size() >= 2) {
    juce::String type = tokens[1].toLowerCase();
    juce::String name =
        tokens.size() >= 3 ? tokens.joinIntoString(" ", 2, -1) : "New Track";

    return "{\"command\":\"create_track\",\"params\":{\"type\":\"" + type +
           "\",\"name\":\"" + name + "\"}}";
  } else if (command == "delete" && tokens.size() >= 2) {
    juce::String trackId = tokens[1];
    return "{\"command\":\"delete_track\",\"params\":{\"trackId\":\"" +
           trackId + "\"}}";
  } else if (command == "rename" && tokens.size() >= 3) {
    juce::String trackId = tokens[1];
    juce::String name = tokens.joinIntoString(" ", 2, -1);
    return "{\"command\":\"rename_track\",\"params\":{\"trackId\":\"" +
           trackId + "\",\"name\":\"" + name + "\"}}";
  } else if (command == "clips" && tokens.size() >= 2) {
    juce::String trackId = tokens[1];
    return "{\"command\":\"list_clips\",\"params\":{\"trackId\":\"" + trackId +
           "\"}}";
  } else if (command == "volume" && tokens.size() >= 3) {
    juce::String trackId = tokens[1];
    double volumeDb = tokens[2].getDoubleValue();
    return "{\"command\":\"set_track_volume\",\"params\":{\"trackId\":\"" +
           trackId + "\",\"volumeDb\":" + juce::String(volumeDb) + "}}";
  } else if (command == "pan" && tokens.size() >= 3) {
    juce::String trackId = tokens[1];
    double pan = tokens[2].getDoubleValue();
    return "{\"command\":\"set_track_pan\",\"params\":{\"trackId\":\"" +
           trackId + "\",\"pan\":" + juce::String(pan) + "}}";
  }

  // Unknown shorthand
  return {};
}

//==============================================================================
// UI Helpers
//==============================================================================

void WingmanPanel::addMessage(const juce::String &message, bool isUserInput) {
  juce::ignoreUnused(isUserInput);

  messageHistory.add(message);

  // Rebuild history display
  historyDisplay->setText(messageHistory.joinIntoString("\n"));

  // Scroll to bottom
  scrollHistoryToBottom();
}

void WingmanPanel::scrollHistoryToBottom() {
#ifndef ZENITH_USE_SKIA
  historyDisplay->moveCaretToEnd();
  // scrollToMakeSureCursorIsVisible() is protected in JUCE 8
  // moveCaretToEnd() already scrolls to make caret visible
#endif
  // Skia version auto-scrolls with setText()
}

//==============================================================================
// Mode Management
//==============================================================================

void WingmanPanel::setMode(Mode newMode) {
  if (currentMode == newMode)
    return;

  currentMode = newMode;

  // Cancel any pending batch when switching modes
  if (hasPendingBatch) {
    cancelPendingBatch();
  }

  // Update UI
  if (currentMode == Mode::Command) {
#ifndef ZENITH_USE_SKIA
    commandInput->setTextToShowWhenEmpty("Enter command (JSON or shorthand)...",
                                         juce::Colours::grey);
#else
    commandInput->setPlaceholder("Enter command (JSON or shorthand)...");
#endif
    addMessage("──────────────────────────────────────────────────────────",
               false);
    addMessage("Mode switched to: COMMAND", false);
    addMessage("Use JSON or shorthand commands.", false);
    addMessage("", false);
  } else // Mode::AI
  {
#ifndef ZENITH_USE_SKIA
    commandInput->setTextToShowWhenEmpty("Enter natural language request...",
                                         juce::Colours::grey);
#else
    commandInput->setPlaceholder("Enter natural language request...");
#endif
    addMessage("──────────────────────────────────────────────────────────",
               false);
    addMessage("Mode switched to: AI", false);
    addMessage(
        "Type natural language requests (e.g., 'make a 4 bar drum loop').",
        false);
    addMessage("Server: " + aiBridgeClient.getServerUrl(), false);
    addMessage("", false);
  }

  DBG("WingmanPanel: Mode changed to " +
      juce::String(currentMode == Mode::Command ? "Command" : "AI"));
}

//==============================================================================
// AI Mode Handlers
//==============================================================================

void WingmanPanel::executeAIRequest(const juce::String &naturalLanguage) {
  addMessage("(Sending to AI bridge server...)", false);

  // Get session graph by executing get_session_graph command
  juce::String graphResponse = commandAPI.executeCommandString(
      "{\"command\":\"get_session_graph\",\"params\":{}}");

  juce::var graphVar;
  auto parseResult = juce::JSON::parse(graphResponse, graphVar);

  if (parseResult.failed()) {
    addMessage("✗ Error: Failed to get session graph", false);
    addMessage("", false);
    return;
  }

  juce::var sessionGraph = graphVar.getProperty("result", juce::var());

  // Send request to AI bridge
  aiBridgeClient.sendRequest(naturalLanguage, sessionGraph, "zenith-core");

  // Enable typing indicator
  isWaitingForAI = true;
  typingIndicatorPhase = 0.0f;

  addMessage("(Waiting for AI response...)", false);
  addMessage("", false);
}

void WingmanPanel::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &aiBridgeClient) {
    // AI response available
    handleAIResponse();
  }
}

void WingmanPanel::handleAIResponse() {
  // Disable typing indicator
  isWaitingForAI = false;

  while (aiBridgeClient.hasPendingResponse()) {
    auto response = aiBridgeClient.popNextResponse();

    if (response.status == "error") {
      addMessage("✗ AI Error: " + response.errorMessage, false);
      addMessage("", false);
    } else if (response.status == "ok") {
      // Show AI's thought process and proposed commands
      showAIPlan(response.thought, response.commands);
    } else {
      addMessage("✗ Unknown AI response status: " + response.status, false);
      addMessage("", false);
    }
  }
}

void WingmanPanel::showAIPlan(const juce::String &thought,
                              const juce::Array<juce::var> &commands) {
  // Display AI's reasoning
  if (thought.isNotEmpty()) {
    addMessage("──────────────────────────────────────────────────────────",
               false);
    addMessage("AI Plan:", false);
    addMessage(thought, false);
    addMessage("", false);
  }

  // Display proposed commands
  if (commands.isEmpty()) {
    addMessage("✗ AI did not generate any commands.", false);
    addMessage("", false);
    return;
  }

  addMessage("Proposed commands (" + juce::String(commands.size()) + "):",
             false);

  for (int i = 0; i < commands.size(); ++i) {
    juce::String cmdStr = juce::JSON::toString(commands[i], false);
    addMessage("  " + juce::String(i + 1) + ". " + cmdStr, false);
  }

  addMessage("", false);

  // Enter pending batch state
  hasPendingBatch = true;
  pendingCommands = commands;

  addMessage("Type 'yes' to apply these commands, or 'no' to cancel.", false);
  addMessage("", false);
}

void WingmanPanel::executePendingBatch() {
  if (!hasPendingBatch || pendingCommands.isEmpty()) {
    addMessage("✗ No pending batch to execute.", false);
    addMessage("", false);
    hasPendingBatch = false;
    return;
  }

  addMessage("Executing batch (" + juce::String(pendingCommands.size()) +
                 " commands)...",
             false);

  // Execute batch via CommandAPI (single undo transaction)
  juce::var batchResponse =
      commandAPI.executeBatch(pendingCommands, "Wingman AI batch");

  // Clear pending state
  int commandCount = pendingCommands.size();
  hasPendingBatch = false;
  pendingCommands.clear();

  // Report results
  bool success = batchResponse.getProperty("success", false);

  if (!success) {
    // Batch failed
    juce::String error =
        batchResponse.getProperty("error", "Unknown error").toString();
    int failedIndex = batchResponse.getProperty("failedIndex", -1);
    int successCount = batchResponse.getProperty("successCount", 0);

    addMessage("✗ Batch failed at command #" + juce::String(failedIndex + 1) +
                   ": " + error,
               false);
    addMessage("  (" + juce::String(successCount) +
                   " commands succeeded before failure)",
               false);
  } else {
    // Batch succeeded
    int count = batchResponse.getProperty("result", juce::var())
                    .getProperty("count", commandCount);
    addMessage("✓ Batch completed successfully (" + juce::String(count) +
                   " commands applied).",
               false);
  }

  addMessage("", false);

  DBG("WingmanPanel: Batch execution completed (status: " +
      juce::String(success ? "success" : "failed") + ")");
}

void WingmanPanel::cancelPendingBatch() {
  if (!hasPendingBatch)
    return;

  int commandCount = pendingCommands.size();

  hasPendingBatch = false;
  pendingCommands.clear();

  addMessage("Batch cancelled (" + juce::String(commandCount) +
                 " commands discarded).",
             false);
  addMessage("", false);

  DBG("WingmanPanel: Batch cancelled");
}
