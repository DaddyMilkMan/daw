/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    In-DAW command console implementation with AI mode

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../Source/commands/CommandAPI.h"
#include "../Source/network/AIBridgeClient.h"
#include "../Source/commands/SessionGraph.h"

//==============================================================================
WingmanPanel::WingmanPanel(zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient)
    : commandAPI(api), aiBridgeClient(aiClient)
{
    // Register as listener for AI responses
    aiBridgeClient.addChangeListener(this);
    // Create history display (read-only, multi-line)
    historyDisplay = std::make_unique<juce::TextEditor>("History");
    historyDisplay->setMultiLine(true);
    historyDisplay->setReadOnly(true);
    historyDisplay->setScrollbarsShown(true);
    historyDisplay->setCaretVisible(false);
    historyDisplay->setPopupMenuEnabled(false);
    historyDisplay->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    historyDisplay->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
    historyDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);
    historyDisplay->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3a3a3a));
    addAndMakeVisible(*historyDisplay);

    // Create command input (single line)
    commandInput = std::make_unique<juce::TextEditor>("Command");
    commandInput->setMultiLine(false);
    commandInput->setReturnKeyStartsNewLine(false);
    commandInput->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));
    commandInput->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    commandInput->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    commandInput->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff5a5a5a));
    commandInput->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::cyan);
    commandInput->addListener(this);
    commandInput->setTextToShowWhenEmpty("Enter command (JSON or shorthand)...", juce::Colours::grey);
    addAndMakeVisible(*commandInput);

    // Create mode toggle buttons
    commandModeButton = std::make_unique<juce::TextButton>("Command");
    commandModeButton->setClickingTogglesState(true);
    commandModeButton->setRadioGroupId(1);
    commandModeButton->setToggleState(true, juce::dontSendNotification);
    commandModeButton->onClick = [this]() { setMode(Mode::Command); };
    addAndMakeVisible(*commandModeButton);

    aiModeButton = std::make_unique<juce::TextButton>("AI");
    aiModeButton->setClickingTogglesState(true);
    aiModeButton->setRadioGroupId(1);
    aiModeButton->onClick = [this]() { setMode(Mode::AI); };
    addAndMakeVisible(*aiModeButton);

    // Create clear button
    clearButton = std::make_unique<juce::TextButton>("Clear");
    clearButton->onClick = [this]() { clearHistory(); };
    addAndMakeVisible(*clearButton);

    // Welcome message
    addMessage("╔════════════════════════════════════════════════════════════╗", false);
    addMessage("║           Wingman AI v1 - Command Console                 ║", false);
    addMessage("╚════════════════════════════════════════════════════════════╝", false);
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

WingmanPanel::~WingmanPanel()
{
    aiBridgeClient.removeChangeListener(this);
    commandInput->removeListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void WingmanPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw title bar
    auto bounds = getLocalBounds();
    auto titleBar = bounds.removeFromTop(30);

    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(titleBar);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Wingman Console", titleBar.reduced(10, 0), juce::Justification::centredLeft);
}

void WingmanPanel::resized()
{
    auto bounds = getLocalBounds();

    // Title bar (30px)
    bounds.removeFromTop(30);

    // Margin
    bounds.reduce(10, 10);

    // Top row: Mode toggle buttons + Clear button
    auto topRow = bounds.removeFromTop(25);

    // Mode toggle buttons (left side, 160px total)
    auto modeBounds = topRow.removeFromLeft(160);
    commandModeButton->setBounds(modeBounds.removeFromLeft(80));
    aiModeButton->setBounds(modeBounds);

    // Clear button (right side, 80px)
    clearButton->setBounds(topRow.removeFromRight(80));

    // Space below buttons
    bounds.removeFromTop(10);

    // Command input (bottom, 30px)
    auto inputBounds = bounds.removeFromBottom(30);
    commandInput->setBounds(inputBounds);

    // Space above input
    bounds.removeFromBottom(10);

    // History display (remaining space)
    historyDisplay->setBounds(bounds);
}

