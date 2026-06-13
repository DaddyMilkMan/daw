//! ui_pretty.zig — a polished DAW frame using the antialiased renderer: vector
//! text, rounded panels, soft shadows, gradients. The "web-quality" target.

const std = @import("std");
const r2d = @import("render2d.zig");
const project = @import("project.zig");
const Color = r2d.Color;
const Canvas = r2d.Canvas;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

const palette = [_]Color{
    .{ .r = 245, .g = 158, .b = 88 },
    .{ .r = 122, .g = 211, .b = 140 },
    .{ .r = 96, .g = 206, .b = 240 },
    .{ .r = 178, .g = 140, .b = 248 },
};

fn mix(a: Color, b: Color, t: f32) Color {
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - t) + @as(f32, @floatFromInt(b.r)) * t),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - t) + @as(f32, @floatFromInt(b.g)) * t),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - t) + @as(f32, @floatFromInt(b.b)) * t),
    };
}

pub fn draw(cv: *Canvas, p: *project.Project, bar: u64) void {
    const W: i32 = @intCast(cv.width);
    const H: i32 = @intCast(cv.height);

    const bg = Color.rgb(20, 22, 28);
    const bg2 = Color.rgb(14, 15, 20);
    const panel = Color.rgb(33, 36, 45);
    const panel_hi = Color.rgb(42, 46, 57);
    const accent = Color.rgb(96, 210, 235);
    const text = Color.rgb(232, 236, 244);
    const dim = Color.rgb(138, 147, 163);

    cv.vGradient(0, 0, W, H, bg, bg2);

    // ---- header ----
    cv.fillRect(0, 0, W, 60, Color.rgb(26, 29, 37));
    cv.fillRect(0, 60, W, 1, Color.rgb(12, 13, 17));
    cv.textAA(24, 16, "Zenith", accent, fd);
    cv.textAA(132, 26, "studio", dim, fb);

    // transport buttons (rounded)
    cv.fillRoundedRect(220, 16, 30, 28, 8, accent);
    cv.textAA(230, 23, ">", Color.rgb(16, 20, 24), fb);
    cv.fillRoundedRect(256, 16, 30, 28, 8, panel_hi);
    cv.fillRoundedRect(266, 25, 10, 10, 2, dim);
    cv.fillRoundedRect(292, 16, 30, 28, 8, panel_hi);
    cv.fillRoundedRect(302, 24, 12, 12, 6, Color.rgb(238, 92, 92));

    cv.textAA(348, 14, "120", text, fd);
    cv.textAA(348, 38, "BPM  4/4", dim, fb);
    cv.textAA(470, 16, "00 : 00 : 04", text, fd);

    // status pill (rounded)
    cv.fillRoundedRect(W - 196, 18, 180, 26, 13, Color.rgb(28, 40, 44));
    cv.fillRoundedRect(W - 188, 26, 10, 10, 5, accent);
    cv.textAA(W - 170, 22, "100% Zig  -  CLAP ready", accent, fb);

    // ---- arrangement ----
    cv.textAA(24, 74, "Arrangement", dim, fb);
    const total: f64 = @floatFromInt(4 * bar);
    const tl_x: i32 = 168;
    const tl_y: i32 = 96;
    const tl_w: i32 = W - tl_x - 24;
    const row_h: i32 = 44;
    const row_gap: i32 = 8;
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;

    var b: usize = 0;
    while (b <= 4) : (b += 1) {
        const gx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(b * bar)) * scale));
        cv.fillRect(gx, tl_y, 1, 4 * (row_h + row_gap) - row_gap, Color.rgb(46, 50, 61));
    }
    for (p.tracks.items, 0..) |t, ti| {
        const ry = tl_y + @as(i32, @intCast(ti)) * (row_h + row_gap);
        const col = palette[ti % palette.len];
        // header card
        cv.fillRoundedRect(24, ry, 134, row_h, 9, panel);
        cv.fillRoundedRect(24, ry + 8, 4, row_h - 16, 2, col);
        cv.textAA(40, ry + 8, t.name.items, text, fb);
        cv.textAA(40, ry + 24, if (t.instrument == .sampler) "Sampler" else "Synth", dim, fb);
        // lane
        cv.fillRoundedRect(tl_x, ry, tl_w, row_h, 8, Color.rgb(28, 31, 39));
        for (t.clips.items) |clip| {
            const cx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.start)) * scale)) + 4;
            const cw = @max(@as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.length)) * scale)) - 6, 10);
            cv.dropShadow(cx, ry + 6, cw, row_h - 12, 7, 3);
            cv.fillRoundedRect(cx, ry + 6, cw, row_h - 12, 7, mix(col, panel, 0.35));
            cv.fillRoundedRect(cx, ry + 6, cw, 16, 7, mix(col, Color.rgb(255, 255, 255), 0.15));
            cv.textAA(cx + 8, ry + 8, clip.name.items, Color.rgb(20, 22, 28), fb);
        }
    }

    // ---- mixer ----
    const mxy: i32 = 320;
    cv.textAA(24, mxy, "Mixer", dim, fb);
    const sw: i32 = 96;
    const sh: i32 = 250;
    const fh: i32 = 168;
    for (p.tracks.items, 0..) |t, ti| {
        const sx = 24 + @as(i32, @intCast(ti)) * (sw + 14);
        const sy = mxy + 22;
        const col = palette[ti % palette.len];
        cv.dropShadow(sx, sy, sw, sh, 12, 4);
        cv.fillRoundedRect(sx, sy, sw, sh, 12, panel);
        cv.fillRoundedRect(sx + 14, sy + 14, sw - 28, 4, 2, col);
        cv.textAA(sx + 14, sy + 24, t.name.items, text, fb);

        // pan
        cv.textAA(sx + 14, sy + 44, "Pan", dim, fb);
        cv.fillRoundedRect(sx + 14, sy + 62, sw - 28, 5, 2, panel_hi);
        const pcx = sx + sw / 2 + @as(i32, @intFromFloat(t.pan * @as(f32, @floatFromInt(sw / 2 - 16))));
        cv.fillRoundedRect(pcx - 4, sy + 58, 9, 13, 4, accent);

        // fader
        const fx = sx + sw / 2 - 4;
        const fy = sy + 78;
        cv.fillRoundedRect(fx, fy, 8, fh, 4, panel_hi);
        const fill_h: i32 = @intFromFloat(t.gain * @as(f32, @floatFromInt(fh)));
        cv.fillRoundedRect(fx, fy + fh - fill_h, 8, fill_h, 4, mix(col, accent, 0.3));
        const knob_y = fy + fh - @as(i32, @intFromFloat(t.gain * @as(f32, @floatFromInt(fh - 18)))) - 18;
        cv.dropShadow(fx - 13, knob_y, 34, 18, 6, 2);
        cv.fillRoundedRect(fx - 13, knob_y, 34, 18, 6, Color.rgb(228, 232, 240));
        cv.fillRoundedRect(fx - 13, knob_y + 8, 34, 2, 1, Color.rgb(120, 126, 140));

        // meter
        const mh: i32 = @intFromFloat(t.gain * 0.9 * @as(f32, @floatFromInt(fh)));
        cv.fillRoundedRect(sx + sw - 22, fy, 9, fh, 4, Color.rgb(18, 20, 25));
        var k: i32 = 0;
        while (k < mh) : (k += 1) {
            const tt = @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(fh));
            const mc = mix(Color.rgb(90, 220, 130), Color.rgb(240, 200, 70), tt);
            cv.fillRect(sx + sw - 22, fy + fh - 1 - k, 9, 1, mc);
        }
    }

    cv.textAA(24, H - 24, "zenith - 100% zig - antialiased renderer", dim, fb);
}
