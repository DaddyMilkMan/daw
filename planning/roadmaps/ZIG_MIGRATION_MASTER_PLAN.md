# Zenith DAW — Zig Ownership Migration Master Plan

**Status:** Active program plan
**Created:** 2026-06-12
**Owner:** Micah Cooley (solo)
**Endgame:** Zero JUCE. A DAW whose engine, data model, DSP, and platform layer
are owned, in Zig, optimized specifically for Zenith — with C++ retained only
where it is genuinely irreplaceable (vendor plugin SDKs), and even that
quarantined behind a C ABI.

> This is a *staged* plan. The product boots and runs at every milestone.
> There is never an 18-month "rewrite in the dark" window. We reach 100%
> ownership by shrinking JUCE to nothing one subsystem at a time, behind a
> stable seam, measuring as we go.

---

## 0. Guiding principles

1. **Own the lines where ownership changes the outcome; rent the rest until you can replace it cheaply.** The hot path (graph, scheduler, DSP, data model, UI responsiveness) is where "perfectly optimized for this DAW" is *felt*. Device-driver glue and plugin-SDK plumbing are bottlenecked by code we don't control — own them last, or wrap them forever.
2. **The seam is the strategy.** A flat C ABI (`zenith_platform.h`) between the Zig engine and the platform services is what makes the migration incremental, reversible, and always-shipping.
3. **Never hand-repair C++ we intend to delete or rewrite.** Triage every broken file: *fix-keep*, *rewrite-in-Zig*, or *delete*. Only fix-keep files get C++ repair effort.
4. **Measure every replacement.** Each Zig kernel ships behind the same ABI as its C++ predecessor, A/B'd under a benchmark + golden-audio harness. C++ stays as fallback until Zig wins on correctness *and* speed.
5. **Boot beats green.** We do not need a fully-green 290K-LOC C++ build. We need the smallest coherent app that boots and runs as the migration baseline.
6. **0% JUCE-derived — clean-room, own everything.** (Owner decision, 2026-06-12.) We do NOT port or translate JUCE source — not for license reasons (project stays open source) but for genuine ownership and originality, the way Ableton/FL/Logic/Reaper all built their own. Own 100% of the **logic, architecture, and implementation** in Zig, written from scratch or from public **specifications** (CoreAudio/WASAPI/ALSA/PipeWire docs, the VST3 SDK, the CLAP spec). The only third-party things we *link* are the unavoidable vendor **interfaces** — the VST3 SDK headers (the plugin format itself), the CLAP header (open, MIT, ≈ native Zig), and the OS audio/MIDI syscalls. Everything above those interfaces is ours.
   - **Purity spectrum (per-subsystem choice):** *pragmatic-pure* = own all logic, link the unavoidable vendor interfaces, optionally lean on PD/MIT micro-libs (`dr_libs`, `miniaudio`) for the most thankless leaves (codecs/device backends) — recommended default. *absolute-pure* = write even codecs and device backends against raw OS APIs (WAV/AIFF/FLAC very doable; MP3/AAC are hard + patent-adjacent — defer or use a PD decoder). Choose absolute-pure only where the pride/learning is worth the time.
   - JUCE may still be read to *understand a problem*, but no JUCE code is translated or shipped. During the transition JUCE remains the temporary `platform_juce.cpp` seam implementation only, shrinking to zero as native Zig replaces it.

---

## Phase 0 — Stabilize the baseline (weeks, not months)

Goal: a **booting, running ZenithDAW** built from a single, de-duplicated source
tree, with CI that actually verifies it. This is required for *every* downstream
path and is mostly cleanup we want regardless of Zig.

### 0.1 Delete the dead half of the repo  *(P0)*
- `modules/` (945 files / ~297K LOC) — never compiled (no `add_subdirectory(modules)`). Diff against `apps/desktop/Source/` to confirm no unique fix lives only there, then **delete**.
- Top-level `linux/ mac/ windows/` — stale diverged duplicates of `apps/desktop/Source/platform/*`. Migrate any unique fix, then **delete**.
- `ai_client/` — orphaned twin of `ai/` (whole tree excluded from build). **Delete** after confirming `ai/` is canonical.
- Orphaned cmake module files (`cmake/Modules.cmake`, `Zenith{Core,DSP,UIUnified,EngineCore,Network,Commands,Effects,AI}.cmake`, `apps/desktop/cmake/ZenithLibraries.cmake`). **Delete.**
- **Effect:** removes ~half the codebase and ends all "which file is canonical?" ambiguity. Do this FIRST so we never repair a file in the dead tree.

