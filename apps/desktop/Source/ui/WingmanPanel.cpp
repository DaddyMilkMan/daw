/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Grok Integration)
    Author:  Marcus Williams (UX Team)

    Complete Wingman Panel Implementation

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../SimpleLogger.h"
#include "SettingsComponent.h"
#include "ZenithLookAndFeel.h"

//==============================================================================
namespace {
namespace WingmanStrings {
// Speakers
const juce::String kSpeakerWingman = "Wingman";
const juce::String kSpeakerSystem = "System";
const juce::String kSpeakerYou = "You";

// Status Messages
const juce::String kStatusSearching = "Searching samples...";
const juce::String kStatusDownloading = "Downloading...";
const juce::String kStatusReady = "Ready";
const juce::String kStatusError = "Error";
const juce::String kStatusProcessing = "Processing...";

// Chat Messages
const juce::String kMsgSearchFailed = "❌ Search failed. Please try again.";
const juce::String kMsgSampleHunterUnavailable =
    "❌ Sample Hunter unavailable.";
const juce::String kMsgNoResults =
    "⚠️ No search results available. Use 'find <query>' first.";
const juce::String kMsgSpecifyNumber =
    "⚠️ Please specify a number: 'import 1', 'import 2', etc.";
} // namespace WingmanStrings
} // namespace

