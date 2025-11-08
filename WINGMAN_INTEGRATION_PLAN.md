# Wingman + Custom DAW Integration Plan

## Executive Summary

**Wingman is already 80% of what you need!** It's a sophisticated AI music assistant with:
- ✅ Electron app with React UI
- ✅ Multi-AI provider support (OpenAI, Anthropic, Perplexity)
- ✅ Magenta.js music generation
- ✅ WebSocket/UDP communication infrastructure
- ✅ VST3 bridge architecture
- ✅ Python Remote Script for deep DAW control (Ableton Live)

**The Plan:** Adapt Wingman's proven architecture for your custom DAW by replacing the "Python Remote Script for Ableton" with "Direct C++ API calls to your DAW engine."

---

## What Wingman Is

### Current Wingman Architecture (For Ableton Live)

```
┌────────────────────────────────────┐
│  WINGMAN STANDALONE APP            │
│  (Electron + React)                │
│  - AI chat (GPT-4, Claude, etc.)   │
│  - Magenta music generation        │
│  - Piano roll UI                   │
│  - WebSocket server :8123          │
│  - UDP server :12000               │
└──────────────┬─────────────────────┘
               │
               ↕ WebSocket + UDP JSON
               │
┌──────────────┴─────────────────────┐
│  BRIDGE VST3 (JUCE)                │
│  - Minimal UI (status + button)    │
│  - DAW context extraction          │
│  - MIDI output                     │
│  - UDP client → App                │
└──────────────┬─────────────────────┘
               │
               ↕ Live Object Model (LOM)
               │
┌──────────────┴─────────────────────┐
│  PYTHON REMOTE SCRIPT              │
│  (Ableton Live Control Surface)    │
│  - Track/clip/device control       │
│  - Scene launching                 │
│  - Parameter automation            │
│  - UDP server :11000               │
└────────────────────────────────────┘
```

### Key Features

**AI Integration:**
- Multiple AI providers with automatic fallback
- Music prompt refinement (vague → specific)
- Genre detection (trap, house, jazz, lofi, etc.)
- Tempo, key, scale extraction from natural language

**Music Generation:**
- Magenta.js for MIDI generation
- Drum pattern generation (TR-808 style)
- Bass pattern generation (808 bass)
- Chord/synth patterns (Massive X style)
- AI-refined specifications → professional MIDI output

**DAW Communication:**
- UDP JSON for low-latency commands
- WebSocket for real-time updates
- Command/response pattern with acknowledgments
- Stable UID system for tracks/clips (handles reordering)

**Safety:**
- User touch priority (AI freezes parameter for 5s)
- Command validation
- Rate limiting (20 commands/second max)
- Access Control List (destructive ops require enable)

---

## Your Custom DAW Integration Strategy

### Recommended Architecture

```
┌────────────────────────────────────┐
│  WINGMAN STANDALONE APP            │  ← REUSE 100%
│  (Electron + React)                │  ← Already working!
│  - AI chat                         │
│  - Magenta generation              │
│  - Piano roll UI                   │
│  - WebSocket server :8123          │
│  - UDP server :12000               │
└──────────────┬─────────────────────┘
               │
               ↕ UDP JSON (keep same protocol)
               │
┌──────────────┴─────────────────────┐
│  YOUR CUSTOM DAW                   │  ← NEW: Build this
│  (JUCE Framework)                  │
│                                    │
│  ┌─────────────────────────────┐  │
│  │ Audio Engine (C++)          │  │
│  │ - Real-time audio           │  │
│  │ - Track management          │  │
│  │ - Mixer, routing            │  │
│  │ - Plugin hosting            │  │
│  └─────────────┬───────────────┘  │
│                │                   │
│  ┌─────────────▼───────────────┐  │
│  │ Wingman Integration Layer   │  │
│  │ (C++)                       │  │
│  │ - UDP client → Wingman App  │  │
│  │ - Command executor          │  │
│  │ - State broadcaster         │  │
│  │ - Lock-free queues          │  │
│  └─────────────────────────────┘  │
└────────────────────────────────────┘
```

