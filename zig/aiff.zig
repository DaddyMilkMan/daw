//! aiff.zig — clean-room AIFF reader/writer (Apple/SGI uncompressed PCM).
//! Big-endian, signed PCM 8/16/24/32-bit. The sample rate is stored as an
//! 80-bit IEEE-754 extended float, which we encode/decode here. Pure Zig.
//!
//! Reference oracle: the AIFF-C / EA IFF 85 spec + JUCE's AiffAudioFormat for
//! the feature set — original implementation.

const std = @import("std");
const wav = @import("wav.zig");

pub const AudioData = wav.AudioData; // same interleaved-f32 container
pub const AiffError = error{ NotForm, NotAiff, NoComm, NoSsnd, Unsupported };

// --- 80-bit IEEE-754 extended <-> f64 (the AIFF sample-rate encoding) ---
fn f64ToExtended(num: f64) [10]u8 {
    var bytes = [_]u8{0} ** 10;
    if (num == 0) return bytes;

    var sign: u16 = 0;
    var x = num;
    if (x < 0) {
        sign = 0x8000;
        x = -x;
    }
    const fr = std.math.frexp(x); // x = significand * 2^exponent
    var significand = fr.significand;
    var exponent: i32 = fr.exponent;
    // Force the C frexp convention the algorithm below assumes: significand in [0.5,1).
    while (significand >= 1.0) {
        significand *= 0.5;
        exponent += 1;
    }
    while (significand != 0 and significand < 0.5) {
        significand *= 2.0;
        exponent -= 1;
    }

    exponent += 16382;
    if (exponent < 0) { // denormal
        significand = std.math.ldexp(significand, exponent);
        exponent = 0;
    }
    exponent += 1;

    significand = std.math.ldexp(significand, 32);
    const hi_f = std.math.floor(significand);
    const hi_mant: u32 = @intFromFloat(hi_f);
    significand = std.math.ldexp(significand - hi_f, 32);
    const lo_mant: u32 = @intFromFloat(std.math.floor(significand));

    const e: u16 = sign | @as(u16, @intCast(exponent & 0x7FFF));
    bytes[0] = @truncate(e >> 8);
    bytes[1] = @truncate(e);
    bytes[2] = @truncate(hi_mant >> 24);
    bytes[3] = @truncate(hi_mant >> 16);
    bytes[4] = @truncate(hi_mant >> 8);
    bytes[5] = @truncate(hi_mant);
    bytes[6] = @truncate(lo_mant >> 24);
    bytes[7] = @truncate(lo_mant >> 16);
    bytes[8] = @truncate(lo_mant >> 8);
    bytes[9] = @truncate(lo_mant);
    return bytes;
}

fn extendedToF64(b: []const u8) f64 {
    var exponent: i32 = ((@as(i32, b[0]) & 0x7F) << 8) | @as(i32, b[1]);
    const hi_mant: u32 = (@as(u32, b[2]) << 24) | (@as(u32, b[3]) << 16) | (@as(u32, b[4]) << 8) | @as(u32, b[5]);
    const lo_mant: u32 = (@as(u32, b[6]) << 24) | (@as(u32, b[7]) << 16) | (@as(u32, b[8]) << 8) | @as(u32, b[9]);
    var f: f64 = 0;
    if (exponent == 0 and hi_mant == 0 and lo_mant == 0) {
        f = 0;
    } else if (exponent == 0x7FFF) {
        f = std.math.inf(f64);
    } else {
        exponent -= 16383;
        f = std.math.ldexp(@as(f64, @floatFromInt(hi_mant)), exponent - 32);
        f += std.math.ldexp(@as(f64, @floatFromInt(lo_mant)), exponent - 64);
    }
    return if (b[0] & 0x80 != 0) -f else f;
}

// --- write ---
/// Write interleaved f32 [-1,1] to a 16- or 24-bit signed big-endian AIFF.
pub fn write(path: []const u8, samples: []const f32, sample_rate: u32, channels: u16, bits: u16) !void {
    if (bits != 16 and bits != 24) return AiffError.Unsupported;
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const w = bw.writer();

    const bytes_per: usize = bits / 8;
    const data_bytes: u32 = @intCast(samples.len * bytes_per);
    const num_frames: u32 = if (channels == 0) 0 else @intCast(samples.len / channels);
    const ssnd_size: u32 = 8 + data_bytes; // offset+blockSize+samples
    const form_size: u32 = 4 + (8 + 18) + (8 + ssnd_size); // "AIFF" + COMM + SSND

    try w.writeAll("FORM");
    try w.writeInt(u32, form_size, .big);
    try w.writeAll("AIFF");

    // COMM
    try w.writeAll("COMM");
    try w.writeInt(u32, 18, .big);
    try w.writeInt(i16, @intCast(channels), .big);
    try w.writeInt(u32, num_frames, .big);
    try w.writeInt(i16, @intCast(bits), .big);
    try w.writeAll(&f64ToExtended(@floatFromInt(sample_rate)));

    // SSND
    try w.writeAll("SSND");
    try w.writeInt(u32, ssnd_size, .big);
    try w.writeInt(u32, 0, .big); // offset
    try w.writeInt(u32, 0, .big); // blockSize
    for (samples) |s| {
        const c = std.math.clamp(s, -1.0, 1.0);
        if (bits == 16) {
            try w.writeInt(i16, @intFromFloat(c * 32767.0), .big);
        } else {
            const v: i32 = @intFromFloat(c * 8388607.0);
            const u: u32 = @bitCast(v);
            try w.writeByte(@truncate(u >> 16));
            try w.writeByte(@truncate(u >> 8));
            try w.writeByte(@truncate(u));
        }
    }
    try bw.flush();
}

