# CommandParser - Natural Language Interface for Zenith DAW

The CommandParser is a natural language processing module that converts text commands into structured DAW actions. It enables users to control the DAW using conversational language instead of clicking through menus.

## Features

- **Pattern-based parsing** with regex matching
- **Parameter extraction** (tempo, keys, bars, track types, etc.)
- **Confidence scoring** to handle ambiguous inputs
- **Intelligent suggestions** when commands aren't recognized
- **Context-aware help** system
- **Extensible architecture** for adding new command types

## Supported Commands

### Transport Controls

Control playback and recording:

```
"play"
"start"
"pause"
"stop"
"record"
```

**Examples:**
- "play the track"
- "pause"
- "stop playback"
- "start recording"

### Tempo Commands

Set the project tempo:

```
"set tempo to [number] BPM"
"change tempo to [number]"
"[number] BPM"
```

**Examples:**
- "set tempo to 120 BPM"
- "change tempo to 140"
- "make it 95 BPM"
- "tempo 128"

### Track Management

Create and manage tracks:

```
"add a [type] track"
"create a new [type] track called [name]"
"delete track [number]"
```

**Track Types:** `midi`, `audio`, `instrument`

**Examples:**
- "add a new MIDI track"
- "create an audio track called Vocals"
- "add an instrument track named Synth Lead"
- "delete track 2"

### Generate MIDI - Chords

Generate chord progressions:

```
"generate a [bars]-bar chord progression in [key] [mode]"
"create chords in [key] [mode]"
"suggest chords for [key] [mode]"
```

**Keys:** C, C#/Db, D, D#/Eb, E, F, F#/Gb, G, G#/Ab, A, A#/Bb, B
**Modes:** major, minor

**Examples:**
- "generate a 4-bar chord progression in A minor"
- "create an 8-bar chord sequence in C major"
- "suggest chords in E minor"
- "make chords for D major"

### Generate MIDI - Drums

Create drum patterns:

```
"generate a [bars]-bar [style] drum pattern"
"create [style] drums"
"make a beat"
```

**Styles:** trap, house, techno, hip-hop, rock, jazz

**Examples:**
- "generate an 8-bar trap drum pattern"
- "create a house beat"
- "make a 4-bar hip-hop drum loop"
- "create drums"

### Generate MIDI - Bass

Generate bass lines:

```
"generate a [bars]-bar bass line in [key] [mode]"
"create bass in [key] [mode]"
"add a bass line"
```

**Examples:**
- "generate a 4-bar bass line in C minor"
- "create bass for A major"
- "add an 8-bar bass pattern in E minor"

### Generate MIDI - Melody

Create melodic content:

```
"generate a [bars]-bar melody in [key] [mode]"
"create a melody in [key] [mode]"
```

**Examples:**
- "generate a 4-bar melody in G major"
- "create an 8-bar melody in D minor"
- "make a melody for C major"

### Insert Loops/Samples

Browse and insert audio loops:

```
"insert a [category] loop"
"add a [category] sample"
```

**Categories:** drum, bass, synth, vocal, fx

**Examples:**
- "insert a drum loop"
- "add a bass sample"
- "load a synth loop"

### Mixing

Auto-mix functionality:

```
"auto-mix"
"mix this track"
"balance the mix"
```

**Examples:**
- "auto-mix this project"
- "balance the levels"
- "mix this track"

### Help

Get help and see available commands:

```
"help"
"what can you do"
"commands"
"capabilities"
```

## Usage in Code

### Basic Usage

```typescript
import { CommandParser } from '../lib/CommandParser';

// Parse and execute a command
const response = CommandParser.parseAndExecute("set tempo to 120 BPM");

console.log(response.message);
// Output: "✓ Tempo set to 120 BPM"

console.log(response.dawActions);
// Output: [{ type: 'setTempo', value: 120 }]
```

### Two-Step Usage

```typescript
// Step 1: Parse the command
const parsed = CommandParser.parse("generate a 4-bar chord progression in A minor");

console.log(parsed);
// Output: {
//   type: 'generate',
//   action: 'chords',
//   parameters: { bars: 4, key: 'A', mode: 'minor' },
//   confidence: 0.95,
//   originalInput: '...'
// }

// Step 2: Execute the parsed command
const response = CommandParser.execute(parsed);

console.log(response.success); // true
console.log(response.message); // Detailed success message
console.log(response.dawActions); // Array of DAW actions to execute
console.log(response.actions); // Optional UI action buttons
```

### Handling Unknown Commands

```typescript
const response = CommandParser.parseAndExecute("do something weird");

if (!response.success) {
  console.log(response.message);
  // Output: "I didn't quite understand that. Did you mean:..."

  console.log(response.suggestions);
  // Output: ["• add a new track", "• set tempo to 120", ...]
}
```

## Architecture

### Command Flow

1. **User Input** → Natural language text
2. **Pattern Matching** → Regex patterns try to match the input
3. **Parameter Extraction** → Extract values (tempo, keys, bars, etc.)
4. **Confidence Scoring** → Rate how confident we are in the parse
5. **Command Execution** → Generate response and DAW actions
6. **Action Dispatch** → Execute DAW actions via Electron API

### ParsedCommand Structure

```typescript
interface ParsedCommand {
  type: CommandType;        // 'transport', 'tempo', 'track', etc.
  action?: string;          // Specific action: 'play', 'create', 'chords', etc.
  parameters: Record<string, any>; // Extracted parameters
  confidence: number;       // 0-1, how confident we are
  originalInput: string;    // Original user input
}
```

