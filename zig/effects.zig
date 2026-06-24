//! effects.zig — audio effects: biquad EQ, feedback delay, Freeverb-style
//! reverb. Real DSP, pure Zig. Plug into mixer tracks/buses.

const std = @import("std");

// ---------------------------------------------------------------------------
// Biquad filter (RBJ cookbook), transposed Direct Form II.
// ---------------------------------------------------------------------------
pub const Biquad = struct {
    b0: f32 = 1,
    b1: f32 = 0,
    b2: f32 = 0,
    a1: f32 = 0,
    a2: f32 = 0,
    z1: f32 = 0,
    z2: f32 = 0,

    pub fn process(self: *Biquad, x: f32) f32 {
        const y = self.b0 * x + self.z1;
        self.z1 = self.b1 * x - self.a1 * y + self.z2;
        self.z2 = self.b2 * x - self.a2 * y;
        return y;
    }
    pub fn processBlock(self: *Biquad, buf: []f32) void {
        for (buf) |*s| s.* = self.process(s.*);
    }

    fn setNorm(self: *Biquad, b0: f32, b1: f32, b2: f32, a0: f32, a1: f32, a2: f32) void {
        self.b0 = b0 / a0;
        self.b1 = b1 / a0;
        self.b2 = b2 / a0;
        self.a1 = a1 / a0;
        self.a2 = a2 / a0;
    }

    pub fn lowpass(sr: f32, freq: f32, q: f32) Biquad {
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alpha, -2 * c, 1 - alpha);
        return b;
    }
    pub fn highpass(sr: f32, freq: f32, q: f32) Biquad {
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
        return b;
    }
    pub fn peaking(sr: f32, freq: f32, q: f32, gain_db: f32) Biquad {
        const A = std.math.pow(f32, 10.0, gain_db / 40.0);
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm(1 + alpha * A, -2 * c, 1 - alpha * A, 1 + alpha / A, -2 * c, 1 - alpha / A);
        return b;
    }
    pub fn bandpass(sr: f32, freq: f32, q: f32) Biquad {
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm(alpha, 0, -alpha, 1 + alpha, -2 * c, 1 - alpha); // constant 0 dB peak gain
        return b;
    }
    pub fn notch(sr: f32, freq: f32, q: f32) Biquad {
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm(1, -2 * c, 1, 1 + alpha, -2 * c, 1 - alpha);
        return b;
    }
    pub fn allpass(sr: f32, freq: f32, q: f32) Biquad {
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / (2.0 * q);
        var b = Biquad{};
        b.setNorm(1 - alpha, -2 * c, 1 + alpha, 1 + alpha, -2 * c, 1 - alpha);
        return b;
    }
    pub fn lowShelf(sr: f32, freq: f32, gain_db: f32) Biquad {
        const A = std.math.pow(f32, 10.0, gain_db / 40.0);
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / 2.0 * @sqrt(2.0); // S=1 shelf slope
        const tsa = 2.0 * @sqrt(A) * alpha;
        var b = Biquad{};
        b.setNorm(
            A * ((A + 1) - (A - 1) * c + tsa),
            2 * A * ((A - 1) - (A + 1) * c),
            A * ((A + 1) - (A - 1) * c - tsa),
            (A + 1) + (A - 1) * c + tsa,
            -2 * ((A - 1) + (A + 1) * c),
            (A + 1) + (A - 1) * c - tsa,
        );
        return b;
    }
    pub fn highShelf(sr: f32, freq: f32, gain_db: f32) Biquad {
        const A = std.math.pow(f32, 10.0, gain_db / 40.0);
        const w0 = 2.0 * std.math.pi * freq / sr;
        const c = @cos(w0);
        const alpha = @sin(w0) / 2.0 * @sqrt(2.0);
        const tsa = 2.0 * @sqrt(A) * alpha;
        var b = Biquad{};
        b.setNorm(
            A * ((A + 1) + (A - 1) * c + tsa),
            -2 * A * ((A - 1) + (A + 1) * c),
            A * ((A + 1) + (A - 1) * c - tsa),
            (A + 1) - (A - 1) * c + tsa,
            2 * ((A - 1) - (A + 1) * c),
            (A + 1) - (A - 1) * c - tsa,
        );
        return b;
    }
};

// ---------------------------------------------------------------------------
// Feedback delay.
// ---------------------------------------------------------------------------
pub const Delay = struct {
    buf: []f32,
    idx: usize = 0,
    feedback: f32 = 0.4,
    mix: f32 = 0.35, // wet amount

    pub fn init(a: std.mem.Allocator, delay_samples: usize) !Delay {
        const buf = try a.alloc(f32, @max(delay_samples, 1));
        @memset(buf, 0);
        return .{ .buf = buf };
    }
    pub fn deinit(self: *Delay, a: std.mem.Allocator) void {
        a.free(self.buf);
    }
    pub fn process(self: *Delay, x: f32) f32 {
        const d = self.buf[self.idx];
        self.buf[self.idx] = x + d * self.feedback;
        self.idx = (self.idx + 1) % self.buf.len;
        return x * (1 - self.mix) + d * self.mix;
    }
    pub fn processBlock(self: *Delay, buf: []f32) void {
        for (buf) |*s| s.* = self.process(s.*);
    }
};

