//! mp3.zig — MP3 decode via libmpg123 (thin hand-declared C ABI, no -dev headers).
//!
//! MP3 patents expired (2017), so decoding is unencumbered. libmpg123 is the
//! decode "leaf"; we bind its opaque-handle API and decode straight to f32 into
//! the same `AudioData` our other readers produce. (libsndfile 1.0.x here is too
//! old to carry MP3, so we use libmpg123 directly.)

const std = @import("std");
const wav = @import("wav.zig");

pub const AudioData = wav.AudioData;

const mpg123_handle = opaque {};

const MPG123_OK: c_int = 0;
const MPG123_DONE: c_int = -12;
const MPG123_ENC_FLOAT_32: c_int = 0x200;

extern fn mpg123_init() callconv(.c) c_int;
extern fn mpg123_new(decoder: ?[*:0]const u8, err: ?*c_int) callconv(.c) ?*mpg123_handle;
extern fn mpg123_open(mh: *mpg123_handle, path: [*:0]const u8) callconv(.c) c_int;
extern fn mpg123_getformat(mh: *mpg123_handle, rate: *c_long, channels: *c_int, encoding: *c_int) callconv(.c) c_int;
extern fn mpg123_format_none(mh: *mpg123_handle) callconv(.c) c_int;
extern fn mpg123_format(mh: *mpg123_handle, rate: c_long, channels: c_int, encodings: c_int) callconv(.c) c_int;
extern fn mpg123_read(mh: *mpg123_handle, out: [*]u8, size: usize, done: *usize) callconv(.c) c_int;
extern fn mpg123_close(mh: *mpg123_handle) callconv(.c) c_int;
extern fn mpg123_delete(mh: *mpg123_handle) callconv(.c) void;

pub const Mp3Error = error{ Init, Open, Format };

var g_init = std.once(initOnce);
fn initOnce() void {
    _ = mpg123_init();
}

/// Decode an MP3 file to interleaved f32 (the file's native rate/channels).
pub fn decode(a: std.mem.Allocator, path: [:0]const u8) !AudioData {
    g_init.call();
    const mh = mpg123_new(null, null) orelse return Mp3Error.Init;
    defer mpg123_delete(mh);
    if (mpg123_open(mh, path.ptr) != MPG123_OK) return Mp3Error.Open;
    defer _ = mpg123_close(mh);

    var rate: c_long = 0;
    var channels: c_int = 0;
    var enc: c_int = 0;
    if (mpg123_getformat(mh, &rate, &channels, &enc) != MPG123_OK) return Mp3Error.Format;

    // pin the output format to float32 at the file's native rate/channels
    _ = mpg123_format_none(mh);
    _ = mpg123_format(mh, rate, channels, MPG123_ENC_FLOAT_32);

    var pcm = std.ArrayList(f32).init(a);
    errdefer pcm.deinit();
    var fblock: [4096]f32 = undefined; // f32-aligned decode buffer
    const bytes: [*]u8 = @ptrCast(&fblock);
    while (true) {
        var done: usize = 0; // bytes produced
        const r = mpg123_read(mh, bytes, fblock.len * @sizeOf(f32), &done);
        if (done > 0) try pcm.appendSlice(fblock[0 .. done / @sizeOf(f32)]);
        if (r == MPG123_DONE) break;
        if (r != MPG123_OK) break; // new-format or error: stop cleanly
    }

    return .{
        .samples = try pcm.toOwnedSlice(),
        .sample_rate = @intCast(rate),
        .channels = @intCast(channels),
    };
}

test "MP3 decode produces sane PCM (uses a system .mp3 if available)" {
    const a = std.testing.allocator;
    // Try a few likely-present sample MP3s; skip if none exist on this box.
    const candidates = [_][:0]const u8{
        "/opt/PearAI/app/out/vs/platform/accessibilitySignal/browser/media/warning.mp3",
        "/usr/share/sounds/alsa/test.mp3",
        "testdata/sample.mp3",
    };
    var chosen: ?[:0]const u8 = null;
    for (candidates) |c| {
        if (std.fs.cwd().access(c, .{})) |_| {
            chosen = c;
            break;
        } else |_| {}
    }
    const path = chosen orelse return error.SkipZigTest;

    var ad = try decode(a, path);
    defer ad.deinit(a);
    try std.testing.expect(ad.samples.len > 0);
    try std.testing.expect(ad.channels >= 1 and ad.channels <= 2);
    try std.testing.expect(ad.sample_rate >= 8000 and ad.sample_rate <= 192000);
}