### CommandResponse Structure

```typescript
interface CommandResponse {
  success: boolean;         // Whether the command succeeded
  message: string;          // Human-readable response
  parsedCommand?: ParsedCommand; // The parsed command
  dawActions?: DAWAction[]; // Actions to execute
  suggestions?: string[];   // Suggestions for failed commands
  actions?: Array<{ label: string; onClick: () => void }>; // UI buttons
}
```

### DAWAction Structure

```typescript
interface DAWAction {
  type: string;             // Action type: 'setTempo', 'createTrack', etc.
  [key: string]: any;       // Additional parameters for the action
}
```

## Adding New Commands

To add a new command type:

1. **Add the pattern** to `COMMAND_PATTERNS`:

```typescript
{
  regex: /your pattern here/i,
  type: 'yourCommandType',
  action: 'yourAction',
  extractor: (match) => ({
    // Extract parameters from regex groups
    param1: match[1],
    param2: parseInt(match[2]),
  }),
  confidence: 0.9,
}
```

2. **Add a handler** method:

```typescript
private static handleYourCommand(cmd: ParsedCommand): CommandResponse {
  const { param1, param2 } = cmd.parameters;

  return {
    success: true,
    message: `✓ Did the thing with ${param1} and ${param2}`,
    parsedCommand: cmd,
    dawActions: [{ type: 'yourAction', param1, param2 }],
  };
}
```

3. **Add to the switch** in `execute()`:

```typescript
case 'yourCommandType':
  return this.handleYourCommand(parsedCommand);
```

## Pattern Matching Tips

### Order Matters
Patterns are tested in order. Put more specific patterns first:

```typescript
// ✓ Good: Specific pattern first
{ regex: /set tempo to (\d+) bpm/i, ... }
{ regex: /(\d+) bpm/i, ... }

// ✗ Bad: Generic pattern first would match everything
{ regex: /(\d+) bpm/i, ... }
{ regex: /set tempo to (\d+) bpm/i, ... }  // Never reached!
```

### Use Non-Capturing Groups
Use `(?:...)` for optional parts you don't need to capture:

```typescript
/(?:set|change)?\s*tempo\s*(?:to)?\s*(\d+)/i
// Matches: "set tempo to 120", "tempo 120", "change tempo 120"
// Captures only the number
```

### Make Things Optional
Use `?` for optional words:

```typescript
/(?:add|create)\s+(?:a|an)?\s*(?:new)?\s*track/i
// Matches: "add track", "create a track", "add a new track"
```

## Examples

### Complete Example Session

```
User: "help"
Assistant: [Shows full help message with all command categories]

User: "set tempo to 128"
Assistant: "✓ Tempo set to 128 BPM"

User: "add a MIDI track called Melody"
Assistant: "✓ Created MIDI track: 'Melody'"

User: "generate a 4-bar chord progression in C minor"
Assistant: "✓ Generated 4-bar chord progression in C minor
✓ Created track: 'Chords - C minor'
The progression is ready to edit!"

User: "create an 8-bar trap beat"
Assistant: "✓ Generated 8-bar trap drum pattern
✓ Created track: 'Trap Drums'
✓ Pattern includes kick, snare, hi-hats, and percussion
Ready to edit!"

User: "do something weird"
Assistant: "I didn't quite understand that. Did you mean:
• 'add a new track'
• 'set tempo to 120'
• 'generate chords in A minor'
• Type 'help' for all commands"
```

## Testing

Test the parser with various inputs:

```typescript
const testCases = [
  "play",
  "set tempo to 140 BPM",
  "add a new MIDI track",
  "generate a 4-bar chord progression in A minor",
  "create an 8-bar trap beat",
  "insert a drum loop",
  "auto-mix this track",
  "help",
];

testCases.forEach(input => {
  const response = CommandParser.parseAndExecute(input);
  console.log(`Input: "${input}"`);
  console.log(`Success: ${response.success}`);
  console.log(`Message: ${response.message}`);
  console.log(`Actions: ${JSON.stringify(response.dawActions)}`);
  console.log('---');
});
```

## Future Enhancements

Potential improvements:

1. **Fuzzy matching** for typo tolerance
2. **Multi-step commands** ("create a track and set tempo to 120")
3. **Context awareness** (remember previous commands)
4. **Parameter defaults** from project settings
5. **Voice-to-text** integration
6. **Machine learning** for better understanding
7. **Custom user patterns** (let users define their own shortcuts)
8. **Undo/Redo** integration
9. **Batch commands** ("create 4 MIDI tracks")
10. **Variable substitution** ("create a track called ${selectedInstrument}")

## Integration with Wingman

The CommandParser is integrated with the WingmanSidebar component:

```typescript
// In WingmanSidebar.tsx
import { CommandParser } from '../lib/CommandParser';

const handleSend = async () => {
  // ... user input handling ...

  const response = CommandParser.parseAndExecute(userInput);

  // Display response message
  const aiMessage = {
    content: response.message,
    actions: response.actions, // Optional UI buttons
  };

  // Execute DAW actions
  if (response.success && response.dawActions) {
    response.dawActions.forEach(action => {
      if (action.type === 'setTempo') {
        window.electron.setTempo(action.value);
      }
      // ... handle other action types ...
    });
  }
};
```

## License

Part of Zenith DAW - An AI-Native Digital Audio Workstation
