//! inspect.zig — instrumentation for driving/observing Zenith like a browser.
//!
//! Three capabilities an automation agent needs (the "Playwright for the DAW"):
//!   • snapshot  — the full app state as JSON (the DOM/accessibility-tree analog)
//!   • screenshot — render the current state to a PNG (viewable anywhere)
//!   • logger    — a structured event log streamed to a file
//! All headless and dependency-free, so the app's state is fully observable
//! off-screen (for tests, agents, CI) — not just on a live GPU window.

const std = @import("std");
const r2d = @import("render2d.zig");
const png = @import("png.zig");
const Color = r2d.Color;

pub const TrackView = struct {
    name: []const u8,
    gain: f32 = 0.8,
    pan: f32 = 0.0,
    mute: bool = false,
    solo: bool = false,
    peak: f32 = 0.0, // meter level 0..1
    kind: []const u8 = "audio", // "audio" | "instrument"
};

pub const DawState = struct {
    bpm: f32 = 120,
    playing: bool = false,
    playhead: f32 = 0, // 0..1 across the arrangement
    bars: u32 = 8,
    sample_rate: u32 = 48000,
    master_peak: f32 = 0,
    tracks: []const TrackView = &.{},
};

// --- JSON snapshot (the observable state) ---
pub fn writeJson(state: DawState, path: []const u8) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const w = bw.writer();
    try w.print(
        \\{{
        \\  "transport": {{ "playing": {}, "bpm": {d:.2}, "playhead": {d:.4}, "bars": {d}, "sample_rate": {d} }},
        \\  "master": {{ "peak": {d:.4} }},
        \\  "tracks": [
    , .{ state.playing, state.bpm, state.playhead, state.bars, state.sample_rate, state.master_peak });
    for (state.tracks, 0..) |t, i| {
        try w.print(
            \\
            \\    {{ "name": "{s}", "kind": "{s}", "gain": {d:.3}, "pan": {d:.3}, "mute": {}, "solo": {}, "peak": {d:.4} }}{s}
        , .{ t.name, t.kind, t.gain, t.pan, t.mute, t.solo, t.peak, if (i + 1 < state.tracks.len) "," else "" });
    }
    try w.writeAll("\n  ]\n}\n");
    try bw.flush();
}

// --- screenshot (render the state to a PNG) ---
const BG = Color.rgb(18, 19, 23);
const PANEL = Color.rgb(28, 30, 36);
const ACCENT = Color.rgb(90, 170, 255);
const TXT = Color.rgb(210, 214, 222);
const DIM = Color.rgb(120, 126, 138);

fn meterColor(level: f32) Color {
    if (level > 0.9) return Color.rgb(230, 70, 60);
    if (level > 0.7) return Color.rgb(235, 190, 70);
    return Color.rgb(80, 200, 120);
}

pub fn render(c: *r2d.Canvas, state: DawState) void {
    const W: i32 = @intCast(c.width);
    c.clear(BG);

    // transport bar
    c.fillRect(0, 0, W, 40, PANEL);
    c.text(16, 14, "ZENITH", ACCENT, 2);
    c.fillRect(120, 12, 16, 16, if (state.playing) Color.rgb(80, 200, 120) else DIM);
    c.text(146, 14, if (state.playing) "PLAY" else "STOP", TXT, 1);
    var tb: [32]u8 = undefined;
    const bpm = std.fmt.bufPrint(&tb, "{d:.0} BPM", .{state.bpm}) catch "BPM";
    c.text(220, 14, bpm, TXT, 1);

    // track lanes
    const top: i32 = 52;
    const row_h: i32 = 46;
    const name_w: i32 = 150;
    const tl_x: i32 = name_w + 24;
    const tl_w: i32 = W - tl_x - 16;
    for (state.tracks, 0..) |t, i| {
        const y: i32 = top + @as(i32, @intCast(i)) * row_h;
        c.fillRect(8, y, W - 16, row_h - 6, PANEL);
        c.text(18, y + 8, t.name, TXT, 1);
        c.text(18, y + 24, t.kind, DIM, 1);
        // mute/solo dots
        if (t.mute) c.fillRect(name_w - 30, y + 8, 10, 10, Color.rgb(230, 70, 60));
        if (t.solo) c.fillRect(name_w - 16, y + 8, 10, 10, Color.rgb(235, 190, 70));
        // gain bar
        const gw: i32 = @intFromFloat(@as(f32, 90) * std.math.clamp(t.gain, 0, 1));
        c.fillRect(18, y + 30, gw, 6, ACCENT);
        // meter bar in the timeline area
        const mw: i32 = @intFromFloat(@as(f32, @floatFromInt(tl_w)) * std.math.clamp(t.peak, 0, 1));
        c.fillRect(tl_x, y + 14, mw, 16, meterColor(t.peak));
    }

    // playhead across the arrangement
    const px: i32 = tl_x + @as(i32, @intFromFloat(@as(f32, @floatFromInt(tl_w)) * std.math.clamp(state.playhead, 0, 1)));
    c.fillRect(px, top, 2, @as(i32, @intCast(state.tracks.len)) * row_h, Color.rgb(255, 90, 90));
}

pub fn screenshot(a: std.mem.Allocator, state: DawState, path: []const u8, w: usize, h: usize) !void {
    var canvas = try r2d.Canvas.init(a, w, h);
    defer canvas.deinit();
    render(&canvas, state);
    try png.write(a, path, canvas.pixels, w, h);
}

// --- structured event log ---
pub const Logger = struct {
    file: std.fs.File,
    seq: u64 = 0,

    pub fn open(path: []const u8) !Logger {
        return .{ .file = try std.fs.cwd().createFile(path, .{}) };
    }
    pub fn deinit(self: *Logger) void {
        self.file.close();
    }
    pub fn log(self: *Logger, level: []const u8, comptime fmt: []const u8, args: anytype) void {
        self.seq += 1;
        var buf: [512]u8 = undefined;
        const msg = std.fmt.bufPrint(&buf, fmt, args) catch return;
        var line: [640]u8 = undefined;
        const out = std.fmt.bufPrint(&line, "[{d:0>5}] {s} {s}\n", .{ self.seq, level, msg }) catch return;
        self.file.writeAll(out) catch {};
    }
};

// ===========================================================================
test "json snapshot + png screenshot + log are produced and readable" {
    const a = std.testing.allocator;
    const tracks = [_]TrackView{
        .{ .name = "Drums", .kind = "audio", .gain = 0.8, .peak = 0.62 },
        .{ .name = "Bass", .kind = "instrument", .gain = 0.7, .pan = -0.2, .peak = 0.4, .solo = true },
    };
    const state = DawState{ .playing = true, .bpm = 128, .playhead = 0.5, .master_peak = 0.7, .tracks = &tracks };

    const pid = std.os.linux.getpid();
    var jb: [64]u8 = undefined;
    var pb: [64]u8 = undefined;
    var lb: [64]u8 = undefined;
    const jpath = std.fmt.bufPrint(&jb, "test_inspect.{d}.json", .{pid}) catch "i.json";
    const ppath = std.fmt.bufPrint(&pb, "test_inspect.{d}.png", .{pid}) catch "i.png";
    const lpath = std.fmt.bufPrint(&lb, "test_inspect.{d}.log", .{pid}) catch "i.log";
    defer std.fs.cwd().deleteFile(jpath) catch {};
    defer std.fs.cwd().deleteFile(ppath) catch {};
    defer std.fs.cwd().deleteFile(lpath) catch {};

    try writeJson(state, jpath);
    try screenshot(a, state, ppath, 640, 200);
    var logger = try Logger.open(lpath);
    logger.log("INFO", "transport play at bpm={d:.0}", .{state.bpm});
    logger.deinit();

    // the JSON is non-trivial and the PNG decodes back
    const json = try std.fs.cwd().readFileAlloc(a, jpath, 1 << 20);
    defer a.free(json);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"Drums\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, json, "\"playing\": true") != null);

    const image = @import("image.zig");
    const pbytes = try std.fs.cwd().readFileAlloc(a, ppath, 1 << 22);
    defer a.free(pbytes);
    var img = try image.decodePng(a, pbytes);
    defer img.deinit();
    try std.testing.expectEqual(@as(usize, 640), img.w);
}
