// zenith_dsp.zig — the first native Zig DSP kernel for Zenith DAW.
//
// Clean-room implementation: 100% ours, no JUCE source. Exposed over a flat
// C ABI so the (transitional) C++ engine can call it during the migration,
// and so it can be A/B-validated against the C++ reference it replaces.
//
// This is the seed of the Zig engine. Every kernel that lands here ships
// behind the same C ABI as its C++ predecessor and is proven equivalent
// before the C++ version is retired.

const std = @import("std");

/// Filter response. ABI-compatible with the C `zdsp_filter_type` enum.
pub const FilterType = enum(c_int) {
    lowpass = 0,
    bandpass = 1,
    highpass = 2,
};

/// State-variable filter integrator state. `extern struct` => C ABI layout,
/// so the host can own/allocate it and pass a pointer across the seam.
pub const SvfState = extern struct {
    low: f64 = 0,
    high: f64 = 0,
    band: f64 = 0,
};

/// Clear all integrator state.
export fn zdsp_svf_reset(state: *SvfState) void {
    state.* = .{};
}

/// Chamberlin state-variable filter, one sample.
/// Mirrors apps/desktop/Source/instruments/ZenithFilter.cpp::processSVF so the
/// two can be compared sample-for-sample. State persists across calls.
export fn zdsp_svf_process(
    state: *SvfState,
    input: f32,
    cutoff_hz: f32,
    resonance: f32,
    sample_rate: f64,
    ftype: FilterType,
) f32 {
    const f = std.math.clamp(@as(f64, cutoff_hz) / sample_rate, 0.0001, 0.49);
    const q = @as(f64, resonance) * 10.0 + 1.0;
    const r = 1.0 / q;

    const low1 = state.low + f * state.band;
    const high1 = @as(f64, input) - low1 - r * state.band;
    const band1 = state.band + f * high1;

    state.low = low1;
    state.high = high1;
    state.band = band1;

    return switch (ftype) {
        .lowpass => @floatCast(state.low),
        .highpass => @floatCast(state.high),
        .bandpass => @floatCast(state.band),
    };
}

/// Block processing — the way the engine actually calls DSP. State is hoisted
/// into locals for the duration of the loop so the optimizer can keep it in
/// registers (no per-sample pointer round-trips), and the single C-ABI call
/// amortizes over the whole buffer. This is where Zig's codegen competes
/// on equal footing; comptime specialization + @Vector SIMD come next.
export fn zdsp_svf_process_block(
    state: *SvfState,
    input: [*]const f32,
    output: [*]f32,
    num_samples: usize,
    cutoff_hz: f32,
    resonance: f32,
    sample_rate: f64,
    ftype: FilterType,
) void {
    const f = std.math.clamp(@as(f64, cutoff_hz) / sample_rate, 0.0001, 0.49);
    const q = @as(f64, resonance) * 10.0 + 1.0;
    const r = 1.0 / q;

    var low = state.low;
    var high = state.high;
    var band = state.band;

    var i: usize = 0;
    while (i < num_samples) : (i += 1) {
        const x = @as(f64, input[i]);
        const low1 = low + f * band;
        const high1 = x - low1 - r * band;
        const band1 = band + f * high1;
        low = low1;
        high = high1;
        band = band1;
        output[i] = switch (ftype) {
            .lowpass => @floatCast(low),
            .highpass => @floatCast(high),
            .bandpass => @floatCast(band),
        };
    }

    state.low = low;
    state.high = high;
    state.band = band;
}

/// comptime-specialized inner loop: because `ftype` is a comptime parameter,
/// the `switch` below is resolved at compile time and disappears — each
/// instantiation is a branch-free loop. This is the Zig-specific win: zero
/// runtime cost for the dispatch, no templates, no code duplication by hand.
fn svfBlockSpecialized(
    comptime ftype: FilterType,
    state: *SvfState,
    input: [*]const f32,
    output: [*]f32,
    num_samples: usize,
    f: f64,
    r: f64,
) void {
    var low = state.low;
    var high = state.high;
    var band = state.band;

    var i: usize = 0;
    while (i < num_samples) : (i += 1) {
        const x = @as(f64, input[i]);
        const low1 = low + f * band;
        const high1 = x - low1 - r * band;
        const band1 = band + f * high1;
        low = low1;
        high = high1;
        band = band1;
        output[i] = switch (ftype) { // comptime-known => no runtime branch
            .lowpass => @floatCast(low),
            .highpass => @floatCast(high),
            .bandpass => @floatCast(band),
        };
    }

    state.low = low;
    state.high = high;
    state.band = band;
}

