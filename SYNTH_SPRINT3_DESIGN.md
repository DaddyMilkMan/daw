# Zenith PolySynth Sprint 3 Design - Modulation Matrix & Visualizer

**Date:** 2025-11-27
**Topic:** Modulation Matrix Design & Visualizer Integration

---

## 🎭 **THE PANEL**

*   **ALEX:** EDM Producer. Wants speed and clear feedback.
*   **MAYA:** Beginner. Wants to understand signal flow.
*   **JORDAN:** Sound Designer. Wants a comprehensive routing grid.
*   **EVAN:** UI/UX Artist. Wants a "Cyberpunk Hacking Terminal" aesthetic.

---

## 💬 **THE CONVERSATION**

### **Topic 1: The Modulation Matrix**

**JORDAN:** "Finally. The heart of the synth. I need a grid. Sources on the left (LFOs, Envelopes), Destinations on the top (Cutoff, Pitch, Drive). I want to see every connection at a glance."

**MAYA:** "A giant grid of dots? That sounds confusing. How do I know what's connected to what? Can't we just draw wires like on a modular synth?"

**EVAN:** "Wires are messy, Maya. We're not building a spaghetti factory. But Jordan, a massive spreadsheet is ugly. We need something... *holographic*."

**ALEX:** "I'm with Jordan on the grid, but keep it simple. I don't need 50 destinations. Just the main ones. And make the active connections glow so I can see them in the dark."

**EVAN:** "Here's the vision: **The Neural Grid**.
It's a 8x8 grid in the Advanced drawer.
Rows = Sources (LFO1, LFO2, Env1, Env2, Velocity, ModWheel, KeyTrack, Aftertouch).
Cols = Destinations (Pitch, Cutoff, Res, Drive, Pan, Level, OscShape, FX Mix).
When you hover over a cell, it lights up. Click and drag up/down to set the amount. Positive is Green, Negative is Red."

**MAYA:** "Oh, Red and Green is good! Like traffic lights. But can we highlight the row and column when I hover? So I don't lose my place?"

**EVAN:** "Yes! A crosshair highlight. 'Laser targeting'."

**JORDAN:** "Acceptable. As long as I can fine-tune the values. Dragging is okay, but can I double-click to type a number?"

**ALEX:** "Ain't nobody got time to type numbers. Dragging is fine. Just make it sensitive."

### **Topic 2: The Visualizer**

**ALEX:** "The wavy line is cool, but is it real? I need to see if I'm clipping the output."

**EVAN:** "It will be real audio data, Alex. A 60fps stream from the audio engine. We'll use a ring buffer to pass data from the audio thread to the UI thread safely."

**MAYA:** "Can it change color? Like, blue when it's quiet and orange when it's loud?"

**EVAN:** "Reactive coloration. I love it. We'll map amplitude to brightness."

### **Topic 3: Tooltips (Learning Mode)**

**MAYA:** "Please don't forget me. When I hover over 'LFO', I want a popup."

**EVAN:** "We'll implement a 'Glass Overlay'. When Learning Mode is ON, hovering any control fades the background and spotlights the control, showing a description card."

**ALEX:** "That sounds annoying for pro users."

**EVAN:** "That's why it's a toggle, Alex. Off by default for you."


## 🛠️ **IMPLEMENTATION PLAN**

1.  **Audio Engine:** Add `AudioVisualiserComponent` logic (Ring Buffer) to `ZenithPolySynthProcessor`.
2.  **UI Class:** Update `ZenithVisualizer` to consume the buffer.
3.  **UI Class:** Implement `ZenithModMatrix` component (Grid).
4.  **Integration:** Add `ZenithModMatrix` to `ZenithPolySynthUI` (Advanced Mode).
