# Contribution Examples: Good First Issues

Welcome to the Zenith DAW contribution examples! This guide shows you exactly how to make your first contributions to the project. Each example includes a real-world scenario with step-by-step instructions.

## 🌟 What Makes a "Good First Issue"?

Good first issues are typically:
- Well-defined scope (not too big)
- Self-contained (not dependent on many other changes)
- Don't require deep domain knowledge
- Help improve the project for everyone
- Great learning opportunities

### Types of Good First Issues
1. **Documentation fixes** - Typos, unclear instructions, missing examples
2. **UI polish** - Button spacing, color consistency, layout improvements
3. **Bug fixes** - Small issues in non-critical code paths
4. **Test coverage** - Adding missing unit tests
5. **Code cleanup** - Removing commented code, improving formatting

---

## Example 1: Fixing a Documentation Typo

### 🎯 Issue Description
*Found in `/docs/ARCHITECTURE.md` line 45*
- The text says "exlcusive" instead of "exclusive" rendering
- Confuses new developers learning about GPU acceleration

### Step-by-Step Solution

#### Step 1: Set Up Your Branch
```bash
# Switch to main and get latest
git checkout main
git pull upstream main

# Create feature branch
git checkout -b fix/architecture-typo
```

#### Step 2: Edit the File
```bash
# Open the documentation file
code docs/ARCHITECTURE.md

# Find the typo (around line 45)
# Change "exlcusive" to "exclusive"
```

**Before:**
```markdown
- **Skia**: Exclusive rendering engine (GPU-accelerated)
```

**After:**
```markdown
- **Skia**: Exclusive rendering engine (GPU-accelerated)
```

#### Step 3: Test and Commit
```bash
# Test that the application still builds
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Go back to project root
cd ..

# Add and commit the change
git add docs/ARCHITECTURE.md
git commit -m "Fix typo in architecture documentation

- Change 'exlcusive' to 'exclusive' in GPU rendering description
- Improves clarity for new developers reading the architecture guide
- Fixes typo in line 45 of docs/ARCHITECTURE.md"
```

#### Step 4: Create Pull Request
1. Push your branch: `git push origin fix/architecture-typo`
2. Go to GitHub and create a pull request
3. Title: "Fix typo: 'exclusive' not 'exlcusive' in documentation"
4. Body: "Fixes typo in architecture documentation for better readability"

---

## Example 2: Adding Missing Unit Tests

### 🎯 Issue Description
*The `ZenithPolySynth` class has no unit tests*
- Critical component needs test coverage
- Tests should verify parameter changes and audio processing

### Step-by-Step Solution

#### Step 1: Find Existing Test Structure
```bash
# Look at existing test files
ls apps/desktop/Source/tests/

# Examine a test file to understand the pattern
cat apps/desktop/Source/tests/ZenithTestsUberStrings.cpp
```

#### Step 2: Create Test File
```bash
# Create a new test file for the synthesizer
code apps/desktop/Source/tests/ZenithPolySynthTest.cpp
```

