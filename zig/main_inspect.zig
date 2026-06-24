//! main_inspect.zig — produce a full observable snapshot of a DAW state:
//! a JSON state dump, a PNG screenshot, and an event log. This is the headless
//! "browser-style" inspection surface an automation agent reads to see the app.

const std = @import("std");
const inspect = @import("inspect.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();

    const tracks = [_]inspect.TrackView{
        .{ .name = "Drums", .kind = "audio", .gain = 0.85, .pan = 0.0, .peak = 0.74 },
        .{ .name = "Bass", .kind = "instrument", .gain = 0.70, .pan = -0.25, .peak = 0.55 },
        .{ .name = "Lead Synth", .kind = "instrument", .gain = 0.60, .pan = 0.30, .peak = 0.93, .solo = true },
        .{ .name = "Vocals", .kind = "audio", .gain = 0.80, .pan = 0.0, .peak = 0.0, .mute = true },
        .{ .name = "FX Return", .kind = "audio", .gain = 0.75, .pan = 0.0, .peak = 0.33 },
    };
    const state = inspect.DawState{
        .playing = true,
        .bpm = 124,
        .playhead = 0.42,
        .bars = 8,
        .master_peak = 0.88,
        .tracks = &tracks,
    };

    try inspect.writeJson(state, "inspect_state.json");
    try inspect.screenshot(a, state, "inspect_screenshot.png", 960, 360);

    var logger = try inspect.Logger.open("inspect.log");
    defer logger.deinit();
    logger.log("INFO", "engine started, sr={d}", .{state.sample_rate});
    logger.log("INFO", "transport PLAY at {d:.0} BPM, playhead={d:.2}", .{ state.bpm, state.playhead });
    logger.log("WARN", "track '{s}' is muted", .{tracks[3].name});
    logger.log("INFO", "track '{s}' soloed", .{tracks[2].name});
    logger.log("METER", "master peak {d:.2} (track '{s}' hot at {d:.2})", .{ state.master_peak, tracks[2].name, tracks[2].peak });

    std.debug.print("inspect: wrote inspect_state.json, inspect_screenshot.png, inspect.log\n", .{});
}
