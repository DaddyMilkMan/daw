//! codec.zig — compressed audio decode/encode for FLAC + Ogg/Vorbis.
//!
//! Codecs are "thankless leaves" (per the masterplan's purity spectrum): we don't
//! reinvent FLAC/Vorbis bitstreams. We bind libsndfile through a thin hand-declared
//! C ABI (no -dev headers needed) and decode to the same interleaved-f32 `AudioData`
//! our own WAV/AIFF readers produce. WAV/AIFF stay first-class in wav.zig/aiff.zig;
//! this only handles the formats not worth owning. (Future purity swap: vendored PD
//! single-headers dr_flac/stb_vorbis.)

const std = @import("std");
const wav = @import("wav.zig");
const dsp = @import("dsp.zig");

pub const AudioData = wav.AudioData;

// --- libsndfile ABI (minimal, hand-declared) ---
const SF_INFO = extern struct {
    frames: i64 = 0,
    samplerate: c_int = 0,
    channels: c_int = 0,
    format: c_int = 0,
    sections: c_int = 0,
    seekable: c_int = 0,
};
const SNDFILE = opaque {};

const SFM_READ: c_int = 0x10;
const SFM_WRITE: c_int = 0x20;

// major formats / subtypes (libsndfile sndfile.h)
pub const FORMAT_WAV: c_int = 0x010000;
pub const FORMAT_FLAC: c_int = 0x170000;
pub const FORMAT_OGG: c_int = 0x200000;
pub const SUB_PCM_16: c_int = 0x0002;
pub const SUB_VORBIS: c_int = 0x0060;

extern fn sf_open(path: [*:0]const u8, mode: c_int, info: *SF_INFO) callconv(.c) ?*SNDFILE;
extern fn sf_readf_float(sf: *SNDFILE, ptr: [*]f32, frames: i64) callconv(.c) i64;
extern fn sf_writef_float(sf: *SNDFILE, ptr: [*]const f32, frames: i64) callconv(.c) i64;
extern fn sf_close(sf: *SNDFILE) callconv(.c) c_int;
extern fn sf_strerror(sf: ?*SNDFILE) callconv(.c) [*:0]const u8;

pub const CodecError = error{ OpenFailed, ShortRead };

/// Decode a FLAC / Ogg / WAV / AIFF (anything libsndfile supports) to f32.
pub fn decode(a: std.mem.Allocator, path: [:0]const u8) !AudioData {
    var info = SF_INFO{};
    const sf = sf_open(path.ptr, SFM_READ, &info) orelse {
        std.debug.print("sf_open '{s}': {s}\n", .{ path, sf_strerror(null) });
        return CodecError.OpenFailed;
    };
    defer _ = sf_close(sf);

    const ch: usize = @intCast(info.channels);
    const total: usize = @intCast(info.frames * info.channels);
    const out = try a.alloc(f32, total);
    errdefer a.free(out);
    const got = sf_readf_float(sf, out.ptr, info.frames);
    if (got < info.frames) return CodecError.ShortRead;
    return .{ .samples = out, .sample_rate = @intCast(info.samplerate), .channels = @intCast(ch) };
}

/// Encode interleaved f32 to a `major_format | subtype` file (FLAC, Ogg, ...).
pub fn encode(path: [:0]const u8, samples: []const f32, channels: u16, sr: u32, format: c_int) !void {
    var info = SF_INFO{
        .samplerate = @intCast(sr),
        .channels = @intCast(channels),
        .format = format,
    };
    const sf = sf_open(path.ptr, SFM_WRITE, &info) orelse return CodecError.OpenFailed;
    defer _ = sf_close(sf);
    const frames: i64 = @intCast(samples.len / channels);
    _ = sf_writef_float(sf, samples.ptr, frames);
}

// ===========================================================================
// Tests — self-contained round-trips (encode then decode), no external files.
// ===========================================================================
fn toneInterleaved(a: std.mem.Allocator, freq: f32, frames_n: usize, sr: u32) ![]f32 {
    const s = try a.alloc(f32, frames_n);
    for (s, 0..) |*v, i| v.* = 0.6 * @sin(2.0 * std.math.pi * freq * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(sr)));
    return s;
}

fn dominantFreq(a: std.mem.Allocator, samples: []const f32, sr: u32) !f32 {
    var n: usize = 1;
    while (n * 2 <= samples.len and n < 8192) n *= 2;
    if (n < 8) return 0;
    const off = (samples.len - n) / 2;
    const buf = try a.alloc(dsp.Complex, n);
    defer a.free(buf);
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

test "FLAC encode -> decode round-trip preserves the tone" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const tone = try toneInterleaved(a, 440, 24000, sr);
    defer a.free(tone);
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrintZ(&nb, "test_codec.{d}.flac", .{std.os.linux.getpid()}) catch "test.flac";
    try encode(path, tone, 1, sr, FORMAT_FLAC | SUB_PCM_16);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try decode(a, path);
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, sr), ad.sample_rate);
    try std.testing.expectEqual(@as(u16, 1), ad.channels);
    const f = try dominantFreq(a, ad.samples, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 440), f, 5.0); // lossless
}

test "Ogg/Vorbis encode -> decode round-trip preserves the tone" {
    const a = std.testing.allocator;
    const sr: u32 = 48000;
    const tone = try toneInterleaved(a, 440, 24000, sr);
    defer a.free(tone);
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrintZ(&nb, "test_codec.{d}.ogg", .{std.os.linux.getpid()}) catch "test.ogg";
    try encode(path, tone, 1, sr, FORMAT_OGG | SUB_VORBIS);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try decode(a, path);
    defer ad.deinit(a);
    const f = try dominantFreq(a, ad.samples, sr);
    try std.testing.expectApproxEqAbs(@as(f32, 440), f, 15.0); // lossy
}

test "decode a real Ogg file if the JUCE asset is present" {
    const a = std.testing.allocator;
    const path: [:0]const u8 = "build/_deps/juce-src/examples/Assets/singing.ogg";
    std.fs.cwd().access(path, .{}) catch return error.SkipZigTest;
    var ad = try decode(a, path);
    defer ad.deinit(a);
    try std.testing.expect(ad.samples.len > 0);
    try std.testing.expect(ad.channels >= 1 and ad.channels <= 2);
    var peak: f32 = 0;
    for (ad.samples) |s| peak = @max(peak, @abs(s));
    try std.testing.expect(peak > 0.001); // real, non-silent audio
}
