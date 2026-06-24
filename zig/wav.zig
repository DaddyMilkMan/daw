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

/// Write 24-bit PCM WAV (interleaved f32 in [-1,1]).
pub fn writePcm24(path: []const u8, samples: []const f32, sample_rate: u32, channels: u16) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const w = bw.writer();

    const data_bytes: u32 = @intCast(samples.len * 3);
    const block_align: u16 = channels * 3;
    try w.writeAll("RIFF");
    try w.writeInt(u32, 36 + data_bytes, .little);
    try w.writeAll("WAVE");
    try w.writeAll("fmt ");
    try w.writeInt(u32, 16, .little);
    try w.writeInt(u16, 1, .little); // PCM
    try w.writeInt(u16, channels, .little);
    try w.writeInt(u32, sample_rate, .little);
    try w.writeInt(u32, sample_rate * @as(u32, block_align), .little);
    try w.writeInt(u16, block_align, .little);
    try w.writeInt(u16, 24, .little);
    try w.writeAll("data");
    try w.writeInt(u32, data_bytes, .little);
    for (samples) |s| {
        const c = std.math.clamp(s, -1.0, 1.0);
        const v: i32 = @intFromFloat(c * 8388607.0);
        const u: u32 = @bitCast(v);
        try w.writeByte(@truncate(u));
        try w.writeByte(@truncate(u >> 8));
        try w.writeByte(@truncate(u >> 16));
    }
    try bw.flush();
}

/// Write 32-bit IEEE-float WAV (interleaved f32, no clipping — full range kept).
pub fn writeFloat32(path: []const u8, samples: []const f32, sample_rate: u32, channels: u16) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const w = bw.writer();

    const data_bytes: u32 = @intCast(samples.len * 4);
    const block_align: u16 = channels * 4;
    try w.writeAll("RIFF");
    try w.writeInt(u32, 4 + (8 + 16) + (8 + 4) + (8 + data_bytes), .little);
    try w.writeAll("WAVE");
    try w.writeAll("fmt ");
    try w.writeInt(u32, 16, .little);
    try w.writeInt(u16, 3, .little); // IEEE float
    try w.writeInt(u16, channels, .little);
    try w.writeInt(u32, sample_rate, .little);
    try w.writeInt(u32, sample_rate * @as(u32, block_align), .little);
    try w.writeInt(u16, block_align, .little);
    try w.writeInt(u16, 32, .little);
    try w.writeAll("fact"); // required for non-PCM
    try w.writeInt(u32, 4, .little);
    try w.writeInt(u32, @intCast(if (channels == 0) 0 else samples.len / channels), .little);
    try w.writeAll("data");
    try w.writeInt(u32, data_bytes, .little);
    for (samples) |s| try w.writeInt(u32, @bitCast(s), .little);
    try bw.flush();
}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

pub const AudioData = struct {
    samples: []f32, // interleaved
    sample_rate: u32,
    channels: u16,

    pub fn deinit(self: *AudioData, allocator: std.mem.Allocator) void {
        allocator.free(self.samples);
    }
    pub fn frames(self: AudioData) usize {
        return if (self.channels == 0) 0 else self.samples.len / self.channels;
    }
};

pub const WavError = error{ NotRiff, NotWave, NoFmt, NoData, Unsupported };

fn rdU16(b: []const u8, off: usize) u16 {
    return std.mem.readInt(u16, b[off..][0..2], .little);
}
fn rdU32(b: []const u8, off: usize) u32 {
    return std.mem.readInt(u32, b[off..][0..4], .little);
}

