/**
 * CommandParser Test Suite
 *
 * Run this file to test the CommandParser with various natural language inputs.
 * This demonstrates the capabilities and shows expected outputs.
 *
 * Usage (in console):
 *   import { runTests } from './CommandParser.test'
 *   runTests()
 */

import { CommandParser } from './CommandParser';

export interface TestCase {
  input: string;
  expectedType: string;
  expectedAction?: string;
  description: string;
}

const testCases: TestCase[] = [
  // Transport Commands
  {
    input: "play",
    expectedType: "transport",
    expectedAction: "play",
    description: "Simple play command"
  },
  {
    input: "start playback",
    expectedType: "transport",
    expectedAction: "play",
    description: "Play with natural language"
  },
  {
    input: "pause",
    expectedType: "transport",
    expectedAction: "pause",
    description: "Pause command"
  },
  {
    input: "stop the track",
    expectedType: "transport",
    expectedAction: "stop",
    description: "Stop with natural language"
  },
  {
    input: "record",
    expectedType: "transport",
    expectedAction: "record",
    description: "Record command"
  },

  // Tempo Commands
  {
    input: "set tempo to 120 BPM",
    expectedType: "tempo",
    expectedAction: "set",
    description: "Set tempo with BPM suffix"
  },
  {
    input: "change tempo to 140",
    expectedType: "tempo",
    expectedAction: "set",
    description: "Change tempo without BPM"
  },
  {
    input: "make it 95 bpm",
    expectedType: "tempo",
    expectedAction: "set",
    description: "Casual tempo change"
  },
  {
    input: "128 BPM",
    expectedType: "tempo",
    expectedAction: "set",
    description: "Just the number and BPM"
  },

  // Track Commands
  {
    input: "add a new track",
    expectedType: "track",
    expectedAction: "create",
    description: "Add generic track"
  },
  {
    input: "create a MIDI track",
    expectedType: "track",
    expectedAction: "create",
    description: "Create MIDI track"
  },
  {
    input: "add an audio track called Vocals",
    expectedType: "track",
    expectedAction: "create",
    description: "Create named audio track"
  },
  {
    input: "create an instrument track named Synth Lead",
    expectedType: "track",
    expectedAction: "create",
    description: "Create named instrument track"
  },

  // Generate - Chords
  {
    input: "generate a 4-bar chord progression in A minor",
    expectedType: "generate",
    expectedAction: "chords",
    description: "Generate chord progression with all params"
  },
  {
    input: "create chords in C major",
    expectedType: "generate",
    expectedAction: "chords",
    description: "Generate chords without bar count"
  },
  {
    input: "suggest chords for E minor",
    expectedType: "generate",
    expectedAction: "chords",
    description: "Suggest chords (no generation)"
  },
  {
    input: "make an 8-bar chord sequence in Db major",
    expectedType: "generate",
    expectedAction: "chords",
    description: "Chord sequence with flat key"
  },

  // Generate - Drums
  {
    input: "generate an 8-bar trap drum pattern",
    expectedType: "generate",
    expectedAction: "drums",
    description: "Generate drums with style and bars"
  },
  {
    input: "create a house beat",
    expectedType: "generate",
    expectedAction: "drums",
    description: "Generate house drums"
  },
  {
    input: "make a 4-bar hip-hop drum loop",
    expectedType: "generate",
    expectedAction: "drums",
    description: "Generate hip-hop drums"
  },
  {
    input: "add drums",
    expectedType: "generate",
    expectedAction: "drums",
    description: "Simple drum generation"
  },

  // Generate - Bass
  {
    input: "generate a 4-bar bass line in C minor",
    expectedType: "generate",
    expectedAction: "bass",
    description: "Generate bass with all params"
  },
  {
    input: "create bass for A major",
    expectedType: "generate",
    expectedAction: "bass",
    description: "Generate bass without bar count"
  },

  // Generate - Melody
  {
    input: "generate a 4-bar melody in G major",
    expectedType: "generate",
    expectedAction: "melody",
    description: "Generate melody with all params"
  },
  {
    input: "create an 8-bar melody in D minor",
    expectedType: "generate",
    expectedAction: "melody",
    description: "Generate melody in minor key"
  },

  // Insert Commands
  {
    input: "insert a drum loop",
    expectedType: "insert",
    expectedAction: "loop",
    description: "Insert drum loop"
  },
  {
    input: "add a bass sample",
    expectedType: "insert",
    expectedAction: "loop",
    description: "Add bass sample"
  },

  // Mix Commands
  {
    input: "auto-mix this track",
    expectedType: "mix",
    expectedAction: "auto",
    description: "Auto-mix command"
  },
  {
    input: "balance the mix",
    expectedType: "mix",
    expectedAction: "auto",
    description: "Balance mix"
  },

  // Help Commands
  {
    input: "help",
    expectedType: "help",
    description: "Help command"
  },
  {
    input: "what can you do",
    expectedType: "help",
    description: "Capability query"
  },

  // Unknown Commands (should suggest alternatives)
  {
    input: "do something weird",
    expectedType: "unknown",
    description: "Unrecognized command"
  },
  {
    input: "quantum fluctuate the synth",
    expectedType: "unknown",
    description: "Nonsense command"
  },
];