// --- read ---
fn rdU32(b: []const u8, off: usize) u32 {
    return std.mem.readInt(u32, b[off..][0..4], .big);
}

/// Read an uncompressed AIFF into interleaved f32. Supports 8/16/24/32-bit PCM.
pub fn read(allocator: std.mem.Allocator, path: []const u8) !AudioData {
    const bytes = try std.fs.cwd().readFileAlloc(allocator, path, 1 << 30);
    defer allocator.free(bytes);
    if (bytes.len < 12 or !std.mem.eql(u8, bytes[0..4], "FORM")) return AiffError.NotForm;
    if (!std.mem.eql(u8, bytes[8..12], "AIFF")) return AiffError.NotAiff;

    var pos: usize = 12;
    var comm = false;
    var channels: u16 = 0;
    var bits: u16 = 0;
    var sample_rate: u32 = 0;
    var data_off: usize = 0;
    var data_len: usize = 0;

    while (pos + 8 <= bytes.len) {
        const id = bytes[pos..][0..4];
        const size: usize = rdU32(bytes, pos + 4);
        const body = pos + 8;
        if (std.mem.eql(u8, id, "COMM") and body + 18 <= bytes.len) {
            channels = std.mem.readInt(u16, bytes[body..][0..2], .big);
            bits = std.mem.readInt(u16, bytes[body + 6 ..][0..2], .big);
            sample_rate = @intFromFloat(extendedToF64(bytes[body + 8 .. body + 18]));
            comm = true;
        } else if (std.mem.eql(u8, id, "SSND") and body + 8 <= bytes.len) {
            const offset = rdU32(bytes, body); // skip alignment offset bytes
            data_off = body + 8 + offset;
            data_len = @min(size - 8 - offset, bytes.len - data_off);
        }
        pos = body + size + (size & 1); // word-aligned
    }
    if (!comm) return AiffError.NoComm;
    if (data_off == 0) return AiffError.NoSsnd;

    const bps: usize = bits / 8;
    if (bps == 0) return AiffError.Unsupported;
    const num = data_len / bps;
    const out = try allocator.alloc(f32, num);
    errdefer allocator.free(out);
    const d = bytes[data_off .. data_off + data_len];
    var i: usize = 0;
    while (i < num) : (i += 1) {
        const o = i * bps;
        out[i] = switch (bits) {
            8 => @as(f32, @floatFromInt(@as(i8, @bitCast(d[o])))) / 128.0,
            16 => @as(f32, @floatFromInt(std.mem.readInt(i16, d[o..][0..2], .big))) / 32768.0,
            24 => blk: {
                const u: u32 = (@as(u32, d[o]) << 16) | (@as(u32, d[o + 1]) << 8) | @as(u32, d[o + 2]);
                const v: i32 = if (u & 0x800000 != 0) @bitCast(u | 0xFF000000) else @intCast(u);
                break :blk @as(f32, @floatFromInt(v)) / 8388608.0;
            },
            32 => @as(f32, @floatFromInt(std.mem.readInt(i32, d[o..][0..4], .big))) / 2147483648.0,
            else => return AiffError.Unsupported,
        };
    }
    return .{ .samples = out, .sample_rate = sample_rate, .channels = channels };
}

test "extended-float encodes common sample rates" {
    for ([_]u32{ 44100, 48000, 88200, 96000, 192000, 22050 }) |sr| {
        const ext = f64ToExtended(@floatFromInt(sr));
        const back = extendedToF64(&ext);
        try std.testing.expectApproxEqAbs(@as(f64, @floatFromInt(sr)), back, 0.5);
    }
}

test "aiff round-trip (16-bit stereo)" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.0, 0.5, -0.5, 0.25, 0.999, -0.999 }; // 3 stereo frames
    try write("test_rt.aiff", &samples, 48000, 2, 16);
    defer std.fs.cwd().deleteFile("test_rt.aiff") catch {};
    var ad = try read(a, "test_rt.aiff");
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 48000), ad.sample_rate);
    try std.testing.expectEqual(@as(u16, 2), ad.channels);
    try std.testing.expectEqual(samples.len, ad.samples.len);
    for (samples, ad.samples) |orig, got| try std.testing.expect(@abs(orig - got) < 1e-4);
}

test "aiff round-trip (24-bit) is near-lossless" {
    const a = std.testing.allocator;
    const samples = [_]f32{ 0.123456, -0.7654321, 0.5, -0.5, 0.999 };
    try write("test_rt24.aiff", &samples, 44100, 1, 24);
    defer std.fs.cwd().deleteFile("test_rt24.aiff") catch {};
    var ad = try read(a, "test_rt24.aiff");
    defer ad.deinit(a);
    try std.testing.expectEqual(@as(u32, 44100), ad.sample_rate);
    for (samples, ad.samples) |orig, got| try std.testing.expect(@abs(orig - got) < 1e-5);
}
