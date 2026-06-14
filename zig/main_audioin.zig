//! main_audioin.zig — `zig build audioin`. Two jobs:
//!   1. Enumerate every audio device/channel the OS exposes (audio_devices.zig).
//!   2. Open the default capture stream and show live per-channel input levels,
//!      proving we actually RECEIVE audio from the OS driver (ALSA -> PipeWire).
//!
//! Runtime is capped by ZENITH_AUDIO_SECONDS (default 5) for automation.

const std = @import("std");
const devs = @import("audio_devices.zig");
const alsa = @import("audio_alsa.zig");

fn bar(w: *const std.io.AnyWriter, level: f32, width: usize) void {
    const n: usize = @intFromFloat(std.math.clamp(level, 0, 1) * @as(f32, @floatFromInt(width)));
    var i: usize = 0;
    while (i < width) : (i += 1) {
        _ = w.writeAll(if (i < n) "#" else "-") catch {};
    }
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const stdout = std.io.getStdOut().writer();
    var out = stdout.any();

    // ---- 1. enumerate all devices / channels --------------------------------
    const list = try devs.list(a);
    defer devs.freeList(a, list);
    try out.print("\n=== Audio devices on this PC ({d}) ===\n", .{list.len});
    var n_in: usize = 0;
    var n_out: usize = 0;
    for (list) |d| {
        const tag = switch (d.dir) {
            .input => "IN ",
            .output => "OUT",
            .duplex => "I/O",
        };
        if (d.dir != .output) n_in += 1;
        if (d.dir != .input) n_out += 1;
        try out.print("  [{s}] {s}\n        {s}\n        channels {d}..{d}, rate {d}..{d} Hz\n", .{
            tag, d.name, d.desc, d.ch_min, d.ch_max, d.rate_min, d.rate_max,
        });
    }
    try out.print("  -> {d} capture-capable, {d} playback-capable\n", .{ n_in, n_out });

    // ---- 2. receive audio: open default capture + meter the channels --------
    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_AUDIO_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 5.0;
        } else |_| break :blk 5.0;
    };
    const rate: u32 = 48000;
    var channels: u16 = 2;
    var in = alsa.StreamIn.open("default", rate, channels, 100_000) catch blk: {
        channels = 1;
        break :blk alsa.StreamIn.open("default", rate, channels, 100_000) catch {
            try out.print("\n(no capture device available — skipping live meter)\n", .{});
            return;
        };
    };
    defer in.close();
    try out.print("\n=== Live input — capturing 'default' @ {d} Hz, {d} ch ===\n", .{ rate, channels });

    const frames: usize = 1024;
    const buf = try a.alloc(i16, frames * channels);
    defer a.free(buf);
    var peak = try a.alloc(f32, channels);
    defer a.free(peak);

    const blocks: usize = @intFromFloat(secs * @as(f64, @floatFromInt(rate)) / @as(f64, @floatFromInt(frames)));
    var b: usize = 0;
    while (b < blocks) : (b += 1) {
        const got = in.readBlock(buf) catch break;
        @memset(peak, 0);
        var f: usize = 0;
        while (f < got) : (f += 1) {
            for (0..channels) |c| {
                const s = @abs(@as(f32, @floatFromInt(buf[f * channels + c])) / 32768.0);
                if (s > peak[c]) peak[c] = s;
            }
        }
        // redraw a meter line per channel (carriage return, single updating row)
        _ = out.writeAll("\r") catch {};
        for (0..channels) |c| {
            try out.print("ch{d} ", .{c});
            bar(&out, peak[c], 28);
            try out.print(" {d:>3.0}%  ", .{peak[c] * 100});
        }
    }
    try out.print("\nDone — received {d} blocks of real audio.\n", .{b});
}