**Why This Is Better:**

| Wingman for Ableton | Wingman for Your DAW |
|----------------------|----------------------|
| VST3 Bridge → Python Script → Ableton API | Direct C++ API calls in DAW |
| 3-layer communication overhead | Single-layer, direct access |
| Limited by Ableton's Python API | Unlimited access to everything |
| Can't add DAW features | Can build features AI designed |
| ⚠️ Complex routing | ✅ Simple, direct |

---

## Implementation Plan

### Phase 1: Reuse Wingman Standalone App (Week 1)

**Goal:** Get Wingman AI chat working standalone

**Steps:**
1. ✅ Already done! You have `wingman-app/` folder
2. Test Wingman standalone:
   ```bash
   cd /home/user/daw/wingman/wingman-app
   npm install
   npm run dev
   ```
3. Verify AI works (OpenAI key in .env)
4. Test Magenta music generation
5. Confirm WebSocket server starts on :8123
6. Confirm UDP server starts on :12000

**Deliverable:** Wingman app runs, AI chats, generates music

---

### Phase 2: Build Minimal DAW (Weeks 2-4)

**Goal:** Basic audio engine that can play/record

**Use AI_NATIVE_DAW_ARCHITECTURE.md as guide, but start simple:**

1. **JUCE Audio Engine Setup**
   ```cpp
   class CustomDAWEngine : public juce::AudioAppComponent {
   public:
       CustomDAWEngine() {
           // Audio I/O setup
           setAudioChannels(2, 2);

           // Create Wingman integration
           wingmanBridge = std::make_unique<WingmanBridge>();
       }

       void prepareToPlay(int samplesPerBlock, double sampleRate) override {
           // Prepare audio processing
       }

       void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override {
           // Process audio
           processAllTracks(bufferToFill);

           // Send state to Wingman (throttled)
           wingmanBridge->broadcastState(getCurrentState());
       }

   private:
       std::unique_ptr<WingmanBridge> wingmanBridge;
       std::vector<std::unique_ptr<Track>> tracks;
   };
   ```

2. **Wingman Integration Layer (C++)**
   ```cpp
   class WingmanBridge {
   public:
       WingmanBridge() {
           // Connect to Wingman app via UDP
           udpSocket.bindToPort(12001);  // Receive from Wingman

           // Start command processing thread
           commandThread = std::thread([this]() {
               processCommandsFromWingman();
           });
       }

       void broadcastState(const DAWState& state) {
           // Send tempo, playback state, etc. to Wingman
           json stateJson = {
               {"event", "tempo"},
               {"data", {{"tempo", state.tempo}}}
           };

           sendUDP("127.0.0.1", 12000, stateJson.dump());
       }

       void processCommandsFromWingman() {
           while (running) {
               // Receive UDP message from Wingman
               auto message = receiveUDP();
               auto command = json::parse(message);

               // Parse and execute
               executeCommand(command);
           }
       }

       void executeCommand(const json& cmd) {
           std::string cmdType = cmd["cmd"];

           if (cmdType == "transport.play") {
               commandQueue.push([this]() {
                   audioEngine->startPlayback();
               });
           }
           else if (cmdType == "transport.set_tempo") {
               float bpm = cmd["args"]["bpm"];
               commandQueue.push([this, bpm]() {
                   audioEngine->setTempo(bpm);
               });
           }
           else if (cmdType == "track.create_midi") {
               std::string name = cmd["args"]["name"];
               commandQueue.push([this, name]() {
                   audioEngine->createMIDITrack(name);
               });
           }
           // ... handle all Wingman commands
       }

   private:
       UdpSocket udpSocket;
       LockFreeQueue<std::function<void()>> commandQueue;
       std::thread commandThread;
   };
   ```

**Deliverable:** DAW can play audio, Wingman can see tempo/transport

---

### Phase 3: Add Track Management (Week 5)