//==============================================================================
namespace zenith {
WingmanPanel::WingmanPanel(CommandAPI &api, AIBridgeClient &client,
                           Engine &engine)
    : commandAPI(api), aiBridgeClient(client), engine_(engine) {
  logToFile("WingmanPanel: Constructor started");
  // Create Grok controller
  logToFile("WingmanPanel: Creating GrokDAWController...");
  grokController = std::make_unique<GrokDAWController>(commandAPI);

  //==========================================================================
  // Conversation Display
  conversationDisplay = std::make_unique<juce::TextEditor>("Conversation");
  conversationDisplay->setMultiLine(true);
  conversationDisplay->setReadOnly(true);
  conversationDisplay->setScrollbarsShown(true);
  conversationDisplay->setCaretVisible(false);
  conversationDisplay->setPopupMenuEnabled(true);
  conversationDisplay->setColour(juce::TextEditor::backgroundColourId,
                                 ZenithLookAndFeel::Colors::panel);
  conversationDisplay->setColour(juce::TextEditor::textColourId,
                                 ZenithLookAndFeel::Colors::textPrimary);
  conversationDisplay->setColour(juce::TextEditor::outlineColourId,
                                 ZenithLookAndFeel::Colors::border);
  conversationDisplay->setFont(ZenithLookAndFeel::getFontMedium());
  addAndMakeVisible(conversationDisplay.get());

  // Welcome message
  appendToConversation(
      "Wingman",
      "Hello! I'm Wingman, your AI assistant. I can control the DAW, generate "
      "presets, and help you create music. What would you like to do?");

  //==========================================================================
  // Input Field
  inputField = std::make_unique<juce::TextEditor>("Input");
  inputField->setMultiLine(false);
  inputField->setReturnKeyStartsNewLine(false);
  inputField->setPopupMenuEnabled(true);
  inputField->setColour(juce::TextEditor::backgroundColourId,
                        ZenithLookAndFeel::Colors::backgroundPanel);
  inputField->setColour(juce::TextEditor::textColourId,
                        ZenithLookAndFeel::Colors::textPrimary);
  inputField->setColour(juce::TextEditor::outlineColourId,
                        ZenithLookAndFeel::Colors::accent);
  inputField->setFont(ZenithLookAndFeel::getFontMedium());
  inputField->setTextToShowWhenEmpty("Ask Wingman anything...",
                                     ZenithLookAndFeel::Colors::textSecondary);
  inputField->addListener(this);
  addAndMakeVisible(inputField.get());

  //==========================================================================
  // Send Button
  sendButton = std::make_unique<juce::TextButton>("Send");
  sendButton->setButtonText("Send");
  sendButton->setColour(juce::TextButton::buttonColourId,
                        ZenithLookAndFeel::Colors::accent);
  sendButton->setColour(juce::TextButton::textColourOffId,
                        ZenithLookAndFeel::Colors::textPrimary);
  sendButton->addListener(this);
  addAndMakeVisible(sendButton.get());

  //==========================================================================
  // Mode Selector
  modeLabel = std::make_unique<juce::Label>("ModeLabel", "Mode:");
  modeLabel->setColour(juce::Label::textColourId,
                       ZenithLookAndFeel::Colors::textPrimary);
  modeLabel->setFont(ZenithLookAndFeel::getFontMedium());
  addAndMakeVisible(modeLabel.get());

  modeSelector = std::make_unique<juce::ComboBox>("Mode");
  modeSelector->addItem("⚡ Fast (Quick responses)", 1);
  modeSelector->addItem("🧠 Thinking (Deep analysis)", 2);
  modeSelector->setSelectedId(1); // Default to Fast
  modeSelector->setColour(juce::ComboBox::backgroundColourId,
                          ZenithLookAndFeel::Colors::backgroundPanel);
  modeSelector->setColour(juce::ComboBox::textColourId,
                          ZenithLookAndFeel::Colors::textPrimary);
  modeSelector->setColour(juce::ComboBox::outlineColourId,
                          ZenithLookAndFeel::Colors::accent);
  modeSelector->onChange = [this]() { updateModeFromSelector(); };
  addAndMakeVisible(modeSelector.get());

  //==========================================================================
  // Status Label
  statusLabel = std::make_unique<juce::Label>("Status", "Ready");
  statusLabel->setColour(juce::Label::textColourId,
                         juce::Colours::green); // Keep green for status
  statusLabel->setFont(ZenithLookAndFeel::getFontSmall());
  statusLabel->setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(statusLabel.get());

  //==========================================================================
  // Clear Button
  clearButton = std::make_unique<juce::TextButton>("Clear");
  clearButton->setButtonText("Clear");
  clearButton->setColour(juce::TextButton::buttonColourId,
                         ZenithLookAndFeel::Colors::panel);
  clearButton->setColour(juce::TextButton::textColourOffId,
                         ZenithLookAndFeel::Colors::textSecondary);
  clearButton->addListener(this);
  addAndMakeVisible(clearButton.get());

  //==========================================================================
  // Settings Button
  settingsButton = std::make_unique<juce::TextButton>("Settings");
  settingsButton->setButtonText("⚙");
  settingsButton->setColour(juce::TextButton::buttonColourId,
                            ZenithLookAndFeel::Colors::panel);
  settingsButton->setColour(juce::TextButton::textColourOffId,
                            ZenithLookAndFeel::Colors::textSecondary);
  settingsButton->addListener(this);
  addAndMakeVisible(settingsButton.get());

  // Update status based on Grok readiness (Moved to END of constructor)
  logToFile("WingmanPanel: Initializing Grok...");
  initializeGrok();
  logToFile("WingmanPanel: Grok initialized (or failed gracefully)");

  // Register as SampleHunterAgent listener
  if (auto *sampleHunter = engine_.getSampleHunterAgent()) {
    sampleHunter->addListener(this);
    logToFile("WingmanPanel: Registered as SampleHunterAgent listener");
  }
}

WingmanPanel::~WingmanPanel() {
  inputField->removeListener(this);

  // Unregister from SampleHunterAgent
  if (auto *sampleHunter = engine_.getSampleHunterAgent()) {
    sampleHunter->removeListener(this);
  }
}

//==============================================================================
void WingmanPanel::paint(juce::Graphics &g) {
  // Background
  g.fillAll(ZenithLookAndFeel::Colors::background);

  // Header
  g.setColour(ZenithLookAndFeel::Colors::panel);
  g.fillRect(0, 0, getWidth(), 40);

  // Title
  g.setColour(ZenithLookAndFeel::Colors::accent);
  g.setFont(ZenithLookAndFeel::getFontLarge().boldened());
  g.drawText("Wingman AI Assistant", 10, 0, getWidth() - 20, 40,
             juce::Justification::centredLeft);
}

void WingmanPanel::resized() {
  auto bounds = getLocalBounds();

  // Header area (40px)
  auto headerArea = bounds.removeFromTop(40);
  settingsButton->setBounds(headerArea.removeFromRight(40).reduced(5));

  // Status bar (30px)
  auto statusArea = bounds.removeFromBottom(30);
  statusLabel->setBounds(statusArea.reduced(5));

  // Input area (80px)
  auto inputArea = bounds.removeFromBottom(80);
  inputArea.reduce(10, 10);

  // Mode selector row
  auto modeRow = inputArea.removeFromTop(30);
  modeLabel->setBounds(modeRow.removeFromLeft(50));
  modeSelector->setBounds(modeRow.removeFromLeft(200).reduced(0, 2));
  modeRow.removeFromLeft(10);
  clearButton->setBounds(modeRow.removeFromLeft(80).reduced(0, 2));

  inputArea.removeFromTop(10);

  // Input field and send button
  auto inputRow = inputArea;
  sendButton->setBounds(inputRow.removeFromRight(80).reduced(0, 2));
  inputRow.removeFromRight(10);
  inputField->setBounds(inputRow.reduced(0, 2));

  // Conversation display (remaining space)
  conversationDisplay->setBounds(bounds.reduced(10));
}

//==============================================================================
bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
  bool success = grokController->initialize(apiKey);

