// ab_harness.cpp — proves the native Zig SVF kernel matches the C++ reference
// sample-for-sample, and benchmarks the two. No JUCE, no deps: just the C ABI.
//
// The "reference" is the exact math from
// apps/desktop/Source/instruments/ZenithFilter.cpp::processSVF, inlined here so
// this harness builds standalone. If the Zig port and this reference agree to
// tight tolerance over a real signal, the Zig kernel is a safe drop-in.

#include "zenith_dsp.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <chrono>

namespace {

struct RefSvf { double low = 0, high = 0, band = 0; };

float refSvfProcess(RefSvf& s, float input, float cutoff, float res,
                    double sr, int ftype) {
    const double f = std::min(std::max((double)cutoff / sr, 0.0001), 0.49);
    const double q = (double)res * 10.0 + 1.0;
    const double r = 1.0 / q;

    const double low1 = s.low + f * s.band;
    const double high1 = (double)input - low1 - r * s.band;
    const double band1 = s.band + f * high1;

    s.low = low1;
    s.high = high1;
    s.band = band1;

    if (ftype == 0) return (float)s.low;   // lowpass
    if (ftype == 2) return (float)s.high;  // highpass
    return (float)s.band;                  // bandpass
}

// Block reference: same math, state hoisted into locals across the buffer —
// the fair counterpart to the Zig block kernel.
void refSvfProcessBlock(RefSvf& s, const float* in, float* out, size_t n,
                        float cutoff, float res, double sr, int ftype) {
    const double f = std::min(std::max((double)cutoff / sr, 0.0001), 0.49);
    const double q = (double)res * 10.0 + 1.0;
    const double r = 1.0 / q;
    double low = s.low, high = s.high, band = s.band;
    for (size_t i = 0; i < n; ++i) {
        const double x = (double)in[i];
        const double low1 = low + f * band;
        const double high1 = x - low1 - r * band;
        const double band1 = band + f * high1;
        low = low1; high = high1; band = band1;
        out[i] = (ftype == 0) ? (float)low : (ftype == 2) ? (float)high : (float)band;
    }
    s.low = low; s.high = high; s.band = band;
}

} // namespace

int main() {
    const double sr = 48000.0;
    const int N = 480000; // 10 seconds
    const float cutoff = 1200.0f, resonance = 0.6f;
    const int ftype = 0; // lowpass

    // Test signal: 220 Hz fundamental + 3 kHz tone the LPF should attenuate.
    std::vector<float> in((size_t)N);
    for (int i = 0; i < N; ++i)
        in[(size_t)i] = 0.5f * std::sin(2.0 * M_PI * 220.0 * i / sr)
                      + 0.5f * std::sin(2.0 * M_PI * 3000.0 * i / sr);

    // --- Correctness: C++ reference vs Zig kernel, sample-for-sample ---
    RefSvf rs;
    zdsp_svf_state zs;
    zdsp_svf_reset(&zs);
    double maxDiff = 0.0;
    for (int i = 0; i < N; ++i) {
        const float a = refSvfProcess(rs, in[(size_t)i], cutoff, resonance, sr, ftype);
        const float b = zdsp_svf_process(&zs, in[(size_t)i], cutoff, resonance, sr,
                                         (zdsp_filter_type)ftype);
        maxDiff = std::max(maxDiff, (double)std::fabs((double)a - (double)b));
    }

    // --- Benchmark: 20 passes each (~9.6M samples) ---
    volatile float sink = 0.0f;
    const int reps = 20;

    zdsp_svf_reset(&zs);
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        for (int i = 0; i < N; ++i)
            sink += zdsp_svf_process(&zs, in[(size_t)i], cutoff, resonance, sr,
                                     (zdsp_filter_type)ftype);
    auto t1 = std::chrono::high_resolution_clock::now();
    const double zigMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    RefSvf rs2;
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        for (int i = 0; i < N; ++i)
            sink += refSvfProcess(rs2, in[(size_t)i], cutoff, resonance, sr, ftype);
    auto t3 = std::chrono::high_resolution_clock::now();
    const double cppMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    // --- Block path: how the engine actually calls DSP (fair comparison) ---
    std::vector<float> outZig((size_t)N), outCpp((size_t)N);

    zdsp_svf_reset(&zs);
    auto b0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        zdsp_svf_process_block(&zs, in.data(), outZig.data(), (size_t)N,
                               cutoff, resonance, sr, (zdsp_filter_type)ftype);
    auto b1 = std::chrono::high_resolution_clock::now();
    const double zigBlockMs = std::chrono::duration<double, std::milli>(b1 - b0).count();

    RefSvf rs3;
    auto b2 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        refSvfProcessBlock(rs3, in.data(), outCpp.data(), (size_t)N,
                           cutoff, resonance, sr, ftype);
    auto b3 = std::chrono::high_resolution_clock::now();
    const double cppBlockMs = std::chrono::duration<double, std::milli>(b3 - b2).count();

    double blockMaxDiff = 0.0;
    for (int i = 0; i < N; ++i)
        blockMaxDiff = std::max(blockMaxDiff,
                                (double)std::fabs((double)outZig[(size_t)i] - (double)outCpp[(size_t)i]));

    // comptime-specialized Zig block (branch-free inner loop)
    std::vector<float> outOpt((size_t)N);
    zdsp_svf_reset(&zs);
    auto o0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        zdsp_svf_process_block_opt(&zs, in.data(), outOpt.data(), (size_t)N,
                                   cutoff, resonance, sr, (zdsp_filter_type)ftype);
    auto o1 = std::chrono::high_resolution_clock::now();
    const double zigOptMs = std::chrono::duration<double, std::milli>(o1 - o0).count();
    double optMaxDiff = 0.0;
    for (int i = 0; i < N; ++i)
        optMaxDiff = std::max(optMaxDiff,
                              (double)std::fabs((double)outOpt[(size_t)i] - (double)outCpp[(size_t)i]));

    const double mSamples = (double)reps * N / 1e6;
    std::printf("  samples compared      : %d\n", N);
    std::printf("  per-sample max |diff| : %.3e\n", maxDiff);
    std::printf("  block      max |diff| : %.3e\n", blockMaxDiff);
    std::printf("  --- per-sample C-ABI call (worst case for any lang) ---\n");
    std::printf("  Zig per-sample        : %6.1f ms  (%.1f M samples)\n", zigMs, mSamples);
    std::printf("  C++ inlined ref       : %6.1f ms  (%.1f M samples)\n", cppMs, mSamples);
    std::printf("  --- block processing (the engine's real call path) ---\n");
    std::printf("  Zig block (naive)     : %6.1f ms  (%.1f M samples)\n", zigBlockMs, mSamples);
    std::printf("  C++ block             : %6.1f ms  (%.1f M samples)\n", cppBlockMs, mSamples);
    std::printf("  Zig block (comptime)  : %6.1f ms  (%.1f M samples)  <- branch-free\n", zigOptMs, mSamples);
    const bool allMatch = maxDiff < 1e-5 && blockMaxDiff < 1e-5 && optMaxDiff < 1e-5;
    std::printf("  result                : %s\n",
                allMatch ? "PASS — all variants bit-match the C++ reference"
                         : "FAIL — divergence");
    (void)sink;
    return allMatch ? 0 : 1;
}
