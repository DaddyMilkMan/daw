//! ui.zig — the interactive Zenith DAW view. Docked layout (title bar /
//! arrangement panel / mixer dock) that fills the window at any size, polished
//! antialiased rendering, vector icons, MIDI note previews, hover/selection.

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
};

const palette = [_]Color{
    .{ .r = 245, .g = 158, .b = 88 },
    .{ .r = 122, .g = 211, .b = 140 },
    .{ .r = 96, .g = 206, .b = 240 },
    .{ .r = 178, .g = 140, .b = 248 },
};

const c_bg = Color.rgb(20, 22, 28);
const c_bg2 = Color.rgb(14, 15, 21);
const c_panel = Color.rgb(31, 34, 43);
const c_card = Color.rgb(38, 42, 52);
const c_lane = Color.rgb(25, 27, 35);
const c_accent = Color.rgb(96, 210, 235);
const c_text = Color.rgb(232, 236, 244);
const c_dim = Color.rgb(140, 148, 164);

fn mix(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
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

fn titleBar(ui: *uikit.Ui, tb: L, state: *State) void {
    const cv = ui.cv;
    const W = tb.w;
    cv.fillRect(tb.x, tb.y, tb.w, tb.h, Color.rgb(27, 30, 38));
    cv.fillRect(tb.x, tb.y + tb.h - 1, tb.w, 1, Color.rgb(11, 12, 16));
    cv.textAA(22, tb.y + 12, "Zenith", c_accent, fd);

    const ty = tb.y + 15;
    const picol = if (state.playing) Color.rgb(16, 20, 24) else c_text;
    if (ui.iconSlot(1, 158, ty, 38, 28, state.playing)) state.playing = !state.playing;
    if (state.playing) icons.pause(cv, 177, ty + 14, 13, picol) else icons.play(cv, 177, ty + 14, 14, picol);
    _ = ui.iconSlot(2, 204, ty, 38, 28, false);
    icons.stop(cv, 223, ty + 14, 12, c_dim);
    _ = ui.iconSlot(3, 250, ty, 38, 28, false);
    icons.record(cv, 269, ty + 14, 12, Color.rgb(238, 92, 92));

    cv.textAA(312, tb.y + 8, "120", c_text, fd);
    cv.textAA(312, tb.y + 38, "BPM   4 / 4", c_dim, fb);
    cv.textAA(@divTrunc(W, 2) - 64, tb.y + 12, "00 : 00 : 04", c_text, fd);
    cv.textAA(W - 320, tb.y + 12, if (state.playing) "Playing" else "Stopped", if (state.playing) c_accent else c_dim, fu);
    cv.textAA(W - 320, tb.y + 34, "100% Zig  -  CLAP ready", c_dim, fb);

    // window controls
    if (ui.iconSlot(900, W - 108, tb.y + 16, 30, 24, false)) state.window_action = .minimize;
    icons.minimize(cv, W - 93, tb.y + 28, 11, c_dim);
    if (ui.iconSlot(901, W - 74, tb.y + 16, 30, 24, false)) state.window_action = .maximize;
    icons.maximize(cv, W - 59, tb.y + 27, 10, c_dim);
    if (ui.iconSlot(902, W - 40, tb.y + 16, 30, 24, false)) state.window_action = .close;
    icons.close(cv, W - 25, tb.y + 28, 10, if (ui.hoverOf(902) > 0.1) Color.rgb(248, 120, 120) else c_dim);

    if (state.window_action == .none and ui.pressed and ui.in.my < tb.y + tb.h and
        (ui.in.mx < 156 or (ui.in.mx > 290 and ui.in.mx < W - 330)))
        state.window_action = .move;
}

fn arrangement(ui: *uikit.Ui, p: *project.Project, bar: u64, region: L, state: *State) void {
    const cv = ui.cv;
    cv.textAA(24, region.y + 6, "Arrangement", c_dim, fu);
    var area = region;
    area.gapTop(32);
    const card = L{ .x = 16, .y = area.y, .w = region.w - 32, .h = area.h - 8 };
    cv.fillRoundedRect(card.x, card.y, card.w, card.h, 12, c_panel);

    const inner = card.shrink(12);
    const ntracks: i32 = @intCast(p.tracks.items.len);
    const gap: i32 = 8;
    const row_h: i32 = std.math.clamp(@divTrunc(inner.h - (ntracks - 1) * gap, ntracks), 48, 92);
    const hdr_w: i32 = 168;
    const tl_x = inner.x + hdr_w + 10;
    const tl_w = inner.w - hdr_w - 10;
    const total: f64 = @floatFromInt(4 * bar);
    const scale: f64 = @as(f64, @floatFromInt(tl_w)) / total;

    for (p.tracks.items, 0..) |t, ti| {
        const ry = inner.y + @as(i32, @intCast(ti)) * (row_h + gap);
        const col = palette[ti % palette.len];
        cv.fillRoundedRect(inner.x, ry, hdr_w, row_h, 9, c_card);
        cv.fillRoundedRect(inner.x + 8, ry + 10, 4, row_h - 20, 2, col);
        cv.textAA(inner.x + 20, ry + 9, t.name.items, c_text, fu);
        cv.textAA(inner.x + 20, ry + 9 + 22, if (t.instrument == .sampler) "Sampler" else "Synth", c_dim, fb);
        cv.fillRoundedRect(tl_x, ry, tl_w, row_h, 8, c_lane);
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
            cv.dropShadow(cb.x, cb.y, cb.w, cb.h, 7, 3);
            cv.fillRoundedRect(cb.x, cb.y, cb.w, cb.h, 7, mix(col, c_panel, if (hovered) 0.16 else 0.34));
            cv.fillRoundedRect(cb.x, cb.y, cb.w, 18, 7, mix(col, Color.rgb(255, 255, 255), 0.16));
            drawClipNotes(cv, clip, L{ .x = cb.x, .y = cb.y + 18, .w = cb.w, .h = cb.h - 20 }, mix(col, Color.rgb(255, 255, 255), 0.4));
            cv.textAA(cb.x + 8, cb.y + 1, clip.name.items, Color.rgb(16, 18, 24), fb);
            if (selected) {
                cv.fillRoundedRect(cb.x, cb.y, cb.w, 2, 1, c_accent);
                cv.fillRoundedRect(cb.x, cb.y + cb.h - 2, cb.w, 2, 1, c_accent);
                cv.fillRoundedRect(cb.x, cb.y, 2, cb.h, 1, c_accent);
                cv.fillRoundedRect(cb.x + cb.w - 2, cb.y, 2, cb.h, 1, c_accent);
            }
        }
    }
}

