//! dsp.zig — Zenith's DSP toolkit (JUCE's juce_dsp, clean-room in Zig).
//!
//! Pure, allocation-light DSP kernels: FFT, convolution, oversampling, dynamics
//! (compressor/limiter/gate), metering (peak/RMS/LUFS per ITU-R BS.1770), a Moog
//! ladder filter, saturation, modulation (LFO/envelope-follower), gain/pan laws,
//! and a DC blocker. The RBJ biquad family lives in `effects.zig`; we re-export it
//! here so callers get one toolkit namespace.
//!
//! Reference oracle: JUCE's juce_dsp module (read for the feature set / behaviour,
//! not translated). Everything here is original Zig.

const std = @import("std");

pub const Biquad = @import("effects.zig").Biquad;

// ===========================================================================
// FFT — iterative radix-2 Cooley-Tukey, in place. Length must be a power of two.
// ===========================================================================
pub const Complex = struct {
    re: f32 = 0,
    im: f32 = 0,

    pub fn add(a: Complex, b: Complex) Complex {
        return .{ .re = a.re + b.re, .im = a.im + b.im };
    }
    pub fn sub(a: Complex, b: Complex) Complex {
        return .{ .re = a.re - b.re, .im = a.im - b.im };
    }
    pub fn mul(a: Complex, b: Complex) Complex {
        return .{ .re = a.re * b.re - a.im * b.im, .im = a.re * b.im + a.im * b.re };
    }
    pub fn mag(a: Complex) f32 {
        return @sqrt(a.re * a.re + a.im * a.im);
    }
};

pub fn isPowerOfTwo(n: usize) bool {
    return n != 0 and (n & (n - 1)) == 0;
}

/// In-place FFT (or inverse FFT, which is 1/N-scaled). `data.len` must be a power of two.
pub fn fft(data: []Complex, inverse: bool) void {
    const n = data.len;
    std.debug.assert(isPowerOfTwo(n));
    if (n < 2) return;

    // bit-reversal permutation
    var j: usize = 0;
    var i: usize = 1;
    while (i < n) : (i += 1) {
        var bit = n >> 1;
        while (j & bit != 0) : (bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std.mem.swap(Complex, &data[i], &data[j]);
    }

    // butterflies
    var len: usize = 2;
    while (len <= n) : (len <<= 1) {
        const sign: f32 = if (inverse) 1.0 else -1.0;
        const ang = sign * 2.0 * std.math.pi / @as(f32, @floatFromInt(len));
        const wlen = Complex{ .re = @cos(ang), .im = @sin(ang) };
        var start: usize = 0;
        while (start < n) : (start += len) {
            var w = Complex{ .re = 1, .im = 0 };
            var k: usize = 0;
            const half = len >> 1;
            while (k < half) : (k += 1) {
                const u = data[start + k];
                const v = data[start + k + half].mul(w);
                data[start + k] = u.add(v);
                data[start + k + half] = u.sub(v);
                w = w.mul(wlen);
            }
        }
    }

    if (inverse) {
        const inv_n = 1.0 / @as(f32, @floatFromInt(n));
        for (data) |*c| {
            c.re *= inv_n;
            c.im *= inv_n;
        }
    }
}

// Analysis windows (fill `w` with the window of its length).
pub fn hannWindow(w: []f32) void {
    const n = w.len;
    if (n < 2) {
        for (w) |*s| s.* = 1;
        return;
    }
    for (w, 0..) |*s, k| {
        const x = 2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1));
        s.* = 0.5 - 0.5 * @cos(x);
    }
}
pub fn hammingWindow(w: []f32) void {
    const n = w.len;
    if (n < 2) {
        for (w) |*s| s.* = 1;
        return;
    }
    for (w, 0..) |*s, k| {
        const x = 2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1));
        s.* = 0.54 - 0.46 * @cos(x);
    }
}
pub fn blackmanHarrisWindow(w: []f32) void {
    const n = w.len;
    if (n < 2) {
        for (w) |*s| s.* = 1;
        return;
    }
    for (w, 0..) |*s, k| {
        const x = 2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1));
        s.* = 0.35875 - 0.48829 * @cos(x) + 0.14128 * @cos(2 * x) - 0.01168 * @cos(3 * x);
    }
}

