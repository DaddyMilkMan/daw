//! ui.zig — the interactive Zenith DAW view. Docked layout (title bar / left
//! browser / arrangement / mixer dock) that fills the window at any size, with
//! web-grade rendering: gradient glass surfaces, hairline rims, soft glows,
//! a timeline ruler, mute/solo + sends, vector icons, MIDI note previews.

const std = @import("std");
const r2d = @import("render2d.zig");
const project = @import("project.zig");
const uikit = @import("uikit.zig");
const icons = @import("icons.zig");
const L = @import("layout.zig").Box;
const Color = r2d.Color;

const fb = &@import("font_body.zig").font; // 15
const fu = &@import("font_ui.zig").font; // 18
const fd = &@import("font_display.zig").font; // 30 bold

pub const WinAction = enum { none, close, minimize, maximize, move };
pub const State = struct {
    playing: bool = false,
    window_action: WinAction = .none,
    sel_track: i32 = -1,
    sel_clip: i32 = -1,
    master_gain: f32 = 0.8,
    sends: [8][2]f32 = .{
        .{ 0.28, 0.10 }, .{ 0.40, 0.16 }, .{ 0.22, 0.30 }, .{ 0.34, 0.12 },
        .{ 0.18, 0.08 }, .{ 0.30, 0.20 }, .{ 0.26, 0.14 }, .{ 0.36, 0.18 },
    },
    mutes: [8]bool = [_]bool{false} ** 8,
    solos: [8]bool = [_]bool{false} ** 8,
};

const palette = [_]Color{
    .{ .r = 245, .g = 158, .b = 88 }, // orange
    .{ .r = 122, .g = 211, .b = 140 }, // green
    .{ .r = 96, .g = 206, .b = 240 }, // cyan
    .{ .r = 178, .g = 140, .b = 248 }, // violet
    .{ .r = 240, .g = 132, .b = 170 }, // pink
};

// Refined "web dashboard" palette: bluish charcoal, layered elevation.
const c_bg_top = Color.rgb(26, 28, 36);
const c_bg_bot = Color.rgb(13, 14, 19);
const c_panel_top = Color.rgb(37, 40, 51);
const c_panel_bot = Color.rgb(27, 29, 38);
const c_card_top = Color.rgb(47, 51, 64);
const c_card_bot = Color.rgb(34, 37, 48);
const c_lane = Color.rgb(21, 23, 30);
const c_lane_alt = Color.rgb(24, 26, 34);
const c_border = Color.rgba(255, 255, 255, 16);
const c_rim = Color.rgba(255, 255, 255, 30);
const c_grid = Color.rgba(255, 255, 255, 10);
const c_accent = Color.rgb(96, 210, 235);
const c_accent2 = Color.rgb(178, 140, 248);
const c_text = Color.rgb(236, 239, 246);
const c_dim = Color.rgb(140, 148, 164);
const c_faint = Color.rgb(96, 103, 119);
const c_amber = Color.rgb(240, 176, 72);
const c_green = Color.rgb(122, 211, 140);
const c_red = Color.rgb(238, 96, 96);