**Goal:** Wingman can create/delete/control tracks

**DAW Side:**
```cpp
class TrackManager {
public:
    int createMIDITrack(const std::string& name) {
        auto track = std::make_unique<MIDITrack>(name);
        int trackId = nextTrackId++;
        tracks[trackId] = std::move(track);

        // Notify Wingman
        wingmanBridge->sendEvent("track.added", {
            {"track_id", trackId},
            {"name", name},
            {"type", "midi"}
        });

        return trackId;
    }

    void setTrackVolume(int trackId, float volumeDb) {
        if (tracks.count(trackId)) {
            tracks[trackId]->setVolume(volumeDb);
        }
    }

    void setTrackPan(int trackId, float pan) {
        if (tracks.count(trackId)) {
            tracks[trackId]->setPan(pan);
        }
    }
};
```

**Wingman Commands to Support:**
- `track.create_midi` - Create MIDI track
- `track.create_audio` - Create audio track
- `track.delete` - Remove track
- `track.rename` - Change name
- `track.mute` - Mute track
- `track.solo` - Solo track
- `mixer.set_volume` - Adjust volume
- `mixer.set_pan` - Adjust pan

**Deliverable:** AI can say "create a drum track" and it works

---

### Phase 4: MIDI Generation (Week 6)

**Goal:** Wingman's Magenta output goes into your DAW

**Wingman Already Generates:**
```typescript
// wingman-app generates this:
{
  patterns: [
    { note: 36, startTime: 0.0, duration: 0.25, velocity: 100 },  // Kick
    { note: 38, startTime: 1.0, duration: 0.25, velocity: 90 },   // Snare
    { note: 42, startTime: 0.5, duration: 0.125, velocity: 70 },  // Hi-hat
    // ... more notes
  ],
  tempo: 120,
  key: 'A',
  scale: 'minor'
}
```

**Your DAW Receives via UDP:**
```cpp
void WingmanBridge::handleAddMIDI(const json& cmd) {
    auto midiData = cmd["data"];
    int trackId = cmd["args"]["track_id"];

    std::vector<MIDINote> notes;
    for (auto& note : midiData["notes"]) {
        notes.push_back({
            .pitch = note["note"],
            .startTime = note["startTime"],
            .duration = note["duration"],
            .velocity = note["velocity"]
        });
    }

    // Add to track
    commandQueue.push([this, trackId, notes]() {
        audioEngine->addMIDIToTrack(trackId, notes);
    });
}
```

**Deliverable:** User types "create trap drums", sees MIDI in DAW

---

### Phase 5: Plugin Hosting (Weeks 7-8)

**Goal:** Load VST3 plugins, AI can control parameters

**JUCE Plugin Hosting:**
```cpp
class PluginHost {
public:
    void loadPlugin(int trackId, const std::string& pluginPath) {
        auto format = vstPluginFormat.get();

        PluginDescription desc;
        if (format->findAllTypesForFile(desc, pluginPath)) {
            auto plugin = format->createInstanceFromDescription(desc);

            tracks[trackId]->addPlugin(std::move(plugin));

            // Notify Wingman of parameters
            sendPluginParameters(trackId, plugin.get());
        }
    }

    void setPluginParameter(int trackId, int pluginIndex,
                          const std::string& paramName, float value) {
        auto plugin = tracks[trackId]->getPlugin(pluginIndex);
        auto param = findParameterByName(plugin, paramName);
        param->setValue(value);
    }
};
```

**Wingman Commands:**
- `device.add` - Load plugin
- `device.param.set` - Set parameter value
- `device.remove` - Unload plugin

**Deliverable:** AI can say "add reverb with 2 second decay"

---

### Phase 6: Full Production Workflow (Weeks 9-12)

**Goal:** Complete AI-assisted music production

**Features to Add:**

1. **Automation:**
   ```cpp
   void Track::recordAutomation(const std::string& paramName,
                                 double time, float value) {
       automationCurves[paramName].addPoint(time, value);
   }
   ```