// ===========================================================================
// Convolution.
// ===========================================================================

/// Offline linear convolution via FFT. Returns a freshly-allocated buffer of
/// length `x.len + h.len - 1` (caller frees). Good for baking IRs / cabs.
pub fn convolveFft(a: std.mem.Allocator, x: []const f32, h: []const f32) ![]f32 {
    const out_len = x.len + h.len - 1;
    var n: usize = 1;
    while (n < out_len) n <<= 1;

    const xa = try a.alloc(Complex, n);
    defer a.free(xa);
    const ha = try a.alloc(Complex, n);
    defer a.free(ha);
    @memset(xa, Complex{});
    @memset(ha, Complex{});
    for (x, 0..) |v, k| xa[k] = .{ .re = v };
    for (h, 0..) |v, k| ha[k] = .{ .re = v };

    fft(xa, false);
    fft(ha, false);
    for (xa, 0..) |*c, k| c.* = c.mul(ha[k]);
    fft(xa, true);

    const out = try a.alloc(f32, out_len);
    for (out, 0..) |*s, k| s.* = xa[k].re;
    return out;
}

/// Streaming time-domain FIR convolver — exact, good for short IRs / real-time.
pub const FirConvolver = struct {
    taps: []f32,
    ring: []f32,
    pos: usize = 0,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator, taps: []const f32) !FirConvolver {
        const t = try a.dupe(f32, taps);
        const ring = try a.alloc(f32, taps.len);
        @memset(ring, 0);
        return .{ .taps = t, .ring = ring, .allocator = a };
    }
    pub fn deinit(self: *FirConvolver) void {
        self.allocator.free(self.taps);
        self.allocator.free(self.ring);
    }
    pub fn process(self: *FirConvolver, x: f32) f32 {
        self.ring[self.pos] = x;
        var acc: f32 = 0;
        var idx = self.pos;
        for (self.taps) |t| {
            acc += t * self.ring[idx];
            idx = if (idx == 0) self.ring.len - 1 else idx - 1;
        }
        self.pos = (self.pos + 1) % self.ring.len;
        return acc;
    }
};

// ===========================================================================
// Oversampling — linear-phase 2x up/down with a windowed-sinc half-band-ish FIR.
// Use: up -> nonlinear stage -> down to suppress aliasing.
// ===========================================================================
pub const Oversampler2x = struct {
    up: FirConvolver,
    down: FirConvolver,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator, taps_count: usize) !Oversampler2x {
        const taps = try a.alloc(f32, taps_count);
        defer a.free(taps);
        designLowpass(taps, 0.25); // cutoff at 0.25 of the 2x rate = 0.5 of base Nyquist
        // Upsampler gets 2x gain (energy of inserted zeros); downsampler unity.
        const up_taps = try a.alloc(f32, taps_count);
        defer a.free(up_taps);
        for (up_taps, 0..) |*t, k| t.* = taps[k] * 2.0;
        return .{
            .up = try FirConvolver.init(a, up_taps),
            .down = try FirConvolver.init(a, taps),
            .allocator = a,
        };
    }
    pub fn deinit(self: *Oversampler2x) void {
        self.up.deinit();
        self.down.deinit();
    }
    /// One input sample -> two oversampled samples (already anti-imaged).
    pub fn processUp(self: *Oversampler2x, x: f32) [2]f32 {
        const a0 = self.up.process(x);
        const a1 = self.up.process(0);
        return .{ a0, a1 };
    }
    /// Two oversampled samples -> one base-rate sample (anti-aliased + decimated).
    pub fn processDown(self: *Oversampler2x, pair: [2]f32) f32 {
        const y = self.down.process(pair[0]);
        _ = self.down.process(pair[1]);
        return y;
    }
};

