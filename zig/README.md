# Zenith — Zig DSP spike

The seed of the native Zig engine. **Clean-room: 100% ours, no JUCE source.**
This proves the toolchain, the C-ABI seam, and the A/B-validation workflow that
every future kernel will follow (see `planning/roadmaps/ZIG_MIGRATION_MASTER_PLAN.md`).

## Files
- `zenith_dsp.zig` — native Zig SVF filter kernel, exported over a flat C ABI.
- `zenith_dsp.h` — the C ABI (first slice of the broader `zenith_platform.h` seam).
- `ab_harness.cpp` — feeds a real signal through the Zig kernel and a C++
  reference (the exact math from `instruments/ZenithFilter.cpp`), asserts they
  match bit-for-bit, and benchmarks both.
- `build.sh` — `zig build-lib` → `zig c++` link → run.

## Run
```bash
./build.sh        # needs zig (tested on 0.14.1) — nothing else
```

## What it establishes (2026-06-12)
- **Correctness:** Zig kernel matches the C++ reference to `0.000e+00` over
  480k samples — per-sample *and* block paths. The harness guards every change.
- **Interop:** Zig → C-ABI static lib → linked into C++ works end-to-end.
- **Performance (honest):** a *naive* port runs ~15–20% slower than C++ `-O2`
  on this serial filter; `comptime` branch-elimination did not close it. The
  benchmark also favors C++ (its reference is inlinable; the Zig kernel is an
  opaque library call), so "same ballpark" is the fair read. The real Zig wins
  are systems-level (graph, allocation, cache, threading) and earned DSP work
  (`@Vector` SIMD across voices/channels) — not free from a literal translation.

## Next
- Add a fair benchmark (C++ reference in its own TU).
- `@Vector` SIMD pass on a parallelizable kernel (e.g. multi-voice/oscillator).
- Grow `zenith_dsp.h` toward the full `zenith_platform.h` seam.
