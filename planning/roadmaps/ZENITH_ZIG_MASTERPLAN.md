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
- 🟡 ALSA **output** (one device, blocking) — `StreamOut`
- ❌ Real **RT audio callback** (dedicated high-priority thread, lock-free handoff, xrun-robust)
- ✅ Audio **input / capture** (`StreamIn`, ALSA) — ✅ **to-timeline** (`audio_track.Recorder` → `AudioClip`); ❌ duplex
- ❌ **Duplex** (simultaneous in+out for monitoring)
- ❌ Device **enumeration & selection** (list devices, sample rate, buffer size)
- ❌ **PipeWire-native** backend (Linux modern)
- ❌ **JACK** backend (Linux pro)
- ❌ **WASAPI** + **ASIO** (Windows)
- ❌ **CoreAudio** (macOS)
- ❌ Sample-rate conversion at the device boundary

### 4.2 Platform — MIDI  *(JUCE: juce_audio_devices MIDI + MidiMessage)*
- ✅ ALSA seq **input** (auto-connect every source + hot-plug), MIDI 2.0 **UMP** input (`midi2_alsa.zig`) with MIDI-1.0 fallback (`midi_alsa.zig`)
- ✅ **Message parsing — notes + CC + pitch bend + channel & poly aftertouch + program change** on BOTH paths (the MIDI-1.0 fallback rebuilds canonical bytes → shared `midi2.fromMidi1`, so it decodes to the same Messages as the UMP path). **Velocity reaches the synth** (scales amplitude + brightness); mod wheel→vibrato, CC7/CC11→expression gain, **CC64 sustain pedal**, CC71→resonance, bend→pitch, all routed live (`routeMidi`).
- ✅ **MPE** — per-channel pitch bend / pressure / CC74-timbre voiced per-note, plus MIDI 2.0 native per-note pitch bend + poly pressure + **Registered Per-Note Controllers** (index 74 → per-note timbre); routed in-order through the engine's tagged event queue (`Ev`) to the synth's per-voice expression. ❌ MPE zone configuration (RPN 6)
- ✅ **SysEx** (`sysex.zig`): Universal SysEx classification (identity, GM, master volume, tuning) + manufacturer ids; received on both paths (UMP SysEx7 reassembly + MIDI-1.0 ext data); replies to a device **Identity Request** with Zenith's identity. ❌ vendor-specific editors, bulk dump
- ✅ **MIDI clock / transport sync** (`midi_clock.zig`): Start/Continue/Stop drive the transport; the 24-ppqn clock estimates external tempo (`ClockSync`). ❌ song-position-pointer locate, send-clock-out (Zenith as master)
- ✅ MIDI device **enumeration + selection**: `Midi2Input.listSources`/`connectOnlyMatching`; `ZENITH_MIDI_IN=<name>` picks a source, OR the **in-DAW transport-bar dropdown** (frosted-glass, hot-plug-refreshed) selects one live (else auto-connect all).
- 🟡 MIDI **output** — UMP out via `midi2_alsa.Midi2Output` (echoes input + sends SysEx replies); ❌ routing MIDI tracks/clips to hardware, dedicated out device picker
- 🟡 Sample-accurate **timestamping**: clip-note onsets are sample-accurate (`renderSegmented`/`offsetInBlock` split the synth render at exact frame offsets); ❌ sub-block offsets for *live* input (applied at block start — no input timestamp)
- ❌ Cross-platform MIDI (Windows WinMM/WinRT, macOS CoreMIDI) — **not implementable/verifiable on this Linux box**; the `MidiInput`/`Midi2Input` API is the seam a platform backend would sit behind

### 4.3 Platform — Audio file formats  *(JUCE: juce_audio_formats)*
- ✅ WAV **write** (`wav.zig`: 16-bit + **24-bit** + **32-bit float**, mono/multichannel)
- ✅ WAV **read** (16/24/32-bit PCM + IEEE float32, multichannel, chunk-skipping)
- ✅ **AIFF** read/write (`aiff.zig`: 8/16/24/32-bit signed BE PCM + 80-bit extended sample-rate codec)
- ✅ **FLAC** + **Ogg/Vorbis** decode *and* encode (`codec.zig` — libsndfile via hand-declared ABI; round-trip + real-file decode verified). *Codec "leaf" lib, not clean-room; future purity swap = vendored PD dr_flac/stb_vorbis.*
- ✅ **MP3** decode (`mp3.zig` — libmpg123 hand-declared ABI; verified against a real .mp3; patents expired 2017)
- ✅ **Unified loader** `audio_file.loadAny` — dispatches by extension across WAV/AIFF (owned) + FLAC/Ogg/MP3 (leaves) → one `AudioData`
- ✅ **Streaming** from disk (`wav.WavStream`: header-only open + random-access `readFrames`, never loads the whole file) + `audio_track.StreamClip` (reads its span per render block; verified bit-equal to the in-memory render). ❌ streaming for compressed formats (FLAC/Ogg/MP3 still whole-file)
- ❌ Sample metadata (loop points, root note, embedded markers)

### 4.4 Platform — Windowing / input / native UI shell  *(JUCE: juce_gui_basics/extra)*
- 🟡 **Window creation**: X11 — **borderless/custom chrome** (Motif hints), **live resize**
  (ConfigureNotify → recreate framebuffer + reflow), move/resize/min/maximize via EWMH
  `_NET_WM_MOVERESIZE` (cross-WM). Verified resizing through normal/large/small/ultra-wide.
  ❌ Wayland, Win32, Cocoa
- 🟡 Input events: X11 mouse + keyboard + close + resize done; ❌ scroll, touch, drag-drop, other platforms
- ❌ HiDPI / scaling, multi-monitor
- ❌ Native **file dialogs**, **menus**, clipboard, system tray
- ❌ GPU surface creation (GL/Vulkan/Metal) for the renderer