/// Windowed-sinc lowpass FIR (Hamming window), normalized to unity DC gain.
pub fn designLowpass(taps: []f32, cutoff_norm: f32) void {
    const n = taps.len;
    const m = @as(f32, @floatFromInt(n - 1)) / 2.0;
    var sum: f32 = 0;
    for (taps, 0..) |*t, k| {
        const x = @as(f32, @floatFromInt(k)) - m;
        const sinc = if (@abs(x) < 1e-6) 2.0 * cutoff_norm else @sin(2.0 * std.math.pi * cutoff_norm * x) / (std.math.pi * x);
        const win = 0.54 - 0.46 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1)));
        t.* = sinc * win;
        sum += t.*;
    }
    if (sum != 0) for (taps) |*t| {
        t.* /= sum;
    };
}

// ===========================================================================
// Dynamics — feed-forward peak compressor, brickwall limiter, noise gate.
// ===========================================================================
fn timeCoef(time_ms: f32, sr: f32) f32 {
    if (time_ms <= 0) return 0;
    return @exp(-1.0 / ((time_ms / 1000.0) * sr));
}

pub const Compressor = struct {
    threshold_db: f32 = -18,
    ratio: f32 = 4,
    knee_db: f32 = 6,
    makeup_db: f32 = 0,
    att: f32,
    rel: f32,
    env: f32 = 0, // linear level detector

    pub fn init(sr: f32, attack_ms: f32, release_ms: f32) Compressor {
        return .{ .att = timeCoef(attack_ms, sr), .rel = timeCoef(release_ms, sr) };
    }
    pub fn process(self: *Compressor, x: f32) f32 {
        const target = @abs(x);
        const coef = if (target > self.env) self.att else self.rel;
        self.env = target + coef * (self.env - target);

        const level_db = 20.0 * @log10(self.env + 1e-9);
        const over = level_db - self.threshold_db;
        var gr_db: f32 = 0; // gain reduction (positive dB)
        const half_knee = self.knee_db * 0.5;
        const slope = 1.0 - 1.0 / self.ratio;
        if (self.knee_db > 0 and over > -half_knee and over < half_knee) {
            const t = over + half_knee;
            gr_db = slope * t * t / (2.0 * self.knee_db);
        } else if (over >= half_knee) {
            gr_db = slope * over;
        }
        const gain = std.math.pow(f32, 10.0, (self.makeup_db - gr_db) / 20.0);
        return x * gain;
    }
};

/// Lookahead brickwall limiter — output magnitude never exceeds `ceiling`.
pub const Limiter = struct {
    ceiling: f32 = 0.98,
    ring: []f32,
    pos: usize = 0,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator, sr: f32, lookahead_ms: f32, ceiling: f32) !Limiter {
        var l: usize = @intFromFloat(@max(1.0, (lookahead_ms / 1000.0) * sr));
        if (l < 1) l = 1;
        const ring = try a.alloc(f32, l);
        @memset(ring, 0);
        return .{ .ceiling = ceiling, .ring = ring, .allocator = a };
    }
    pub fn deinit(self: *Limiter) void {
        self.allocator.free(self.ring);
    }
    pub fn process(self: *Limiter, x: f32) f32 {
        self.ring[self.pos] = x;
        self.pos = (self.pos + 1) % self.ring.len;
        const out = self.ring[self.pos]; // oldest sample (delayed by lookahead)
        // gain = min over the lookahead window so no upcoming peak survives
        var g: f32 = 1;
        for (self.ring) |s| {
            const a = @abs(s);
            if (a > self.ceiling) {
                const gg = self.ceiling / a;
                if (gg < g) g = gg;
            }
        }
        return out * g;
    }
};