### 0.2 Fix CI to point at the repo root  *(P0)*
- `.github/workflows/build.yml` runs `cmake -S apps/desktop` which has no `CMakeLists.txt`. Change to `-S .`. No green build is real until CI proves it.

### 0.3 Reconfigure for the current path  *(P0)*
- The existing `build/` was configured at the old path `/home/micah/Desktop/sylorlabs projects/zenith-daw` (note the space + lowercase). `compile_commands.json` still points there. Delete `build/`, reconfigure clean at the current path.

### 0.4 Build-repair TRIAGE — not "make everything green"  *(P0)*
The build is red because dozens of never-compiled, AI-generated files have
`.cpp`/`.h` drift against each other and against shared type defs. (Confirmed:
`ZenithFilter` had 6 distinct bugs; `ZenithPolySynthVoice` has ~8 references to
enum values/members that were never added.) **Do not blanket-fix these.** For
each broken subsystem, choose:

| Disposition | When | Action |
|---|---|---|
| **Fix-keep** | Load-bearing, on the boot path, kept through the whole transition (engine core, project/ValueTree model, transport, mixer routing, file I/O) | Reconcile header/impl, get it compiling |
| **Rewrite-in-Zig** | Prime Zig target AND badly broken (synth voice, filters, DSP kernels, oscillators) | Do NOT hand-repair. Minimally exclude/stub to reach boot, then rewrite natively in Zig in Phase 2 |
| **Delete** | Dead, duplicate, or speculative scaffold (modules/, ai_client/, dead collab/cloud, MCP if not pursued) | Remove |

- Acceptable for the baseline: the app boots with **no built-in instrument** if the synth is slated for Zig rewrite. Add the Zig synth later. A booting host > a perfect-but-unbuildable synth.

### 0.5 Honest status docs  *(P1, highest ROI/hour)*
- Strip every false "✅ Complete" badge (collaboration, stem-sep, genre, safety "100% coverage", VST3, instruments). Replace with real status. Point rendering docs at `ui/framework/SkiaOpenGLRenderer` (the real renderer; `rendering/SkiaRenderer` is dead).

**Phase 0 exit criteria:** one de-duplicated tree · `cmake -S . -B build` configures clean · ZenithDAW boots · CI is green on a real build · docs no longer lie.

---

## Phase 1 — Define the seam + first Zig kernel (weeks)

Goal: prove Zig interop *in this codebase* and establish the boundary the whole
migration rides on.

### 1.1 Extract a headless `zenith_core`  *(prereq for everything)*
- Today even `Engine.h`/`Track.h` pull in `juce_gui_basics`/`juce_graphics`, so the core can't build headless. Strip GUI includes from engine headers; make `zenith_core` a real static library target (not just include paths) that builds and tests with no GUI stack.
- This is the de-JUCE'ing groundwork — valuable even if we never wrote a line of Zig.

### 1.2 Author `zenith_platform.h` — the C ABI seam
The contract between the Zig engine and platform services. Flat C, no C++ types
crossing the boundary. Sketch:

```c
// zenith_platform.h — stable C ABI between Zig engine and platform services.
#ifdef __cplusplus
extern "C" {
#endif

typedef struct zp_audio_buffer {
    float**  channels;     // [numChannels][numFrames], non-interleaved
    int32_t  num_channels;
    int32_t  num_frames;
    double   sample_rate;
} zp_audio_buffer;

typedef struct zp_midi_event {
    uint32_t frame_offset;
    uint8_t  data[4];
    uint8_t  size;
} zp_midi_event;

// --- Audio device I/O (impl: JUCE first, native Zig/backend later) ---
typedef void (*zp_audio_callback)(void* user,
                                  const zp_audio_buffer* in,
                                  zp_audio_buffer* out,
                                  const zp_midi_event* midi, int32_t midi_count);
int32_t zp_audio_open (const char* device, double sr, int32_t buffer_frames,
                       zp_audio_callback cb, void* user);
void    zp_audio_close(void);

// --- MIDI I/O ---
int32_t zp_midi_open_inputs (void);
int32_t zp_midi_send        (int32_t port, const zp_midi_event* ev);

// --- Plugin host (impl: JUCE/VST3/AU; CLAP may go native Zig) ---
typedef struct zp_plugin zp_plugin;
zp_plugin* zp_plugin_load   (const char* uri);
void       zp_plugin_process(zp_plugin*, zp_audio_buffer* io,
                             const zp_midi_event* midi, int32_t midi_count);
int32_t    zp_plugin_param_count(zp_plugin*);
void       zp_plugin_set_param  (zp_plugin*, int32_t idx, float value);
void       zp_plugin_unload (zp_plugin*);

// --- Audio file decode/encode (impl: dr_libs/libsndfile — C, Zig-friendly) ---
int32_t zp_file_decode(const char* path, zp_audio_buffer* out_alloc);
int32_t zp_file_encode(const char* path, const zp_audio_buffer* in, int32_t format);

#ifdef __cplusplus
}
#endif
```