/// Optimized block entry point — dispatches once to a comptime-specialized loop.
export fn zdsp_svf_process_block_opt(
    state: *SvfState,
    input: [*]const f32,
    output: [*]f32,
    num_samples: usize,
    cutoff_hz: f32,
    resonance: f32,
    sample_rate: f64,
    ftype: FilterType,
) void {
    const f = std.math.clamp(@as(f64, cutoff_hz) / sample_rate, 0.0001, 0.49);
    const q = @as(f64, resonance) * 10.0 + 1.0;
    const r = 1.0 / q;

    switch (ftype) {
        .lowpass => svfBlockSpecialized(.lowpass, state, input, output, num_samples, f, r),
        .bandpass => svfBlockSpecialized(.bandpass, state, input, output, num_samples, f, r),
        .highpass => svfBlockSpecialized(.highpass, state, input, output, num_samples, f, r),
    }
}

//==============================================================================
// Voice-parallel SVF — SIMD across independent voices (the real Zig win).
//
// A polysynth runs many independent filters (one per voice), each with its own
// cutoff. They can't be vectorized across time (serial recurrence) but they ARE
// independent across voices — so we put voice v in SIMD lane v and advance all
// of them in lockstep, one @Vector op per time step. This is where Zig's
// explicit, guaranteed SIMD competes directly with C++ auto-vectorization.
//==============================================================================

const SIMD_VOICES = 8;

/// Process SIMD_VOICES independent lowpass SVFs in lockstep.
///   state_*  : [SIMD_VOICES] integrator state, host-owned
///   f_coef   : [SIMD_VOICES] per-voice integrator coefficient
///   r_coef   : [SIMD_VOICES] per-voice damping (1/Q)
///   input    : [num_samples * SIMD_VOICES], interleaved by voice (lane-major)
///   output   : [num_samples * SIMD_VOICES]
export fn zdsp_svf_voices_simd(
    state_low: [*]f64,
    state_high: [*]f64,
    state_band: [*]f64,
    f_coef: [*]const f64,
    r_coef: [*]const f64,
    input: [*]const f32,
    output: [*]f32,
    num_samples: usize,
) void {
    const V = SIMD_VOICES;
    const Vec = @Vector(V, f64);

    const lo_ptr: *[V]f64 = @ptrCast(state_low);
    const hi_ptr: *[V]f64 = @ptrCast(state_high);
    const ba_ptr: *[V]f64 = @ptrCast(state_band);

    const f: Vec = @as(*const [V]f64, @ptrCast(f_coef)).*;
    const r: Vec = @as(*const [V]f64, @ptrCast(r_coef)).*;

    var lo: Vec = lo_ptr.*;
    var hi: Vec = hi_ptr.*;
    var ba: Vec = ba_ptr.*;

    var i: usize = 0;
    while (i < num_samples) : (i += 1) {
        const base = i * V;
        const xf: @Vector(V, f32) = @as(*const [V]f32, @ptrCast(input + base)).*;
        const x: Vec = @floatCast(xf);

        const low1 = lo + f * ba;
        const high1 = x - low1 - r * ba;
        const band1 = ba + f * high1;
        lo = low1;
        hi = high1;
        ba = band1;

        const of: @Vector(V, f32) = @floatCast(lo);
        @as(*[V]f32, @ptrCast(output + base)).* = of;
    }

    lo_ptr.* = lo;
    hi_ptr.* = hi;
    ba_ptr.* = ba;
}

/// Tuned variant: all-f32, full native vector width, no f32<->f64 round-trip.
/// This is the version that actually exploits Zig's control over the layout.
export fn zdsp_svf_voices_simd_f32(
    state_low: [*]f32,
    state_high: [*]f32,
    state_band: [*]f32,
    f_coef: [*]const f32,
    r_coef: [*]const f32,
    input: [*]const f32,
    output: [*]f32,
    num_samples: usize,
) void {
    const V = SIMD_VOICES;
    const Vec = @Vector(V, f32);

    const lo_ptr: *[V]f32 = @ptrCast(state_low);
    const hi_ptr: *[V]f32 = @ptrCast(state_high);
    const ba_ptr: *[V]f32 = @ptrCast(state_band);

    const f: Vec = @as(*const [V]f32, @ptrCast(f_coef)).*;
    const r: Vec = @as(*const [V]f32, @ptrCast(r_coef)).*;

    var lo: Vec = lo_ptr.*;
    var hi: Vec = hi_ptr.*;
    var ba: Vec = ba_ptr.*;

    var i: usize = 0;
    while (i < num_samples) : (i += 1) {
        const base = i * V;
        const x: Vec = @as(*const [V]f32, @ptrCast(input + base)).*;

        const low1 = lo + f * ba;
        const high1 = x - low1 - r * ba;
        const band1 = ba + f * high1;
        lo = low1;
        hi = high1;
        ba = band1;

        @as(*[V]f32, @ptrCast(output + base)).* = lo;
    }

    lo_ptr.* = lo;
    hi_ptr.* = hi;
    ba_ptr.* = ba;
}