fn mix(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

/// A glass card: gradient fill, hairline border, soft top rim highlight.
fn card(cv: *r2d.Canvas, b: L, radius: i32, top: Color, bot: Color) void {
    cv.fillRoundedRectV(b.x, b.y, b.w, b.h, radius, top, bot);
    cv.strokeRoundedRect(b.x, b.y, b.w, b.h, radius, c_border);
    cv.fillRoundedRect(b.x + radius, b.y + 1, b.w - 2 * radius, 1, 0, c_rim);
}

fn label(cv: *r2d.Canvas, x: i32, y: i32, s: []const u8) void {
    cv.textAA(x, y, s, c_faint, fb);
}

fn fillClip(c: *project.Clip, pitches: []const u8) !void {
    const n = pitches.len;
    if (n == 0) return;
    const step = c.length / n;
    for (pitches, 0..) |pitch, i| {
        try c.notes.append(.{ .start = @as(u64, i) * step, .len = step * 3 / 4, .pitch = pitch, .velocity = 100 });
    }
}

pub fn buildDemoProject(a: std.mem.Allocator, bar: u64) !project.Project {
    var p = project.Project.init(a);
    const drums = try p.addTrack("Drums", .sampler);
    drums.gain = 0.85;
    inline for (0..4) |i| {
        const c = try drums.addClip("Beat", i * bar);
        c.length = bar - 4000;
        try fillClip(c, &[_]u8{ 36, 42, 38, 42, 36, 42, 38, 45 });
    }
    const bass = try p.addTrack("Bass", .synth);
    bass.gain = 0.7;
    bass.pan = -0.15;
    {
        const c1 = try bass.addClip("Verse", 0);
        c1.length = 2 * bar - 4000;
        try fillClip(c1, &[_]u8{ 40, 40, 43, 45, 40, 47, 43, 45 });
        const c2 = try bass.addClip("Drop", 2 * bar);
        c2.length = 2 * bar - 4000;
        try fillClip(c2, &[_]u8{ 45, 45, 48, 50, 45, 52, 48, 43 });
    }
    const lead = try p.addTrack("Lead", .synth);
    lead.gain = 0.6;
    lead.pan = 0.25;
    {
        const c = try lead.addClip("Riff", bar);
        c.length = 2 * bar - 4000;
        try fillClip(c, &[_]u8{ 72, 76, 79, 76, 74, 72, 71, 69 });
    }
    const keys = try p.addTrack("Keys", .synth);
    keys.gain = 0.65;
    keys.pan = 0.1;
    {
        const c = try keys.addClip("Stab", 2 * bar);
        c.length = bar + bar / 2 - 4000;
        try fillClip(c, &[_]u8{ 60, 67, 64, 69, 60, 72, 67, 64 });
    }
    const pad = try p.addTrack("Pad", .synth);
    pad.gain = 0.5;
    pad.pan = -0.3;
    {
        const c = try pad.addClip("Chords", 0);
        c.length = 4 * bar - 4000;
        try fillClip(c, &[_]u8{ 60, 64, 67, 72, 65, 69, 72, 60 });
    }
    return p;
}

fn drawClipNotes(cv: *r2d.Canvas, clip: project.Clip, b: L, nc: Color) void {
    if (clip.length == 0) return;
    const lo: f32 = 32;
    const hi: f32 = 92;
    for (clip.notes.items) |note| {
        const nx = b.x + @as(i32, @intFromFloat(@as(f32, @floatFromInt(note.start)) / @as(f32, @floatFromInt(clip.length)) * @as(f32, @floatFromInt(b.w))));
        const nw = @max(@as(i32, @intFromFloat(@as(f32, @floatFromInt(note.len)) / @as(f32, @floatFromInt(clip.length)) * @as(f32, @floatFromInt(b.w)))), 2);
        const pn = std.math.clamp((@as(f32, @floatFromInt(note.pitch)) - lo) / (hi - lo), 0.0, 1.0);
        const ny = b.y + b.h - 3 - @as(i32, @intFromFloat(pn * @as(f32, @floatFromInt(@max(b.h - 6, 1)))));
        cv.fillRoundedRect(nx + 1, ny, nw - 1, 3, 1, nc);
    }
}

/// Small labelled toggle (M/S). Hit-tested via iconSlot, tinted when active.
fn miniToggle(ui: *uikit.Ui, id: u32, x: i32, y: i32, w: i32, h: i32, lbl: []const u8, on: bool, oncol: Color) bool {
    const cv = ui.cv;
    const clicked = ui.iconSlot(id, x, y, w, h, false);
    if (on) cv.fillRoundedRect(x, y, w, h, 5, oncol);
    const tw = r2d.Canvas.textWidth(lbl, fb);
    cv.textAA(x + @divTrunc(w - tw, 2), y + @divTrunc(h - 12, 2), lbl, if (on) Color.rgb(18, 20, 26) else c_dim, fb);
    return clicked;
}

fn titleBar(ui: *uikit.Ui, tb: L, state: *State) void {
    const cv = ui.cv;
    const W = tb.w;
    cv.vGradient(tb.x, tb.y, tb.w, tb.h, Color.rgb(34, 37, 47), Color.rgb(24, 26, 34));
    cv.fillRect(tb.x, tb.y, tb.w, 1, c_rim);
    cv.fillRect(tb.x, tb.y + tb.h - 1, tb.w, 1, Color.rgb(9, 10, 14));

    cv.textAA(22, tb.y + 13, "Zenith", c_accent, fd);

    // transport — a segmented "glass" control housing the icons
    const grp = L{ .x = 146, .y = tb.y + 12, .w = 138, .h = 34 };
    card(cv, grp, 9, Color.rgb(30, 33, 42), Color.rgb(23, 25, 33));
    const ty = tb.y + 15;
    if (state.playing) cv.glow(170, ty + 14, 22, Color.rgba(96, 210, 235, 110));
    const picol = if (state.playing) Color.rgb(14, 18, 22) else c_text;
    if (ui.iconSlot(1, 151, ty, 38, 28, state.playing)) state.playing = !state.playing;
    if (state.playing) icons.pause(cv, 170, ty + 14, 13, picol) else icons.play(cv, 170, ty + 14, 14, picol);
    _ = ui.iconSlot(2, 192, ty, 38, 28, false);
    icons.stop(cv, 211, ty + 14, 12, c_dim);
    _ = ui.iconSlot(3, 233, ty, 38, 28, false);
    icons.record(cv, 252, ty + 14, 12, c_red);

    cv.fillRect(300, tb.y + 12, 1, 34, c_border);
    cv.textAA(316, tb.y + 8, "120", c_text, fd);
    cv.textAA(316, tb.y + 38, "BPM   4 / 4", c_dim, fb);
    cv.fillRect(412, tb.y + 12, 1, 34, c_border);
    cv.textAA(@divTrunc(W, 2) - 64, tb.y + 12, "00 : 00 : 04", c_text, fd);

    // status cluster (right, before window controls)
    const dotc = if (state.playing) c_green else c_faint;
    cv.fillRoundedRect(W - 336, tb.y + 18, 7, 7, 3, dotc);
    if (state.playing) cv.glow(W - 333, tb.y + 21, 8, Color.rgba(122, 211, 140, 150));
    cv.textAA(W - 322, tb.y + 12, if (state.playing) "Playing" else "Stopped", if (state.playing) c_accent else c_dim, fu);
    cv.textAA(W - 322, tb.y + 34, "100% Zig  -  CLAP ready", c_faint, fb);

    // window controls
    if (ui.iconSlot(900, W - 108, tb.y + 16, 30, 24, false)) state.window_action = .minimize;
    icons.minimize(cv, W - 93, tb.y + 28, 11, c_dim);
    if (ui.iconSlot(901, W - 74, tb.y + 16, 30, 24, false)) state.window_action = .maximize;
    icons.maximize(cv, W - 59, tb.y + 27, 10, c_dim);
    if (ui.iconSlot(902, W - 40, tb.y + 16, 30, 24, false)) state.window_action = .close;
    icons.close(cv, W - 25, tb.y + 28, 10, if (ui.hoverOf(902) > 0.1) Color.rgb(248, 120, 120) else c_dim);

    if (state.window_action == .none and ui.pressed and ui.in.my < tb.y + tb.h and
        (ui.in.mx < 144 or (ui.in.mx > 286 and ui.in.mx < W - 346)))
        state.window_action = .move;
}

fn browser(ui: *uikit.Ui, b: L) void {
    const cv = ui.cv;
    cv.vGradient(b.x, b.y, b.w, b.h, c_panel_top, c_panel_bot);
    cv.fillRect(b.x + b.w - 1, b.y, 1, b.h, Color.rgb(9, 10, 14));
    cv.fillRect(b.x, b.y, b.w, 1, c_rim);

    var y = b.y + 16;
    label(cv, b.x + 18, y, "BROWSER");
    y += 26;

    const cats = [_]struct { name: []const u8, col: Color }{
        .{ .name = "Sounds", .col = c_accent },
        .{ .name = "Drums", .col = palette[0] },
        .{ .name = "Instruments", .col = palette[3] },
        .{ .name = "Audio FX", .col = palette[1] },
        .{ .name = "MIDI FX", .col = palette[4] },
        .{ .name = "Samples", .col = c_accent2 },
        .{ .name = "Plug-ins", .col = c_dim },
    };
    for (cats, 0..) |cat, i| {
        const rh: i32 = 30;
        const ry = y + @as(i32, @intCast(i)) * rh;
        const sel = (i == 0);
        if (sel) {
            cv.fillRoundedRectV(b.x + 8, ry, b.w - 18, rh - 4, 7, Color.rgb(45, 60, 70), Color.rgb(36, 47, 56));
            cv.strokeRoundedRect(b.x + 8, ry, b.w - 18, rh - 4, 7, c_border);
            cv.fillRoundedRect(b.x + 8, ry + 5, 3, rh - 14, 1, c_accent);
        }
        cv.fillRoundedRect(b.x + 22, ry + 9, 8, 8, 2, cat.col);
        cv.textAA(b.x + 40, ry + 5, cat.name, if (sel) c_text else c_dim, fb);
    }
    y += @as(i32, @intCast(cats.len)) * 30 + 14;

    cv.fillRect(b.x + 16, y, b.w - 34, 1, c_border);
    y += 14;
    label(cv, b.x + 18, y, "DEVICES");
    y += 26;
    const devs = [_]struct { name: []const u8, tag: []const u8, col: Color }{
        .{ .name = "Operator", .tag = "INST", .col = palette[3] },
        .{ .name = "Analog", .tag = "INST", .col = palette[3] },
        .{ .name = "Reverb", .tag = "FX", .col = palette[1] },
        .{ .name = "EQ Eight", .tag = "FX", .col = palette[1] },
        .{ .name = "Compressor", .tag = "FX", .col = palette[1] },
        .{ .name = "Saturator", .tag = "FX", .col = palette[1] },
    };
    for (devs, 0..) |dev, i| {
        const rh: i32 = 28;
        const ry = y + @as(i32, @intCast(i)) * rh;
        if (ry + rh > b.y + b.h - 8) break;
        cv.fillRoundedRect(b.x + 22, ry + 9, 6, 6, 2, dev.col);
        cv.textAA(b.x + 38, ry + 4, dev.name, c_dim, fb);
        const tw = r2d.Canvas.textWidth(dev.tag, fb);
        cv.fillRoundedRect(b.x + b.w - 22 - tw - 10, ry + 4, tw + 10, 18, 5, Color.rgb(30, 33, 42));
        cv.textAA(b.x + b.w - 22 - tw - 5, ry + 4, dev.tag, c_faint, fb);
    }
}

fn ruler(cv: *r2d.Canvas, x: i32, y: i32, w: i32, h: i32, bars: i32) void {
    cv.fillRoundedRectV(x, y, w, h, 6, Color.rgb(30, 33, 42), Color.rgb(25, 27, 35));
    cv.strokeRoundedRect(x, y, w, h, 6, c_border);
    const bw = @divTrunc(w, bars);
    var i: i32 = 0;
    while (i < bars) : (i += 1) {
        const bx = x + i * bw;
        if (i > 0) cv.fillRect(bx, y + 4, 1, h - 8, c_border);
        var buf: [8]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{i + 1}) catch "";
        cv.textAA(bx + 8, y + @divTrunc(h - 12, 2), s, c_dim, fb);
        // beat ticks
        var be: i32 = 1;
        while (be < 4) : (be += 1) {
            const tx = bx + @divTrunc(bw * be, 4);
            cv.fillRect(tx, y + h - 7, 1, 4, c_grid);
        }
    }
}