fn channelStrip(ui: *uikit.Ui, b: L, name: []const u8, col: Color, gain: *f32, pan: ?*f32, id: u32) void {
    const cv = ui.cv;
    cv.dropShadow(b.x, b.y, b.w, b.h, 12, 4);
    cv.fillRoundedRect(b.x, b.y, b.w, b.h, 12, c_card);
    cv.fillRoundedRect(b.x + 12, b.y + 12, b.w - 24, 4, 2, col);
    cv.textAA(b.x + 12, b.y + 22, name, c_text, fu);
    var fy = b.y + 50;
    if (pan) |pp| {
        cv.textAA(b.x + 12, b.y + 50, "Pan", c_dim, fb);
        _ = ui.hSlider(id + 1000, b.x + 12, b.y + 70, b.w - 24, 6, pp, -1.0, 1.0);
        fy = b.y + 86;
    }
    const fader_h = b.y + b.h - 16 - fy;
    const fx = b.x + @divTrunc(b.w, 2) - 18;
    _ = ui.vFader(id, fx, fy, 8, fader_h, gain);
    // meter
    const mx2 = b.x + @divTrunc(b.w, 2) + 14;
    cv.fillRoundedRect(mx2, fy, 10, fader_h, 4, Color.rgb(17, 19, 24));
    const mh: i32 = @intFromFloat(gain.* * 0.92 * @as(f32, @floatFromInt(fader_h)));
    var k: i32 = 0;
    while (k < mh) : (k += 1) {
        const tt = @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(@max(fader_h, 1)));
        cv.fillRect(mx2, fy + fader_h - 1 - k, 10, 1, mix(Color.rgb(90, 220, 130), Color.rgb(245, 196, 70), tt));
    }
    var vbuf: [8]u8 = undefined;
    const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
    cv.textAA(b.x + 12, b.y + b.h - 16, vs, c_dim, fb);
}

fn mixer(ui: *uikit.Ui, p: *project.Project, region: L, state: *State) void {
    const cv = ui.cv;
    cv.textAA(24, region.y + 4, "Mixer", c_dim, fu);
    var area = region;
    area.gapTop(28);
    const card = L{ .x = 16, .y = area.y, .w = region.w - 32, .h = area.h - 12 };
    cv.fillRoundedRect(card.x, card.y, card.w, card.h, 12, c_panel);
    const inner = card.shrink(12);

    const strip_w: i32 = 116;
    const gap: i32 = 12;
    for (p.tracks.items, 0..) |*t, ti| {
        const sx = inner.x + @as(i32, @intCast(ti)) * (strip_w + gap);
        channelStrip(ui, L{ .x = sx, .y = inner.y, .w = strip_w, .h = inner.h }, t.name.items, palette[ti % palette.len], &t.gain, &t.pan, @intCast(100 + ti));
    }
    // master strip on the right
    const msx = inner.x + inner.w - strip_w;
    channelStrip(ui, L{ .x = msx, .y = inner.y, .w = strip_w, .h = inner.h }, "Master", c_accent, &state.master_gain, null, 800);
}

pub fn frame(ui: *uikit.Ui, p: *project.Project, bar: u64, state: *State) void {
    const cv = ui.cv;
    const W: i32 = @intCast(cv.width);
    const H: i32 = @intCast(cv.height);
    state.window_action = .none;

    cv.vGradient(0, 0, W, H, c_bg, c_bg2);

    var root = L{ .x = 0, .y = 0, .w = W, .h = H };
    const tb = root.cutTop(58);
    const mix_h: i32 = std.math.clamp(@divTrunc(H * 42, 100), 200, 340);
    const mixBox = root.cutBottom(mix_h);
    const arrBox = root;

    titleBar(ui, tb, state);
    arrangement(ui, p, bar, arrBox, state);
    mixer(ui, p, mixBox, state);
}
