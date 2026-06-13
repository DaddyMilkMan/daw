//! ui.zig — the interactive Zenith DAW view. Polished (antialiased vector text,
//! rounded panels, soft shadows, gradients) AND interactive (play, faders, pans,
//! custom window chrome). Shared by the live window and the headless tests.

const std = @import("std");
const r2d = @import("render2d.zig");
const project = @import("project.zig");
const uikit = @import("uikit.zig");
const Color = r2d.Color;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

pub const WinAction = enum { none, close, minimize, maximize, move };
pub const State = struct { playing: bool = false, window_action: WinAction = .none };

pub const palette = [_]Color{
    .{ .r = 245, .g = 158, .b = 88 },
    .{ .r = 122, .g = 211, .b = 140 },
    .{ .r = 96, .g = 206, .b = 240 },
    .{ .r = 178, .g = 140, .b = 248 },
};

fn mix(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

pub fn buildDemoProject(a: std.mem.Allocator, bar: u64) !project.Project {
    var p = project.Project.init(a);
    const drums = try p.addTrack("Drums", .sampler);
    drums.gain = 0.85;
    inline for (0..4) |i| {
        const c = try drums.addClip("beat", i * bar);
        c.length = bar - 4000;
    }
    const bass = try p.addTrack("Bass", .synth);
    bass.gain = 0.7;
    bass.pan = -0.15;
    {
        const c1 = try bass.addClip("A", 0);
        c1.length = 2 * bar - 4000;
        const c2 = try bass.addClip("B", 2 * bar);
        c2.length = 2 * bar - 4000;
    }
    const lead = try p.addTrack("Lead", .synth);
    lead.gain = 0.6;
    lead.pan = 0.25;
    {
        const c = try lead.addClip("riff", bar);
        c.length = 2 * bar - 4000;
    }
    const pad = try p.addTrack("Pad", .synth);
    pad.gain = 0.5;
    pad.pan = -0.3;
    {
        const c = try pad.addClip("chords", 0);
        c.length = 4 * bar - 4000;
    }
    return p;
}

pub fn frame(ui: *uikit.Ui, p: *project.Project, bar: u64, state: *State) void {
    const cv = ui.cv;
    const W: i32 = @intCast(cv.width);
    const H: i32 = @intCast(cv.height);
    state.window_action = .none;

    const bg = Color.rgb(22, 24, 30);
    const bg2 = Color.rgb(15, 16, 22);
    const panel = Color.rgb(33, 36, 45);
    const lane = Color.rgb(27, 30, 38);
    const accent = Color.rgb(96, 210, 235);
    const text = Color.rgb(232, 236, 244);
    const dim = Color.rgb(140, 148, 164);

    cv.vGradient(0, 0, W, H, bg, bg2);

    // ---- custom title bar / transport ----
    const tb_h: i32 = 52;
    cv.fillRect(0, 0, W, tb_h, Color.rgb(28, 31, 39));
    cv.fillRect(0, tb_h, W, 1, Color.rgb(12, 13, 17));
    cv.textAA(20, 12, "Zenith", accent, fd);

    if (ui.button(1, 150, 15, 30, 24, ">", state.playing)) state.playing = !state.playing;
    cv.fillRoundedRect(186, 15, 30, 24, 7, Color.rgb(40, 44, 54));
    cv.fillRoundedRect(196, 22, 10, 10, 2, dim);
    cv.fillRoundedRect(222, 15, 30, 24, 7, Color.rgb(40, 44, 54));
    cv.fillRoundedRect(232, 23, 8, 8, 4, Color.rgb(235, 88, 88));

    cv.textAA(286, 9, "120", text, fd);
    cv.textAA(286, 34, "BPM  4/4", dim, fb);
    cv.textAA(@divTrunc(W, 2) - 56, 12, "00:00:04", text, fd);
    cv.textAA(W - 280, 12, if (state.playing) "Playing" else "Stopped", if (state.playing) accent else dim, fb);
    cv.textAA(W - 280, 30, "100% Zig · CLAP", dim, fb);

    // window controls
    if (ui.button(900, W - 102, 16, 28, 22, "_", false)) state.window_action = .minimize;
    if (ui.button(901, W - 70, 16, 28, 22, "[]", false)) state.window_action = .maximize;
    if (ui.button(902, W - 38, 16, 28, 22, "x", false)) state.window_action = .close;
    if (state.window_action == .none and ui.pressed and ui.in.my < tb_h and
        (ui.in.mx < 148 or (ui.in.mx > 252 and ui.in.mx < W - 290)))
        state.window_action = .move;

    // ---- arrangement ----
    cv.textAA(20, tb_h + 12, "Arrangement", dim, fb);
    const total: f64 = @floatFromInt(4 * bar);
    const tl_x: i32 = 168;
    const tl_y: i32 = tb_h + 34;
    const tl_w: i32 = W - tl_x - 20;
    const row_h: i32 = 44;
    const row_gap: i32 = 8;
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;

    for (p.tracks.items, 0..) |t, ti| {
        const ry = tl_y + @as(i32, @intCast(ti)) * (row_h + row_gap);
        const col = palette[ti % palette.len];
        cv.fillRoundedRect(20, ry, 138, row_h, 9, panel);
        cv.fillRoundedRect(20, ry + 9, 4, row_h - 18, 2, col);
        cv.textAA(36, ry + 7, t.name.items, text, fb);
        cv.textAA(36, ry + 24, if (t.instrument == .sampler) "Sampler" else "Synth", dim, fb);
        cv.fillRoundedRect(tl_x, ry, tl_w, row_h, 8, lane);
        for (t.clips.items) |clip| {
            const cx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.start)) * scale)) + 4;
            const cw = @max(@as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.length)) * scale)) - 6, 10);
            cv.dropShadow(cx, ry + 6, cw, row_h - 12, 7, 3);
            cv.fillRoundedRect(cx, ry + 6, cw, row_h - 12, 7, mix(col, panel, 0.32));
            cv.fillRoundedRect(cx, ry + 6, cw, 15, 7, mix(col, Color.rgb(255, 255, 255), 0.12));
            cv.textAA(cx + 8, ry + 9, clip.name.items, Color.rgb(20, 22, 28), fb);
        }
    }

    // ---- mixer ----
    const mx_y: i32 = tl_y + 4 * (row_h + row_gap) + 14;
    cv.textAA(20, mx_y, "Mixer", dim, fb);
    cv.textAA(74, mx_y + 2, "drag faders / pans", Color.rgb(96, 102, 118), fb);
    const sw: i32 = 92;
    const sh: i32 = @max(H - (mx_y + 30) - 16, 120);
    const fader_h: i32 = sh - 90;
    for (p.tracks.items, 0..) |*t, ti| {
        const sx = 20 + @as(i32, @intCast(ti)) * (sw + 12);
        const sy = mx_y + 24;
        const col = palette[ti % palette.len];
        cv.dropShadow(sx, sy, sw, sh, 12, 4);
        cv.fillRoundedRect(sx, sy, sw, sh, 12, panel);
        cv.fillRoundedRect(sx + 12, sy + 12, sw - 24, 4, 2, col);
        cv.textAA(sx + 12, sy + 22, t.name.items, text, fb);
        cv.textAA(sx + 12, sy + 40, "Pan", dim, fb);
        _ = ui.hSlider(@intCast(200 + ti), sx + 12, sy + 58, sw - 24, 6, &t.pan, -1.0, 1.0);
        const fx = sx + sw / 2 - 4;
        const fy = sy + 78;
        _ = ui.vFader(@intCast(100 + ti), fx, fy, 8, fader_h, &t.gain);
        // gradient meter
        const mh: i32 = @intFromFloat(t.gain * 0.9 * @as(f32, @floatFromInt(fader_h)));
        cv.fillRoundedRect(sx + sw - 20, fy, 9, fader_h, 4, Color.rgb(18, 20, 25));
        var k: i32 = 0;
        while (k < mh) : (k += 1) {
            const tt = @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(fader_h));
            cv.fillRect(sx + sw - 20, fy + fader_h - 1 - k, 9, 1, mix(Color.rgb(90, 220, 130), Color.rgb(240, 200, 70), tt));
        }
    }

    cv.textAA(20, H - 22, "zenith — 100% zig, no juce", Color.rgb(96, 102, 118), fb);

    if (ui.in.mx >= 0 and ui.in.my >= 0) {
        cv.fillRect(ui.in.mx - 6, ui.in.my, 13, 1, accent);
        cv.fillRect(ui.in.mx, ui.in.my - 6, 1, 13, accent);
    }
}
