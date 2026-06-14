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
- ✅ Audio **input / capture** (`StreamIn`, ALSA; verified capturing a tone) — ❌ to-timeline, duplex
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
- 🟡 Delay lines + reverb (`effects.zig`: feedback delay, Freeverb-style reverb); ❌ modulation (LFOs, env followers), chorus/flanger

### 4.7 Engine — the DAW core  *(was Zenith's own C++; rebuild in Zig)*
- 🟡 **Transport / clock / playhead**: tempo + looping playhead done; ❌ stop/record-arm, time signature, metronome, linear (non-loop) mode
- 🟡 **Track model** (`project.zig`: instrument/gain/pan/clips per track); ❌ audio tracks, track types
- 🟡 **Clip / region model** + **arrangement timeline** (`arrangement.zig`: clips at frame positions, linear multi-track scheduler, record-to-clip); ❌ audio clips, loop regions, clip editing
- 🟡 **Mixer / routing graph**: per-track gain/pan → stereo master done (`mixer.zig`); ❌ sends, buses, mute/solo, metering, PDC (plugin delay compensation)
- 🟡 **Sequencer/playback**: loop-based + linear timeline scheduling done (`arrangement.zig`); ❌ advanced (swing, latency-comp scheduling)
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
- 🟡 **CLAP** host (`clap_abi.zig`/`main_clap.zig`): load/instantiate/activate/process + sample-accurate **note events** to instruments verified (hosted plugin played a scale 8/8); ❌ audio/note-port extensions, param events, real-plugin/dir scanning, GUI
- ❌ Plugin **scan / sandbox (out-of-process) / blacklist**
- ❌ **VST3** host (Steinberg C++ SDK — bind the interface, don't reinvent the format)
- ❌ **AU** host (macOS, Obj-C runtime)
- ❌ Plugin **parameter automation**, preset/state save-load
- ❌ Hosting plugin **editor windows** (embed the plugin's own UI)

### 4.10 Instruments & effects
- ✅ ZenithPolySynth (basic) in Zig
- 🟡 Sampler (`sampler.zig`): load mono sample, pitch per MIDI note, polyphonic + AR env. ❌ multisampling, velocity layers, loop points, stereo
- ❌ Synth depth: mod matrix, LFOs, sub/noise, unison, more filter models, FM/sync
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
- 🟡 DAW views: transport + timeline + **interactive mixer** (play toggles, faders/pans drag → state) wired to the live window; ❌ arranger/piano-roll editing, browser, sample editor
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
- 🟡 **M5 — Audio recording** (`audio_alsa.zig` `StreamIn` + `main_record.zig`): ALSA
  capture → WAV. Verified end-to-end (recorded a 440Hz tone, measured 440.4Hz). ❌ capture
  to the timeline/clips, duplex monitoring, punch in/out.
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
- NEXT: tempo-sync the loop to the UI BPM; record-arm (capture → disk via wav.zig); UMP *output* +
  high-res 32-bit controllers driving synth params; native rawmidi UMP for hardware; SVG stroke
  geometry; Windows/macOS audio+MIDI backends.

*(Add new dated entries as milestones complete.)*