/// Read a WAV file into interleaved f32 samples. Supports PCM 16/24/32-bit and
/// IEEE float32, mono or multichannel. Skips unknown chunks.
pub fn readPcm(allocator: std.mem.Allocator, path: []const u8) !AudioData {
    const bytes = try std.fs.cwd().readFileAlloc(allocator, path, 1 << 30);
    defer allocator.free(bytes);

    if (bytes.len < 12 or !std.mem.eql(u8, bytes[0..4], "RIFF")) return WavError.NotRiff;
    if (!std.mem.eql(u8, bytes[8..12], "WAVE")) return WavError.NotWave;

    var pos: usize = 12;
    var fmt_found = false;
    var audio_format: u16 = 0;
    var channels: u16 = 0;
    var sample_rate: u32 = 0;
    var bits: u16 = 0;
    var data_off: usize = 0;
    var data_len: usize = 0;

    while (pos + 8 <= bytes.len) {
        const id = bytes[pos..][0..4];
        const size: usize = rdU32(bytes, pos + 4);
        const body = pos + 8;
        if (std.mem.eql(u8, id, "fmt ") and body + 16 <= bytes.len) {
            audio_format = rdU16(bytes, body + 0);
            channels = rdU16(bytes, body + 2);
            sample_rate = rdU32(bytes, body + 4);
            bits = rdU16(bytes, body + 14);
            fmt_found = true;
        } else if (std.mem.eql(u8, id, "data")) {
            data_off = body;
            data_len = @min(size, bytes.len - body);
        }
        pos = body + size + (size & 1); // chunks are word-aligned
    }
    if (!fmt_found) return WavError.NoFmt;
    if (data_off == 0) return WavError.NoData;

    const bytes_per_sample: usize = bits / 8;
    if (bytes_per_sample == 0) return WavError.Unsupported;
    const num_samples = data_len / bytes_per_sample;
    const out = try allocator.alloc(f32, num_samples);
    errdefer allocator.free(out);

    const d = bytes[data_off .. data_off + data_len];
    var i: usize = 0;
    while (i < num_samples) : (i += 1) {
        const o = i * bytes_per_sample;
        out[i] = switch (audio_format) {
            1 => switch (bits) {
                16 => @as(f32, @floatFromInt(std.mem.readInt(i16, d[o..][0..2], .little))) / 32768.0,
                24 => blk: {
                    const u: u32 = @as(u32, d[o]) | (@as(u32, d[o + 1]) << 8) | (@as(u32, d[o + 2]) << 16);
                    const v: i32 = if (u & 0x800000 != 0) @bitCast(u | 0xFF000000) else @intCast(u);
                    break :blk @as(f32, @floatFromInt(v)) / 8388608.0;
                },
                32 => @as(f32, @floatFromInt(std.mem.readInt(i32, d[o..][0..4], .little))) / 2147483648.0,
                else => return WavError.Unsupported,
            },
            3 => switch (bits) {
                32 => @bitCast(rdU32(d, o)),
                else => return WavError.Unsupported,
            },
            else => return WavError.Unsupported,
        };
    }
    return .{ .samples = out, .sample_rate = sample_rate, .channels = channels };
}

// Process-unique temp path: wav's tests run in multiple test binaries
// concurrently (aiff.zig imports wav), so fixed filenames can collide.
fn testTmpPath(buf: []u8, base: []const u8) []const u8 {
    const pid = std.os.linux.getpid();
    return std.fmt.bufPrint(buf, "{s}.{d}", .{ base, pid }) catch base;
}

/// Decode one interleaved sample at byte offset `o` in `d` to f32.
fn decodeSample(audio_format: u16, bits: u16, d: []const u8, o: usize) f32 {
    return switch (audio_format) {
        1 => switch (bits) {
            16 => @as(f32, @floatFromInt(std.mem.readInt(i16, d[o..][0..2], .little))) / 32768.0,
            24 => blk: {
                const u: u32 = @as(u32, d[o]) | (@as(u32, d[o + 1]) << 8) | (@as(u32, d[o + 2]) << 16);
                const v: i32 = if (u & 0x800000 != 0) @bitCast(u | 0xFF000000) else @intCast(u);
                break :blk @as(f32, @floatFromInt(v)) / 8388608.0;
            },
            32 => @as(f32, @floatFromInt(std.mem.readInt(i32, d[o..][0..4], .little))) / 2147483648.0,
            else => 0,
        },
        3 => switch (bits) {
            32 => @bitCast(rdU32(d, o)),
            else => 0,
        },
        else => 0,
    };
}

