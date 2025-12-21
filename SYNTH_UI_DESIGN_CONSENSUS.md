# Zenith PolySynth UI Design - Persona Conversation & Consensus

**Date:** 2025-11-27  
**Topic:** Final UI Design Agreement (Skia Implementation)

---

## 🎭 **THE PANEL**

*   **ALEX:** EDM Producer. Practical, wants speed and big controls.
*   **MAYA:** Beginner. Wants clarity, friendliness, and learning aids.
*   **JORDAN:** Sound Designer. Wants depth, precision, and organization.
*   **EVAN:** UI/UX Artist. Perfectionist, loves glassmorphism, animations, and "vibes".

---

## 💬 **THE CONVERSATION**

**EVAN:** "Okay everyone, look. The current plan is... fine. But it's boring. It's 2025! We need *Glassmorphism*. We need *Glow*. We need it to feel like you're touching a hologram."

**ALEX:** "As long as I can find the Cutoff knob in a dark club, I don't care if it's a hologram. Just make the main knobs BIG. I hate tiny controls."

**MAYA:** "Holograms sound cool! But... sometimes those glass interfaces are hard to read. Can we make sure the words are bright? And I still need those tooltips."

**JORDAN:** "I'm fine with pretty, Evan, but where does my Modulation Matrix go? If you blur everything, how do I see my routing connections? I need precision, not just 'vibes'."

**EVAN:** "Jordan, darling, we hide the clutter! Picture this: A sleek, dark, frosted glass panel floating in the center. The background is a subtle, animated nebula that reacts to the sound. In the middle, a stunning, real-time 3D oscilloscope."

**MAYA:** "Ooh, a moving nebula? That sounds relaxing."

**ALEX:** "Okay, the oscilloscope is cool. I like seeing the waveform. But the controls?"

**EVAN:** "We do a 'Macro Control' layout for the main view. Four massive, glowing orbs—I mean knobs—for Cutoff, Resonance, 'Motion' (LFO), and 'Space' (Reverb). They glow brighter when you turn them."

**JORDAN:** "That's too simple. I need to see my oscillators. I need to see the envelopes."

**ALEX:** "Yeah, I agree with Jordan. I need to switch waveforms quickly. Don't hide the oscillators."

**EVAN:** "Fine. How about this:
The 'Simple Mode' is a 600x400 glass card.
Top half: The Visualizer (Oscilloscope + Spectrum).
Bottom half: Three sections.
1. **Source:** Waveform icons (glowing lines), Sub knob, Noise knob.
2. **Filter:** HUGE central Cutoff knob with a glowing ring. Smaller Res knob.
3. **Mod/FX:** Envelope sliders (minimalist lines), and an FX strip at the bottom."

**MAYA:** "I like 'minimalist lines' for envelopes! It looks like the shape of the sound."

**JORDAN:** "And the advanced stuff?"

**EVAN:** "You click a subtle 'Expand' button, and the glass panel *slides* open. The window grows to 800x600. The Modulation Matrix slides out from the bottom. The LFOs appear on the sides. It's like opening the hood of a spaceship."

**JORDAN:** "Okay, that works. As long as the matrix is clear. No blurry text in the matrix, Evan."

**EVAN:** "Fine, high contrast for the data. But the borders will glow."

**ALEX:** "Can we color code it? Like, Oscillators are Blue, Filter is Orange, LFOs are Purple?"

**EVAN:** "Yes! Neon accents. Blue for Osc, Amber for Filter, Pink for Modulation. It'll look sick."

**MAYA:** "And tooltips? Please?"

**EVAN:** "We'll add a 'Learning Mode' toggle. When on, hovering over anything gives you a nice, frosted-glass popup explaining what it does. When off, it's clean."

**ALEX:** "Sold. Let's build it."

### **Layout (Advanced Mode - 800x600):**
*   *Expands downwards/outwards.*
*   **Reveals:**
    *   Full Oscillator controls (Detune, Mix).
    *   Filter 2 controls.
    *   Modulation Matrix (Grid layout).
    *   LFO Controls (Waveform selectors, Rate, Amount).

### **Evan's "Nitpicky" Requirements:**
*   **Spacing:** 20px padding between all groups. 10px between elements.
*   **Typography:** Sans-serif, clean, uppercase headers (e.g., "OSCILLATOR", "FILTER").
*   **Animations:** Knobs must have smooth rotation. Buttons must have hover glow.
*   **Corners:** 16px rounded corners on the main window. 8px on inner panels.

---

## 🛠️ **IMPLEMENTATION PLAN (Skia)**

1.  **`ZenithPolySynthUI` Class:** Main container, handles background and expansion.
2.  **`SkiaKnob`:** Custom component with glowing ring and shadow.
3.  **`SkiaSlider`:** Minimalist vertical slider for envelopes.
4.  **`SkiaVisualizer`:** Renders the audio buffer as a line graph.
5.  **`SkiaButton`:** Glassy button with hover state.
6.  **Layout Logic:** Dynamic positioning based on Simple/Advanced mode.

**Next Step:** Start coding `ZenithPolySynthUI.h` and `ZenithPolySynthUI.cpp`.