/// Downward noise gate with attack/release on the open/close gain.
pub const Gate = struct {
    threshold: f32 = 0.05, // linear
    floor: f32 = 0.0, // gain when fully closed
    att: f32,
    rel: f32,
    env: f32 = 0,
    gain: f32 = 0,

    pub fn init(sr: f32, attack_ms: f32, release_ms: f32) Gate {
        return .{ .att = timeCoef(attack_ms, sr), .rel = timeCoef(release_ms, sr) };
    }
    pub fn process(self: *Gate, x: f32) f32 {
        const target = @abs(x);
        const dcoef = if (target > self.env) self.att else self.rel;
        self.env = target + dcoef * (self.env - target);
        const want: f32 = if (self.env >= self.threshold) 1.0 else self.floor;
        const gcoef = if (want > self.gain) self.att else self.rel;
        self.gain = want + gcoef * (self.gain - want);
        return x * self.gain;
    }
};

// ===========================================================================
// Metering.
// ===========================================================================
pub const PeakMeter = struct {
    peak: f32 = 0,
    decay: f32 = 0.9999,
    pub fn process(self: *PeakMeter, x: f32) void {
        const a = @abs(x);
        self.peak = if (a > self.peak) a else self.peak * self.decay;
    }
    pub fn db(self: PeakMeter) f32 {
        return gainToDb(self.peak);
    }
};

pub const RmsMeter = struct {
    mean_sq: f32 = 0,
    coef: f32, // smoothing
    pub fn init(sr: f32, window_ms: f32) RmsMeter {
        return .{ .coef = timeCoef(window_ms, sr) };
    }
    pub fn process(self: *RmsMeter, x: f32) void {
        self.mean_sq = x * x + self.coef * (self.mean_sq - x * x);
    }
    pub fn rms(self: RmsMeter) f32 {
        return @sqrt(self.mean_sq);
    }
    pub fn db(self: RmsMeter) f32 {
        return gainToDb(self.rms());
    }
};

/// Integrated loudness per ITU-R BS.1770 (K-weighting + mean-square).
/// Coefficients are the published 48 kHz values; the engine runs at 48 kHz.
pub const LoudnessMeter = struct {
    // Stage 1: high-shelf pre-filter. Stage 2: RLB high-pass.
    shelf: Biquad,
    hp: Biquad,
    sum_sq: f64 = 0,
    count: u64 = 0,

    pub fn init() LoudnessMeter {
        var shelf = Biquad{};
        shelf.b0 = 1.53512485958697;
        shelf.b1 = -2.69169618940638;
        shelf.b2 = 1.19839281085285;
        shelf.a1 = -1.69065929318241;
        shelf.a2 = 0.73248077421585;
        var hp = Biquad{};
        hp.b0 = 1.0;
        hp.b1 = -2.0;
        hp.b2 = 1.0;
        hp.a1 = -1.99004745483398;
        hp.a2 = 0.99007225036621;
        return .{ .shelf = shelf, .hp = hp };
    }
    pub fn process(self: *LoudnessMeter, x: f32) void {
        const y = self.hp.process(self.shelf.process(x));
        self.sum_sq += @as(f64, y) * @as(f64, y);
        self.count += 1;
    }
    pub fn processBlock(self: *LoudnessMeter, buf: []const f32) void {
        for (buf) |s| self.process(s);
    }
    /// Integrated loudness in LUFS for everything fed so far (mono channel).
    pub fn lufs(self: LoudnessMeter) f32 {
        if (self.count == 0) return -std.math.inf(f32);
        const mean = self.sum_sq / @as(f64, @floatFromInt(self.count));
        return -0.691 + 10.0 * @as(f32, @floatCast(std.math.log10(mean + 1e-12)));
    }
};