  if (success) {
    setStatus("Grok Ready", juce::Colour(0xff00ff00));
  } else {
    setStatus("Grok initialization failed", juce::Colour(0xffff0000));
  }

  return success;
}

bool WingmanPanel::isGrokReady() const { return grokController->isReady(); }

//==============================================================================
void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor &editor) {
  if (&editor == inputField.get()) {
    sendCommand();
  }
}

void WingmanPanel::buttonClicked(juce::Button *button) {
  if (button == sendButton.get()) {
    sendCommand();
  } else if (button == clearButton.get()) {
    conversationDisplay->clear();
    grokController->clearHistory();
    appendToConversation("Wingman",
                         "Conversation cleared. How can I help you?");
  } else if (button == settingsButton.get()) {
    showSettings();
  }
}

//==============================================================================
void WingmanPanel::sendCommand() {
  if (isProcessing)
    return;

  auto command = inputField->getText().trim();
  if (command.isEmpty())
    return;

  // Clear input first
  inputField->clear();

  // Add to conversation
  appendToConversation("You", command);

  //==========================================================================
  // Phase 1: Check for import command (e.g., "import 1", "import 2")
  //==========================================================================
  if (handleImportCommand(command)) {
    return; // Handled
  }

  //==========================================================================
  // Phase 2: Check for sample search intent
  //==========================================================================
  juce::String sampleQuery;
  if (detectSampleSearchIntent(command, sampleQuery)) {
    auto *sampleHunter = engine_.getSampleHunterAgent();
    if (sampleHunter) {
      isSampleSearchActive_ = true;
      lastSearchQuery_ = sampleQuery;
      lastSearchResults_.clear();

      appendToConversation(WingmanStrings::kSpeakerWingman,
                           "🔍 Searching Freesound for \"" + sampleQuery +
                               "\"...");
      setStatus(WingmanStrings::kStatusSearching, juce::Colour(0xffffff00));

      sampleHunter->hunt(sampleQuery);
      return; // Don't send to Grok
    } else {
      appendToConversation(
          WingmanStrings::kSpeakerWingman,
          "Sample Hunter is not available. Please check engine configuration.");
      return;
    }
  }

  //==========================================================================
  // Phase 3: Standard Grok processing
  //==========================================================================
  if (!isGrokReady()) {
    appendToConversation(WingmanStrings::kSpeakerSystem,
                         "Grok API key not configured. Click ⚙ to set it up.");
    return;
  }

  // Set processing state
  isProcessing = true;
  setStatus(WingmanStrings::kStatusProcessing, juce::Colour(0xffffff00));
  sendButton->setEnabled(false);

  // Send to Grok
  grokController->executeCommand(
      command, currentMode,
      [this](juce::String response) {
        // Success
        juce::MessageManager::callAsync([this, response]() {
          appendToConversation(WingmanStrings::kSpeakerWingman, response);
          setStatus(WingmanStrings::kStatusReady, juce::Colour(0xff00ff00));
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String error) {
        // Error
        juce::MessageManager::callAsync([this, error]() {
          appendToConversation("Error", error);
          setStatus("Error", juce::Colour(0xffff0000));
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String status) {
        // Progress
        juce::MessageManager::callAsync(
            [this, status]() { setStatus(status, juce::Colour(0xffffff00)); });
      });
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &message) {
  juce::String timestamp =
      juce::Time::getCurrentTime().toString(false, true, false, true);
  juce::String entry =
      "[" + timestamp + "] " + speaker + ": " + message + "\n\n";

  conversationDisplay->moveCaretToEnd();
  conversationDisplay->insertTextAtCaret(entry);
  conversationDisplay->moveCaretToEnd();
}

void WingmanPanel::setStatus(const juce::String &status, juce::Colour colour) {
  statusLabel->setText(status, juce::dontSendNotification);
  statusLabel->setColour(juce::Label::textColourId, colour);
}

void WingmanPanel::updateModeFromSelector() {
  int selectedId = modeSelector->getSelectedId();
  currentMode = (selectedId == 2) ? GrokMode::Thinking : GrokMode::Fast;

  juce::String modeName = (currentMode == GrokMode::Fast) ? "Fast" : "Thinking";
  setStatus("Mode: " + modeName, juce::Colour(0xff00aaff));
}

void WingmanPanel::showSettings() {
  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(new SettingsComponent(engine_));
  options.content->setSize(600, 500);
  options.dialogTitle = "Zenith DAW Settings";
  options.dialogBackgroundColour = ZenithLookAndFeel::Colors::background;
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;

  options.launchAsync();
}

//==============================================================================
// Sample Hunter Logic
//==============================================================================

juce::String WingmanPanel::cleanQueryFiller(const juce::String &rawQuery) {
  // Strip common filler words from natural language queries
  // "me a punchy snare" -> "punchy snare"
  // "for some 808 kicks" -> "808 kicks"
  juce::String query = rawQuery;

  // Filler prefixes to strip (order matters - longer first)
  // Optimization: Static const to avoid reallocation
  static const char *const fillers[] = {
      "me a ",   "me some ", "me an ", "for a ", "for some ",
      "for an ", "for ",     "some ",  "a ",     "an "};

  juce::String lower = query.toLowerCase();
  for (const auto *filler : fillers) {
    if (lower.startsWith(filler)) {
      query = query.substring(juce::String(filler).length());
      break; // Only strip one prefix
    }
  }

  // Also strip trailing "sample", "samples", "sound", "sounds"
  lower = query.toLowerCase();
  static const char *const suffixes[] = {" samples", " sample", " sounds",
                                         " sound"};
  for (const auto *suffix : suffixes) {
    if (lower.endsWith(suffix)) {
      query = query.dropLastCharacters(juce::String(suffix).length());
      break;
    }
  }

  return query.trim();
}

bool WingmanPanel::detectSampleSearchIntent(const juce::String &message,
                                            juce::String &outQuery) {
  juce::String lower = message.toLowerCase();

  // Pattern matching with extraction points
  // Format: {prefix, extraction_start_offset}
  struct Pattern {
    const char *prefix;
    int offset;
  };
  const Pattern patterns[] = {
      {"find me ", 8},      {"find ", 5},          {"search for ", 11},
      {"search ", 7},       {"get me ", 7},        {"get ", 4},
      {"hunt for ", 9},     {"hunt ", 5},          {"i need a ", 9},
      {"i need ", 7},       {"can you find ", 13}, {"give me ", 8},
      {"looking for ", 12}, {"look for ", 9}};

  for (const auto &p : patterns) {
    if (lower.startsWith(p.prefix)) {
      juce::String rawQuery = message.substring(p.offset).trim();
      outQuery = cleanQueryFiller(rawQuery);
      return outQuery.isNotEmpty();
    }
  }

  return false;
}

bool WingmanPanel::handleImportCommand(const juce::String &message) {
  juce::String lower = message.toLowerCase();

  // Check if this looks like an import command
  if (!lower.startsWith("import ") && !lower.startsWith("load ") &&
      !lower.startsWith("use ")) {
    return false;
  }

  // Check if we have results to import from
  if (!isSampleSearchActive_ || lastSearchResults_.empty()) {
    appendToConversation(WingmanStrings::kSpeakerWingman,
                         WingmanStrings::kMsgNoResults);
    return true; // Handled (with error)
  }

  int spacePos = message.indexOf(" ");
  if (spacePos <= 0) {
    appendToConversation(WingmanStrings::kSpeakerWingman,
                         WingmanStrings::kMsgSpecifyNumber);
    return true;
  }

  juce::String indexStr = message.substring(spacePos + 1).trim();
  int index = indexStr.getIntValue();

  // Check for non-numeric input (getIntValue returns 0 for non-numbers)
  if (index == 0 && !indexStr.startsWith("0")) {
    appendToConversation(WingmanStrings::kSpeakerWingman,
                         "⚠️ Invalid input '" + indexStr +
                             "'. Please use a number (1-" +
                             juce::String(lastSearchResults_.size()) + ").");
    return true;
  }

  // Check bounds
  if (index < 1 || index > static_cast<int>(lastSearchResults_.size())) {
    appendToConversation(WingmanStrings::kSpeakerWingman,
                         "⚠️ Invalid selection '" + juce::String(index) +
                             "'. Choose 1-" +
                             juce::String(lastSearchResults_.size()) + ").");
    return true;
  }

  // Valid index - proceed with download
  if (auto *hunter = engine_.getSampleHunterAgent()) {
    hunter->downloadResult(index - 1); // 0-based
    appendToConversation(WingmanStrings::kSpeakerWingman,
                         "⬇️ Downloading '" +
                             lastSearchResults_[index - 1].title + "'...");
    setStatus(WingmanStrings::kStatusDownloading, juce::Colour(0xffffff00));
    return true;
  }

  appendToConversation(WingmanStrings::kSpeakerWingman,
                       WingmanStrings::kMsgSampleHunterUnavailable);
  return true;
}

void WingmanPanel::displaySearchResults(
    const std::vector<ai::FoundSample> &results) {
  if (results.empty()) {
    appendToConversation("Wingman",
                         "No samples found for \"" + lastSearchQuery_ + "\".");
    return;
  }

  juce::String list = "Found " + juce::String(results.size()) + " samples:\n";
  for (size_t i = 0; i < results.size(); ++i) {
    list += juce::String(i + 1) + ". " + results[i].title + " (" +
            juce::String(results[i].duration, 1) + "s)\n";
  }
  list += "\nType 'import <number>' to add to a new track.";

  appendToConversation("Wingman", list);
}

//==============================================================================
// SampleHunterAgent::Listener
//==============================================================================

void WingmanPanel::sampleDownloaded(const ai::FoundSample &sample) {
  // Safe async callback using Component::SafePointer
  juce::Component::SafePointer<WingmanPanel> safeThis(this);

  juce::MessageManager::callAsync([safeThis, sample]() {
    if (safeThis) {
      safeThis->appendToConversation(WingmanStrings::kSpeakerWingman,
                                     "✅ Downloaded '" + sample.title +
                                         "'. Analyzing...");
    }
  });
}

void WingmanPanel::sampleAnalyzed(const ai::FoundSample &sample) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);

  juce::MessageManager::callAsync([safeThis, sample]() {
    if (safeThis) {
      juce::String details = "📊 Analysis: ";
      if (sample.detectedBpm > 0)
        details += juce::String(sample.detectedBpm, 1) + " BPM, ";
      if (sample.detectedKey.isNotEmpty())
        details += "Key: " + sample.detectedKey;

      if (sample.detectedBpm > 0 || sample.detectedKey.isNotEmpty()) {
        safeThis->appendToConversation(WingmanStrings::kSpeakerWingman,
                                       details);
      }
    }
  });
}

void WingmanPanel::sampleImported(const juce::File &file) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);

  juce::MessageManager::callAsync([safeThis, file]() {
    if (safeThis) {
      safeThis->engine_.createTrack(file.getFileNameWithoutExtension(),
                                    "audio");
      safeThis->appendToConversation(WingmanStrings::kSpeakerWingman,
                                     "🎉 Created track for '" +
                                         file.getFileName() + "'.");
      safeThis->setStatus(WingmanStrings::kStatusReady,
                          juce::Colour(0xff00ff00));
    }
  });
}

