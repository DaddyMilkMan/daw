# ZENITH — Zig DAW Master Build Plan

**The single source of truth for building Zenith as a zero-JUCE, clean-room Zig DAW.**
Read this first when starting Zenith work. Update the checkboxes as tasks complete.
Append to the Progress Log. Keep it honest — no "done" that isn't verified.

- **Owner:** Micah Cooley (solo)
- **Started:** 2026-06-12
- **Branch:** `zig-migration/phase-0-cleanup` (off `master`)
- **Toolchain:** Zig 0.14.1, Linux-first
- **Endgame:** A DAW whose engine, DSP, data model, instruments, and platform layer
  are 100% owned Zig. The only third-party code linked is unavoidable vendor
  *interfaces* (VST3 SDK headers, CLAP header, OS audio/MIDI syscalls).
- **CROSS-PLATFORM is a standing goal for everything** (Linux + Windows + macOS).
  Engine/DSP/data-model = portable Zig (endian-explicit formats, `std.fs`). ALL
  platform code (audio/MIDI/window/dialogs) sits behind a clean interface/seam so
  per-OS backends slot in later without touching the engine. Linux-first today;
  never bake Linux-only assumptions into engine-level code.

---

## 1. Vision & Decisions

- **Path B — greenfield.** We build a new Zig DAW from scratch. The legacy C++/JUCE
  codebase (`apps/desktop/`) is a **reference oracle only** — read it to understand a
  problem; never translate/ship its code. It gets deleted as Zig replaces it.
- **0% JUCE-derived** (owner decision 2026-06-12). Not for license (project is open
  source) but for genuine ownership/originality — the way Ableton/FL/Logic/Reaper all
  built their own. Own all *logic*; link only vendor *interfaces*.
- **Purity spectrum (per subsystem):** *pragmatic-pure* (own all logic, allow PD/MIT
  micro-libs like dr_libs for thankless leaves) is the default. *absolute-pure* (write
  even codecs against raw OS APIs) only where the learning/pride is worth it.
- **Performance reality (measured 2026-06-12):** Zig ≈ C++ within noise (LLVM both).
  Choosing Zig costs ~nothing on speed but does NOT magically beat C++. The real wins
  are ownership, transparency, `comptime`, explicit memory layout. Don't oversell speed.
- **Strategic wedge:** an AI-native DAW ("the DAW that produces with you"), modern GPU
  UI, Linux-first. Owning the stack serves the wedge — it doesn't replace the need to go
  deep on one thing. Do NOT chase "beat everyone at everything" breadth.
- **Conventions:** commit completed units proactively (no asking); branch off master;
  every kernel/subsystem gets a test or A/B harness; verify end-to-end, not just compile.

---

## 2. Architecture

- **The seam:** a flat C ABI (`zenith_platform.h`) separates the engine from platform
  services (audio/MIDI device, file I/O, plugin host, windowing). Lets us swap backends
  and keep the engine pure. Currently realized piecemeal (`audio_alsa.zig`,
  `midi_alsa.zig`); to be formalized into one header.
- **Layering (target):**
  `platform (devices/files/host/window)` → `dsp (kernels)` → `engine (transport,
  tracks, graph, mixer)` → `instruments/effects` → `app/UI`.
- **`zig/` module map (current):** `synth.zig`, `zenith_dsp.zig`(+`.h`), `wav.zig`,
  `audio_alsa.zig`, `midi_alsa.zig`, `demo.zig`, `main_render/play/live.zig`, `build.zig`.

---

## 3. Current State — what's built ✅

- ✅ **Synth** (`synth.zig`): 16-voice subtractive — polyBLEP saw → linear ADSR →
  Chamberlin SVF lowpass. Polyphonic, voice allocation. *(This is Zenith's own instrument,
  not a JUCE replacement.)*
- ✅ **DSP kernel** (`zenith_dsp.zig`): SVF filter over a C ABI; A/B bit-exact vs C++;
  voice-parallel `@Vector` SIMD variant. One filter — not a library.
- ✅ **WAV write** (`wav.zig`): 16-bit PCM mono/stereo.
- ✅ **Audio output** (`audio_alsa.zig`): ALSA playback — one-shot + streaming
  (`StreamOut`). Linux only, blocking write loop.
- ✅ **MIDI input** (`midi_alsa.zig`): ALSA sequencer, system-visible "Zenith MIDI In"
  port. Note on/off only.
- ✅ **Real-time loop** (`main_live.zig`): poll MIDI → synth → stream. Verified
  end-to-end (aplaymidi → port → synth → recorded WAV).