### 4.5 Platform — Core runtime  *(JUCE: juce_core/events — mostly Zig stdlib)*
- 🟡 Strings/files/threads/time/alloc — **Zig stdlib covers this for free**
- ❌ **Message/event loop** + timers (UI + async)
- 🟡 **Lock-free audio FIFOs** (SPSC note queue in `audio_engine.zig`); ❌ generic ring-buffer util
- ❌ Thread pool / async task system
- 🟡 Logging + **observability/instrumentation** — the **Talkback** harness (`window_glx.zig`): (a) headless snapshot (`inspect.zig`: JSON state + PNG screenshot + event log; `png.zig`); (b) **live scripted control** — `window_glx` replays a `ZENITH_SCRIPT` of input (move/moveid/clickid/rmove/down/up/key/wait/shot/dumpids/quit) into the real GPU app and grabs `glReadPixels` PNGs, no app-UI changes. By-id targeting via the `uireg.zig` widget registry (the "DOM") is the default; by-pixel is the fallback. The triad: **console** (logs) + **DOM** (`dumpids`) + **screenshots** (`shot`), timestamp-correlated. **Verified: drove the real showcase + DAW — transport, faders, mute/solo, pan, send knobs, and the piano-roll editor all responded, captured in screenshots** (`tools/talkback/`). ❌ settings/config persistence, an input-injection API for in-process tests