**Test File Contents:**
```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_test_utils/juce_test_utils.h>

class ZenithPolySynthTest : public juce::UnitTest
{
public:
    ZenithPolySynthTest() : juce::UnitTest("ZenithPolySynth", "Audio") {}

    void runTest() override
    {
        beginTest("Parameter Changes");
        testParameterChanges();

        beginTest("Audio Processing");
        testAudioProcessing();

        beginTest("MIDI Input");
        testMidiInput();
    }

    void testParameterChanges()
    {
        // Create synthesizer instance
        auto synth = std::make_unique<ZenithPolySynth>();

        // Test parameter changes don't crash
        synth->setParameter(0, 0.5f);  // Cutoff frequency
        synth->setParameter(1, 0.3f);  // Resonance
        synth->setParameter(2, 0.7f);  // Attack time

        // Verify parameters were set
        expect(synth->getParameter(0) == 0.5f);
        expect(synth->getParameter(1) == 0.3f);
        expect(synth->getParameter(2) == 0.7f);
    }

    void testAudioProcessing()
    {
        auto synth = std::make_unique<ZenithPolySynth>();
        synth->prepareToPlay(44100, 512);

        // Create test audio buffer
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        // Test that processing doesn't crash
        juce::MidiBuffer midiBuffer;
        expect(!synth->processBlock(buffer, midiBuffer));
    }

    void testMidiInput()
    {
        auto synth = std::make_unique<ZenithPolySynth>();
        synth->prepareToPlay(44100, 512);

        // Create MIDI note-on message
        juce::MidiBuffer midiBuffer;
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

        // Process MIDI
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        synth->processBlock(buffer, midiBuffer);

        // Verify MIDI was processed (check output isn't silent)
        expect(buffer.getRMSLevel(0, 0, buffer.getNumSamples()) > 0.0f);
    }
};

// Create static test instance
static ZenithPolySynthTest zenithPolySynthTest;
```

#### Step 3: Update CMake Configuration
```bash
# Find the CMakeLists.txt for tests
code apps/desktop/Source/tests/CMakeLists.txt

# Add the new test to the list
# Look for "ZenithTestsUberStrings" and add "ZenithPolySynthTest" after it
```

#### Step 4: Test and Commit
```bash
# Build with tests
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . -j$(nproc)

# Run the new test
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests --gtest_filter="*ZenithPolySynth*"

# Commit the test
cd ..
git add apps/desktop/Source/tests/ZenithPolySynthTest.cpp apps/desktop/Source/tests/CMakeLists.txt
git commit -m "Add comprehensive unit tests for ZenithPolySynth

- Test parameter changes and validation
- Test audio processing with MIDI input
- Ensure synthesizer responds correctly to controls
- Improve code coverage for critical audio component"
```

---

## Example 3: UI Component Polish

### 🎯 Issue Description
*Transport bar buttons need better spacing and styling*
- Current layout looks cramped
- New users find it hard to distinguish buttons
- Inconsistent with modern UI standards

### Step-by-Step Solution

#### Step 1: Locate UI Component
```bash
# Find the transport bar component
find . -name "*Transport*" -type f
# Look for TransportBar.h or TransportBar.cpp
```

#### Step 2: Examine Current Code
```bash
# Open the transport bar component
code apps/desktop/Source/ui/TransportBar.h
code apps/desktop/Source/ui/TransportBar.cpp
```

#### Step 3: Improve Spacing and Styling
**In TransportBar.cpp:**

**Before:**
```cpp
void TransportBar::paint(juce::Graphics& g)
{
    // Button painting without proper spacing
    playButton.setBounds(10, 10, 40, 40);
    stopButton.setBounds(60, 10, 40, 40);
    recordButton.setBounds(110, 10, 40, 40);
}
```

**After:**
```cpp
void TransportBar::paint(juce::Graphics& g)
{
    // Improved spacing and layout
    const int buttonSize = 48;
    const int spacing = 16;
    const int startY = 20;

    // Center buttons vertically with better spacing
    playButton.setBounds(20, startY, buttonSize, buttonSize);
    stopButton.setBounds(20 + buttonSize + spacing, startY, buttonSize, buttonSize);
    recordButton.setBounds(20 + 2 * (buttonSize + spacing), startY, buttonSize, buttonSize);
}
```

#### Step 4: Add Visual Feedback
**In TransportBar.cpp:**
```cpp
void TransportBar::resized()
{
    // Add button hover effects
    playButton.setMouseEnterButton(true);
    stopButton.setMouseEnterButton(true);
    recordButton.setMouseEnterButton(true);

    // Redraw on state changes
    repaint();
}
```