void WingmanPanel::huntingProgressChanged(float progress,
                                          const juce::String &status) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);

  juce::MessageManager::callAsync([safeThis, status, progress]() {
    if (safeThis) {
      // Progress 0.0 - 1.0 (convert to %)
      juce::String pStr =
          (progress > 0) ? " (" + juce::String(int(progress * 100)) + "%)" : "";
      safeThis->setStatus(status + pStr, juce::Colour(0xffffff00));
    }
  });
}

void WingmanPanel::huntingComplete(const ai::HuntingStats &stats,
                                   bool success) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);

  juce::MessageManager::callAsync([safeThis, stats, success]() {
    if (!safeThis)
      return;

    if (success) {
      if (auto *hunter = safeThis->engine_.getSampleHunterAgent()) {
        safeThis->lastSearchResults_ = hunter->getFoundSamples();
        safeThis->displaySearchResults(safeThis->lastSearchResults_);
      }
      safeThis->setStatus(WingmanStrings::kStatusReady,
                          juce::Colour(0xff00ff00));
    } else {
      safeThis->appendToConversation(WingmanStrings::kSpeakerWingman,
                                     WingmanStrings::kMsgSearchFailed);
      safeThis->setStatus(WingmanStrings::kStatusError,
                          juce::Colour(0xffff0000));
    }
    // Update active state
    safeThis->isSampleSearchActive_ = !safeThis->lastSearchResults_.empty();
  });
}

} // namespace zenith