// ===========================================================================
// Moog-style 4-pole ladder filter (cascaded one-poles + global feedback).
// ===========================================================================
pub const Ladder = struct {
    g: f32 = 0.5, // one-pole coefficient (from cutoff)
    k: f32 = 0, // feedback (resonance), 0..4
    y: [4]f32 = .{ 0, 0, 0, 0 },

    pub fn setCutoff(self: *Ladder, sr: f32, cutoff_hz: f32) void {
        const fc = std.math.clamp(cutoff_hz / sr, 0.0, 0.49);
        self.g = 1.0 - @exp(-2.0 * std.math.pi * fc);
    }
    pub fn setResonance(self: *Ladder, res: f32) void {
        self.k = std.math.clamp(res, 0.0, 4.0);
    }
    pub fn process(self: *Ladder, x: f32) f32 {
        const in = x - self.k * self.y[3];
        self.y[0] += self.g * (in - self.y[0]);
        self.y[1] += self.g * (self.y[0] - self.y[1]);
        self.y[2] += self.g * (self.y[1] - self.y[2]);
        self.y[3] += self.g * (self.y[2] - self.y[3]);
        return self.y[3];
    }
};

// ===========================================================================
// Saturation / waveshaping.
// ===========================================================================
pub fn softClip(x: f32) f32 {
    return std.math.tanh(x);
}
pub fn hardClip(x: f32, limit: f32) f32 {
    return std.math.clamp(x, -limit, limit);
}
pub const Saturator = struct {
    drive: f32 = 2,
    pub fn process(self: Saturator, x: f32) f32 {
        const d = @max(self.drive, 1e-3);
        return std.math.tanh(d * x) / std.math.tanh(d);
    }
};

// ===========================================================================
// Modulation.
// ===========================================================================
pub const Lfo = struct {
    pub const Shape = enum { sine, triangle, saw, square };
    phase: f32 = 0, // 0..1
    inc: f32 = 0,
    shape: Shape = .sine,

    pub fn init(sr: f32, rate_hz: f32, shape: Shape) Lfo {
        return .{ .inc = rate_hz / sr, .shape = shape };
    }
    pub fn next(self: *Lfo) f32 {
        const p = self.phase;
        self.phase += self.inc;
        if (self.phase >= 1.0) self.phase -= 1.0;
        return switch (self.shape) {
            .sine => @sin(p * 2.0 * std.math.pi),
            .triangle => 4.0 * @abs(p - 0.5) - 1.0,
            .saw => 2.0 * p - 1.0,
            .square => if (p < 0.5) @as(f32, 1.0) else -1.0,
        };
    }
};

pub const EnvelopeFollower = struct {
    att: f32,
    rel: f32,
    env: f32 = 0,
    pub fn init(sr: f32, attack_ms: f32, release_ms: f32) EnvelopeFollower {
        return .{ .att = timeCoef(attack_ms, sr), .rel = timeCoef(release_ms, sr) };
    }
    pub fn process(self: *EnvelopeFollower, x: f32) f32 {
        const target = @abs(x);
        const coef = if (target > self.env) self.att else self.rel;
        self.env = target + coef * (self.env - target);
        return self.env;
    }
};

// ===========================================================================
// Gain / pan laws + DC blocker.
// ===========================================================================
pub fn dbToGain(db: f32) f32 {
    return std.math.pow(f32, 10.0, db / 20.0);
}
pub fn gainToDb(g: f32) f32 {
    return 20.0 * @log10(@max(g, 1e-9));
}
/// Constant-power stereo pan. `pan` in [-1, 1]: -1 = hard left, +1 = hard right.
pub fn panConstantPower(pan: f32) [2]f32 {
    const p = (std.math.clamp(pan, -1.0, 1.0) + 1.0) * 0.5 * (std.math.pi / 2.0);
    return .{ @cos(p), @sin(p) };
}

pub const DcBlocker = struct {
    r: f32 = 0.995,
    x1: f32 = 0,
    y1: f32 = 0,
    pub fn process(self: *DcBlocker, x: f32) f32 {
        const y = x - self.x1 + self.r * self.y1;
        self.x1 = x;
        self.y1 = y;
        return y;
    }
    pub fn processBlock(self: *DcBlocker, buf: []f32) void {
        for (buf) |*s| s.* = self.process(s.*);
    }
};

// ===========================================================================
// Tests.
// ===========================================================================
const expect = std.testing.expect;
const expectApproxEqAbs = std.testing.expectApproxEqAbs;