// ---------------------------------------------------------------------------
// Streaming reader — keep the file open and pull frames on demand, so a long
// recording or sample never has to live wholly in RAM. `open` reads only the
// header; `readFrames` seeks to and decodes just the requested span.
// ---------------------------------------------------------------------------
pub const WavStream = struct {
    file: std.fs.File,
    audio_format: u16,
    bits: u16,
    channels: u16,
    sample_rate: u32,
    block_align: u16,
    data_offset: u64,
    data_len: u64,

    pub fn open(path: []const u8) !WavStream {
        const file = try std.fs.cwd().openFile(path, .{});
        errdefer file.close();
        var hdr: [12]u8 = undefined;
        if ((try file.readAll(&hdr)) < 12 or !std.mem.eql(u8, hdr[0..4], "RIFF")) return WavError.NotRiff;
        if (!std.mem.eql(u8, hdr[8..12], "WAVE")) return WavError.NotWave;
        const end = try file.getEndPos();

        var audio_format: u16 = 0;
        var channels: u16 = 0;
        var sample_rate: u32 = 0;
        var bits: u16 = 0;
        var data_offset: u64 = 0;
        var data_len: u64 = 0;
        var fmt_found = false;

        var pos: u64 = 12;
        while (pos + 8 <= end) {
            var ch: [8]u8 = undefined;
            try file.seekTo(pos);
            if ((try file.readAll(&ch)) < 8) break;
            const size: u64 = std.mem.readInt(u32, ch[4..8], .little);
            const body = pos + 8;
            if (std.mem.eql(u8, ch[0..4], "fmt ") and body + 16 <= end) {
                var fb: [16]u8 = undefined;
                try file.seekTo(body);
                _ = try file.readAll(&fb);
                audio_format = rdU16(&fb, 0);
                channels = rdU16(&fb, 2);
                sample_rate = rdU32(&fb, 4);
                bits = rdU16(&fb, 14);
                fmt_found = true;
            } else if (std.mem.eql(u8, ch[0..4], "data")) {
                data_offset = body;
                data_len = @min(size, end - body);
            }
            pos = body + size + (size & 1);
        }
        if (!fmt_found) return WavError.NoFmt;
        if (data_offset == 0) return WavError.NoData;
        const bps = bits / 8;
        if (bps == 0) return WavError.Unsupported;
        return .{
            .file = file,
            .audio_format = audio_format,
            .bits = bits,
            .channels = channels,
            .sample_rate = sample_rate,
            .block_align = channels * bps,
            .data_offset = data_offset,
            .data_len = data_len,
        };
    }
    pub fn close(self: *WavStream) void {
        self.file.close();
    }
    pub fn frames(self: WavStream) u64 {
        return if (self.block_align == 0) 0 else self.data_len / self.block_align;
    }

    /// Decode up to `out.len / channels` frames starting at frame `frame_pos`
    /// into `out` (interleaved f32). Returns frames produced. Reads only that
    /// span from disk — never the whole file.
    pub fn readFrames(self: *WavStream, out: []f32, frame_pos: u64) !usize {
        const total = self.frames();
        if (frame_pos >= total) return 0;
        const ba = self.block_align;
        const bps = self.bits / 8;
        const want = @min(out.len / self.channels, total - frame_pos);
        try self.file.seekTo(self.data_offset + frame_pos * ba);

        var done: usize = 0; // frames decoded
        var buf: [4096]u8 = undefined;
        const frames_per_buf = buf.len / ba;
        while (done < want) {
            const this = @min(frames_per_buf, want - done);
            const nbytes = this * ba;
            const got = try self.file.readAll(buf[0..nbytes]);
            const got_frames = got / ba;
            var f: usize = 0;
            while (f < got_frames) : (f += 1) {
                var c: usize = 0;
                while (c < self.channels) : (c += 1) {
                    const o = f * ba + c * bps;
                    out[(done + f) * self.channels + c] = decodeSample(self.audio_format, self.bits, buf[0..got], o);
                }
            }
            done += got_frames;
            if (got < nbytes) break; // short read = EOF
        }
        return done;
    }
};