#### Step 5: Test and Commit
```bash
# Build and test the UI changes
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Test the application to see the improved UI
# Check that buttons are properly spaced and responsive

# Commit the changes
cd ..
git add apps/desktop/Source/ui/TransportBar.cpp apps/desktop/Source/ui/TransportBar.h
git commit -m "Improve transport bar UI layout and spacing

- Increase button size from 40x40 to 48x48 for better touch targets
- Add 16px spacing between buttons for visual clarity
- Center buttons vertically in the transport bar
- Improve accessibility and modern UI appearance"
```

---

## Example 4: Bug Fix - Button State Synchronization

### 🎯 Issue Description
*Play button shows "playing" but transport is actually stopped*
- Button state gets out of sync with actual transport state
- Confuses users when they can't start playback

### Step-by-Step Solution

#### Step 1: Understand the Problem
```bash
# Find transport-related files
grep -r "playButton" apps/desktop/Source/
grep -r "transport" apps/desktop/Source/ | head -10
```

#### Step 2: Locate the Issue
```bash
# Look for transport control code
code apps/desktop/Source/commands/TransportCommands.cpp
```

#### Step 3: Fix the State Synchronization
**Before (buggy code):**
```cpp
void TransportCommands::playButtonClicked()
{
    if (engine.isPlaying()) {
        engine.stop();
        playButton.setToggleState(false, juce::dontSendNotification);
    } else {
        engine.play();
        playButton.setToggleState(true, juce::dontSendNotification);
    }
}
```

**After (fixed code):**
```cpp
void TransportCommands::playButtonClicked()
{
    if (engine.isPlaying()) {
        engine.stop();
    } else {
        engine.play();
    }

    // Always update button state to match engine state
    playButton.setToggleState(engine.isPlaying(), juce::dontSendNotification);
}

// Also add callback to keep button in sync with engine
void TransportCommands::engineStateChanged(bool isPlaying)
{
    playButton.setToggleState(isPlaying, juce::dontSendNotification);
}
```

#### Step 4: Add Test Case
```cpp
// In TransportCommandsTest.cpp
void TransportCommandsTest::testPlayButtonStateSync()
{
    TransportCommands transport;

    // Initially stopped
    expect(!transport.getPlayButtonState());

    // Simulate engine starting playback
    transport.engineStateChanged(true);
    expect(transport.getPlayButtonState());

    // Simulate engine stopping
    transport.engineStateChanged(false);
    expect(!transport.getPlayButtonState());
}
```

#### Step 5: Test and Commit
```bash
# Test the fix manually
cd build
cmake --build .
./Zenith.exe  # Test play/stop button synchronization

# Test with unit tests
./build/ZenithDAWTests_artefacts/Release/ZenithDAWTests --gtest_filter="*Transport*"

# Commit the fix
cd ..
git add apps/desktop/Source/commands/TransportCommands.cpp apps/desktop/Source/commands/TransportCommands.h apps/desktop/Source/tests/TransportCommandsTest.cpp
git commit -m "Fix play button state synchronization with transport

- Update button state based on actual engine state, not local variable
- Add engineStateChanged callback to keep UI in sync
- Prevent button showing wrong state after transport changes
- Add test case to verify state synchronization"
```

---

## Example 5: Adding Keyboard Shortcuts

### 🎯 Issue Description
*Play button doesn't have keyboard shortcut*
- Users expect Space to play/pause like other DAWs
- Missing accessibility feature

### Step-by-Step Solution

#### Step 1: Find Key Handling Code
```bash
# Look for keyboard input handling
grep -r "keyPressed" apps/desktop/Source/ | head -5
```

#### Step 2: Add Keyboard Shortcut
**In main window component:**
```cpp
void MainWindow::keyPressed(const juce::KeyPress& key)
{
    // Space bar toggles play/pause
    if (key == juce::KeyPress::spaceKey) {
        transportCommands.playButtonClicked();
        return;
    }

    // Escape stops playback
    if (key == juce::KeyPress::escapeKey) {
        transportCommands.stopButtonClicked();
        return;
    }

    // Pass to parent class
    Component::keyPressed(key);
}
```