- ✅ **Live looper** (`transport.zig` + `sequence.zig` + `main_loop.zig`): tempo/looping
  transport, MIDI record + loop-aware replay + overdub. Verified: pass-0 input replays
  across later loops.

**That is ~1–2% of a full JUCE replacement, and the easiest part.** Everything in §4 below
is the rest.

---

## 4. The Full Scope — everything to build

Status: ✅ done · 🟡 partial · ❌ not started

### 4.1 Platform — Audio device I/O  *(JUCE: juce_audio_devices)*
- 🟡 ALSA **output** (one device, blocking)
- ❌ Real **RT audio callback** (dedicated high-priority thread, lock-free handoff, xrun-robust)
- ❌ Audio **input / capture** (record from mic/line)
- ❌ **Duplex** (simultaneous in+out for monitoring)
- ❌ Device **enumeration & selection** (list devices, sample rate, buffer size)
- ❌ **PipeWire-native** backend (Linux modern)
- ❌ **JACK** backend (Linux pro)
- ❌ **WASAPI** + **ASIO** (Windows)
- ❌ **CoreAudio** (macOS)
- ❌ Sample-rate conversion at the device boundary

### 4.2 Platform — MIDI  *(JUCE: juce_audio_devices MIDI + MidiMessage)*
- 🟡 ALSA seq **input**, note on/off only
- ❌ Full MIDI **message parsing**: CC, pitchbend, aftertouch (poly+channel), program change, SysEx, MIDI clock/transport, **MPE**
- ❌ MIDI **output** (to hardware / other apps)
- ❌ MIDI device **enumeration/selection**
- ❌ Sample-accurate **timestamping** (events placed at frame offsets in the block)
- ❌ Cross-platform MIDI (Windows/macOS)