test "fft round-trips and finds a tone's bin" {
    const a = std.testing.allocator;
    const n = 64;
    const data = try a.alloc(Complex, n);
    defer a.free(data);

    // a pure cosine at bin 5
    for (data, 0..) |*c, k| {
        const ph = 2.0 * std.math.pi * 5.0 * @as(f32, @floatFromInt(k)) / @as(f32, n);
        c.* = .{ .re = @cos(ph), .im = 0 };
    }
    const orig = try a.dupe(Complex, data);
    defer a.free(orig);

    fft(data, false);
    // peak magnitude should be at bin 5 (and its mirror n-5)
    var max_bin: usize = 0;
    var max_mag: f32 = 0;
    for (data[0 .. n / 2], 0..) |c, k| {
        if (c.mag() > max_mag) {
            max_mag = c.mag();
            max_bin = k;
        }
    }
    try expect(max_bin == 5);

    fft(data, true);
    for (data, 0..) |c, k| try expectApproxEqAbs(orig[k].re, c.re, 1e-3);
}

test "convolveFft matches hand convolution" {
    const a = std.testing.allocator;
    const x = [_]f32{ 1, 2, 3 };
    const h = [_]f32{ 1, 1 };
    const out = try convolveFft(a, &x, &h);
    defer a.free(out);
    // [1,2,3]*[1,1] = [1,3,5,3]
    const expected = [_]f32{ 1, 3, 5, 3 };
    try expect(out.len == expected.len);
    for (expected, 0..) |e, k| try expectApproxEqAbs(e, out[k], 1e-3);
}

test "FirConvolver emits the impulse response" {
    const a = std.testing.allocator;
    var fc = try FirConvolver.init(a, &[_]f32{ 0.5, 0.25, 0.125 });
    defer fc.deinit();
    try expectApproxEqAbs(@as(f32, 0.5), fc.process(1.0), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.25), fc.process(0.0), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.125), fc.process(0.0), 1e-6);
}

test "oversampler passes DC through up+down" {
    const a = std.testing.allocator;
    var os = try Oversampler2x.init(a, 32);
    defer os.deinit();
    var last: f32 = 0;
    // feed DC for long enough to flush the FIR group delay
    for (0..200) |_| {
        const up = os.processUp(1.0);
        last = os.processDown(up);
    }
    try expectApproxEqAbs(@as(f32, 1.0), last, 0.05);
}

test "compressor reduces gain above threshold" {
    var c = Compressor.init(48000, 1.0, 50.0);
    c.threshold_db = -10;
    c.ratio = 4;
    c.knee_db = 0;
    // DC at 0 dBFS: over = 10 dB, gr = 10*(1-1/4) = 7.5 dB -> gain ~0.42
    var y: f32 = 0;
    for (0..20000) |_| y = c.process(1.0);
    try expectApproxEqAbs(dbToGain(-7.5), y, 0.03);
}

test "limiter never exceeds the ceiling" {
    const a = std.testing.allocator;
    var lim = try Limiter.init(a, 48000, 1.0, 0.5);
    defer lim.deinit();
    var prng = std.Random.DefaultPrng.init(12345);
    const rnd = prng.random();
    var max_out: f32 = 0;
    for (0..5000) |_| {
        const x = (rnd.float(f32) * 2.0 - 1.0) * 2.0; // up to +/-2.0
        const y = lim.process(x);
        max_out = @max(max_out, @abs(y));
    }
    try expect(max_out <= 0.5 + 1e-4);
}

test "gate attenuates quiet, passes loud" {
    var g = Gate.init(48000, 0.5, 5.0);
    g.threshold = 0.1;
    var loud: f32 = 0;
    for (0..5000) |_| loud = g.process(0.8);
    try expect(@abs(loud) > 0.7); // open, near unity
    var quiet: f32 = 1;
    for (0..5000) |_| quiet = g.process(0.01);
    try expect(@abs(quiet) < 0.01); // closed
}