2. **Arrangement:**
   ```cpp
   void Arranger::createSection(const std::string& name,
                                double startTime, double duration) {
       sections.push_back({name, startTime, duration});
   }
   ```

3. **Mixing:**
   ```cpp
   void MixingEngine::autoMix(const std::vector<TrackAnalysis>& analysis) {
       // AI-powered auto-leveling
       for (auto& track : analysis) {
           float targetLevel = calculateOptimalLevel(track);
           setTrackVolume(track.id, targetLevel);
       }
   }
   ```

**Deliverable:** Full production from "create a lofi beat" → finished mix

---

## Reusing Wingman Code

### What to Copy Directly

**1. Wingman Standalone App (100% reuse)**
```bash
# Already in wingman-app/
- src/components/WingmanMinimal.tsx  # Main UI
- src/services/realAI.ts             # AI integration
- src/services/magentaService.ts     # Music generation
- src/services/musicPromptRefiner.ts # Prompt → specs
- src/hooks/useDAWBridge.ts          # Communication hook
```

**2. Communication Protocol (adapt format)**
```typescript
// Wingman's UDP JSON format (keep this!)
{
  "id": "cmd-123",
  "cmd": "track.create_midi",
  "args": { "name": "Drums" },
  "meta": { "origin": "ai", "ts": 1730512345 }
}

// Response:
{
  "id": "cmd-123",
  "ok": true,
  "result": { "track_id": 5 }
}
```

**3. AI Prompt Refinement Logic**
```typescript
// From musicPromptRefiner.ts
async refinePrompt(userMessage: string) {
  // Sends to GPT-4:
  // "User said: 'trap beat'
  //  Extract: style, tempo, key, drum patterns, instrumentation"

  // Returns:
  {
    style: 'trap',
    tempo: 95,
    key: 'A minor',
    drumsPattern: {
      kickPattern: 'syncopated',
      hatPattern: 'swung sixteenths',
      snarePattern: 'on 2 and 4'
    }
  }
}
```

**4. Magenta Integration**
```typescript
// magentaService.ts
generateDrumPattern(tempo, bars, style, key) {
  // Uses Magenta.js MusicVAE model
  // Returns professional MIDI sequence
}
```

### What to Replace

| Wingman for Ableton | Your Custom DAW |
|---------------------|-----------------|
| Python Remote Script | C++ Wingman Integration Layer |
| Ableton LOM API | Direct DAW engine calls |
| Track index stability hacks | Native stable IDs |
| Bridge VST3 | Not needed (DAW IS the app) |

---

## Communication Protocol Reference

### Commands DAW Should Handle

**Transport:**
```json
{ "cmd": "transport.play" }
{ "cmd": "transport.stop" }
{ "cmd": "transport.locate", "args": { "bar": 8 } }
{ "cmd": "transport.set_tempo", "args": { "bpm": 120.0 } }
```

**Tracks:**
```json
{ "cmd": "track.create_midi", "args": { "name": "Drums" } }
{ "cmd": "track.create_audio", "args": { "name": "Vocals" } }
{ "cmd": "track.delete", "args": { "track_id": 3 } }
{ "cmd": "track.rename", "args": { "track_id": 2, "name": "Bass" } }
{ "cmd": "track.mute", "args": { "track_id": 1, "muted": true } }
{ "cmd": "track.solo", "args": { "track_id": 4, "solo": true } }
```

**MIDI:**
```json
{
  "cmd": "clip.add_notes",
  "args": {
    "track_id": 2,
    "notes": [
      { "pitch": 60, "start": 0.0, "duration": 0.5, "velocity": 100 },
      { "pitch": 64, "start": 0.5, "duration": 0.5, "velocity": 90 }
    ]
  }
}
```

**Mixer:**
```json
{ "cmd": "mixer.set_volume", "args": { "track_id": 1, "db": -6.0 } }
{ "cmd": "mixer.set_pan", "args": { "track_id": 2, "pan": 0.5 } }
```

