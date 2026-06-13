//! main_ui.zig — render the first Zenith UI frame: a transport bar, a clip
//! timeline drawn from a real project, and mixer strips. Output to a BMP (the
//! live window blits the same buffer later). 100% Zig.

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const project = @import("project.zig");
const Color = r2d.Color;

const TrackVis = struct { color: Color };
const palette = [_]Color{
    .{ .r = 240, .g = 150, .b = 70 }, // drums - orange
    .{ .r = 110, .g = 200, .b = 120 }, // bass - green
    .{ .r = 90, .g = 200, .b = 240 }, // lead - cyan
    .{ .r = 180, .g = 130, .b = 245 }, // pad - purple
};

fn buildDemo(p: *project.Project, bar: u64) !void {
    const drums = try p.addTrack("DRUMS", .sampler);
    drums.gain = 0.85;
    drums.pan = 0.0;
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
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 960;
    const H: usize = 560;

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();

    // theme
    const bg = Color.rgb(24, 26, 32);
    const panel = Color.rgb(32, 35, 43);
    const panel2 = Color.rgb(40, 44, 54);
    const accent = Color.rgb(90, 210, 230);
    const text_dim = Color.rgb(150, 158, 172);
    const text_hi = Color.rgb(232, 236, 244);

    cv.clear(bg);

    // ---- transport bar ----
    cv.vGradient(0, 0, @intCast(W), 46, Color.rgb(44, 48, 60), Color.rgb(30, 33, 41));
    cv.fillRect(0, 46, @intCast(W), 2, Color.rgb(14, 15, 19));
    cv.text(16, 14, "ZENITH", accent, 2);
    // transport buttons
    cv.fillRect(170, 12, 26, 22, panel2);
    cv.text(178, 15, ">", text_hi, 2); // play
    cv.fillRect(202, 12, 26, 22, panel2);
    cv.fillRect(210, 18, 10, 10, text_dim); // stop square
    cv.fillRect(234, 12, 26, 22, panel2);
    cv.fillRect(244, 17, 6, 6, Color.rgb(235, 80, 80)); // record dot (approx)
    cv.outline(244, 17, 7, 7, Color.rgb(235, 80, 80));
    cv.text(300, 15, "120 BPM", text_hi, 1);
    cv.text(300, 28, "4/4", text_dim, 1);
    cv.text(440, 15, "00:00:04", text_hi, 2);
    cv.text(760, 15, "ZIG  NO JUCE", text_dim, 1);
    cv.text(760, 28, "CLAP READY", accent, 1);

    // ---- build a project to visualize ----
    const bar: u64 = 96000;
    var p = project.Project.init(a);
    defer p.deinit();
    try buildDemo(&p, bar);
    const total: f64 = @floatFromInt(4 * bar);

    // ---- timeline ----
    const tl_x: i32 = 150;
    const tl_y: i32 = 64;
    const tl_w: i32 = 794;
    const row_h: i32 = 40;
    const row_gap: i32 = 6;
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;

    cv.text(16, 56, "ARRANGEMENT", text_dim, 1);
    // bar grid lines
    var b: usize = 0;
    while (b <= 4) : (b += 1) {
        const gx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(b * bar)) * scale));
        cv.fillRect(gx, tl_y - 4, 1, @intCast(4 * (row_h + row_gap) + 8), Color.rgb(48, 52, 63));
    }

    for (p.tracks.items, 0..) |t, ti| {
        const ry = tl_y + @as(i32, @intCast(ti)) * (row_h + row_gap);
        const col = palette[ti % palette.len];
        // header
        cv.fillRect(8, ry, 134, row_h, panel);
        cv.fillRect(8, ry, 4, row_h, col);
        cv.text(20, ry + 8, t.name.items, text_hi, 1);
        const inst = if (t.instrument == .sampler) "SAMPLER" else "SYNTH";
        cv.text(20, ry + 22, inst, text_dim, 1);
        // lane background
        cv.fillRect(tl_x, ry, tl_w, row_h, panel);
        // clips
        for (t.clips.items) |clip| {
            const cx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.start)) * scale));
            const cw = @max(@as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.length)) * scale)), 6);
            cv.fillRect(cx + 1, ry + 4, cw - 2, row_h - 8, .{ .r = col.r, .g = col.g, .b = col.b, .a = 70 });
            cv.fillRect(cx + 1, ry + 4, cw - 2, 3, col);
            cv.outline(cx + 1, ry + 4, cw - 2, row_h - 8, col);
            cv.text(cx + 6, ry + 12, clip.name.items, text_hi, 1);
        }
    }

    // ---- mixer ----
    const mx_y: i32 = 250;
    cv.fillRect(0, mx_y - 8, @intCast(W), 2, Color.rgb(14, 15, 19));
    cv.text(16, mx_y, "MIXER", text_dim, 1);
    const strip_w: i32 = 84;
    const strip_h: i32 = 250;
    const fader_h: i32 = 170;
    for (p.tracks.items, 0..) |t, ti| {
        const sx = 16 + @as(i32, @intCast(ti)) * (strip_w + 12);
        const sy = mx_y + 18;
        const col = palette[ti % palette.len];
        cv.fillRect(sx, sy, strip_w, strip_h, panel);
        cv.fillRect(sx, sy, strip_w, 3, col);
        cv.text(sx + 8, sy + 8, t.name.items, text_hi, 1);
        // pan indicator
        cv.text(sx + 8, sy + 24, "PAN", text_dim, 1);
        const pan_cx = sx + strip_w / 2 + @as(i32, @intFromFloat(t.pan * @as(f32, @floatFromInt(strip_w / 2 - 8))));
        cv.fillRect(sx + 6, sy + 38, strip_w - 12, 4, panel2);
        cv.fillRect(pan_cx - 2, sy + 36, 5, 8, accent);
        // fader track + knob (height ∝ gain)
        const fx = sx + strip_w / 2 - 3;
        const fy = sy + 52;
        cv.fillRect(fx, fy, 6, fader_h, panel2);
        const knob_y = fy + fader_h - @as(i32, @intFromFloat(t.gain * @as(f32, @floatFromInt(fader_h - 14)))) - 14;
        cv.fillRect(fx - 10, knob_y, 26, 14, Color.rgb(70, 76, 90));
        cv.fillRect(fx - 10, knob_y + 6, 26, 2, accent);
        // level meter
        const meter_h: i32 = @intFromFloat(t.gain * @as(f32, @floatFromInt(fader_h)));
        cv.fillRect(sx + strip_w - 14, fy, 8, fader_h, Color.rgb(20, 22, 27));
        cv.fillRect(sx + strip_w - 14, fy + fader_h - meter_h, 8, meter_h, .{ .r = col.r, .g = col.g, .b = col.b, .a = 200 });
    }

    // footer
    cv.text(16, @as(i32, @intCast(H)) - 16, "zenith - 100% zig daw - synth/sampler/mixer/arrangement/clap", text_dim, 1);

    try bmp.write("zenith_ui.bmp", cv.pixels, W, H);
    std.debug.print("rendered Zenith UI frame -> zenith_ui.bmp ({d}x{d})\n", .{ W, H });
}