/**
 * Run all test cases and log results
 */
export function runTests(): void {
  console.log('='.repeat(80));
  console.log('CommandParser Test Suite');
  console.log('='.repeat(80));
  console.log();

  let passed = 0;
  let failed = 0;

  testCases.forEach((testCase, index) => {
    console.log(`Test ${index + 1}/${testCases.length}: ${testCase.description}`);
    console.log(`Input: "${testCase.input}"`);

    const parsed = CommandParser.parse(testCase.input);
    const response = CommandParser.execute(parsed);

    // Check if type matches
    const typeMatches = parsed.type === testCase.expectedType;
    const actionMatches = !testCase.expectedAction || parsed.action === testCase.expectedAction;

    if (typeMatches && actionMatches) {
      console.log('✓ PASS');
      passed++;
    } else {
      console.log('✗ FAIL');
      console.log(`  Expected: type=${testCase.expectedType}, action=${testCase.expectedAction}`);
      console.log(`  Got: type=${parsed.type}, action=${parsed.action}`);
      failed++;
    }

    console.log(`Confidence: ${(parsed.confidence * 100).toFixed(0)}%`);
    console.log(`Response: ${response.message.split('\n')[0]}...`);

    if (response.dawActions && response.dawActions.length > 0) {
      console.log(`DAW Actions: ${response.dawActions.map(a => a.type).join(', ')}`);
    }

    console.log('-'.repeat(80));
    console.log();
  });

  console.log('='.repeat(80));
  console.log(`Results: ${passed} passed, ${failed} failed out of ${testCases.length} tests`);
  console.log('='.repeat(80));
}

/**
 * Run a single interactive test
 */
export function testCommand(input: string): void {
  console.log('='.repeat(80));
  console.log(`Testing: "${input}"`);
  console.log('-'.repeat(80));

  const parsed = CommandParser.parse(input);
  console.log('\nParsed Command:');
  console.log(JSON.stringify(parsed, null, 2));

  const response = CommandParser.execute(parsed);
  console.log('\nResponse:');
  console.log(JSON.stringify(response, null, 2));

  console.log('\nFormatted Message:');
  console.log(response.message);
  console.log('='.repeat(80));
}

/**
 * Demonstrate example conversation
 */
export function demonstrateConversation(): void {
  console.log('='.repeat(80));
  console.log('Example Wingman Conversation');
  console.log('='.repeat(80));
  console.log();

  const conversation = [
    "help",
    "set tempo to 128",
    "add a MIDI track called Melody",
    "generate a 4-bar chord progression in C minor",
    "create an 8-bar trap beat",
    "insert a bass loop",
    "auto-mix this track",
    "do something weird", // This should fail with suggestions
  ];

  conversation.forEach((input, index) => {
    console.log(`\n${'-'.repeat(80)}`);
    console.log(`User: ${input}`);
    console.log('-'.repeat(80));

    const response = CommandParser.parseAndExecute(input);
    console.log(`Wingman: ${response.message}`);

    if (response.dawActions && response.dawActions.length > 0) {
      console.log(`\nExecuting: ${response.dawActions.map(a => a.type).join(', ')}`);
    }

    if (response.actions && response.actions.length > 0) {
      console.log(`\nQuick Actions: ${response.actions.map(a => a.label).join(' | ')}`);
    }
  });

  console.log('\n' + '='.repeat(80));
}

// Export for use in browser console or tests
export default {
  runTests,
  testCommand,
  demonstrateConversation,
  testCases,
};
