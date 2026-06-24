//! timestretch.zig — time-stretch (warp) + pitch-shift, pure Zig.
//!
//! Time-stretch via WSOLA (Waveform-Similarity Overlap-Add): change a clip's
//! duration without changing its pitch by overlap-adding windowed grains, using
//! cross-correlation to pick splice points that keep the waveform continuous.
//! Pitch-shift = time-stretch by the inverse ratio, then resample back to the
//! original length (so duration is preserved and pitch moves).

const std = @import("std");
const resample = @import("resample.zig");

const W: usize = 1024; // analysis/synthesis window
const HS: usize = 256; // synthesis hop
const SEARCH: i64 = 200; // ± cross-correlation search range (samples)

fn hann(i: usize) f32 {
    return 0.5 - 0.5 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(W - 1)));
}

/// Normalized cross-correlation between input[a..a+W] and a prediction frame.
fn xcorr(input: []const f32, a: i64, pred: []const f32) f32 {
    var num: f32 = 0;
    var den: f32 = 0;
    var i: usize = 0;
    while (i < W) : (i += 1) {
        const idx = a + @as(i64, @intCast(i));
        const s = if (idx >= 0 and idx < input.len) input[@intCast(idx)] else 0;
        num += s * pred[i];
        den += s * s;
    }
    return num / (@sqrt(den) + 1e-9);
}

/// Time-stretch `input` by `factor` (>1 = longer/slower, <1 = shorter/faster).
/// Pitch is preserved. Returns a freshly-allocated mono buffer.
pub fn timeStretch(a: std.mem.Allocator, input: []const f32, factor: f32) ![]f32 {
    if (factor <= 0) return error.BadFactor;
    if (input.len < W * 2) {
        // too short for WSOLA — fall back to plain resample (varispeed)
        const out_len = @max(1, @as(usize, @intFromFloat(@as(f32, @floatFromInt(input.len)) * factor)));
        const out = try a.alloc(f32, out_len);
        for (out, 0..) |*v, i| v.* = resample.hermite(input, @as(f64, @floatFromInt(i)) / factor);
        return out;
    }

    const ana_hop = @as(f32, @floatFromInt(HS)) / factor; // analysis advances slower/faster
    const out_len = @as(usize, @intFromFloat(@as(f32, @floatFromInt(input.len)) * factor)) + W;
    const out = try a.alloc(f32, out_len);
    @memset(out, 0);
    const norm = try a.alloc(f32, out_len); // window-sum for normalization
    defer a.free(norm);
    @memset(norm, 0);

    var pred: [W]f32 = undefined; // "natural continuation" we try to match
    var ana_ideal: f32 = 0;
    var chosen: i64 = 0;
    @memcpy(&pred, input[0..W]);

    var out_pos: usize = 0;
    while (out_pos + W <= out_len) {
        // search around the ideal analysis index for the best splice
        const ideal: i64 = @intFromFloat(ana_ideal);
        var best_delta: i64 = 0;
        var best_score: f32 = -1e30;
        var d: i64 = -SEARCH;
        while (d <= SEARCH) : (d += 1) {
            const score = xcorr(input, ideal + d, &pred);
            if (score > best_score) {
                best_score = score;
                best_delta = d;
            }
        }
        chosen = ideal + best_delta;

        // overlap-add the chosen windowed frame
        var i: usize = 0;
        while (i < W) : (i += 1) {
            const idx = chosen + @as(i64, @intCast(i));
            const s = if (idx >= 0 and idx < input.len) input[@intCast(idx)] else 0;
            const w = hann(i);
            out[out_pos + i] += s * w;
            norm[out_pos + i] += w;
        }

        // the next prediction = what naturally follows the chosen frame by HS
        var j: usize = 0;
        while (j < W) : (j += 1) {
            const idx = chosen + @as(i64, @intCast(HS + j));
            pred[j] = if (idx >= 0 and idx < input.len) input[@intCast(idx)] else 0;
        }

        out_pos += HS;
        ana_ideal += ana_hop;
    }

    // normalize the overlap-add (where windows summed to non-zero)
    for (out, norm) |*v, nrm| {
        if (nrm > 1e-6) v.* /= nrm;
    }
    return out;
}

