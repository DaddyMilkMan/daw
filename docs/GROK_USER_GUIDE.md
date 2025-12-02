# Grok AI Integration - User Guide

**Created:** 2025-11-29  
**Author:** Carlos Santos (Documentation Team)  
**Version:** 1.0

---

## 🎉 Welcome to Wingman with Grok AI!

Wingman is your AI-powered assistant in Zenith DAW, now supercharged with Grok 4.1 from xAI. Control your entire DAW with natural language, generate custom synth presets, and create music faster than ever!

---

## 🚀 Quick Start

### Step 1: Get Your Grok API Key

1. Visit [console.x.ai](https://console.x.ai)
2. Sign up or log in
3. Navigate to "API Keys"
4. Click "Create New Key"
5. Copy your API key (starts with `xai-`)

### Step 2: Configure Wingman

1. Open Zenith DAW
2. Click the **Wingman** panel (right side)
3. Click the **⚙ Settings** button
4. Paste your Grok API key
5. Click **Save**

✅ **You're ready!** The status should show "Grok Ready" in green.

---

## 💬 Using Wingman

### Basic Commands

Just type naturally! Here are some examples:

**Track Management:**
- "Create a MIDI track called Bass"
- "Delete track 2"
- "Set the volume of track 1 to -6 dB"

**Preset Loading:**
- "Load the Deep Bass preset on track 1"
- "List all available presets for the synth"

**Preset Generation:** ⭐
- "Create a warm pad sound for ambient music"
- "Make me an aggressive bass for dubstep"
- "Generate a pluck sound for house music"

**MIDI Control:**
- "Add a C4 note at beat 1 for 2 beats"
- "Set the tempo to 128 BPM"

**Complex Workflows:**
- "Set up a basic house track with kick, bass, and chords"
- "Create a track, load a preset, and add some notes"

---

## ⚡ Fast vs 🧠 Thinking Mode

### Fast Mode (Default)
- **Best for:** Quick commands, simple tasks
- **Speed:** <2 seconds
- **Use when:** You know exactly what you want

**Example:** "Create a track called Drums"

### Thinking Mode
- **Best for:** Complex tasks, creative decisions
- **Speed:** 5-10 seconds
- **Use when:** You need Grok to figure things out

**Example:** "Analyze my mix and suggest improvements"

**Switch modes** using the dropdown at the bottom of Wingman panel.

---

## 🎨 AI Preset Generation

One of Wingman's coolest features is generating custom synth presets from descriptions!

### How It Works

1. Describe the sound you want
2. Wingman uses Grok to generate parameters
3. The preset is created and ready to use

### Examples

**Warm Pad:**
```
"Create a warm pad sound with slow attack for ambient music"
```

**Aggressive Bass:**
```
"Make an aggressive bass with lots of resonance for dubstep"
```

**Pluck Sound:**
```
"Generate a bright pluck for house music"
```

**Genre-Specific:**
```
"Create a synthwave lead sound"
```

### What Grok Considers

- **Sound type:** Pad, bass, lead, pluck, etc.
- **Genre:** House, dubstep, ambient, etc.
- **Characteristics:** Warm, aggressive, bright, dark, etc.
- **Musical context:** Attack, release, filter settings

---

## 🎛️ Full DAW Control

Wingman can control **everything** in your DAW:

### Tracks
- Create, delete, rename tracks
- Set volume, pan, mute, solo
- List all tracks

### Clips
- Create, delete, move clips
- Split clips at specific times
- List clips on a track

### MIDI
- Add, delete, move notes
- Set velocity and length
- Get all notes in a clip

### Instruments
- Load and save presets
- Get and set parameters
- Generate new presets with AI

### Plugins
- Add and remove plugins
- Set plugin parameters
- List plugins on a track

### Automation
- Add automation points
- Clear automation
- Get automation data

### Project
- Set tempo
- Add markers
- Export audio
- Undo/redo

---

## 💡 Tips & Tricks

### Be Specific
❌ "Make it sound better"  
✅ "Increase the filter cutoff to 2000 Hz on track 1"

### Use Natural Language
You don't need to memorize commands! Wingman understands:
- "Create a track" = "Make a new track" = "Add a track"

### Combine Commands
"Create a MIDI track called Bass, load the Deep Bass preset, and add a C2 note at beat 1"

### Ask for Help
"What can you do?"  
"How do I create a preset?"  
"List all available commands"

### Experiment with Presets
Try different descriptions:
- "Warm and lush"
- "Bright and punchy"
- "Dark and moody"
- "Aggressive and distorted"

---

## 🔧 Troubleshooting

### "Grok API key not configured"
- Click ⚙ Settings
- Enter your API key
- Click Save

### "Grok initialization failed"
- Check your API key is correct
- Ensure you have internet connection
- Verify your xAI account is active

### Commands Not Working
- Check the status indicator (bottom of panel)
- Try rephrasing your command
- Use Thinking mode for complex tasks

### Preset Generation Issues
- Be more specific in your description
- Include genre for better results
- Try different sound type keywords (pad, bass, lead)

---

## 📊 Command Reference

### Track Commands
- `create_track` - Create new track
- `list_tracks` - List all tracks
- `delete_track` - Delete a track
- `rename_track` - Rename a track
- `set_track_volume` - Set volume
- `set_track_pan` - Set pan

### Preset Commands
- `list_presets` - List available presets
- `load_preset` - Load a preset
- `save_preset` - Save current settings as preset
- `create_preset` - Create preset from parameters
- `delete_preset` - Delete a preset

### Instrument Commands
- `get_instrument_parameters` - Get current parameters
- `set_instrument_parameter` - Set a parameter
- `get_instrument_parameter_schema` - Get parameter info

### MIDI Commands
- `add_note` - Add MIDI note
- `delete_note` - Delete note
- `get_notes` - List all notes
- `set_note_velocity` - Change velocity
- `set_note_length` - Change length

---

## 🎵 Example Workflows

### Creating a Track with Preset
```
You: "Create a MIDI track called Lead and load the Synthwave Lead preset"
Wingman: "Created MIDI track 'Lead' and loaded preset 'Synthwave Lead'. Ready to record!"
```

### Generating a Custom Preset
```
You: "Generate a warm pad sound for ambient music"
Wingman: "Created preset 'Warm Ambient Pad' with layered saw and sine waves, gentle low-pass filter, and slow attack. Perfect for ambient textures!"
```

### Complex Multi-Step Task
```
You: "Set up a basic house track: kick on track 1, bass on track 2, chords on track 3, set tempo to 128 BPM"
Wingman: "Created your house track template! Track 1 (Kick) ready for audio, Track 2 (Bass) with House Bass preset, Track 3 (Chords) with House Chords preset. Tempo set to 128 BPM. Let's make some music!"
```

---

## 🔒 Security & Privacy

- **API keys** are stored encrypted using platform-specific secure storage
- **Conversations** are sent to Grok API (xAI's servers)
- **No audio** is sent to Grok
- **Project data** is only sent when you explicitly ask Wingman to analyze it

---

## 📞 Support

### Need Help?
- Check this guide
- Ask Wingman: "What can you do?"
- Visit [Zenith DAW Documentation](https://github.com/your-repo/docs)

### Report Issues
- GitHub Issues: [github.com/your-repo/issues](https://github.com/your-repo/issues)
- Include: Error message, what you were trying to do, Wingman conversation log

---

## 🎉 Have Fun!

Wingman is here to make music production faster and more creative. Experiment, explore, and enjoy having an AI assistant that truly understands your DAW!

**Happy music making!** 🎵

---

**Version:** 1.0  
**Last Updated:** 2025-11-29  
**Author:** Zenith DAW Team