- Implement v1 of this seam in a **single quarantined C++ TU** (`platform_juce.cpp`) that uses JUCE for `zp_audio_*`, `zp_midi_*`, `zp_plugin_*`, `zp_file_*`. JUCE is now behind the seam, not under the whole app.

### 1.3 First Zig kernel — proof of interop
- Pick one pure leaf DSP kernel (start with the biquad/SVF, or a resampler). Implement in Zig, expose via the same flat C signature, link into the C++ build via `zig build-lib`/`zig cc`.
- Stand up a **benchmark + golden-audio A/B harness**: same input → C++ vs Zig output, assert bit-tolerance + compare CPU. Keep C++ as fallback.
- **Decision gate:** confirm Zig interop ergonomics, build integration (Zig + CMake), and perf are acceptable *before* committing to the broader rewrite.

**Phase 1 exit criteria:** headless `zenith_core` lib · `zenith_platform.h` implemented via quarantined JUCE TU · one Zig DSP kernel shipping behind the seam, A/B-validated.

---

## Phase 2 — Migrate the hot path to Zig (months, incremental, always shipping)

Replace, one subsystem at a time, behind the seam. Order = (optimization payoff
× tractability) ÷ risk:

1. **DSP kernels** (filters, oscillators, FFT/spectral, resampler, dither, dynamics) → Zig. Highest payoff, cleanest leaves, comptime + SIMD shine. Each A/B'd.
2. **Built-in instruments** (ZenithPolySynth/Sampler) → Zig, on top of the Zig DSP kernels. (This is why we don't hand-repair the broken C++ synth — it's reborn here.)
3. **Audio graph / scheduler / mixer** → Zig engine core, driven by the `zp_audio_callback`. The real-time path becomes fully owned.
4. **Project data model** → replace `juce::ValueTree` (656 refs / 104 files) with a native Zig document model + observer + undo. Biggest single job; do it behind an interface so the UI migrates incrementally.
5. **CLAP host** → native Zig (CLAP is C, Zig-friendly). VST3/AU **stay** on the JUCE-backed seam (vendor C++/Obj-C SDKs; ~zero optimization payoff, brutal to rebuild).

At each step JUCE's footprint shrinks. The app boots and runs the whole time.

---

## Phase 3 — Shrink JUCE to the irreducible core (later)

Replace the remaining JUCE-backed seam implementations where worth it:

- **Audio device I/O** → native backends (CoreAudio / WASAPI / ALSA+PipeWire / JACK). High effort, ~zero sonic payoff (bottleneck is the driver), perpetual maintenance — do only when ownership matters more than the maintenance tax.
- **MIDI I/O** → native per-platform. Medium.
- **Windowing / input / native dialogs** → the UI already renders in Skia (245 files), but `juce::Component` is still the widget base in 96 files and `juce::Graphics` lingers in 71. Finish the Skia migration, then replace the windowing/event layer (GLFW/SDL or hand-rolled per-platform).
- **VST3/AU hosting** → realistically the **last/never** to leave JUCE. Even Reaper binds the vendor SDKs rather than reinventing them. If JUCE survives anywhere, it is here — quarantined in `platform_juce.cpp`, a few percent of its original surface.

**Endgame:** JUCE is either gone or reduced to a quarantined plugin-host adapter behind `zenith_platform.h`. The engine, DSP, data model, instruments, and (optionally) device I/O are owned Zig.

---

## Honest cost framing

- Phase 0: weeks. Mostly deletion + triage + CI. Do regardless of Zig.
- Phase 1: weeks. The seam + first kernel. The real go/no-go on Zig.
- Phase 2: months, incremental, shipping throughout. This is where the Zig payoff compounds.
- Phase 3: optional, ongoing. Device I/O and windowing only if/when ownership beats the maintenance tax. VST3/AU likely never.
- A *full* from-scratch replacement of everything (incl. audio backends, VST3/AU host, windowing) is ~1.5–2+ yrs of foundation work for a solo dev before *today's* parity. The phased plan reaches ~90% of the ownership payoff in a fraction of that, while shipping — and leaves the last 10% as a deliberate later choice, not an upfront bet.

---

## Strategic guardrail (from the competitive audit)

