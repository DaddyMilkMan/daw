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
- **Performance (honest, measured):** Zig ≈ C++. On the single serial filter a
  naive port was ~15% slower; on the voice-parallel SIMD workload (`simd_bench`,
  Zig `@Vector` vs C++ `-O3 -march=native` auto-vec) the two trade places within
  run-to-run noise — the tuned all-f32 Zig kernel even edged C++ in one run. Both
  go through LLVM, so machine code is comparable. **Conclusion: choosing Zig
  costs ~nothing on speed, but does not reliably beat C++ on raw throughput.**
  Pick Zig for ownership / transparency / `comptime` / explicit memory layout —
  not for a mythical speed win. "More control" lets you *reliably reach* C++'s
  performance ceiling; it doesn't raise it.

## Next
- Add a fair benchmark (C++ reference in its own TU).
- `@Vector` SIMD pass on a parallelizable kernel (e.g. multi-voice/oscillator).
- Grow `zenith_dsp.h` toward the full `zenith_platform.h` seam.
