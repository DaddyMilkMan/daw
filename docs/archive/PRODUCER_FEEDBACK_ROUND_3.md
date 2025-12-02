# 📉 INTERNAL MEMO: PRODUCER FEEDBACK SESSION - ROUND 3 (THEY ARE STILL HERE)
**Date:** 2025-11-29
**Topic:** Zenith DAW Beta Review (Post-Round 2 Fixes)
**Attendees:** The 5 "Karens" (They have ordered lunch and are not leaving)

---

## 🗣️ THE TRANSCRIPT

### 1. KAREN "THE ANALOG PURIST" (Eating a Salad)
> *"The unison spread is... acceptable. But now I hear **aliasing**. When I play high notes with the Saw wave, it sounds like digital trash. You need **Band-Limited Oscillators (BLEP)**. I don't care if it burns CPU. I want purity. Also, the filter resonance drops the volume too much. I need **Automatic Gain Compensation** on the filter resonance."*

### 2. KAREN "THE WORKFLOW QUEEN" (Pacing around the room)
> *"The undo works now. Good. But I'm trying to arrange my track. I need to **Loop a Region**. I select a clip and press 'L', and nothing happens. I have to manually drag the loop markers? Barbaric. And why can't I **Bounce in Place**? I want to render this MIDI clip to audio instantly to save CPU. Right click -> Bounce. Make it happen."*

### 3. KAREN "THE VISUALIST" (Staring at the screen with a magnifying glass)
> *"The knobs have shadows now. Cute. But the **meters**... they are just green bars. I want **RMS vs Peak** metering. I want to see the dynamic range. And the 'Wingman' text is too small. I have a 4K monitor. Where is the **UI Scaling** option? I want to scale the whole interface to 150%."*

### 4. KAREN "THE CRASHER" (Clicking randomly)
> *"I found a new way to break it. I loaded a preset while the transport was playing. It clicked and popped. You need **parameter smoothing** or crossfading when switching presets. And I tried to record automation. It recorded, but the lines are jagged. I need **Automation Thinning/Smoothing**. My hand isn't a robot."*

### 5. KAREN "THE FEATURE CREEP" ( scrolling through TikTok)
> *"Chords are cool. But now I want **Style Transfer**. I want to upload a MIDI file of a Bach prelude and have the AI rewrite it as a Jazz standard. And can the AI **generate lyrics**? I have a melody but no words. Wingman should be my lyricist too."*

---

## 📋 SUMMARY OF ROUND 3 DEMANDS

1.  **Analog Purist:** "Anti-Aliasing (BLEP)" & "Filter Gain Compensation".
2.  **Workflow Queen:** "Loop Selection Shortcut (L)" & "Bounce in Place".
3.  **Visualist:** "RMS/Peak Metering" & "UI Scaling".
4.  **Crasher:** "Preset Switching Crossfade" & "Automation Smoothing".
5.  **Feature Creep:** "MIDI Style Transfer" & "Lyric Generation".

**Manager's Note:** They are getting comfortable. The "Anti-Aliasing" is a heavy DSP task. "Bounce in Place" is complex. "Lyric Generation" is easy (just a prompt). "UI Scaling" is hard with Skia if not planned.

**Priority:** Let's tackle the "Loop Shortcut" and "Lyric Generation" first to keep them distracted, then look at "Filter Gain Comp".
