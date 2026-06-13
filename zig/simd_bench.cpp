// simd_bench.cpp — does Zig's explicit @Vector SIMD beat C++ auto-vectorization
// on a realistic voice-parallel filter workload?
//
// Fair fight: the C++ reference is structured for auto-vectorization (SoA,
// __restrict, fixed lane count) and compiled -O3 -march=native. The Zig kernel
// uses @Vector. Same f64 math, same interleaved data. We check correctness then
// time both over ZDSP_SIMD_VOICES independent filters.

#include "zenith_dsp.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <chrono>

static constexpr int V = ZDSP_SIMD_VOICES;

// C++ reference: V independent SVFs. SoA + __restrict + compile-time V give the
// optimizer everything it needs to vectorize the inner voice loop.
static void cppVoices(double* __restrict lo, double* __restrict hi, double* __restrict ba,
                      const double* __restrict f, const double* __restrict r,
                      const float* __restrict in, float* __restrict out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        const size_t base = i * (size_t)V;
        for (int v = 0; v < V; ++v) {
            const double x = (double)in[base + (size_t)v];
            const double low1 = lo[v] + f[v] * ba[v];
            const double high1 = x - low1 - r[v] * ba[v];
            const double band1 = ba[v] + f[v] * high1;
            lo[v] = low1; hi[v] = high1; ba[v] = band1;
            out[base + (size_t)v] = (float)low1;
        }
    }
}

// All-f32 C++ reference, same auto-vec-friendly structure.
static void cppVoicesF32(float* __restrict lo, float* __restrict hi, float* __restrict ba,
                         const float* __restrict f, const float* __restrict r,
                         const float* __restrict in, float* __restrict out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        const size_t base = i * (size_t)V;
        for (int v = 0; v < V; ++v) {
            const float x = in[base + (size_t)v];
            const float low1 = lo[v] + f[v] * ba[v];
            const float high1 = x - low1 - r[v] * ba[v];
            const float band1 = ba[v] + f[v] * high1;
            lo[v] = low1; hi[v] = high1; ba[v] = band1;
            out[base + (size_t)v] = low1;
        }
    }
}

int main() {
    const double sr = 48000.0;
    const int n = 480000;
    const int reps = 20;

    double f[V], r[V];
    for (int v = 0; v < V; ++v) {
        const double cutoff = 400.0 + 250.0 * v; // each voice a different cutoff
        const double res = 0.5;
        f[v] = std::min(std::max(cutoff / sr, 0.0001), 0.49);
        r[v] = 1.0 / (res * 10.0 + 1.0);
    }

    std::vector<float> in((size_t)n * V), outC((size_t)n * V), outZ((size_t)n * V);
    for (int i = 0; i < n; ++i)
        for (int v = 0; v < V; ++v)
            in[(size_t)i * V + (size_t)v] = 0.5f * std::sin(2.0 * M_PI * (200.0 + 30.0 * v) * i / sr);

    // --- correctness ---
    {
        double lo[V] = {0}, hi[V] = {0}, ba[V] = {0};
        double lz[V] = {0}, hz[V] = {0}, bz[V] = {0};
        cppVoices(lo, hi, ba, f, r, in.data(), outC.data(), (size_t)n);
        zdsp_svf_voices_simd(lz, hz, bz, f, r, in.data(), outZ.data(), (size_t)n);
    }
    double maxDiff = 0.0;
    for (size_t k = 0; k < (size_t)n * V; ++k)
        maxDiff = std::max(maxDiff, (double)std::fabs((double)outC[k] - (double)outZ[k]));

    // --- benchmark ---
    volatile float sink = 0.0f;

    double loc[V] = {0}, hic[V] = {0}, bac[V] = {0};
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        cppVoices(loc, hic, bac, f, r, in.data(), outC.data(), (size_t)n);
    auto t1 = std::chrono::high_resolution_clock::now();
    const double cppMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    sink += outC[0];

    double loz[V] = {0}, hiz[V] = {0}, baz[V] = {0};
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        zdsp_svf_voices_simd(loz, hiz, baz, f, r, in.data(), outZ.data(), (size_t)n);
    auto t3 = std::chrono::high_resolution_clock::now();
    const double zigMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    sink += outZ[0];

    // --- tuned all-f32 round ---
    std::vector<float> ff(V), rr(V);
    for (int v = 0; v < V; ++v) { ff[v] = (float)f[v]; rr[v] = (float)r[v]; }
    std::vector<float> outCf((size_t)n * V), outZf((size_t)n * V);
    {
        float lo[V] = {0}, hi[V] = {0}, ba[V] = {0};
        float lz[V] = {0}, hz[V] = {0}, bz[V] = {0};
        cppVoicesF32(lo, hi, ba, ff.data(), rr.data(), in.data(), outCf.data(), (size_t)n);
        zdsp_svf_voices_simd_f32(lz, hz, bz, ff.data(), rr.data(), in.data(), outZf.data(), (size_t)n);
    }
    double maxDiffF32 = 0.0;
    for (size_t k = 0; k < (size_t)n * V; ++k)
        maxDiffF32 = std::max(maxDiffF32, (double)std::fabs((double)outCf[k] - (double)outZf[k]));

    float locf[V] = {0}, hicf[V] = {0}, bacf[V] = {0};
    auto u0 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        cppVoicesF32(locf, hicf, bacf, ff.data(), rr.data(), in.data(), outCf.data(), (size_t)n);
    auto u1 = std::chrono::high_resolution_clock::now();
    const double cppF32Ms = std::chrono::duration<double, std::milli>(u1 - u0).count();
    sink += outCf[0];

    float lozf[V] = {0}, hizf[V] = {0}, bazf[V] = {0};
    auto u2 = std::chrono::high_resolution_clock::now();
    for (int rep = 0; rep < reps; ++rep)
        zdsp_svf_voices_simd_f32(lozf, hizf, bazf, ff.data(), rr.data(), in.data(), outZf.data(), (size_t)n);
    auto u3 = std::chrono::high_resolution_clock::now();
    const double zigF32Ms = std::chrono::duration<double, std::milli>(u3 - u2).count();
    sink += outZf[0];

    const double mVoiceSamples = (double)reps * n * V / 1e6;
    std::printf("  voices                : %d (SIMD lanes)\n", V);
    std::printf("  voice-samples         : %.1f M\n", mVoiceSamples);
    std::printf("  --- f64 (naive Zig: f32<->f64 round-trip) ---\n");
    std::printf("  max |C++ - Zig|       : %.3e\n", maxDiff);
    std::printf("  C++ -O3 -march=native : %6.1f ms\n", cppMs);
    std::printf("  Zig @Vector SIMD      : %6.1f ms   (%.2fx C++)\n", zigMs, cppMs / zigMs);
    std::printf("  --- f32 (tuned: full vector width, no conversions) ---\n");
    std::printf("  max |C++ - Zig|       : %.3e\n", maxDiffF32);
    std::printf("  C++ -O3 -march=native : %6.1f ms\n", cppF32Ms);
    std::printf("  Zig @Vector SIMD      : %6.1f ms   (%.2fx C++)\n", zigF32Ms, cppF32Ms / zigF32Ms);
    std::printf("  result                : %s\n",
                (maxDiff < 1e-5 && maxDiffF32 < 1e-3) ? "PASS — outputs agree" : "FAIL — divergence");
    (void)sink;
    return (maxDiff < 1e-5 && maxDiffF32 < 1e-3) ? 0 : 1;
}