test "loudness rises ~6 LU when amplitude doubles" {
    var m1 = LoudnessMeter.init();
    var m2 = LoudnessMeter.init();
    const sr: f32 = 48000;
    for (0..48000) |k| {
        const ph = 2.0 * std.math.pi * 1000.0 * @as(f32, @floatFromInt(k)) / sr;
        const s = @sin(ph) * 0.1;
        m1.process(s);
        m2.process(s * 2.0);
    }
    const d = m2.lufs() - m1.lufs();
    try expectApproxEqAbs(@as(f32, 6.02), d, 0.2);
}

test "ladder is a lowpass: DC passes at res 0, Nyquist attenuated" {
    var f = Ladder{};
    f.setCutoff(48000, 1000);
    f.setResonance(0);
    var dc: f32 = 0;
    for (0..4000) |_| dc = f.process(1.0);
    try expectApproxEqAbs(@as(f32, 1.0), dc, 0.02);

    var f2 = Ladder{};
    f2.setCutoff(48000, 1000);
    f2.setResonance(0);
    var peak: f32 = 0;
    var sign: f32 = 1;
    for (0..4000) |_| {
        const y = f2.process(sign);
        sign = -sign;
        peak = @max(peak, @abs(y));
    }
    try expect(peak < 0.05);
}

test "saturation is bounded and normalized" {
    try expect(softClip(100.0) <= 1.0 and softClip(100.0) > 0.99);
    try expect(softClip(-100.0) >= -1.0);
    const s = Saturator{ .drive = 3 };
    try expectApproxEqAbs(@as(f32, 1.0), s.process(1.0), 1e-5); // normalized unity at full scale
    try expect(@abs(s.process(0.0)) < 1e-6);
}

test "lfo has the right period and range" {
    var lfo = Lfo.init(48000, 100.0, .sine); // 100 Hz -> 480 samples/cycle
    var minv: f32 = 1;
    var maxv: f32 = -1;
    for (0..480) |_| {
        const v = lfo.next();
        minv = @min(minv, v);
        maxv = @max(maxv, v);
    }
    try expect(maxv > 0.99 and minv < -0.99);
    try expectApproxEqAbs(@as(f32, 0.0), lfo.phase, 1e-3); // back to start after one period
}

test "envelope follower tracks a step" {
    var ef = EnvelopeFollower.init(48000, 1.0, 10.0);
    var e: f32 = 0;
    for (0..5000) |_| e = ef.process(0.7);
    try expectApproxEqAbs(@as(f32, 0.7), e, 0.02);
}

test "pan law is constant power" {
    const c = panConstantPower(0);
    try expectApproxEqAbs(@as(f32, 0.7071), c[0], 1e-3);
    try expectApproxEqAbs(@as(f32, 0.7071), c[1], 1e-3);
    const l = panConstantPower(-1);
    try expectApproxEqAbs(@as(f32, 1.0), l[0], 1e-3);
    try expectApproxEqAbs(@as(f32, 0.0), l[1], 1e-3);
    // power preserved across the sweep
    try expectApproxEqAbs(@as(f32, 1.0), c[0] * c[0] + c[1] * c[1], 1e-3);
}

test "dc blocker removes offset, keeps AC" {
    var dc = DcBlocker{};
    var y: f32 = 0;
    for (0..4000) |_| y = dc.process(1.0);
    try expect(@abs(y) < 0.05); // DC gone

    var dc2 = DcBlocker{};
    const sr: f32 = 48000;
    var amp: f32 = 0;
    for (0..4000) |k| {
        const ph = 2.0 * std.math.pi * 1000.0 * @as(f32, @floatFromInt(k)) / sr;
        const v = dc2.process(@sin(ph) + 0.5); // AC + DC offset
        if (k > 2000) amp = @max(amp, @abs(v));
    }
    try expect(amp > 0.9); // ~unity AC passes
}

test "db/gain round-trip" {
    try expectApproxEqAbs(@as(f32, -6.0), gainToDb(dbToGain(-6.0)), 1e-3);
    try expectApproxEqAbs(@as(f32, 1.0), dbToGain(0.0), 1e-6);
}
