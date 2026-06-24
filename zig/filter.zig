//! filter.zig — Zenith's musical filters: zero-delay-feedback (TPT) topologies
//! that stay stable + in-tune all the way to Nyquist and saturate like analog.
//! 100% Zig. Two models behind one `Filter`:
//!   • svf    — Cytomic/Simper TPT state-variable (LP/HP/BP/notch, clean, stable)
//!   • ladder — zero-delay 4-pole Moog ladder with tanh saturation (the growl)
//! Both coefficient sets are computed each `set()` so a voice can switch models
//! and modulate cutoff at audio rate. RT-safe; no allocation.

const std = @import("std");

/// Fast, accurate tanh (Padé 7/8) — analog-style soft saturation without a libm call.
pub fn tanhApprox(x: f32) f32 {
    const c = std.math.clamp(x, -4.0, 4.0);
    const x2 = c * c;
    const a = c * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)));
    const b = 135135.0 + x2 * (62370.0 + x2 * (3150.0 + x2 * 28.0));
    return a / b;
}

pub const Model = enum { svf, ladder };
pub const Mode = enum { lowpass, highpass, bandpass, notch };

pub const Filter = struct {
    model: Model = .svf,
    mode: Mode = .lowpass,
    sr: f32 = 48000,
    drive: f32 = 1.0, // 1.0 = clean; >1 saturates the input

    // --- SVF (TPT) coefficients + state ---
    g: f32 = 0,
    k: f32 = 1.0, // 1/Q
    a1: f32 = 1,
    a2: f32 = 0,
    a3: f32 = 0,
    ic1: f32 = 0,
    ic2: f32 = 0,

    // --- ladder coefficients + state ---
    gl: f32 = 0, // per-stage instantaneous gain G = g/(1+g)
    res: f32 = 0, // 0..4 feedback
    s: [4]f32 = .{ 0, 0, 0, 0 },

    pub fn setSampleRate(self: *Filter, sr: f32) void {
        self.sr = sr;
    }

    /// Set cutoff (Hz), resonance (0..1), and drive (1 = clean). Cheap enough to
    /// call per sample for audio-rate filter modulation.
    pub fn set(self: *Filter, cutoff_hz: f32, resonance: f32, drive: f32) void {
        const fc = std.math.clamp(cutoff_hz / self.sr, 0.0, 0.49);
        const g = @tan(std.math.pi * fc);
        // SVF: map resonance 0..1 -> Q 0.5..~25 (quadratic so the top is usable)
        const q = 0.5 + resonance * resonance * 24.5;
        self.k = 1.0 / q;
        self.g = g;
        self.a1 = 1.0 / (1.0 + g * (g + self.k));
        self.a2 = g * self.a1;
        self.a3 = g * self.a2;
        // ladder: TPT one-pole coefficient + feedback (0..4, ~4 = self-oscillation)
        self.gl = g / (1.0 + g);
        self.res = std.math.clamp(resonance, 0.0, 1.0) * 4.0;
        self.drive = drive;
    }

    pub fn reset(self: *Filter) void {
        self.ic1 = 0;
        self.ic2 = 0;
        self.s = .{ 0, 0, 0, 0 };
    }

    pub fn process(self: *Filter, x: f32) f32 {
        const xin = if (self.drive > 1.0001) tanhApprox(x * self.drive) else x;
        return switch (self.model) {
            .svf => self.svf(xin),
            .ladder => self.ladder(xin),
        };
    }

    fn svf(self: *Filter, v0: f32) f32 {
        const v3 = v0 - self.ic2;
        const v1 = self.a1 * self.ic1 + self.a2 * v3;
        const v2 = self.ic2 + self.a2 * self.ic1 + self.a3 * v3;
        self.ic1 = 2.0 * v1 - self.ic1;
        self.ic2 = 2.0 * v2 - self.ic2;
        return switch (self.mode) {
            .lowpass => v2,
            .highpass => v0 - self.k * v1 - v2,
            .bandpass => v1,
            .notch => v0 - self.k * v1,
        };
    }

    fn ladder(self: *Filter, x: f32) f32 {
        const G = self.gl;
        const G2 = G * G;
        const G4 = G2 * G2;
        // each TPT one-pole's state contribution z_i = s_i * (1 - G)
        const omg = 1.0 - G;
        const z0 = self.s[0] * omg;
        const z1 = self.s[1] * omg;
        const z2 = self.s[2] * omg;
        const z3 = self.s[3] * omg;
        const sigma = G2 * G * z0 + G2 * z1 + G * z2 + z3;
        const k = self.res;
        // Estimate the 4th-pole output via the closed-form zero-delay solve, then
        // SATURATE only the feedback (tanh) — this gives the analog growl and makes
        // self-oscillation self-limiting, while the forward stages stay linear so
        // unity DC gain is preserved at low resonance.
        const y3_lin = (G4 * x + sigma) / (1.0 + k * G4);
        const u = x - k * tanhApprox(y3_lin);
        var xi: f32 = u;
        inline for (0..4) |i| {
            const v = (xi - self.s[i]) * G;
            const y = v + self.s[i];
            self.s[i] = y + v; // linear TPT one-pole state update
            xi = y;
        }
        const lp = xi;
        return switch (self.mode) {
            .lowpass => lp,
            // derive other responses from the ladder taps (approximate)
            .highpass => x - lp,
            .bandpass => (self.s[1] * omg) - lp,
            .notch => x - lp + (self.s[1] * omg),
        };
    }
};

