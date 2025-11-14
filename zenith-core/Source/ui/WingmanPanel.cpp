/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    In-DAW command console implementation

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "commands/CommandAPI.h"

//==============================================================================
WingmanPanel::WingmanPanel(zenith::CommandAPI& api)
    : commandAPI(api)
{
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

    // Create clear button
    clearButton = std::make_unique<juce::TextButton>("Clear");
    clearButton->onClick = [this]() { clearHistory(); };
    addAndMakeVisible(*clearButton);

    // Welcome message
    addMessage("╔════════════════════════════════════════════════════════════╗", false);
    addMessage("║           Wingman v0 - AI Command Console                 ║", false);
    addMessage("╚════════════════════════════════════════════════════════════╝", false);
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

    // Clear button (top right, 80x25)
    auto clearBounds = bounds.removeFromTop(25);
    clearButton->setBounds(clearBounds.removeFromRight(80));

    // Space below clear button
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

    // Add user input to history
    addMessage("> " + input, true);

    // Parse input (try shorthand first, then JSON)
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
    else if (command == "help")
    {
        // Special case: show help (no actual command)
        addMessage("Available shorthand commands:", false);
        addMessage("  tracks", false);
        addMessage("  create <audio|midi> <name>", false);
        addMessage("  delete <trackId>", false);
        addMessage("  rename <trackId> <newName>", false);
        addMessage("  clips <trackId>", false);
        addMessage("  volume <trackId> <volumeDb>", false);
        addMessage("  pan <trackId> <pan>", false);
        addMessage("  graph", false);
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
    historyDisplay->scrollToMakeSureCursorIsVisible();
}