test "WavStream random-access reads match the full decode (16 + 24 bit)" {
    const a = std.testing.allocator;
    var samples: [1000]f32 = undefined;
    for (&samples, 0..) |*s, i| s.* = @sin(2.0 * std.math.pi * 5.0 * @as(f32, @floatFromInt(i)) / 1000.0) * 0.8;

    inline for (.{ "16", "24" }) |tag| {
        var nb: [64]u8 = undefined;
        const path = std.fmt.bufPrint(&nb, "test_stream{s}.{d}.wav", .{ tag, std.os.linux.getpid() }) catch "test_stream.wav";
        if (comptime std.mem.eql(u8, tag, "16")) try writePcm16(path, &samples, 48000, 1) else try writePcm24(path, &samples, 48000, 1);
        defer std.fs.cwd().deleteFile(path) catch {};

        var ref = try readPcm(a, path); // full in-memory decode (the oracle)
        defer ref.deinit(a);

        var st = try WavStream.open(path);
        defer st.close();
        try std.testing.expectEqual(@as(u64, 1000), st.frames());

        // read a window in the middle and compare to the full decode
        var win: [100]f32 = undefined;
        const got = try st.readFrames(&win, 500);
        try std.testing.expectEqual(@as(usize, 100), got);
        const tol: f32 = if (comptime std.mem.eql(u8, tag, "16")) 1e-4 else 1e-5;
        for (0..100) |i| try std.testing.expect(@abs(win[i] - ref.samples[500 + i]) < tol);

        // read past EOF -> fewer frames
        var tail: [200]f32 = undefined;
        try std.testing.expectEqual(@as(usize, 50), try st.readFrames(&tail, 950));
    }
}

test "wav write/read round-trip (16-bit)" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.0, 0.5, -0.5, 0.999, -0.999, 0.25, -0.25 };
    var nb: [64]u8 = undefined;
    const path = testTmpPath(&nb, "test_rt.wav");
    try writePcm16(path, &samples, 44100, 1);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try readPcm(a, path);
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 44100), ad.sample_rate);
    try std.testing.expectEqual(@as(u16, 1), ad.channels);
    try std.testing.expectEqual(samples.len, ad.samples.len);
    for (samples, ad.samples) |orig, got| {
        try std.testing.expect(@abs(orig - got) < 1e-4); // 16-bit quantization
    }
}

test "wav round-trip (24-bit) is near-lossless" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.0, 0.5, -0.5, 0.123456, -0.7654321, 0.999, -0.999 };
    var nb: [64]u8 = undefined;
    const path = testTmpPath(&nb, "test_rt24.wav");
    try writePcm24(path, &samples, 48000, 1);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try readPcm(a, path);
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 48000), ad.sample_rate);
    for (samples, ad.samples) |orig, got| try std.testing.expect(@abs(orig - got) < 1e-5);
}

test "wav round-trip (float32) is exact" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.0, 1.5, -2.25, 0.123456789, -0.7, 3.14159 }; // beyond [-1,1] kept
    var nb: [64]u8 = undefined;
    const path = testTmpPath(&nb, "test_rtf.wav");
    try writeFloat32(path, &samples, 96000, 1);
    defer std.fs.cwd().deleteFile(path) catch {};
    var ad = try readPcm(a, path);
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 96000), ad.sample_rate);
    for (samples, ad.samples) |orig, got| try std.testing.expectEqual(orig, got);
}