//==============================================================================
// Command Execution
//==============================================================================

void WingmanPanel::executeCommand(const juce::String& input)
{
    if (input.trim().isEmpty())
        return;

    // Check if we're in pending batch state
    if (hasPendingBatch)
    {
        juce::String lowerInput = input.trim().toLowerCase();

        if (lowerInput == "yes" || lowerInput == "y")
        {
            addMessage("> yes", true);
            executePendingBatch();
            return;
        }
        else if (lowerInput == "no" || lowerInput == "n")
        {
            addMessage("> no", true);
            cancelPendingBatch();
            return;
        }
        else
        {
            addMessage("> " + input, true);
            addMessage("Please type 'yes' to apply the batch or 'no' to cancel.", false);
            addMessage("", false);
            return;
        }
    }

    // Add user input to history
    addMessage("> " + input, true);

    // Route based on mode
    if (currentMode == Mode::AI)
    {
        // AI mode: send natural language to AI bridge
        executeAIRequest(input);
    }
    else
    {
        // Command mode: parse and execute locally
        juce::String jsonCommand;

        if (input.trimStart().startsWith("{"))
        {
            // Already JSON
            jsonCommand = input;
        }
        else
        {
            // Try shorthand parser
            jsonCommand = parseShorthand(input);

            if (jsonCommand.isEmpty())
            {
                addMessage("ERROR: Could not parse command. Use 'help' for syntax.", false);
                addMessage("", false);
                return;
            }
        }

        // Execute command
        juce::String response = commandAPI.executeCommandString(jsonCommand);

        // Parse response to check for success/error
        juce::var responseVar;
        auto parseResult = juce::JSON::parse(response, responseVar);

        if (parseResult.failed())
        {
            addMessage("ERROR: Failed to parse response: " + parseResult.getErrorMessage(), false);
            addMessage(response, false);
        }
        else
        {
            bool success = responseVar.getProperty("success", false);

            if (success)
            {
                // Pretty-print result
                juce::var result = responseVar.getProperty("result", juce::var());
                juce::String resultString = juce::JSON::toString(result, true, 2);
                addMessage("✓ Success:", false);
                addMessage(resultString, false);
            }
            else
            {
                // Show error
                juce::String error = responseVar.getProperty("error", "Unknown error").toString();
                addMessage("✗ Error: " + error, false);
            }
        }

        addMessage("", false);  // Blank line for spacing
    }
}

void WingmanPanel::clearHistory()
{
    messageHistory.clear();
    historyDisplay->clear();

    addMessage("History cleared.", false);
    addMessage("", false);
}

//==============================================================================
// TextEditor::Listener
//==============================================================================

void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == commandInput.get())
    {
        juce::String input = commandInput->getText();
        commandInput->clear();

        executeCommand(input);
    }
}

//==============================================================================
// Shorthand Parser
//==============================================================================