"Beat Ableton/FL/Logic at everything" is unwinnable solo and dilutes the build.
The migration above is the *means*; the *wedge* is a deep, action-taking,
AI-native DAW ("the DAW that produces with you") on a modern GPU UI, Linux-first.
Owning the stack in Zig serves that wedge — it does not replace the need to pick
one and go deep. Reach plain parity on the ~15 non-negotiables (warp markers +
content library first); do not chase breadth.

---

## Appendix — Phase 0 work already started (2026-06-12)

**0.1 Dead-tree deletion — DONE (on branch `zig-migration/phase-0-cleanup`, recoverable from git history):**
- Deleted `modules/` (982 files / ~297K LOC — verified diverged fork, never compiled, no `add_subdirectory`), top-level `linux/ mac/ windows/` (57 files — stale dupes of `apps/desktop/Source/platform/*`), and 10 orphaned cmake files (`ZenithCore/DSP/EngineCore/AI/Commands/AudioUtils/Network/Testing/UIUnified/Effects.cmake` — none `include()`d by the active build).
- Verified safe first: nothing compiled `#include`s `modules/`; command sources come from `apps/desktop/Source/commands/`; no active cmake reference to any deleted path.
- **Result: real source files 2076 → 1101 (-47%).** Build config intact.
- **DEFERRED:** `ai_client/` (88 files / 32K LOC) — a *diverged* twin of the canonical `ai/` (every feature exists in both). Worth a diff-for-salvage pass (AI is the strategic wedge) before deleting, not a blind `rm`.

**MILESTONE "Zig makes sound" — DONE (2026-06-12, Path B greenfield, `zig/`, no JUCE):**
- Pure-Zig subtractive synth: polyBLEP saw -> linear ADSR -> Chamberlin SVF lowpass, 16-voice polyphonic (`synth.zig`).
- WAV writer (`wav.zig`) + ALSA device backend over libasound (`audio_alsa.zig`).
- `zig build render` -> `zenith_hello.wav` (verified: peak 95% FS, musical structure = arpeggio C-E-G-C then held C-major chord). `zig build play` -> live ALSA playback ran clean (open/set_params/writei/drain/close).
- `build.zig` drives it. This is the seed of the real engine — all forward effort now lands here, not on the throwaway C++.
- NEXT: real-time audio callback loop (synthesize on the fly, low latency) instead of pre-render-then-play; then MIDI input ("play Zenith live from a keyboard, in Zig"); then formalize the device side of `zenith_platform.h`.

**1.3 First Zig kernel — DONE as a spike (`zig/`, builds + runs with zig 0.14.1, no JUCE):**
- `zig/zenith_dsp.zig` — native Zig SVF filter over a flat C ABI (`zig/zenith_dsp.h`, first slice of `zenith_platform.h`).
- `zig/ab_harness.cpp` — A/B vs the C++ `ZenithFilter::processSVF` reference. Result: **bit-exact (`0.000e+00`)** across per-sample and block paths; toolchain + C-ABI interop proven end-to-end.
- **Honest perf finding:** naive port ~15–20% slower than C++ `-O2` on this serial filter; `comptime` branch-elimination did not close it (benchmark also favors C++'s inlinable reference). Conclusion: Zig's payoff is control/transparency + systems-level + earned SIMD work, NOT free speed from literal translation. Don't oversell it in planning.

**Source fixes (validated by isolated syntax-compile against the real JUCE config):**
- `instruments/ZenithFilter.{h,cpp}` — reconciled header↔impl; **fixed the per-sample
  state-reset bug** (analog filters now hold integrator state and actually filter);
  corrected enum names; replaced the broken `processSEM` (undefined-var) with a stable
  TPT SVF; added a local `softClip`. `ZenithFilter.cpp` now compiles clean (RC 0).
- `instruments/ZenithPolySynthDefs.h` — extended `FilterModelType` with Moog/MS20/SEM/TB303.
- `instruments/ZenithPolySynthVoice.{h,cpp}` — fixed nonexistent `juce::ADSR::isNoteActive()`
  → idiomatic `!isActive()`; reconciled `computeLFOValue` signature; renamed
  `setAmpEnv/setModEnv` → `setAmpEnvelope/setModEnvelope` (match real callers); added
  `applyModulationToDestination` decl; removed bogus `.active` writes. **Still has
  pre-existing drift** (references `isActive_`, `ModulationSource::{Amp,Mod}Envelope`,
  `LFOWaveform::Random`, `getNotePressure()`, `filter1Cutoff_` that don't exist) — this
  file is a **rewrite-in-Zig candidate**, not worth full C++ reconciliation. Disposition: minimally
  stub/exclude to boot, then rebuild in Phase 2.
- `instruments/CMakeLists.txt` — fixed reference to nonexistent `ZenithSamplerEditor_Mod.cpp`.