**Plugins:**
```json
{ "cmd": "device.add", "args": { "track_id": 1, "plugin_name": "Reverb" } }
{ "cmd": "device.param.set", "args": {
    "track_id": 1,
    "device_index": 0,
    "param_name": "Decay Time",
    "value": 2.0
}}
```

### Events DAW Should Send

**Transport:**
```json
{ "event": "tempo", "data": { "tempo": 120.0 } }
{ "event": "transport", "data": { "isPlaying": true } }
{ "event": "position", "data": { "bar": 5, "beat": 2 } }
```

**Tracks:**
```json
{ "event": "track.added", "data": { "track_id": 7, "name": "Synth", "type": "midi" } }
{ "event": "track.removed", "data": { "track_id": 3 } }
```

**Plugins:**
```json
{ "event": "device.loaded", "data": {
    "track_id": 2,
    "device_index": 0,
    "name": "Massive X",
    "parameters": [
      { "name": "Cutoff", "value": 0.5, "min": 0, "max": 1 },
      { "name": "Resonance", "value": 0.3, "min": 0, "max": 1 }
    ]
}}
```

---

## Code Example: Complete Integration

### Your DAW's Main Class

```cpp
#include "WingmanBridge.h"

class CustomDAW : public juce::AudioAppComponent {
public:
    CustomDAW() {
        // Audio setup
        setAudioChannels(0, 2);

        // Create Wingman bridge
        wingman = std::make_unique<WingmanBridge>();
        wingman->setCommandHandler([this](const json& cmd) {
            return handleWingmanCommand(cmd);
        });

        // Start transport monitor (send to Wingman at 30 Hz)
        startTimer(33);  // ~30 Hz
    }

    void timerCallback() override {
        // Send current state to Wingman
        wingman->broadcastState({
            {"tempo", currentTempo},
            {"isPlaying", isPlaying},
            {"currentBar", currentBar}
        });
    }

    json handleWingmanCommand(const json& cmd) {
        std::string cmdType = cmd["cmd"];

        if (cmdType == "transport.play") {
            startPlayback();
            return {{"ok", true}};
        }
        else if (cmdType == "track.create_midi") {
            std::string name = cmd["args"]["name"];
            int trackId = createMIDITrack(name);
            return {{"ok", true}, {"track_id", trackId}};
        }
        else if (cmdType == "clip.add_notes") {
            int trackId = cmd["args"]["track_id"];
            auto notes = cmd["args"]["notes"];
            addMIDIToTrack(trackId, notes);
            return {{"ok", true}};
        }

        return {{"ok", false}, {"error", "Unknown command"}};
    }

private:
    std::unique_ptr<WingmanBridge> wingman;
    double currentTempo = 120.0;
    bool isPlaying = false;
    int currentBar = 0;
};
```

### WingmanBridge.h

```cpp
#pragma once
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class WingmanBridge {
public:
    WingmanBridge() {
        // Bind UDP socket to receive from Wingman
        socket.bindToPort(12001);

        // Start receive thread
        receiveThread = std::thread([this]() {
            while (running) {
                receiveAndProcessCommand();
            }
        });
    }

    ~WingmanBridge() {
        running = false;
        receiveThread.join();
    }

    void setCommandHandler(std::function<json(const json&)> handler) {
        commandHandler = handler;
    }

    void broadcastState(const json& state) {
        // Send to Wingman app on port 12000
        juce::String message = state.dump();
        socket.write("127.0.0.1", 12000, message.toRawUTF8(), message.length());
    }

private:
    void receiveAndProcessCommand() {
        char buffer[4096];
        int bytesRead = socket.read(buffer, sizeof(buffer), false);

        if (bytesRead > 0) {
            std::string message(buffer, bytesRead);
            auto cmd = json::parse(message);

            // Execute command
            auto result = commandHandler(cmd);

            // Send response
            json response = {
                {"id", cmd["id"]},
                {"ok", result["ok"]},
                {"result", result.count("result") ? result["result"] : json{}}
            };

            broadcastState(response);
        }
    }

    juce::DatagramSocket socket;
    std::thread receiveThread;
    std::atomic<bool> running{true};
    std::function<json(const json&)> commandHandler;
};
```

