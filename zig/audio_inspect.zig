//! audio_inspect.zig — analyze an audio signal: peak, RMS, dominant frequency,
//! and linear↔dB conversion. The numeric backbone of the "what's playing" monitor
//! (the DAW logs a per-track report built from these). Pure Zig; FFT via dsp.zig.

const std = @import("std");
const dsp = @import("dsp.zig");

pub const Stats = struct {
    peak: f32 = 0, // max |sample| (linear 0..1+)
    rms: f32 = 0, // root-mean-square (linear)
    dominant_hz: f32 = 0, // strongest spectral component
};

pub fn dbFromLinear(x: f32) f32 {
    return if (x <= 1e-6) -120.0 else 20.0 * std.math.log10(x);
}

/// Dominant frequency (Hann-windowed FFT peak) over a power-of-two window taken
/// from the middle of `samples`. Returns 0 if too short.
pub fn dominantHz(a: std.mem.Allocator, samples: []const f32, sr: u32) f32 {
    var n: usize = 1;
    while (n * 2 <= samples.len and n < 16384) n *= 2;
    if (n < 8) return 0;
    const buf = a.alloc(dsp.Complex, n) catch return 0;
    defer a.free(buf);
    const off = (samples.len - n) / 2;
    for (buf, 0..) |*c, k| {
        const w = 0.5 - 0.5 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1)));
        c.* = .{ .re = samples[off + k] * w, .im = 0 };
    }
    dsp.fft(buf, false);
    var max_bin: usize = 1;
    var max_mag: f32 = 0;
    for (buf[1 .. n / 2], 1..) |c, k| {
        if (c.mag() > max_mag) {
            max_mag = c.mag();
            max_bin = k;
        }
    }
    return @as(f32, @floatFromInt(max_bin)) * @as(f32, @floatFromInt(sr)) / @as(f32, @floatFromInt(n));
}

pub fn analyze(a: std.mem.Allocator, samples: []const f32, sr: u32) Stats {
    var peak: f32 = 0;
    var sum: f64 = 0;
    for (samples) |s| {
        const m = @abs(s);
        if (m > peak) peak = m;
        sum += @as(f64, s) * @as(f64, s);
    }
    const rms: f32 = if (samples.len == 0) 0 else @floatCast(@sqrt(sum / @as(f64, @floatFromInt(samples.len))));
    return .{ .peak = peak, .rms = rms, .dominant_hz = dominantHz(a, samples, sr) };
}

test "analyze a 440 Hz sine: peak/rms/dominant" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const n = 24000;
    const buf = try a.alloc(f32, n);
    defer a.free(buf);
    for (buf, 0..) |*v, i| v.* = 0.6 * @sin(2.0 * std.math.pi * 440.0 * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(sr)));
    const st = analyze(a, buf, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 0.6), st.peak, 1e-3);
    try std.testing.expectApproxEqAbs(@as(f32, 0.6 / std.math.sqrt2), st.rms, 1e-2);
    try std.testing.expectApproxEqAbs(@as(f32, 440), st.dominant_hz, 5.0);
    try std.testing.expectApproxEqAbs(@as(f32, -6.02), dbFromLinear(0.5), 0.05);
}
