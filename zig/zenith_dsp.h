/*
 * zenith_dsp.h — C ABI for Zenith's native Zig DSP kernels.
 *
 * This is the first slice of the broader `zenith_platform.h` seam (see
 * planning/roadmaps/ZIG_MIGRATION_MASTER_PLAN.md). The C++ engine includes
 * this header and calls the kernels; the implementations live in Zig.
 * Flat C only — no C++ types cross the boundary.
 */
#ifndef ZENITH_DSP_H
#define ZENITH_DSP_H

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum zdsp_filter_type {
    ZDSP_LOWPASS = 0,
    ZDSP_BANDPASS = 1,
    ZDSP_HIGHPASS = 2
} zdsp_filter_type;

/* Integrator state — host-owned, matches the Zig `SvfState` extern struct. */
typedef struct zdsp_svf_state {
    double low;
    double high;
    double band;
} zdsp_svf_state;

void  zdsp_svf_reset(zdsp_svf_state* state);
float zdsp_svf_process(zdsp_svf_state* state,
                       float input,
                       float cutoff_hz,
                       float resonance,
                       double sample_rate,
                       zdsp_filter_type ftype);

/* Block processing — the engine's real call path. */
void  zdsp_svf_process_block(zdsp_svf_state* state,
                             const float* input,
                             float* output,
                             size_t num_samples,
                             float cutoff_hz,
                             float resonance,
                             double sample_rate,
                             zdsp_filter_type ftype);

/* Same, but dispatches to a comptime-specialized (branch-free) inner loop. */
void  zdsp_svf_process_block_opt(zdsp_svf_state* state,
                                 const float* input,
                                 float* output,
                                 size_t num_samples,
                                 float cutoff_hz,
                                 float resonance,
                                 double sample_rate,
                                 zdsp_filter_type ftype);

/* Voice-parallel SVF: ZDSP_SIMD_VOICES independent lowpass filters via SIMD.
 * input/output are interleaved lane-major: idx = sample*ZDSP_SIMD_VOICES + voice. */
#define ZDSP_SIMD_VOICES 8
void  zdsp_svf_voices_simd(double* state_low,
                           double* state_high,
                           double* state_band,
                           const double* f_coef,
                           const double* r_coef,
                           const float* input,
                           float* output,
                           size_t num_samples);

/* Tuned all-f32 variant (full native vector width, no f32<->f64 conversions). */
void  zdsp_svf_voices_simd_f32(float* state_low,
                               float* state_high,
                               float* state_band,
                               const float* f_coef,
                               const float* r_coef,
                               const float* input,
                               float* output,
                               size_t num_samples);

#ifdef __cplusplus
}
#endif

#endif /* ZENITH_DSP_H */