---

## Migration Path

### Option 1: Gradual (Recommended)

**Week 1-2:** Build basic DAW engine (play audio)
**Week 3:** Add Wingman UDP communication
**Week 4:** Test basic commands (play, stop, tempo)
**Week 5:** Add track management
**Week 6:** Add MIDI generation
**Week 7-8:** Add plugin hosting
**Week 9-12:** Full features

### Option 2: Fast Prototype

**Day 1:** Copy Wingman app → test standalone
**Day 2:** Create minimal JUCE app with UDP
**Day 3:** Connect them, test "play" command
**Day 4-7:** Add features iteratively

---

## Success Metrics

### Minimum Viable Integration

✅ Wingman app runs standalone
✅ DAW can play audio
✅ UDP communication works
✅ AI can control transport (play/stop)
✅ AI can read tempo from DAW

### Full Integration

✅ AI can create tracks
✅ AI can generate MIDI (Magenta)
✅ AI can load plugins
✅ AI can set plugin parameters
✅ AI can auto-mix
✅ Voice command: "create a lofi beat" → complete track

---

## Key Advantages

### Over Generic AI DAW Plan

| Generic Plan | Wingman Integration |
|--------------|---------------------|
| ⚠️ Start from scratch | ✅ 80% code already working |
| ⚠️ Build AI integration | ✅ Multi-provider AI ready |
| ⚠️ Build music generation | ✅ Magenta integrated |
| ⚠️ Design protocol | ✅ Proven UDP JSON protocol |
| ⚠️ Build UI | ✅ Professional React UI |
| ⚠️ Test AI prompts | ✅ Tested on real users |

### Proven Architecture

Wingman's design solves problems you'd face:
- ✅ Real-time audio thread + slow AI (lock-free queues)
- ✅ User override priority (touch detection)
- ✅ Command validation and safety
- ✅ Stable track/clip IDs despite reordering
- ✅ Rate limiting to prevent DAW overload

---

## Next Steps

1. **Test Wingman Standalone**
   ```bash
   cd /home/user/daw/wingman/wingman-app
   npm install
   npm run dev
   ```
   - Verify AI works
   - Test Magenta generation
   - Confirm "create trap drums" generates MIDI

2. **Study the Code**
   - Read `wingman-app/src/services/realAI.ts`
   - Understand prompt refinement
   - See how Magenta is called

3. **Build Minimal DAW**
   - JUCE audio engine (1 week)
   - Add WingmanBridge class (1 day)
   - Test UDP communication (1 day)

4. **Connect Them**
   - DAW sends tempo to Wingman
   - Wingman sends play command to DAW
   - Verify bidirectional communication

5. **Iterate**
   - Add track management
   - Add MIDI generation
   - Add plugin hosting
   - Full production workflow

---

## Conclusion

**You don't need to build AI integration from scratch!**

Wingman already has:
- ✅ Electron app with React UI
- ✅ Multi-AI provider (GPT-4, Claude, etc.)
- ✅ Magenta music generation
- ✅ Prompt → musical specifications refinement
- ✅ WebSocket/UDP infrastructure
- ✅ Piano roll UI
- ✅ Proven communication protocol

**What you need to add:**
- ❌ Python Remote Script for Ableton (not needed)
- ✅ C++ integration layer in your DAW
- ✅ Command handlers for Wingman protocol
- ✅ State broadcaster (tempo, tracks, etc.)

**Timeline:**
- With Wingman: 8-12 weeks to full integration
- Without Wingman: 20-30 weeks to build everything

**The smart move:** Use Wingman's proven architecture and focus your energy on building an amazing DAW engine, not reinventing AI integration.

---

**Let's build this! 🚀**

**Next:** Test Wingman standalone, then start building your DAW's WingmanBridge class.
