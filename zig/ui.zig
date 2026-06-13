//! ui.zig — the Zenith DAW view. Draws transport + clip timeline + mixer from a
//! project, with interactive widgets (play button, faders, pans) via uikit.
//! Used by the BMP renderer, the live window, and the headless UI test.

const std = @import("std");
const r2d = @import("render2d.zig");
const project = @import("project.zig");
const uikit = @import("uikit.zig");
const Color = r2d.Color;

pub const State = struct { playing: bool = false };

pub const palette = [_]Color{
    .{ .r = 240, .g = 150, .b = 70 },
    .{ .r = 110, .g = 200, .b = 120 },
    .{ .r = 90, .g = 200, .b = 240 },
    .{ .r = 180, .g = 130, .b = 245 },
};

pub fn buildDemoProject(a: std.mem.Allocator, bar: u64) !project.Project {
    var p = project.Project.init(a);
    const drums = try p.addTrack("DRUMS", .sampler);
    drums.gain = 0.85;
    inline for (0..4) |i| {
        const c = try drums.addClip("beat", i * bar);
        c.length = bar - 4000;
    }
    const bass = try p.addTrack("BASS", .synth);
    bass.gain = 0.7;
    bass.pan = -0.15;
    {
        const c1 = try bass.addClip("A", 0);
        c1.length = 2 * bar - 4000;
        const c2 = try bass.addClip("B", 2 * bar);
        c2.length = 2 * bar - 4000;
    }
    const lead = try p.addTrack("LEAD", .synth);
    lead.gain = 0.6;
    lead.pan = 0.25;
    {
        const c = try lead.addClip("riff", bar);
        c.length = 2 * bar - 4000;
    }
    const pad = try p.addTrack("PAD", .synth);
    pad.gain = 0.5;
    pad.pan = -0.3;
    {
        const c = try pad.addClip("chords", 0);
        c.length = 4 * bar - 4000;
    }
    return p;
}

/// Draw + handle one interactive frame.
pub fn frame(ui: *uikit.Ui, p: *project.Project, bar: u64, state: *State) void {
    const cv = ui.cv;
    const W: i32 = @intCast(cv.width);
    const H: i32 = @intCast(cv.height);

    const bg = Color.rgb(24, 26, 32);
    const panel = Color.rgb(32, 35, 43);
    const accent = Color.rgb(90, 210, 230);
    const text_dim = Color.rgb(150, 158, 172);
    const text_hi = Color.rgb(232, 236, 244);

    cv.clear(bg);

    // transport
    cv.vGradient(0, 0, W, 46, Color.rgb(44, 48, 60), Color.rgb(30, 33, 41));
    cv.fillRect(0, 46, W, 2, Color.rgb(14, 15, 19));
    cv.text(16, 14, "ZENITH", accent, 2);
    if (ui.button(1, 170, 12, 26, 22, ">", state.playing)) state.playing = !state.playing;
    cv.fillRect(202, 12, 26, 22, Color.rgb(40, 44, 54));
    cv.fillRect(210, 18, 10, 10, text_dim);
    cv.fillRect(234, 12, 26, 22, Color.rgb(40, 44, 54));
    cv.fillRect(244, 17, 7, 7, Color.rgb(235, 80, 80));
    cv.text(300, 15, "120 BPM", text_hi, 1);
    cv.text(300, 28, "4/4", text_dim, 1);
    cv.text(440, 15, "00:00:04", text_hi, 2);
    cv.text(760, 15, if (state.playing) "PLAYING" else "STOPPED", if (state.playing) accent else text_dim, 1);
    cv.text(760, 28, "ZIG / CLAP", text_dim, 1);

    // timeline (static)
    const total: f64 = @floatFromInt(4 * bar);
    const tl_x: i32 = 150;
    const tl_y: i32 = 64;
    const tl_w: i32 = W - tl_x - 16;
    const row_h: i32 = 40;
    const row_gap: i32 = 6;
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;

    cv.text(16, 56, "ARRANGEMENT", text_dim, 1);
    var b: usize = 0;
    while (b <= 4) : (b += 1) {
        const gx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(b * bar)) * scale));
        cv.fillRect(gx, tl_y - 4, 1, 4 * (row_h + row_gap) + 8, Color.rgb(48, 52, 63));
    }
    for (p.tracks.items, 0..) |t, ti| {
        const ry = tl_y + @as(i32, @intCast(ti)) * (row_h + row_gap);
        const col = palette[ti % palette.len];
        cv.fillRect(8, ry, 134, row_h, panel);
        cv.fillRect(8, ry, 4, row_h, col);
        cv.text(20, ry + 8, t.name.items, text_hi, 1);
        cv.text(20, ry + 22, if (t.instrument == .sampler) "SAMPLER" else "SYNTH", text_dim, 1);
        cv.fillRect(tl_x, ry, tl_w, row_h, panel);
        for (t.clips.items) |clip| {
            const cx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.start)) * scale));
            const cw = @max(@as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.length)) * scale)), 6);
            cv.fillRect(cx + 1, ry + 4, cw - 2, row_h - 8, .{ .r = col.r, .g = col.g, .b = col.b, .a = 70 });
            cv.fillRect(cx + 1, ry + 4, cw - 2, 3, col);
            cv.outline(cx + 1, ry + 4, cw - 2, row_h - 8, col);
            cv.text(cx + 6, ry + 12, clip.name.items, text_hi, 1);
        }
    }

    // mixer (interactive)
    const mx_y: i32 = 250;
    cv.fillRect(0, mx_y - 8, W, 2, Color.rgb(14, 15, 19));
    cv.text(16, mx_y, "MIXER  (drag faders / pans)", text_dim, 1);
    const strip_w: i32 = 84;
    const strip_h: i32 = 250;
    const fader_h: i32 = 160;
    for (p.tracks.items, 0..) |*t, ti| {
        const sx = 16 + @as(i32, @intCast(ti)) * (strip_w + 12);
        const sy = mx_y + 18;
        const col = palette[ti % palette.len];
        cv.fillRect(sx, sy, strip_w, strip_h, panel);
        cv.fillRect(sx, sy, strip_w, 3, col);
        cv.text(sx + 8, sy + 8, t.name.items, text_hi, 1);
        cv.text(sx + 8, sy + 24, "PAN", text_dim, 1);
        _ = ui.hSlider(@intCast(200 + ti), sx + 6, sy + 38, strip_w - 12, 6, &t.pan, -1.0, 1.0);
        _ = ui.vFader(@intCast(100 + ti), sx + strip_w / 2 - 3, sy + 56, 6, fader_h, &t.gain);
        // level meter mirrors gain
        const meter_h: i32 = @intFromFloat(t.gain * @as(f32, @floatFromInt(fader_h)));
        cv.fillRect(sx + strip_w - 14, sy + 56, 8, fader_h, Color.rgb(20, 22, 27));
        cv.fillRect(sx + strip_w - 14, sy + 56 + fader_h - meter_h, 8, meter_h, .{ .r = col.r, .g = col.g, .b = col.b, .a = 200 });
    }

    cv.text(16, H - 16, "zenith - 100% zig daw", text_dim, 1);

    // cursor
    if (ui.in.mx >= 0 and ui.in.my >= 0) {
        cv.fillRect(ui.in.mx - 6, ui.in.my, 13, 1, accent);
        cv.fillRect(ui.in.mx, ui.in.my - 6, 1, 13, accent);
    }
}
