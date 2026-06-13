//! wav.zig — minimal 16-bit PCM WAV writer. Pure Zig, no deps.

const std = @import("std");

/// Write mono/stereo f32 samples (range [-1,1]) to a 16-bit PCM WAV file.
pub fn writePcm16(path: []const u8, samples: []const f32, sample_rate: u32, channels: u16) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const w = bw.writer();

    const data_bytes: u32 = @intCast(samples.len * 2);
    const byte_rate: u32 = sample_rate * @as(u32, channels) * 2;

    // RIFF / WAVE header
    try w.writeAll("RIFF");
    try w.writeInt(u32, 36 + data_bytes, .little);
    try w.writeAll("WAVE");

    // fmt chunk
    try w.writeAll("fmt ");
    try w.writeInt(u32, 16, .little); // chunk size
    try w.writeInt(u16, 1, .little); // PCM
    try w.writeInt(u16, channels, .little);
    try w.writeInt(u32, sample_rate, .little);
    try w.writeInt(u32, byte_rate, .little);
    try w.writeInt(u16, channels * 2, .little); // block align
    try w.writeInt(u16, 16, .little); // bits per sample

    // data chunk
    try w.writeAll("data");
    try w.writeInt(u32, data_bytes, .little);
    for (samples) |s| {
        const c = std.math.clamp(s, -1.0, 1.0);
        const v: i16 = @intFromFloat(c * 32767.0);
        try w.writeInt(i16, v, .little);
    }

    try bw.flush();
}