juce::String WingmanPanel::parseShorthand(const juce::String& input)
{
    juce::StringArray tokens;
    tokens.addTokens(input, true);  // true = ignore empty tokens

    if (tokens.isEmpty())
        return {};

    juce::String command = tokens[0].toLowerCase();

    // Simple shorthand commands
    if (command == "tracks")
    {
        return "{\"command\":\"list_tracks\",\"params\":{}}";
    }
    else if (command == "graph")
    {
        return "{\"command\":\"get_session_graph\",\"params\":{}}";
    }
    else if (command == "undo" || command == "u")
    {
        return "{\"command\":\"undo\",\"params\":{}}";
    }
    else if (command == "redo" || command == "r")
    {
        return "{\"command\":\"redo\",\"params\":{}}";
    }
    else if (command == "history" || command == "h")
    {
        return "{\"command\":\"history\",\"params\":{}}";
    }
    else if (command == "help")
    {
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
        return {};  // Don't execute
    }
    else if (command == "create" && tokens.size() >= 2)
    {
        juce::String type = tokens[1].toLowerCase();
        juce::String name = tokens.size() >= 3 ? tokens.joinIntoString(" ", 2, -1) : "New Track";

        return "{\"command\":\"create_track\",\"params\":{\"type\":\"" + type + "\",\"name\":\"" + name + "\"}}";
    }
    else if (command == "delete" && tokens.size() >= 2)
    {
        juce::String trackId = tokens[1];
        return "{\"command\":\"delete_track\",\"params\":{\"trackId\":\"" + trackId + "\"}}";
    }
    else if (command == "rename" && tokens.size() >= 3)
    {
        juce::String trackId = tokens[1];
        juce::String name = tokens.joinIntoString(" ", 2, -1);
        return "{\"command\":\"rename_track\",\"params\":{\"trackId\":\"" + trackId + "\",\"name\":\"" + name + "\"}}";
    }
    else if (command == "clips" && tokens.size() >= 2)
    {
        juce::String trackId = tokens[1];
        return "{\"command\":\"list_clips\",\"params\":{\"trackId\":\"" + trackId + "\"}}";
    }
    else if (command == "volume" && tokens.size() >= 3)
    {
        juce::String trackId = tokens[1];
        double volumeDb = tokens[2].getDoubleValue();
        return "{\"command\":\"set_track_volume\",\"params\":{\"trackId\":\"" + trackId + "\",\"volumeDb\":" + juce::String(volumeDb) + "}}";
    }
    else if (command == "pan" && tokens.size() >= 3)
    {
        juce::String trackId = tokens[1];
        double pan = tokens[2].getDoubleValue();
        return "{\"command\":\"set_track_pan\",\"params\":{\"trackId\":\"" + trackId + "\",\"pan\":" + juce::String(pan) + "}}";
    }

    // Unknown shorthand
    return {};
}

//==============================================================================
// UI Helpers
//==============================================================================

void WingmanPanel::addMessage(const juce::String& message, bool isUserInput)
{
    juce::ignoreUnused(isUserInput);

    messageHistory.add(message);

    // Rebuild history display
    historyDisplay->setText(messageHistory.joinIntoString("\n"));

    // Scroll to bottom
    scrollHistoryToBottom();
}

void WingmanPanel::scrollHistoryToBottom()
{
    historyDisplay->moveCaretToEnd();
    // scrollToMakeSureCursorIsVisible() is protected in JUCE 8
    // moveCaretToEnd() already scrolls to make caret visible
}

//==============================================================================
// Mode Management
//==============================================================================

void WingmanPanel::setMode(Mode newMode)
{
    if (currentMode == newMode)
        return;

    currentMode = newMode;

    // Cancel any pending batch when switching modes
    if (hasPendingBatch)
    {
        cancelPendingBatch();
    }

    // Update UI
    if (currentMode == Mode::Command)
    {
        commandInput->setTextToShowWhenEmpty("Enter command (JSON or shorthand)...", juce::Colours::grey);
        addMessage("──────────────────────────────────────────────────────────", false);
        addMessage("Mode switched to: COMMAND", false);
        addMessage("Use JSON or shorthand commands.", false);
        addMessage("", false);
    }
    else // Mode::AI
    {
        commandInput->setTextToShowWhenEmpty("Enter natural language request...", juce::Colours::grey);
        addMessage("──────────────────────────────────────────────────────────", false);
        addMessage("Mode switched to: AI", false);
        addMessage("Type natural language requests (e.g., 'make a 4 bar drum loop').", false);
        addMessage("Server: " + aiBridgeClient.getServerUrl(), false);
        addMessage("", false);
    }

    DBG("WingmanPanel: Mode changed to " + juce::String(currentMode == Mode::Command ? "Command" : "AI"));
}

//==============================================================================
// AI Mode Handlers
//==============================================================================