// ---------------------------------------------------------------------------
// Tests — frequency response, stability under extreme modulation.
// ---------------------------------------------------------------------------
fn settleDC(f: *Filter) f32 {
    var y: f32 = 0;
    for (0..4000) |_| y = f.process(1.0);
    return y;
}
fn nyquistLevel(f: *Filter) f32 {
    var peak: f32 = 0;
    var sign: f32 = 1;
    for (0..4000) |i| {
        const y = f.process(sign);
        sign = -sign;
        if (i > 2000) peak = @max(peak, @abs(y));
    }
    return peak;
}
fn rmsAtFreq(f: *Filter, freq: f32) f32 {
    f.reset();
    var acc: f32 = 0;
    var n: usize = 0;
    const sr = f.sr;
    var i: usize = 0;
    while (i < 8000) : (i += 1) {
        const x = @sin(2.0 * std.math.pi * freq * @as(f32, @floatFromInt(i)) / sr);
        const y = f.process(x);
        if (i > 4000) {
            acc += y * y;
            n += 1;
        }
    }
    return @sqrt(acc / @as(f32, @floatFromInt(n)));
}

test "SVF lowpass passes DC, kills Nyquist" {
    var f = Filter{ .model = .svf, .mode = .lowpass };
    f.set(1000, 0.0, 1.0);
    try std.testing.expect(settleDC(&f) > 0.95);
    f.reset();
    try std.testing.expect(nyquistLevel(&f) < 0.05);
}

test "SVF highpass kills DC, passes highs" {
    var f = Filter{ .model = .svf, .mode = .highpass };
    f.set(1000, 0.0, 1.0);
    try std.testing.expect(@abs(settleDC(&f)) < 0.05);
    f.reset();
    try std.testing.expect(nyquistLevel(&f) > 0.9);
}

test "SVF bandpass peaks at its center frequency" {
    var f = Filter{ .model = .svf, .mode = .bandpass };
    f.set(1000, 0.7, 1.0);
    const at_center = rmsAtFreq(&f, 1000);
    const at_low = rmsAtFreq(&f, 80);
    const at_high = rmsAtFreq(&f, 10000);
    try std.testing.expect(at_center > at_low * 2.0);
    try std.testing.expect(at_center > at_high * 2.0);
}

test "ladder lowpass passes DC (no res), attenuates highs" {
    var f = Filter{ .model = .ladder, .mode = .lowpass };
    f.set(1000, 0.0, 1.0);
    try std.testing.expect(settleDC(&f) > 0.9);
    f.reset();
    try std.testing.expect(nyquistLevel(&f) < 0.05);
}

test "ladder resonance lifts the cutoff band" {
    var lo = Filter{ .model = .ladder, .mode = .lowpass };
    lo.set(1000, 0.1, 1.0);
    const flat = rmsAtFreq(&lo, 1000);
    var hi = Filter{ .model = .ladder, .mode = .lowpass };
    hi.set(1000, 0.9, 1.0);
    const resonant = rmsAtFreq(&hi, 1000);
    try std.testing.expect(resonant > flat * 1.5); // resonance boosts energy near cutoff
}

test "stable under extreme audio-rate cutoff modulation (no NaN, bounded)" {
    inline for (.{ Model.svf, Model.ladder }) |model| {
        var f = Filter{ .model = model, .mode = .lowpass };
        var peak: f32 = 0;
        var i: usize = 0;
        while (i < 48000) : (i += 1) {
            // sweep cutoff across the whole range every few ms, max resonance
            const fc = 60.0 + 12000.0 * (0.5 + 0.5 * @sin(@as(f32, @floatFromInt(i)) * 0.02));
            f.set(fc, 1.0, 4.0);
            const drive = @sin(@as(f32, @floatFromInt(i)) * 0.05); // hot input
            const y = f.process(drive);
            try std.testing.expect(!std.math.isNan(y));
            peak = @max(peak, @abs(y));
        }
        try std.testing.expect(peak < 8.0); // bounded — never explodes
    }
}