fn arrangement(ui: *uikit.Ui, p: *project.Project, bar: u64, region: L, state: *State) void {
    const cv = ui.cv;
    label(cv, region.x + 8, region.y + 4, "ARRANGEMENT");
    var area = region;
    area.gapTop(24);
    const cardBox = L{ .x = region.x + 8, .y = area.y, .w = region.w - 16, .h = area.h - 8 };
    card(cv, cardBox, 12, c_panel_top, c_panel_bot);

    const inner = cardBox.shrink(12);
    const ntracks: i32 = @intCast(p.tracks.items.len);
    const hdr_w: i32 = 178;
    const tl_x = inner.x + hdr_w + 8;
    const tl_w = inner.w - hdr_w - 8;
    const bars: i32 = 4;
    const total: f64 = @floatFromInt(@as(i32, bars) * @as(i32, @intCast(bar)));
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;
    const bar_px = @divTrunc(tl_w, bars);

    const ruler_h: i32 = 24;
    ruler(cv, tl_x, inner.y, tl_w, ruler_h, bars);

    const grid_top = inner.y + ruler_h + 6;
    const rows_h = inner.h - ruler_h - 6;
    const gap: i32 = 6;
    const row_h: i32 = std.math.clamp(@divTrunc(rows_h - (ntracks - 1) * gap, ntracks), 28, 96);

    for (p.tracks.items, 0..) |t, ti| {
        const ry = grid_top + @as(i32, @intCast(ti)) * (row_h + gap);
        const col = palette[ti % palette.len];

        // track header
        const hdr = L{ .x = inner.x, .y = ry, .w = hdr_w, .h = row_h };
        card(cv, hdr, 9, c_card_top, c_card_bot);
        cv.fillRoundedRect(inner.x + 8, ry + 9, 4, row_h - 18, 2, col);
        const name_y = if (row_h >= 46) ry + 9 else ry + @divTrunc(row_h - 16, 2);
        cv.textAA(inner.x + 22, name_y, t.name.items, c_text, fu);
        if (row_h >= 46) cv.textAA(inner.x + 22, ry + 31, if (t.instrument == .sampler) "Sampler" else "Synth", c_dim, fb);
        if (miniToggle(ui, 340 + @as(u32, @intCast(ti)), inner.x + hdr_w - 46, ry + 9, 18, 15, "M", state.mutes[ti], c_amber)) state.mutes[ti] = !state.mutes[ti];
        if (miniToggle(ui, 360 + @as(u32, @intCast(ti)), inner.x + hdr_w - 25, ry + 9, 18, 15, "S", state.solos[ti], c_green)) state.solos[ti] = !state.solos[ti];

        // lane with bar gridlines
        const lane_bg = if (ti % 2 == 0) c_lane else c_lane_alt;
        cv.fillRoundedRect(tl_x, ry, tl_w, row_h, 8, lane_bg);
        var g: i32 = 1;
        while (g < bars) : (g += 1) cv.fillRect(tl_x + g * bar_px, ry + 4, 1, row_h - 8, c_grid);

        const muted = state.mutes[ti];
        for (t.clips.items, 0..) |clip, ci| {
            const cx = tl_x + @as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.start)) * scale)) + 4;
            const cw = @max(@as(i32, @intFromFloat(@as(f64, @floatFromInt(clip.length)) * scale)) - 6, 12);
            const cb = L{ .x = cx, .y = ry + 5, .w = cw, .h = row_h - 10 };
            const hovered = ui.in.mx >= cb.x and ui.in.mx < cb.x + cb.w and ui.in.my >= cb.y and ui.in.my < cb.y + cb.h;
            if (hovered and ui.pressed) {
                state.sel_track = @intCast(ti);
                state.sel_clip = @intCast(ci);
            }
            const selected = state.sel_track == @as(i32, @intCast(ti)) and state.sel_clip == @as(i32, @intCast(ci));
            const cc = if (muted) mix(col, Color.rgb(70, 74, 86), 0.7) else col;
            cv.dropShadow(cb.x, cb.y, cb.w, cb.h, 7, 3);
            cv.fillRoundedRectV(cb.x, cb.y, cb.w, cb.h, 7, mix(cc, Color.rgb(255, 255, 255), if (hovered) 0.26 else 0.12), mix(cc, c_panel_bot, 0.5));
            cv.fillRoundedRect(cb.x, cb.y, cb.w, 17, 7, mix(cc, Color.rgb(255, 255, 255), 0.22));
            cv.strokeRoundedRect(cb.x, cb.y, cb.w, cb.h, 7, c_border);
            drawClipNotes(cv, clip, L{ .x = cb.x, .y = cb.y + 18, .w = cb.w, .h = cb.h - 21 }, mix(cc, Color.rgb(255, 255, 255), 0.45));
            cv.textAA(cb.x + 8, cb.y + 1, clip.name.items, Color.rgb(14, 16, 22), fb);
            if (selected) {
                cv.strokeRoundedRect(cb.x - 1, cb.y - 1, cb.w + 2, cb.h + 2, 8, c_accent);
                cv.strokeRoundedRect(cb.x, cb.y, cb.w, cb.h, 7, c_accent);
            }
        }
    }
}