#### Step 3: Update UI to Show Shortcuts
**In TransportBar.cpp:**
```cpp
void TransportBar::paint(juce::Graphics& g)
{
    // Draw keyboard shortcuts
    g.setFont(juce::Font(10.0f));
    g.setColour(juice::Colours::grey);

    // Space next to play button
    g.drawText("Space", playButton.getBounds().translated(0, -15),
               juce::Justification::centred, false);

    // Escape next to stop button
    g.drawText("Esc", stopButton.getBounds().translated(0, -15),
               juce::Justification::centred, false);
}
```

#### Step 4: Add Accessibility Documentation
**In TransportBar.h:**
```cpp
/**
 * Transport bar with keyboard shortcuts
 *
 * Keyboard Controls:
 * - Space: Play/Pause
 * - Escape: Stop
 */
class TransportBar : public juce::Component
{
    // ... existing code ...
};
```

#### Step 5: Test and Commit
```bash
# Test keyboard shortcuts work
cd build
cmake --build .

# Test in application:
# 1. Press Space - should start/stop playback
# 2. Press Escape - should stop playback
# 3. Verify UI shows shortcut hints

# Commit the feature
cd ..
git add apps/desktop/Source/MainWindow.cpp apps/desktop/Source/ui/TransportBar.h apps/desktop/Source/ui/TransportBar.cpp
git commit -m "Add keyboard shortcuts for transport controls

- Space bar toggles play/pause (common DAW convention)
- Escape key stops playback
- Add visual hints showing keyboard shortcuts
- Improve accessibility and user experience"
```

---

## 📝 Template for Creating Your Own Good First Issue

If you want to propose your own good first issue, use this template:

### Issue Template

```markdown
## Issue Title: [Clear and descriptive title]

### Issue Type
- [x] Bug Fix
- [ ] Documentation Improvement
- [ ] UI Enhancement
- [ ] Test Addition
- [ ] Code Cleanup

### Description
[Brief description of the issue - 1-2 sentences]

### Steps to Reproduce
1. [Step 1]
2. [Step 2]
3. [Step 3]

### Expected Behavior
[What should happen]

### Actual Behavior
[What actually happens]

### Location in Code
[File path and approximate line number if known]

### Skills Required
- [ ] Basic C++
- [ ] JUCE framework knowledge
- [ ] UI development
- [ ] Audio programming
- [ ] Testing

### Difficulty
- [x] Easy (1-2 hours)
- [ ] Medium (3-5 hours)
- [ ] Hard (6+ hours)

### Proposed Solution
[Brief outline of how to fix it]
```

---

## 🎯 Tips for Success

### Before You Start
1. **Search existing issues** - Avoid duplicates
2. **Ask questions** - Clarify requirements in discussions
3. **Start small** - Begin with simple fixes before complex features
4. **Test thoroughly** - Ensure your changes don't break anything

### During Development
1. **Commit frequently** - Small, focused commits are easier to review
2. **Write clear messages** - Explain what and why, not just what
3. **Ask for help** - Don't struggle alone in Discord
4. **Keep branches updated** - Rebase regularly with upstream

### After Submission
1. **Be responsive** - Address review feedback promptly
2. **Be patient** - Code review takes time
3. **Learn from feedback** - Each review makes you better
4. **Thank reviewers** - Community appreciation goes far

---

## 🏆 Your First Contribution Checklist

Before submitting your first pull request:

- [ ] Issue is labeled "good first issue"
- [ ] Branch name follows convention: `fix/description` or `feature/description`
- [ ] Code builds successfully (Debug and Release)
- [ ] All existing tests still pass
- [ ] Added new tests if applicable
- [ ] Documentation updated if needed
- [ ] Commit message clear and descriptive
- [ ] Pull request description explains the change
- [ ] Ready for review

Remember: Every expert was once a beginner. Your first contribution is the start of an amazing journey! 🎵✨

---

**Need help?** Join our [Discord server](https://discord.gg/zenith-daw) or ask in [GitHub Discussions](https://github.com/zenith-daw/zenith/discussions)