void WingmanPanel::executeAIRequest(const juce::String& naturalLanguage)
{
    addMessage("(Sending to AI bridge server...)", false);

    // Get session graph by executing get_session_graph command
    juce::String graphResponse = commandAPI.executeCommandString("{\"command\":\"get_session_graph\",\"params\":{}}");

    juce::var graphVar;
    auto parseResult = juce::JSON::parse(graphResponse, graphVar);

    if (parseResult.failed())
    {
        addMessage("✗ Error: Failed to get session graph", false);
        addMessage("", false);
        return;
    }

    juce::var sessionGraph = graphVar.getProperty("result", juce::var());

    // Send request to AI bridge
    aiBridgeClient.sendRequest(naturalLanguage, sessionGraph, "zenith-core");

    addMessage("(Waiting for AI response...)", false);
    addMessage("", false);
}

void WingmanPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &aiBridgeClient)
    {
        // AI response available
        handleAIResponse();
    }
}

void WingmanPanel::handleAIResponse()
{
    while (aiBridgeClient.hasPendingResponse())
    {
        auto response = aiBridgeClient.popNextResponse();

        if (response.status == "error")
        {
            addMessage("✗ AI Error: " + response.errorMessage, false);
            addMessage("", false);
        }
        else if (response.status == "ok")
        {
            // Show AI's thought process and proposed commands
            showAIPlan(response.thought, response.commands);
        }
        else
        {
            addMessage("✗ Unknown AI response status: " + response.status, false);
            addMessage("", false);
        }
    }
}

void WingmanPanel::showAIPlan(const juce::String& thought, const juce::Array<juce::var>& commands)
{
    // Display AI's reasoning
    if (thought.isNotEmpty())
    {
        addMessage("──────────────────────────────────────────────────────────", false);
        addMessage("AI Plan:", false);
        addMessage(thought, false);
        addMessage("", false);
    }

    // Display proposed commands
    if (commands.isEmpty())
    {
        addMessage("✗ AI did not generate any commands.", false);
        addMessage("", false);
        return;
    }

    addMessage("Proposed commands (" + juce::String(commands.size()) + "):", false);

    for (int i = 0; i < commands.size(); ++i)
    {
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

void WingmanPanel::executePendingBatch()
{
    if (!hasPendingBatch || pendingCommands.isEmpty())
    {
        addMessage("✗ No pending batch to execute.", false);
        addMessage("", false);
        hasPendingBatch = false;
        return;
    }

    addMessage("Executing batch (" + juce::String(pendingCommands.size()) + " commands)...", false);

    // Execute batch via CommandAPI (single undo transaction)
    juce::var batchResponse = commandAPI.executeBatch(pendingCommands, "Wingman AI batch");

    // Clear pending state
    int commandCount = pendingCommands.size();
    hasPendingBatch = false;
    pendingCommands.clear();

    // Report results
    bool success = batchResponse.getProperty("success", false);

    if (!success)
    {
        // Batch failed
        juce::String error = batchResponse.getProperty("error", "Unknown error").toString();
        int failedIndex = batchResponse.getProperty("failedIndex", -1);
        int successCount = batchResponse.getProperty("successCount", 0);

        addMessage("✗ Batch failed at command #" + juce::String(failedIndex + 1) + ": " + error, false);
        addMessage("  (" + juce::String(successCount) + " commands succeeded before failure)", false);
    }
    else
    {
        // Batch succeeded
        int count = batchResponse.getProperty("result", juce::var()).getProperty("count", commandCount);
        addMessage("✓ Batch completed successfully (" + juce::String(count) + " commands applied).", false);
    }

    addMessage("", false);

    DBG("WingmanPanel: Batch execution completed (status: " + juce::String(success ? "success" : "failed") + ")");
}

void WingmanPanel::cancelPendingBatch()
{
    if (!hasPendingBatch)
        return;

    int commandCount = pendingCommands.size();

    hasPendingBatch = false;
    pendingCommands.clear();

    addMessage("Batch cancelled (" + juce::String(commandCount) + " commands discarded).", false);
    addMessage("", false);

    DBG("WingmanPanel: Batch cancelled");
}