fn channelStrip(ui: *uikit.Ui, b: L, name: []const u8, col: Color, gain: *f32, pan: ?*f32, id: u32, ti: usize, state: *State, is_master: bool) void {
    const cv = ui.cv;
    cv.dropShadow(b.x, b.y, b.w, b.h, 11, 4);
    card(cv, b, 11, c_card_top, c_card_bot);
    cv.fillRoundedRect(b.x + 11, b.y + 9, b.w - 22, 4, 2, col);
    if (is_master) cv.glow(b.x + b.w - 16, b.y + 11, 10, Color.rgba(96, 210, 235, 90));
    cv.textAA(b.x + 12, b.y + 18, name, c_text, fu);

    const bottom = b.y + b.h - 22; // reserve space for the value readout
    var cy = b.y + 40; // running cursor; adaptively stack controls above the fader
    if (!is_master) {
        // mute / solo
        if (miniToggle(ui, 300 + @as(u32, @intCast(ti)), b.x + 12, cy, 22, 16, "M", state.mutes[ti], c_amber)) state.mutes[ti] = !state.mutes[ti];
        if (miniToggle(ui, 320 + @as(u32, @intCast(ti)), b.x + 38, cy, 22, 16, "S", state.solos[ti], c_green)) state.solos[ti] = !state.solos[ti];
        if (pan != null) label(cv, b.x + b.w - 36, cy + 2, "PAN");
        cy += 22;
        // pan
        if (pan) |pp| {
            _ = ui.hSlider(id + 1000, b.x + 12, cy, b.w - 24, 6, pp, -1.0, 1.0);
            cy += 16;
        }
        // sends A / B — only when the strip is tall enough to keep a real fader
        if (bottom - (cy + 6) > 96) {
            const ay = cy + 12;
            const ax = b.x + @divTrunc(b.w, 2) - 26;
            const bx = b.x + @divTrunc(b.w, 2) + 26;
            _ = ui.knob(200 + @as(u32, @intCast(ti)) * 2, ax, ay, 12, &state.sends[ti][0]);
            _ = ui.knob(201 + @as(u32, @intCast(ti)) * 2, bx, ay, 12, &state.sends[ti][1]);
            cv.textAA(ax - 4, ay + 18, "A", c_faint, fb);
            cv.textAA(bx - 4, ay + 18, "B", c_faint, fb);
            cy += 40;
        }
    } else {
        cy = b.y + 46;
    }

    const fy = cy + 6;
    const fader_h = @max(bottom - fy, 24);
    const fx = b.x + @divTrunc(b.w, 2) - 18;
    _ = ui.vFader(id, fx, fy, 8, fader_h, gain);
    // meter
    const mx2 = b.x + @divTrunc(b.w, 2) + 12;
    cv.fillRoundedRect(mx2, fy, 10, fader_h, 4, Color.rgb(15, 17, 22));
    const lvl = if (!is_master and state.mutes[ti]) 0.0 else gain.* * 0.92;
    const mh: i32 = @intFromFloat(lvl * @as(f32, @floatFromInt(fader_h)));
    var k: i32 = 0;
    while (k < mh) : (k += 1) {
        const tt = @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(@max(fader_h, 1)));
        const mc = if (tt > 0.85) c_red else if (tt > 0.65) c_amber else c_green;
        cv.fillRect(mx2, fy + fader_h - 1 - k, 10, 1, mc);
    }
    var vbuf: [8]u8 = undefined;
    const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
    cv.textAA(b.x + 12, b.y + b.h - 20, vs, c_dim, fb);
}