### 4.6 DSP toolkit  *(JUCE: juce_dsp)* — `dsp.zig` toolkit landed (17 tests)
- ✅ SVF filter
- ✅ polyBLEP oscillator, ADSR *(in synth)*
- ✅ Broader filters: full RBJ biquad set (`effects.zig`: LP/HP/peak/**bandpass/notch/allpass/low+high shelf**) + **Moog 4-pole ladder** (`dsp.Ladder`)
- ✅ **FFT** (in-place iterative radix-2, fwd/inv, windows: Hann/Hamming/Blackman-Harris) — `dsp.fft`
- ✅ **Convolution**: FFT offline (`dsp.convolveFft`) + streaming time-domain FIR (`dsp.FirConvolver`)
- 🟡 **Oversampling**: 2× linear-phase windowed-sinc up/down (`dsp.Oversampler2x`); ❌ 4×/8×, polyphase
- 🟡 Resampler — cubic Hermite (Catmull-Rom) point-read done (`resample.zig`); ❌ high-quality sinc/SRC for device-rate conversion
- ✅ gain/pan laws (`dsp.dbToGain`/`panConstantPower`), metering (`dsp.PeakMeter`/`RmsMeter`/**`LoudnessMeter` LUFS per BS.1770**), DC blocker (`dsp.DcBlocker`); ❌ dither
- ✅ Dynamics: **compressor** (soft-knee, attack/release), **lookahead brickwall limiter**, **noise gate** (`dsp.Compressor`/`Limiter`/`Gate`)
- ✅ Saturation/waveshaping (`dsp.softClip`/`hardClip`/`Saturator`)
- 🟡 Delay lines + reverb (`effects.zig`); ✅ modulation: LFO (sine/tri/saw/square) + envelope follower (`dsp.Lfo`/`EnvelopeFollower`); ❌ chorus/flanger

### 4.7 Engine — the DAW core  *(was Zenith's own C++; rebuild in Zig)*
- 🟡 **Transport / clock / playhead**: tempo + looping playhead done; ❌ stop/record-arm, time signature, metronome, linear (non-loop) mode
- 🟡 **Track model**: MIDI/instrument tracks (`project.zig`) + **audio tracks** (`audio_track.zig`: `AudioTrack` with gain/pan/mute/solo/record-arm); ❌ unify audio tracks into the saved project format, track folders/groups
- 🟡 **Clip / region model** + **arrangement timeline**: MIDI clips (`arrangement.zig`) + **audio clips** (`audio_track.zig`: `AudioClip` — sample buffer at a timeline frame, file-backed via WAV); **MIDI clip editing** done (`pianoroll.zig`: add/remove/move/resize notes + velocity, live in the DAW). ❌ loop regions, audio-clip trim/fades
- 🟡 **Mixer / routing graph**: design `mix_graph.zig` (channels/buses/sends/metering); **LIVE in the DAW** (`audio_engine.zig`): per-track stems → **FX chain (high-pass + compressor)** → gain/pan/**mute/solo** (`mixBlocks`) → master, with a **post-fader reverb send** per channel into a shared `effects.Reverb` bus. The mixer faders/mute/solo/sends drive the real audio; per-track + reverb-bus levels published to the meters + audio monitor. Verified live (solo isolates; send knob 0.28→1.0 raised the reverb bus 0.20→0.40). ❌ PDC, LUFS at master, more FX slots, fader automation wiring
- 🟡 **Sequencer/playback**: MIDI timeline (`arrangement.zig`) + **sample-accurate audio-clip playback** (`AudioTrack.render`) + **sample-accurate synth-clip onsets** (`audio_engine.renderSegmented` splits the block at exact note frames — was quantized to the 256-sample block); ❌ advanced (swing, latency-comp scheduling)
- 🟡 **Recording**: MIDI loop capture + overdub + **audio capture-to-timeline** (`audio_track.Recorder`: feed captured frames → finalize into a clip at the record position; WAV round-trip); ❌ punch in/out, quantize, monitoring
- ✅ **Automation** (`automation.zig`): breakpoint lanes, hold/linear interpolation, binary-search `valueAt`, sample-accurate `render` over a block (verified driving a gain ramp). ❌ wiring lanes to track/plugin params in the live engine; bezier curves
- 🟡 Audio-clip **streaming** (`wav.WavStream`/`StreamClip` — done for WAV); ✅ **warp / time-stretch** (`timestretch.zig`: WSOLA, pitch-preserving) + **pitch-shift** (stretch+resample, length-preserving) — FFT-verified. ❌ formant-correct pitch, transient preservation
- ❌ Quantize / groove, comping (take folders)

### 4.8 Data model & persistence  *(JUCE: juce_data_structures — ValueTree/UndoManager)*
- 🟡 **Document/project model** (`project.zig`: Project/Track/Note/Clip + **audio clips** (`AudioClipRef`: file path + timeline start + source offset/length), gain/pan/instrument): MIDI + audio material; ❌ routing, plugin state in the project
- ✅ **Undo/redo** (snapshot-based `History`)
- ✅ **Save/load** project files (`ZNPR` endian-explicit binary, **v3** = audio clips; version-gated deserialize reads older v2); ❌ migration tooling beyond version-gating
- ❌ Change-notification / observable model for UI binding
- ❌ Auto-save, crash recovery

### 4.9 Plugin hosting  *(JUCE: juce_audio_processors)*  — hardest area
- 🟡 **CLAP** host (`clap_abi.zig`/`main_clap.zig`/`clap_host.zig`): load/instantiate/activate/process + sample-accurate **note events** (scale 8/8) + **audio-ports/note-ports/params extensions** (exact clap.h layouts) + **real-plugin directory scanning** (`findClapFiles`) + **state save/load** (`clap.state` ext + host streams; round-trip verified). Verified: `zig build clapscan` reads descriptors → instantiates → reports ports + params → saves/loads state (Gain/Brightness restored). ❌ param *events* (host→plugin automation), GUI hosting, out-of-process sandbox
- 🟡 Plugin **scan** (CLAP dir scan done); ❌ sandbox (out-of-process), blacklist
- 🟡 **VST3** host (`vst3_abi.zig`/`vst3_host.zig`): clean-room COM ABI from the SDK headers — TUID encoding (non-COM big-endian), `IPluginFactory`/`IComponent`/`IAudioProcessor`/**`IEventList`** vtables, `ProcessData`/`ProcessSetup`/`AudioBusBuffers`/`BusInfo`/**`Event` (48-byte, union @24)** layouts. Loads a module → factory → "Audio Module Class" → createInstance(IComponent) → initialize → queryInterface(IAudioProcessor) → setupProcessing → activateBus (audio+**event**) → setActive → **process() with host IEventList**. Verified: `zig build vst3` hosts an *instrument* — host delivers a note event, plugin synthesizes, **FFT confirms 439.5 Hz** (A4), and **state round-trips** via a host `IBStream` (getState/setState). ✅ **Real-plugin hosting validated**: built a genuine VST3 from the official Steinberg SDK (`tools/realvst3/plugin.cpp` — compiler-generated C++ COM vtables), installed it as a standard `~/.vst3` bundle, hosted it through the full **install→scan→host→uninstall** lifecycle (439.5 Hz + state round-trip), then removed it. ❌ IEditController (param display/automation), real IHostApplication context, plugin editor
- 🟡 **VST2** host (`vst2_abi.zig`/`vst2_host.zig`): clean-room AEffect (no SDK — Steinberg withdrew it) + dispatcher opcodes + host callback + **VstEvents/VstMidiEvent + effProcessEvents**. Loads a .so → `VSTPluginMain` → AEffect (magic-checked) → open/setSampleRate/setBlockSize/resume → **MIDI note via effProcessEvents** → processReplacing(). Verified: `zig build vst2` hosts an *instrument* — **FFT confirms 439.5 Hz** (A4), and **state round-trips** via **chunks** (effGetChunk/effSetChunk). ❌ params/programs UI, real third-party .so testing
- ❌ **AU** host (macOS, Obj-C runtime)
- ❌ Plugin **parameter automation**, preset/state save-load
- ❌ Hosting plugin **editor windows** (embed the plugin's own UI)

### 4.10 Instruments & effects
- ✅ `synth.zig` — **professional hybrid subtractive synth**: 16 voices × (2 band-limited oscillators [sine/tri/saw/square+PWM] + square sub + noise), **unison** (up to 7 detuned), oscB semitone/fine detune + osc mix; **two ADSRs** (amplitude + a dedicated **filter envelope**), **2 LFOs** (vibrato + cutoff), **key-track + velocity** cutoff mod, **glide**, quietest-voice stealing; velocity + sustain + full **per-note/per-channel (MPE)** expression. `Patch` presets (init_saw/fat_bass/super_lead/warm_pad). RT-safe (control-rate modulation).
- ✅ **Filters** (`filter.zig`, Phase 1 of the "flagship synth" plan): **zero-delay-feedback** topologies — Cytomic TPT state-variable (LP/HP/BP/notch, stable to Nyquist) + a **saturating ZDF Moog ladder** (tanh feedback = analog growl, self-limiting). Selectable per patch + `drive`. (Replaced the old Chamberlin SVF.)
- 🔭 **Flagship-synth roadmap** (the "best-sounding, most control, beginner↔expert gating" plan — see Progress Log 2026-06-14 s6): P1 filters ✅ → **P2** oversampling (4× polyphase on the nonlinear path) + analog drift/imperfection + **stereo** out → **P3** generalized **mod matrix** + macros → **P4** per-patch **FX rack** (drive/chorus/delay/reverb/EQ) → **P5** **wavetable** then **FM** engines → **P6** synth **UI** (3 tiers: Play/Shape/Build) + **preset browser** + factory library → **P7** microtuning/scales, MPE polish, voice modes.
- ❌ Other synth depth: osc **hard-sync**, comb/diode filters, preset save/load format.
- 🟡 Sampler (`sampler.zig`): load mono sample, pitch per MIDI note, polyphonic + AR env. ❌ multisampling, velocity layers, loop points, stereo
- 🟡 Stock effects (`effects.zig`): biquad EQ (LP/HP/peak), feedback delay, reverb done; ❌ compressor, limiter, distortion, chorus + mixer integration (per-track FX chains)
- ❌ Preset system + content/sample library

### 4.11 GUI  *(JUCE: gui_basics/graphics/opengl — was ~91K LOC)* — the long pole
- 🟡 **Renderer**: pure-Zig 2D renderer — **subpixel/AA vector text**, AA rounded rects,
  shadows, gradients, **box blur (glassmorphism)** (`render2d.zig`). **OpenGL (GLX) present
  backend** (`window_gl.zig`) uploads the framebuffer as a GPU texture, and a **GLSL
  fragment shader does the glassmorphism blur on the GPU in real time** (verified: animated
  moving frosted panel, 375 fps-frames). ❌ full GL-native vector drawing, Vulkan/Metal
- 🟡 Window + input layer: X11 native window + blit + mouse/keyboard done (`window_x11.zig`/`main_window.zig`); ❌ Wayland/Win32/Cocoa, scroll/drag
- 🟡 Widget/component framework: immediate-mode toolkit done (`uikit.zig`: button/vFader/hSlider, hot/active model); ❌ layout system, more widgets
- 🟡 DAW views: transport + timeline + **interactive mixer** (faders/pan/mute/solo/sends drive real audio + FX) + **piano-roll / clip editor** (`pianoroll.zig`: key×time grid renders a clip's notes; **click to add/remove**, **drag to move** (re-auditions on pitch change), **drag the right edge to resize** (grid-quantized), **velocity lane** at the bottom; notes colored by velocity; **wired into the DAW** — `E` opens the editor over the selected clip, edits feed the live engine via a double-buffered sequencer so they play back instantly, and a `note-on/off` audition hook drives the synth on grab/release. Standalone `zig build pianoroll` too. Verified end-to-end via Talkback). ❌ multi-note select/marquee, copy/paste, browser, sample editor
- 🟡 Theming + meters/faders drawn; ❌ waveform drawing, scopes, full design system
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
- 🟡 **M5 — Audio recording** (`audio_alsa.zig` `StreamIn` + `main_record.zig` + `audio_track.zig`):
  ALSA capture → WAV, and **capture → timeline clip** (`Recorder` feeds blocks → finalizes an
  `AudioClip` at the record position; verified `zig build audiotrack` → 662 Hz round-trip).
  ❌ duplex monitoring, punch in/out.
- 🟡 **M6 — Mixer** (`mixer.zig`): N mono tracks → per-track gain + constant-power pan
  → stereo master. Verified: synth+sampler mixed, channels differ (pan), pan-law unit
  test passes. ❌ buses/sends, mute/solo, metering (later).
- 🟡 **M7 — CLAP plugin hosting** (`clap_abi.zig` + `main_clap.zig` host + `clap_test_plugin.zig`):
  hand-declared CLAP ABI, dlopen → factory → descriptor → create → activate → process.
  Verified: hosted plugin generated a 440Hz sine through the host. ❌ MIDI events to
  plugins, audio/note-port extensions, directory scanning, GUI hosting, real-plugin testing.
- ❌ **M8 — Real RT audio callback + device selection** (proper RT thread, xrun-robust)
- 🟡 **M9 — Project model + save/load + undo** (`project.zig`): Project/Track/Note model,
  endian-explicit binary format (`ZNPR`, cross-platform), file save/load, snapshot-based
  undo (`History`). Verified: save→reload→play melody; round-trip + undo unit tests.
  ❌ change-notification/observable, auto-save, format migration.
- 🟡 **M10 — GUI** (the long pole): **foundation started** — pure-Zig software 2D renderer
  (`render2d.zig`: rects/gradients/alpha/text), generated 8x16 bitmap font (`font_data.zig`),
  BMP writer (`bmp.zig`), and a real DAW frame (`main_ui.zig`: transport + clip timeline from
  project data + mixer with faders/meters). `zig build ui`. ❌ live windowing (X11/Wayland/
  GLFW), input/interaction, the editor views, GPU acceleration.
- 🟡 **M11 — VST3/AU hosting** (hardest): VST3 audio path proven — clean-room COM ABI
  (`vst3_abi.zig`) + host (`vst3_host.zig`/`main_vst3.zig`) load a module, instantiate
  IComponent+IAudioProcessor, and pull audio through process() (`zig build vst3`, peak 0.50
  tone from a Zig VST3 test plugin). ❌ params/state/events, real third-party .vst3, VST2, AU.
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
- **M5 done**: `StreamIn` ALSA capture + `main_record.zig`. Verified end-to-end recording
  a 440Hz tone (measured 440.4Hz). Audio I/O now bidirectional in Zig. `zig build record`.
- **Arrangement done**: project model → clips (format v2); `arrangement.zig` linear
  multi-track scheduler + record-to-clip. Verified: 2-track song, clips at timeline
  positions, lead enters mid-song (pan visible in stereo), save→reload→play. `zig build arrange`.
- **M7 (CLAP host) done (foundation)**: `clap_abi.zig` (hand-declared CLAP ABI),
  `main_clap.zig` host, `clap_test_plugin.zig` (sine .clap). Verified end-to-end: host
  dlopens the .clap, instantiates, processes — hosted plugin rendered a 440Hz sine.
  `zig build clap`. Next for real plugins: events, audio/note-port extensions, scanning.
- This session shipped M1–M9 + arrangement + CLAP host. **The hardest area (plugin
  hosting) is now proven feasible.** Remaining big pieces: GUI (long pole), cross-platform
  backends, real-plugin CLAP/VST3/AU, depth (effects/automation).
- **GUI foundation done**: pure-Zig software 2D renderer + generated bitmap font + BMP
  output + a real DAW frame (transport/timeline-from-project-data/mixer). `zig build ui`.
- **GUI #1 (live windowing) done**: `window_x11.zig` native X11 window + blit + input;
  verified on screen (250 frames, 1950 events). `zig build window`.
- **#2 (effects) done**: `effects.zig` — biquad EQ, feedback delay, Freeverb reverb.
  Verified: delay echoes at exact intervals (0.6ⁿ decay), reverb RT60 tail, biquad tests.
  `zig build fx`. Next: compressor/limiter + per-track FX chains in the mixer.
- **#3 (CLAP note events) done**: host sends sample-accurate note on/off to a hosted CLAP
  instrument (`clap_test_plugin.zig` now a poly synth). Verified: scale played 8/8 correct
  pitches. Real third-party instruments drive the same way. Next: port extensions + scanning.
- **#4 (interactive UI) done**: `uikit.zig` immediate-mode toolkit; mixer faders/pans
  draggable + play button toggles, wired into the live X11 window. Verified headlessly
  (scripted input: play toggled, fader 0.85→0.04). `zig build uitest` / `zig build window`.
- **BATCH COMPLETE (#1-#4):** live windowing, effects, real-plugin CLAP (note events),
  interactive UI — all done + verified.
- **UI quality batch done (4/4):** subpixel text, eased hover/press animation, layout
  engine + knob/toggle widgets, glassmorphism blur + **OpenGL (GLX) GPU present backend**.
  The renderer now looks web-grade (AA/subpixel text, rounded, shadows, frosted glass) and
  presents on the GPU. Remaining visual: blur/draw in a GL *shader* (now CPU-blur→GPU-present).
- NEXT options: piano-roll/clip editing, mixer FX chains (wire effects.zig per track),
  compressor/limiter, GL-shader rendering, Wayland/cross-platform backends, real-plugin scanning.

**2026-06-13 (session — GPU UI toolkit + web-grade rewrite)**
- Researched how web UI reaches its quality (cited report `planning/research/web-ui-rendering.md`)
  then built a from-scratch **GPU 2D renderer** (`gpu2d.zig`) — the GPUI architecture in Zig:
  per-primitive instanced shaders, one draw call per type, **linear-light sRGB + premultiplied**
  compositing, 4× MSAA. Primitives: **SDF rounded rects** (+ hairline border, gradient, material
  depth), **analytic gaussian shadows** (closed-form erf, no blur pass), triangles/lines, **atlas
  text** (stem-darkened, pixel-snapped), and **dual-Kawase backdrop blur / frosted glass**.
- Built our own **flexbox layout engine** (`flex.zig`) — declarative `row/col/grow/percent/fit/
  gap/pad/justify/align`, two-pass measure→arrange, hover/press/click, `rectOf` for overlays.
  Fixes the "hand-computed pixel coords drift / hard for AI to author" problem.
- Extracted reusable modules + named them for open-source reuse: `color.zig`, `widgets.zig`
  (GPU faders/knobs/sliders), `TOOLKIT.md` manifest (per-component dep footprint, AGPL-3.0,
  "original Zig, techniques borrowed not code").
- **Consolidated DAW** `daw.zig` + `main_daw.zig` (`zig build daw`, binary **`zenith`**): the
  ENTIRE DAW (title/browser/arrangement/mixer) laid out by flex, GPU-rendered, interactive,
  with a design-identity pass (cleaner gradients, crisp hairlines, restrained accent) and a
  **frosted-glass value tooltip** on fader/knob hover. Decoupled from the CPU `ui.zig`.
- The CPU path (`render2d.zig`/`ui.zig`/`uikit.zig`/`window_x11.zig`) is now legacy/superseded.
- NEXT: piano-roll on flex, mixer FX chains, more overlays/menus (glass), Wayland backend,
  MSDF text (scale-independent), retire the demo executables once `zenith` covers them.

**2026-06-13 (session 2 — toolkit depth: glass chrome, tables, images, MIDI 2.0, runtime fonts)**
- **Glass title-bar chrome** + body→blur→glass→controls render reorder (committed earlier this session).
- **Real waveform**: synthesized actual drum samples + peak analysis (replaced the fake procedural shape).
- **Typeface variety set** (`tools/genfont.py`): Fira Sans/Noto Serif/Fira Mono/Fira Condensed/Open
  Sans/Cantarell baked; showcase **TYPEFACES** section.
- **Rendered data table** (`widgets.table`): rounded surface, borderless tracked header + hairline,
  zebra rows, hover + selected (accent wash/left bar), right-aligned **tabular** numeric columns.
- **PNG image import** (`image.zig` — own decoder: 8-bit gray/RGB/palette/gray+alpha/RGBA, all 5
  filters, tRNS, std-zlib IDAT only) → `gpu2d.GpuImage` (SRGB8_ALPHA8) + an instanced **image
  pipeline** (`g.image`/`imageUv`, tint + premultiplied alpha). Showcase shows a decoded PNG
  natural + tinted with alpha rounded corners. *(SVG vector rasterizer = next.)*
- **MIDI 2.0 / UMP** (`midi2.zig`): 32/64-bit packets, 16 groups × 16 ch (256-ch), 16-bit velocity,
  32-bit CC/bend, per-note controllers + per-note pitch bend, bank-aware program change; the spec's
  min-center-max scaling (canonical vectors verified) + default MIDI 1.0→2.0 translation;
  `midi_alsa.MidiEvent.toUmp()` bridge. 9 tests in `zig build test`.
- **Runtime TrueType rasterizer** (`ttf.zig`): parses glyf/cmap(0/4/6/12)/loca/hmtx, flattens
  quadratic beziers, scan-converts (analytic-H + 4× vertical SS) into the same `font.Font` the GPU
  atlas consumes. `rasterizeAscii()` loads **any installed .ttf at runtime** (563 on this box) — the
  real path to "thousands of fonts." Showcase shows 3 baked + 3 runtime system fonts side by side.
- **Custom cursors** (`window_glx.setCursor` + CursorShape, core X cursor font, cached): DAW shows
  resize arrows on edges, grabbing while dragging, hand over clickables; showcase mirrors it.
- **Drag effect**: showcase draggable "Drag me" chip — grab → follows cursor with a lift (shadow +
  scale) + grabbing cursor. Rounds out hover/drag/cursor (the "things web has" effects).
- **SVG vector import** (`svg.zig`): viewBox; `<path>` M/L/H/V/C/S/Q/T/Z; rect(+rx)/circle/ellipse/
  line/poly; solid fills + fill-opacity (#hex/rgb/named); AA scan-convert (nonzero/even-odd) → the
  same `image.Image` as PNG → `GpuImage`. Showcase shows PNG (raster) + SVG (vector) side by side.
  "import svgs pngs any image type" ✓ (arcs approximated; gradients/text/full-stroke out of scope).
**2026-06-13 (session 3 — full MIDI 2.0 spec + live audio in the DAW)**
- **MIDI 2.0 brought to the full current spec** (`midi2.zig`, M2-104-UM v1.1.2): all 8 UMP message
  types — Utility (NOOP/JR Clock/JR Timestamp/Delta Clockstamp+TPQN), System, MIDI 1.0 CV, MIDI 2.0
  CV (full opcode set), SysEx7, SysEx8 (8-bit-clean), Flex Data (tempo/time-sig/key), UMP Stream
  (endpoint/function-block discovery, clip markers) + big-endian `toBytes`. 16 tests.
- **Audio device/channel enumeration** (`audio_devices.zig`, `zig build audioin`): lists every PCM
  device the OS exposes via ALSA name hints (PipeWire 1..128ch@384k, Pulse, HDMI hw surround; 67 here)
  + a live capture meter (received real audio).
- **Real-time audio in the live `zenith` app** (`audio_engine.zig`): output thread renders the project
  loop + a polyphonic synth, paced by the audio clock (ALSA→PipeWire), UI↔audio via atomics + a
  lock-free SPSC note queue. **Pressing play now SOUNDS** — verified by recording the output monitor
  (120 BPM drum envelope; peak 0.92). Playhead + VU meters now driven by the real signal.
- **Capture + MIDI 2.0 routed into the engine**: input thread meters live mic (meters react to it);
  `main_daw` opens an ALSA-seq port, up-converts incoming events to UMP (MIDI 2.0) and plays them on
  the synth. Verified live: `aplaymidi`→ Zenith seq client 128 → UMP → synth → sustained chord in the
  recorded output (gap-floor RMS 0.011→0.225).
- **NATIVE MIDI 2.0 now live** (`midi2_alsa.zig`): registers Zenith as a real UMP MIDI 2.0 endpoint
  via ALSA's native UMP seq API (`snd_seq_set_client_midi_version(UMP_MIDI_2_0)` +
  `snd_seq_ump_event_input`), reading raw 32-bit UMP words into `midi2.Ump`. The kernel delivers
  genuine UMP and translates legacy 1.0 senders to MIDI 2.0 for us. Verified (alsa-lib 1.2.11, kernel
  7.0): client shows `[User UMP MIDI2]`; a legacy note vel=100 arrived as native UMP MT=4
  w0=0x40903C00 w1=0xC9240000 → 16-bit vel 0xC924 (the kernel's 2.0 xlate, matching our
  `scaleUp(100,7,16)`). End-to-end: external MIDI → kernel UMP → synth → recorded sound. `main_daw`
  prefers native; legacy 1.0+our-up-convert is the fallback.
- **Universal device compatibility** (auto-connect + hot-plug): both `midi2_alsa` and `midi_alsa`
  auto-subscribe every readable MIDI source at startup and rescan on System-Announce topology events,
  so any device — 1983 DIN, USB 1.0, native 2.0 — just works, no manual `aconnect`. Audio engine
  tries default→pipewire→pulse→plughw→hw. Verified: a virtual keyboard appearing AFTER launch
  auto-connected and played (recorded). Input excludes our own output client (no MIDI loop).
- **UMP OUTPUT + high-res mapping**: `midi2_alsa.Midi2Output` emits native UMP from a 'Zenith Out'
  source (16-bit vel, 32-bit CC/bend). The 32-bit controllers drive the synth — CC74→cutoff,
  CC71→resonance, pitch-bend→pitch (±2 st), pressure→brightness (lock-free atomics, applied per
  block; synth gained a bend multiplier). Verified: listener on 'Zenith Out' got 32-bit CC sweep +
  bend→0xFFFFFFFF; isolated synth bent 262→292 Hz (+1.90 st). Fixed vFader thumb centering.
- NEXT: tempo-sync the loop to the UI BPM; record-arm (capture → disk via wav.zig); per-note (MPE-
  style) controllers → per-voice; native rawmidi UMP for hardware; SVG stroke geometry; Win/mac
  audio+MIDI backends.

**2026-06-14 (session — backend depth campaign: DSP toolkit)**
- User directive: build out the backend ("the backend logic of JUCE") — real plugin
  hosting (VST3/VST2/CLAP), full audio tracks + recording, deeper DSP toolkit (§4.6),
  complete file formats (§4.3). Recon: `external/JUCE` is the reference oracle; VST3 SDK
  headers vendored under `build/_deps/juce-src/.../VST3_SDK/pluginterfaces`; no third-party
  plugins installed (verify hosting against test plugins we build).
- **DSP toolkit landed** (`dsp.zig`, clean-room vs juce_dsp): FFT (radix-2 fwd/inv +
  windows), convolution (FFT offline + streaming FIR), 2× oversampler, dynamics
  (compressor/limiter/gate), metering (peak/RMS/**LUFS** BS.1770), Moog ladder, saturation,
  LFO + envelope follower, gain/pan laws, DC blocker. Extended `effects.zig` Biquad with the
  full RBJ set (bandpass/notch/allpass/low+high shelf). **17 dsp + 5 effects tests pass**
  (`zig build test`). §4.6 now mostly ✅.
- **File formats**: WAV 24-bit + float32 write (`wav.zig`), clean-room **AIFF** read/write
  (`aiff.zig`, incl. 80-bit extended sample-rate codec). 6 round-trip tests. §4.3 mostly ✅;
  FLAC/Ogg/MP3 still pending.
- **CLAP real-plugin hosting**: extension ABI (audio-ports/note-ports/params, exact clap.h
  layouts) in `clap_abi.zig`; test plugin now declares 1 stereo out / 1 note in / 2 params;
  `clap_host.zig` scanner (`findClapFiles` + `scanFile` + `queryPorts`/`paramsExt`);
  `zig build clapscan` verifies end-to-end. (No third-party .clap installed on this box —
  path proven against our real .clap.)
- **VST3 hosting (foundation)**: clean-room COM ABI (`vst3_abi.zig` — IIDs, vtables, struct
  layouts transcribed from the SDK headers) + `vst3_host.zig` (module/bundle load, factory,
  createInstance, queryInterface) + `vst3_test_plugin.zig` (Zig .so tone generator) +
  `main_vst3.zig`/`zig build vst3`. Pulled non-silent audio through process() end-to-end.
  Fixed a flaky test race (wav tests in two binaries → process-unique temp paths).
- **VST2 hosting (foundation)**: clean-room AEffect ABI (`vst2_abi.zig`) + host
  (`vst2_host.zig`/`main_vst2.zig`) + Zig VST2 test plugin (`vst2_test_plugin.zig`).
  `zig build vst2` drives the AEffect lifecycle and pulls non-silent audio through
  processReplacing(). **Plugin-hosting trio (CLAP + VST3 + VST2) now all load + play.**

**2026-06-14 (session 2 — production-grade hosting: instruments play notes)**
- **MIDI/event input to hosted instruments, all 3 formats** — the leap from "hosts a tone"
  to "hosts a playable instrument". Each host now delivers note events and the test plugins
  are real poly-sine synths; verified by **FFT on the output** (dominant freq == the note):
  - VST3: added `Event` (48-byte, union @ offset 24 — matches SDK) + `IEventList` to
    `vst3_abi.zig`; host implements IEventList; plugin grew an event-input bus + synth.
    `zig build vst3` → 439.5 Hz from MIDI 69.
  - VST2: added `VstEvents`/`VstMidiEvent` + `effProcessEvents` to `vst2_abi.zig`; host sends
    a MIDI note; plugin synth. `zig build vst2` → 439.5 Hz.
  - CLAP: already routed note events (`zig build clap`, scale 8/8) — trio now consistent.
- **Plugin state save/load — all 3 formats** (so projects can persist plugin settings):
  CLAP `clap.state` ext + host streams (`IStream`/`OStream`); VST3 `IComponent` getState/setState
  over a host-implemented `IBStream` (memory buffer); VST2 `effGetChunk`/`effSetChunk` + the
  `effFlagsProgramChunks` flag. Each verified by a state round-trip in its `zig build` driver.
- **Plugin state save/load — all 3 formats** (CLAP `clap.state`+streams, VST3 `IBStream`
  getState/setState, VST2 chunks); each round-trip-verified in its build driver.
- **Audio tracks + recording-to-timeline** (`audio_track.zig`): `AudioClip` (sample buffer at
  a timeline frame, WAV-backed), `AudioTrack` (gain/pan/mute/solo, sample-accurate `render`),
  `mixTracks` (mute/solo-aware), `Recorder` (feed captured blocks → finalize a clip at the
  record position). 5 unit tests + `zig build audiotrack` (2-track timeline placement + a
  660 Hz recording round-trip, FFT-verified).
- **Audio tracks wired into the project + engine**: project format **v3** persists audio clips
  (`AudioClipRef`, file-referenced; round-trip verified, version-gated deserialize); `audio_track.fromProject`
  loads them into renderable tracks (honoring source-offset/length); the live `audio_engine`
  now mixes audio tracks (stereo) at a linear timeline playhead alongside the synth/loop
  (`zenith` builds clean). **Full pipeline verified offline**: record → WAV → project → reload →
  timeline render (the recorded take plays at its frame).
- **Compressed file formats — FLAC/Ogg/MP3 decode** (`codec.zig` via libsndfile, `mp3.zig` via
  libmpg123, `audio_file.loadAny` dispatcher). Hand-declared C ABIs (no -dev headers); the libs
  are linked by absolute versioned-.so path (no unversioned symlinks on this box). FLAC/Ogg
  round-trips (encode→decode→FFT) + real `singing.ogg` decode + real `.mp3` decode all verified.
  Pragmatic codec leaves, not clean-room — future purity swap to vendored PD single-headers.
- **Audio-file streaming** (`wav.WavStream` + `audio_track.StreamClip`): open reads only the
  header; `readFrames` seeks + decodes just the requested span (16/24/32 PCM + float). A
  `StreamClip` renders its overlapping window per block straight from disk — verified bit-equal
  to the in-memory render, block-by-block. §4.3 file formats now essentially complete (compressed
  streaming + sample metadata remain).
- **Mixer depth + automation lanes** (both headless-verified, 5+5 tests):
  `mix_graph.zig` — channels→bus/master routing, post-fader aux sends, mute/solo, peak/RMS
  metering at every node. `automation.zig` — breakpoint lanes (hold/linear, binary-search
  `valueAt`, sample-accurate `render`), verified driving a gain ramp.
- **Warp/time-stretch + pitch-shift** (`timestretch.zig`, WSOLA) — FFT-verified.
- **Observability/instrumentation** (`inspect.zig` + `png.zig` + `zig build inspect`): a headless
  snapshot (the seed of the **Talkback** harness) — JSON state snapshot (the DOM/a11y-tree analog), PNG screenshot
  (`png.zig` minimal encoder, viewable + decodes via our own `image.zig`), and a structured event
  log. Lets an agent observe the full app state off-screen. ❌ live-app hookup (the running
  `zenith` calling `inspect.snapshot/screenshot` on a key/IPC) + input-injection control channel.
- NEXT in campaign (ordered): **live-app instrumentation hookup** (zenith dumps a snapshot +
  screenshot on demand) + **GUI record-arm + RT-safe capture** → **wire mix_graph + automation
  into the live engine/project** → plugin param automation + VST3 IEditController.

**2026-06-14 (session 3 — piano-roll editing + tool naming: Talkback & Trellis)**
- **Piano-roll / clip editing, finished + wired into the DAW** (`pianoroll.zig`): drag-move
  notes (column offset preserved, re-auditions on pitch change), drag the right edge to
  **resize** (grid-quantized), a **velocity lane** at the bottom strip, click-empty to add +
  drag, click-note to delete (disambiguated by whether the cell changed); notes colored by
  velocity; optional `audition` hook fires note-on on grab / note-off on release.
  Integrated into `daw.zig`/`main_daw.zig`: **`E`** opens the editor over the selected clip
  (dimmed backdrop), edits feed the live engine through a **double-buffered sequencer**
  (write the inactive buffer, flip an atomic index — RT-safe) so changes play back instantly,
  and the audition hook drives the engine synth. Standalone `zig build pianoroll` retained.
  **Verified end-to-end via Talkback**: scripted add/move/resize/velocity → screenshots +
  the audition/engine logs confirmed each edit.
- **Tooling renamed off the placeholder names** (the user's "don't just copy Playwright/flex"):
  - `flex.zig` → **`trellis.zig`** (the **Trellis** layout engine); `main_flex`/`main_flexmix`
    → `main_trellis`/`main_trellismix`; build steps `flex`/`flexmix` → `trellis`/`trellismix`;
    all `@import`/aliases updated.
  - the scripted-control harness is now **Talkback** (`Automation` struct → `Talkback` in
    `window_glx.zig`); `tools/control/` → `tools/talkback/` (scripts, README, `.gitignore`,
    docs paths all updated). Same observe-and-drive triad (console + DOM + screenshots).
  - Full `zig build test` green after both renames.

**2026-06-14 (session 4 — production-grade MIDI + velocity)**
- **Velocity end-to-end** (it was being dropped at every layer — every note played
  at one loudness). `synth.zig`: voices carry normalized velocity → scales amplitude
  AND opens the filter (soft = darker). Engine `NoteEv`/`pushNote` carry velocity;
  the clip sequencer plays each note at its real velocity. `vel7`/`vel16` helpers.
  Tests: velocity scales output; sustain holds a released note.
- **Expression controllers**: mod wheel (CC1) → 5.5 Hz vibrato LFO, CC7/CC11 →
  expression gain, **CC64 sustain pedal** (note-off defers while pedal down; pedal-up
  frees lifted keys, edge-detected on the RT thread), plus the existing CC71/CC74/
  bend/aftertouch — all routed in `routeMidi`.
- **Sample-accurate clip-note onsets**: was quantized to the 256-sample block (~5.3 ms);
  now `renderSegmented`/`offsetInBlock` split the synth render at exact note frames.
  Tests on the wrap math + silence-before-onset.
- **Full MIDI parsing in the MIDI-1.0 ALSA fallback** (`midi_alsa.zig`): added the
  `control` event variant + parsing for CC / program change / channel & poly aftertouch /
  pitch bend; `MidiEvent.toUmp` rebuilds canonical bytes → shared `midi2.fromMidi1`, so
  the fallback decodes to the SAME Messages as the UMP path. (The MIDI 2.0 UMP path
  already routed CC74/bend/pressure + has UMP output.)
- Verified live via Talkback: opened the editor on a clip in the running `zenith`, the
  sequencer drove the synth (audio monitor showed synth level rise, transport PLAYING).
  Full `zig build test` green. **Honest scope:** the *instrument* is still a single-saw
  subtractive seed — expression now reaches it, but synth depth (multi-osc/wavetable/mod
  matrix) is the next lever; MPE per-note voicing, SysEx, and MIDI-clock sync remain.

**2026-06-14 (session 5 — finish MIDI + a professional synth)**
- **MIDI clock/transport sync** (`midi_clock.zig`): Start/Continue/Stop drive the DAW
  transport; 24-ppqn clock estimates external tempo (`ClockSync`, smoothed). System
  messages decode on both paths (`midi2.decode` → `SystemMsg`).
- **SysEx** (`sysex.zig`): Universal SysEx classification + manufacturer ids, UMP
  SysEx7 (de)packetization, received on both paths; replies to Identity Requests with
  Zenith's identity (manufacturer 0x7D).
- **Device enumeration + selection**: `listSources`/`connectOnlyMatching`; `ZENITH_MIDI_IN`
  picks a source. Verified live (logged "Midi Through: …", excludes own ports).
- **Professional synth rewrite** (`synth.zig`): 2 osc + sub + noise, unison, oscB detune,
  TWO ADSRs (amp + a dedicated filter envelope), 2 LFOs, key-track/velocity cutoff, glide,
  `Patch` presets. Replaces the single-saw seed; RT-safe; API-compatible with the engine.
- **MPE per-note voicing**: the engine note queue became a tagged event queue (`Ev`) so
  per-channel + per-note pitch bend / pressure / timbre apply in-order to the right voice;
  `routeMidi` routes them (MPE-1.0 per-channel + MIDI 2.0 per-note). Synth carries
  per-voice + per-channel expression.
- Tests across all of it (clock tempo lock, sysex round-trips, synth velocity/sustain/
  filter-env/MPE-isolation/presets, engine MPE routing). Verified live: the DAW plays a
  clip through the new synth, no NaN/xruns. **Honest gap:** cross-platform MIDI
  (Win/macOS) is NOT done — can't build/verify CoreMIDI/WinMM on this Linux box; the
  `MidiInput`/`Midi2Input` API is the seam a platform backend slots behind. Synth still
  wants wavetables + a real mod matrix + a UI to be truly "flagship."

**2026-06-14 (session 6 — close MIDI gaps + the flagship-synth plan, Phase 1)**
- **MIDI 2.0 Registered Per-Note Controllers** decode (`per_note_controller`); index 74 → per-note timbre (`engine.noteTimbre` → synth `note_timbre`). Last MPE decode gap closed.
- **In-DAW MIDI device picker**: a frosted-glass dropdown in the transport bar (lists "All sources" + enumerated devices, hot-plug-refreshed) → `connectAllSources`/`connectOnlyMatching`. Verified live via Talkback (selected "Midi Through", button relabeled).
- **Flagship-synth vision** (the user's ask: best-sounding + most control + beginner↔expert gating + a sound library). The plan, recorded so it persists:
  - *Sound* — priority order: the **filter** (ZDF + saturation), **oversampling the nonlinear path** (not just oscillators), **analog drift/imperfection**, **stereo + an FX rack**. Plus more **engines** (wavetable, FM, …) behind one voice architecture.
  - *Control* — everything modulatable: a real **mod matrix** (any source→any dest) + per-voice/MPE (already ours, a differentiator).
  - *The gate* — one patch, **three depths**: **Play** (pick by vibe + 4–8 labeled macros + XY morph), **Shape** (classic front panel), **Build** (full matrix/engines). **Macros are the bridge** — experts wire what beginners turn; semantic controls ("dark↔bright") + tasteful randomize/morph.
  - *Sounds* — a versioned patch format, a tag/audition **preset browser**, 150–300 curated factory presets, designed/A-B'd via Talkback+FFT.
  - *Sequence* — P1 filters → P2 oversampling+drift+stereo → P3 mod matrix+macros → P4 FX rack → P5 wavetable+FM → P6 UI(3 tiers)+browser+library → P7 microtuning/MPE polish.
- **Phase 1 done** (`filter.zig`): ZDF TPT-SVF + saturating Moog ladder, selectable per patch + drive; replaced the Chamberlin SVF. Synth modulation moved to control-rate for RT headroom. 11 tests; ~8s live playback through the ladder with **zero xruns**.

*(Add new dated entries as milestones complete.)*