/// Pitch-shift by `semitones` (preserving duration): stretch by the inverse
/// ratio, then resample back to the original length.
pub fn pitchShift(a: std.mem.Allocator, input: []const f32, semitones: f32) ![]f32 {
    const ratio = std.math.pow(f32, 2.0, semitones / 12.0); // >1 = higher
    const stretched = try timeStretch(a, input, ratio); // longer by `ratio`
    defer a.free(stretched);
    // resample the longer signal back down to input.len -> raises pitch by `ratio`
    const out = try a.alloc(f32, input.len);
    const step = @as(f64, @floatFromInt(stretched.len)) / @as(f64, @floatFromInt(input.len));
    for (out, 0..) |*v, i| v.* = resample.hermite(stretched, @as(f64, @floatFromInt(i)) * step);
    return out;
}

// ===========================================================================
// Tests — verify via FFT that pitch/length behave as intended.
// ===========================================================================
fn tone(a: std.mem.Allocator, freq: f32, n: usize, sr: u32) ![]f32 {
    const s = try a.alloc(f32, n);
    for (s, 0..) |*v, i| v.* = @sin(2.0 * std.math.pi * freq * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(sr)));
    return s;
}

const Complex = struct { re: f32, im: f32 };
fn dominantHz(a: std.mem.Allocator, samples: []const f32, sr: u32) !f32 {
    const dsp = @import("dsp.zig");
    var n: usize = 1;
    while (n * 2 <= samples.len and n < 16384) n *= 2;
    const buf = try a.alloc(dsp.Complex, n);
    defer a.free(buf);
    const off = (samples.len - n) / 2;
    for (buf, 0..) |*c, k| {
        const w = 0.5 - 0.5 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1)));
        c.* = .{ .re = samples[off + k] * w, .im = 0 };
    }
    dsp.fft(buf, false);
    var mb: usize = 1;
    var mm: f32 = 0;
    for (buf[1 .. n / 2], 1..) |c, k| {
        if (c.mag() > mm) {
            mm = c.mag();
            mb = k;
        }
    }
    return @as(f32, @floatFromInt(mb)) * @as(f32, @floatFromInt(sr)) / @as(f32, @floatFromInt(n));
}

test "time-stretch ~doubles length and preserves pitch" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const in = try tone(a, 440, 24000, sr); // 0.5s @ 440 Hz
    defer a.free(in);
    const out = try timeStretch(a, in, 2.0);
    defer a.free(out);
    // length ~2x (within a window's slack)
    const ratio = @as(f32, @floatFromInt(out.len)) / @as(f32, @floatFromInt(in.len));
    try std.testing.expect(ratio > 1.9 and ratio < 2.1);
    // pitch unchanged
    const f = try dominantHz(a, out, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 440), f, 8.0);
}

test "time-stretch to 0.5x halves length, keeps pitch" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const in = try tone(a, 330, 24000, sr);
    defer a.free(in);
    const out = try timeStretch(a, in, 0.5);
    defer a.free(out);
    const ratio = @as(f32, @floatFromInt(out.len)) / @as(f32, @floatFromInt(in.len));
    try std.testing.expect(ratio > 0.45 and ratio < 0.6);
    const f = try dominantHz(a, out, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 330), f, 8.0);
}

test "pitch-shift up one octave doubles frequency, preserves length" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const in = try tone(a, 220, 24000, sr);
    defer a.free(in);
    const out = try pitchShift(a, in, 12.0); // +12 semitones
    defer a.free(out);
    try std.testing.expectEqual(in.len, out.len); // duration preserved
    const f = try dominantHz(a, out, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 440), f, 12.0); // 220 -> ~440
}