fn mixer(ui: *uikit.Ui, p: *project.Project, region: L, state: *State) void {
    const cv = ui.cv;
    label(cv, region.x + 8, region.y + 4, "MIXER");
    var area = region;
    area.gapTop(24);
    const cardBox = L{ .x = region.x + 8, .y = area.y, .w = region.w - 16, .h = area.h - 10 };
    card(cv, cardBox, 12, c_panel_top, c_panel_bot);
    const inner = cardBox.shrink(12);

    const n: i32 = @intCast(p.tracks.items.len);
    const gap: i32 = 10;
    // strips fill the full width (tracks + master), no dead space
    const strip_w = @divTrunc(inner.w - n * gap, n + 1);
    for (p.tracks.items, 0..) |*t, ti| {
        const sx = inner.x + @as(i32, @intCast(ti)) * (strip_w + gap);
        channelStrip(ui, L{ .x = sx, .y = inner.y, .w = strip_w, .h = inner.h }, t.name.items, palette[ti % palette.len], &t.gain, &t.pan, @intCast(100 + ti), ti, state, false);
    }
    const msx = inner.x + n * (strip_w + gap);
    channelStrip(ui, L{ .x = msx, .y = inner.y, .w = inner.x + inner.w - msx, .h = inner.h }, "Master", c_accent, &state.master_gain, null, 800, 0, state, true);
}

pub fn frame(ui: *uikit.Ui, p: *project.Project, bar: u64, state: *State) void {
    const cv = ui.cv;
    const W: i32 = @intCast(cv.width);
    const H: i32 = @intCast(cv.height);
    state.window_action = .none;

    cv.vGradient(0, 0, W, H, c_bg_top, c_bg_bot);

    var root = L{ .x = 0, .y = 0, .w = W, .h = H };
    const tb = root.cutTop(58);
    const browser_w: i32 = std.math.clamp(@divTrunc(W * 20, 100), 170, 230);
    const leftBox = root.cutLeft(browser_w);
    const mix_h: i32 = std.math.clamp(@divTrunc(H * 44, 100), 220, 400);
    const mixBox = root.cutBottom(mix_h);
    const arrBox = root;

    titleBar(ui, tb, state);
    browser(ui, leftBox);
    arrangement(ui, p, bar, arrBox, state);
    mixer(ui, p, mixBox, state);

    // window frame: 1px border + top highlight (borderless polish)
    cv.fillRect(0, 0, W, 1, c_rim);
    cv.outline(0, 0, W, H, Color.rgb(8, 9, 13));
}
