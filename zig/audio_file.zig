//! audio_file.zig — "open any audio file" dispatcher.
//!
//! Routes by extension to the right reader: WAV/AIFF are our own (wav.zig/aiff.zig),
//! FLAC/Ogg/MP3 go through the codec leaves (codec.zig/mp3.zig). All return the same
//! interleaved-f32 `AudioData`, so the rest of the engine is format-agnostic.

const std = @import("std");
const wav = @import("wav.zig");
const aiff = @import("aiff.zig");
const codec = @import("codec.zig");
const mp3 = @import("mp3.zig");

pub const AudioData = wav.AudioData;

fn hasExt(path: []const u8, ext: []const u8) bool {
    if (path.len < ext.len) return false;
    return std.ascii.eqlIgnoreCase(path[path.len - ext.len ..], ext);
}

/// Decode any supported audio file to interleaved f32.
pub fn loadAny(a: std.mem.Allocator, path: [:0]const u8) !AudioData {
    if (hasExt(path, ".wav")) return wav.readPcm(a, path);
    if (hasExt(path, ".aiff") or hasExt(path, ".aif")) return aiff.read(a, path);
    if (hasExt(path, ".mp3")) return mp3.decode(a, path);
    // .flac, .ogg, and anything else libsndfile recognizes
    return codec.decode(a, path);
}

test "loadAny dispatches WAV by extension" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.0, 0.5, -0.5, 0.25 };
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrintZ(&nb, "test_any.{d}.wav", .{std.os.linux.getpid()}) catch "test_any.wav";
    try wav.writePcm16(path, &samples, 44100, 1);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try loadAny(a, path);
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 44100), ad.sample_rate);
    try std.testing.expectEqual(samples.len, ad.samples.len);
}

test "loadAny dispatches Ogg if the asset is present" {
    const a = std.testing.allocator;
    const path: [:0]const u8 = "build/_deps/juce-src/examples/Assets/singing.ogg";
    std.fs.cwd().access(path, .{}) catch return error.SkipZigTest;
    var ad = try loadAny(a, path);
    defer ad.deinit(a);
    try std.testing.expect(ad.samples.len > 0 and ad.channels >= 1);
}
