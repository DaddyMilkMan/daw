//! main_record.zig — M5: record audio input to a WAV file.
//!
//!   zig build record               (records 3s from the default input)
//!   ZENITH_SECONDS=5 ZENITH_PCM=default ZENITH_REC=take.wav ./zig-out/bin/zenith_record

const std = @import("std");
const alsa = @import("audio_alsa.zig");
const wav = @import("wav.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const ch: u16 = 1;

    const seconds: f32 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f32, v) catch 3.0;
        } else |_| break :blk 3.0;
    };
    const dev_name = std.process.getEnvVarOwned(a, "ZENITH_PCM") catch null;
    defer if (dev_name) |d| a.free(d);
    const out_path = std.process.getEnvVarOwned(a, "ZENITH_REC") catch null;
    defer if (out_path) |p| a.free(p);

    const devZ = try a.dupeZ(u8, dev_name orelse "default");
    defer a.free(devZ);
    const out = out_path orelse "record.wav";

    var in = alsa.StreamIn.open(devZ.ptr, sr, ch, 100_000) catch |e| {
        std.debug.print("capture open failed ({any}) on device '{s}'\n", .{ e, devZ });
        return e;
    };
    defer in.close();
    std.debug.print("recording {d:.1}s from '{s}'...\n", .{ seconds, devZ });

    const block: usize = 480; // 10 ms at 48k
    var ibuf: [480]i16 = undefined;
    const total_frames: usize = @intFromFloat(seconds * @as(f32, @floatFromInt(sr)));

    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    var done: usize = 0;
    while (done < total_frames) {
        const want = @min(block / ch, total_frames - done);
        _ = try in.readBlock(ibuf[0 .. want * ch]);
        for (ibuf[0 .. want * ch]) |s| {
            try rec.append(@as(f32, @floatFromInt(s)) / 32768.0);
        }
        done += want;
    }

    try wav.writePcm16(out, rec.items, sr, ch);
    std.debug.print("recorded {d} frames -> {s}\n", .{ done, out });
}