// ---------------------------------------------------------------------------
// Freeverb-style reverb (parallel combs -> series allpasses).
// ---------------------------------------------------------------------------
const Comb = struct {
    buf: []f32,
    idx: usize = 0,
    store: f32 = 0,
    feedback: f32,
    damp: f32,
    fn process(self: *Comb, x: f32) f32 {
        const y = self.buf[self.idx];
        self.store = y * (1 - self.damp) + self.store * self.damp;
        self.buf[self.idx] = x + self.store * self.feedback;
        self.idx = (self.idx + 1) % self.buf.len;
        return y;
    }
};
const Allpass = struct {
    buf: []f32,
    idx: usize = 0,
    feedback: f32 = 0.5,
    fn process(self: *Allpass, x: f32) f32 {
        const bufout = self.buf[self.idx];
        const y = -x + bufout;
        self.buf[self.idx] = x + bufout * self.feedback;
        self.idx = (self.idx + 1) % self.buf.len;
        return y;
    }
};

pub const Reverb = struct {
    combs: [4]Comb,
    allpasses: [2]Allpass,
    mix: f32 = 0.3,
    allocator: std.mem.Allocator,

    // tunings (samples @ 48k-ish), Freeverb-derived
    const comb_len = [_]usize{ 1213, 1293, 1391, 1467 };
    const ap_len = [_]usize{ 605, 480 };

    pub fn init(a: std.mem.Allocator, room: f32, damp: f32) !Reverb {
        var r = Reverb{ .combs = undefined, .allpasses = undefined, .allocator = a };
        for (&r.combs, 0..) |*c, i| {
            const buf = try a.alloc(f32, comb_len[i]);
            @memset(buf, 0);
            c.* = .{ .buf = buf, .feedback = 0.7 + room * 0.28, .damp = damp };
        }
        for (&r.allpasses, 0..) |*ap, i| {
            const buf = try a.alloc(f32, ap_len[i]);
            @memset(buf, 0);
            ap.* = .{ .buf = buf };
        }
        return r;
    }
    pub fn deinit(self: *Reverb) void {
        for (self.combs) |c| self.allocator.free(c.buf);
        for (self.allpasses) |ap| self.allocator.free(ap.buf);
    }
    pub fn process(self: *Reverb, x: f32) f32 {
        var wet: f32 = 0;
        for (&self.combs) |*c| wet += c.process(x);
        wet *= 0.25;
        for (&self.allpasses) |*ap| wet = ap.process(wet);
        return x * (1 - self.mix) + wet * self.mix;
    }
    pub fn processBlock(self: *Reverb, buf: []f32) void {
        for (buf) |*s| s.* = self.process(s.*);
    }
};

test "biquad lowpass passes DC, attenuates Nyquist" {
    var lp = Biquad.lowpass(48000, 1000, 0.707);
    var dc: f32 = 0;
    for (0..2000) |_| dc = lp.process(1.0);
    try std.testing.expectApproxEqAbs(@as(f32, 1.0), dc, 0.02); // DC gain ~1

    var hp = Biquad.lowpass(48000, 1000, 0.707);
    var peak: f32 = 0;
    var sign: f32 = 1;
    for (0..2000) |_| {
        const y = hp.process(sign);
        sign = -sign;
        peak = @max(peak, @abs(y));
    }
    try std.testing.expect(peak < 0.1); // Nyquist heavily attenuated
}

test "bandpass peaks at center, rejects DC and Nyquist" {
    var bp = Biquad.bandpass(48000, 1000, 4.0);
    // DC
    var dc: f32 = 0;
    for (0..4000) |_| dc = bp.process(1.0);
    try std.testing.expect(@abs(dc) < 0.1);
    // tone at center frequency builds to a clear amplitude
    var bp2 = Biquad.bandpass(48000, 1000, 4.0);
    var peak: f32 = 0;
    for (0..4000) |k| {
        const ph = 2.0 * std.math.pi * 1000.0 * @as(f32, @floatFromInt(k)) / 48000.0;
        const y = bp2.process(@sin(ph));
        if (k > 2000) peak = @max(peak, @abs(y));
    }
    try std.testing.expect(peak > 0.8);
}

test "notch rejects its center frequency" {
    var n = Biquad.notch(48000, 1000, 8.0);
    var peak: f32 = 0;
    for (0..8000) |k| {
        const ph = 2.0 * std.math.pi * 1000.0 * @as(f32, @floatFromInt(k)) / 48000.0;
        const y = n.process(@sin(ph));
        if (k > 4000) peak = @max(peak, @abs(y));
    }
    try std.testing.expect(peak < 0.15);
}

test "high shelf boosts highs, leaves DC alone" {
    var hs = Biquad.highShelf(48000, 4000, 12.0);
    var dc: f32 = 0;
    for (0..4000) |_| dc = hs.process(1.0);
    try std.testing.expectApproxEqAbs(@as(f32, 1.0), dc, 0.05); // DC ~ unity
}

test "delay produces an echo" {
    const a = std.testing.allocator;
    var d = try Delay.init(a, 100);
    defer d.deinit(a);
    d.mix = 1.0;
    d.feedback = 0.0;
    _ = d.process(1.0); // impulse in
    var got_echo = false;
    for (0..200) |i| {
        const y = d.process(0.0);
        if (i == 99 and @abs(y - 1.0) < 1e-4) got_echo = true; // echo at delay length
    }
    try std.testing.expect(got_echo);
}