### 4.3 Platform — Audio file formats  *(JUCE: juce_audio_formats)*
- 🟡 WAV **write** (16-bit only; ❌ 24/float write)
- ✅ WAV **read** (16/24/32-bit PCM + IEEE float32, multichannel, chunk-skipping)
- ❌ **AIFF**, **FLAC**, **Ogg/Vorbis** read/write
- ❌ **MP3** decode (patent-aware; consider PD decoder)
- ❌ **Streaming** large files from disk (don't load whole files into RAM)
- ❌ Sample metadata (loop points, root note, embedded markers)

### 4.4 Platform — Windowing / input / native UI shell  *(JUCE: juce_gui_basics/extra)*
- ❌ **Window creation**: X11, **Wayland**, Win32, Cocoa
- ❌ Input events: mouse, keyboard, scroll, touch, drag-drop
- ❌ HiDPI / scaling, multi-monitor
- ❌ Native **file dialogs**, **menus**, clipboard, system tray
- ❌ GPU surface creation (GL/Vulkan/Metal) for the renderer

### 4.5 Platform — Core runtime  *(JUCE: juce_core/events — mostly Zig stdlib)*
- 🟡 Strings/files/threads/time/alloc — **Zig stdlib covers this for free**
- ❌ **Message/event loop** + timers (UI + async)
- ❌ **Lock-free audio FIFOs** (SPSC ring buffers for RT↔non-RT handoff)
- ❌ Thread pool / async task system
- ❌ Logging, settings/config persistence

### 4.6 DSP toolkit  *(JUCE: juce_dsp)*
- ✅ SVF filter
- ✅ polyBLEP oscillator, ADSR *(in synth)*
- ❌ Broader filters (biquad/RBJ set, ladder/analog models — port the C++ ones we fixed)
- ❌ **FFT** (own radix-2/4 or KISS-style)
- ❌ **Convolution** (reverb / cab / IR)
- ❌ **Oversampling** / anti-aliasing helpers
- 🟡 Resampler — cubic Hermite (Catmull-Rom) point-read done (`resample.zig`); ❌ high-quality sinc/SRC for device-rate conversion
- ❌ Dither, gain/pan laws, metering (peak/RMS/LUFS), DC blocker
- ❌ Delay lines, modulation (LFOs, envelope followers)

### 4.7 Engine — the DAW core  *(was Zenith's own C++; rebuild in Zig)*
- 🟡 **Transport / clock / playhead**: tempo + looping playhead done; ❌ stop/record-arm, time signature, metronome, linear (non-loop) mode
- ❌ **Track model**: audio / MIDI / instrument tracks
- ❌ **Clip / region model** + **arrangement timeline**
- 🟡 **Mixer / routing graph**: per-track gain/pan → stereo master done (`mixer.zig`); ❌ sends, buses, mute/solo, metering, PDC (plugin delay compensation)
- 🟡 **Sequencer/playback**: loop-based MIDI scheduling done; ❌ timeline/arrangement playback
- 🟡 **Recording**: MIDI loop capture + overdub done; ❌ audio capture, punch in/out, quantize
- ❌ **Automation**: lanes, curves, sample-accurate application
- ❌ Audio-clip **streaming**, **warp / time-stretch**, pitch-shift
- ❌ Quantize / groove, comping (take folders)

### 4.8 Data model & persistence  *(JUCE: juce_data_structures — ValueTree/UndoManager)*
- 🟡 **Document/project model** (`project.zig`: Project/Track/Note, gain/pan/instrument): basics done; ❌ clips/regions, routing, plugin state
- ✅ **Undo/redo** (snapshot-based `History`)
- ✅ **Save/load** project files (`ZNPR` endian-explicit binary); ❌ versioning/migration beyond v1
- ❌ Change-notification / observable model for UI binding
- ❌ Auto-save, crash recovery

### 4.9 Plugin hosting  *(JUCE: juce_audio_processors)*  — hardest area
- ❌ **CLAP** host (open, C ABI, Zig-friendliest — **do first**)
- ❌ Plugin **scan / sandbox (out-of-process) / blacklist**
- ❌ **VST3** host (Steinberg C++ SDK — bind the interface, don't reinvent the format)
- ❌ **AU** host (macOS, Obj-C runtime)
- ❌ Plugin **parameter automation**, preset/state save-load
- ❌ Hosting plugin **editor windows** (embed the plugin's own UI)

### 4.10 Instruments & effects
- ✅ ZenithPolySynth (basic) in Zig
- 🟡 Sampler (`sampler.zig`): load mono sample, pitch per MIDI note, polyphonic + AR env. ❌ multisampling, velocity layers, loop points, stereo
- ❌ Synth depth: mod matrix, LFOs, sub/noise, unison, more filter models, FM/sync
- ❌ Stock effects: EQ, compressor, limiter, reverb, delay, distortion, chorus
- ❌ Preset system + content/sample library

### 4.11 GUI  *(JUCE: gui_basics/graphics/opengl — was ~91K LOC)* — the long pole
- ❌ **Renderer** decision: own GPU renderer vs Skia-via-Zig bindings vs other
- ❌ Widget/component framework + layout + event routing
- ❌ DAW views: **arranger/timeline**, **piano roll**, **mixer**, **transport**, browser, sample editor
- ❌ Theming/design system, meters/scopes, waveform drawing
- ❌ Accessibility, keyboard shortcuts

### 4.12 The AI wedge  *(the differentiator — after the core is playable)*
- ❌ Action-taking assistant with write access to the project model
- ❌ A few trustworthy "produce-with-you" jobs (gain-staging, mix-translate, MIDI generate/vary, session-debugger)
- ❌ Local/offline model path + provider integration

---

## 5. Milestone Ladder (ordered; each one is usable)

- ✅ **M1 — Zig makes sound** (synth → WAV + ALSA out)
- ✅ **M2 — Play live from MIDI** (ALSA seq in → synth, real-time)
- ✅ **M3 — Transport + MIDI record/loop**: `transport.zig` (clock/playhead, tempo,
  looping) + `sequence.zig` (timed MIDI capture, loop-aware playback) + `main_loop.zig`
  (live looper: play → record → replay → overdub). `zig build loop`. VERIFIED: scripted
  pass-0 notes replay bit-for-bit across loops 1-3 (input only in loop 0); unit tests for
  window-wrap + event layout pass.
- ✅ **M4 — Load & play samples**: WAV reader (`wav.zig`, 16/24/32-bit PCM + float32),
  cubic-Hermite resampler (`resample.zig`), polyphonic `sampler.zig`. Verified: one A3
  sample pitched across MIDI notes to within 0.4% of target frequency; WAV round-trip +
  interp unit-tested. `zig build sampler`.
- ❌ **M5 — Audio recording** (audio input + capture to timeline)
- 🟡 **M6 — Mixer** (`mixer.zig`): N mono tracks → per-track gain + constant-power pan
  → stereo master. Verified: synth+sampler mixed, channels differ (pan), pan-law unit
  test passes. ❌ buses/sends, mute/solo, metering (later).
- ❌ **M7 — CLAP plugin hosting** (host third-party instruments/effects)
- ❌ **M8 — Real RT audio callback + device selection** (proper RT thread, xrun-robust)
- 🟡 **M9 — Project model + save/load + undo** (`project.zig`): Project/Track/Note model,
  endian-explicit binary format (`ZNPR`, cross-platform), file save/load, snapshot-based
  undo (`History`). Verified: save→reload→play melody; round-trip + undo unit tests.
  ❌ change-notification/observable, auto-save, format migration.
- ❌ **M10 — GUI** (the long pole; renderer decision first)
- ❌ **M11 — VST3/AU hosting** (hardest; possibly last)
- ❌ **M12 — Cross-platform** (Windows/macOS audio+MIDI+window backends)
- ❌ **M13 — AI wedge**

---

## 6. The C-ABI seam (`zenith_platform.h`) — to formalize

```c
typedef struct zp_audio_buffer { float** channels; int32_t num_channels, num_frames; double sample_rate; } zp_audio_buffer;
typedef struct zp_midi_event { uint32_t frame_offset; uint8_t data[4]; uint8_t size; } zp_midi_event;

// audio device
typedef void (*zp_audio_callback)(void* user, const zp_audio_buffer* in, zp_audio_buffer* out, const zp_midi_event* midi, int32_t n);
int32_t zp_audio_open(const char* device, double sr, int32_t frames, zp_audio_callback cb, void* user);
void    zp_audio_close(void);
// midi
int32_t zp_midi_open_inputs(void);
int32_t zp_midi_send(int32_t port, const zp_midi_event* ev);
// plugin host (CLAP first; VST3/AU later)
typedef struct zp_plugin zp_plugin;
zp_plugin* zp_plugin_load(const char* uri);
void zp_plugin_process(zp_plugin*, zp_audio_buffer* io, const zp_midi_event* midi, int32_t n);
// files
int32_t zp_file_decode(const char* path, zp_audio_buffer* out_alloc);
int32_t zp_file_encode(const char* path, const zp_audio_buffer* in, int32_t format);
```

---

## 7. Honest scale & risks

- Full parity with what a real DAW does is a **1.5–2+ year** solo effort. We are at the start.
- Three genuinely hard areas: **plugin hosting** (VST3/AU SDK binding), **cross-platform
  device I/O**, **the GUI**. Everything else is "a lot of normal work," not research.
- Mitigation: each milestone is independently usable; ship continuously; A/B and verify
  end-to-end (M2 already caught a real bug that compile + unit-test missed).

---

## 8. Progress Log

**2026-06-12 (session 1)**
- Audit: legacy C++ docs heavily overclaim; build was red (systemic header/impl drift in
  never-compiled AI files); confirmed `modules/`, top-level `linux/mac/windows/`,
  `ai_client/` were dead/duplicate trees.
- Decision: full zero-JUCE Zig rewrite, Path B greenfield, 0% JUCE-derived, open source.
- **Deleted dead trees**: real source 2076 → 1101 files (−47%). (`ai_client/` deferred —
  diverged twin, salvage-diff before deleting.) Commit `498b5294`.
- Fixed legacy instruments (ZenithFilter etc.) enough to compile — but flagged
  rewrite-in-Zig; not pursuing further C++ repair.
- **M1 done**: Zig synth → WAV + ALSA out. Commit `17f876ad`.
- SIMD/perf experiment: Zig ≈ C++ (parity, not a win). Documented honestly.
- **M2 done**: live MIDI input (ALSA seq) + real-time streaming; verified e2e via
  aplaymidi; fixed a real cap-bit bug. Commit `54fbdc23`.
- Created this master plan (consolidates the prior migration doc).
- **M3 done**: live looper (transport + sequence + record/replay/overdub). Verified
  pass-0 input replays identically across loops 1-3. Unit tests for window-wrap + event
  layout. `zig build loop`.
- **M4 done**: WAV reader (16/24/32-bit + float32) + cubic-Hermite resampler + polyphonic
  sampler. Verified: one A3 sample pitched across MIDI notes to <0.4% freq error; WAV
  round-trip + interp unit tests. `zig build sampler`.
- **M6 (basic) done**: `mixer.zig` — N tracks → gain/pan → stereo master. Synth+sampler
  mixed; pan verified (|L-R| rms 3250); pan-law unit test. `zig build mixer`.
- Added **cross-platform** as a standing goal (vision §); platform code behind seams.
- **M9 done**: `project.zig` — Project/Track/Note model, `ZNPR` endian-explicit save/load,
  snapshot undo (`History`). Verified save→reload→play + round-trip/undo tests. `zig build project`.
- NEXT: **M5 — audio recording** (ALSA capture — the missing I/O direction; structure it
  behind a capture interface for cross-platform).

*(Add new dated entries as milestones complete.)*
