//! daw.zig — the Zenith DAW view, assembled entirely from our own toolkit:
//! trellis.zig (layout), widgets.zig (faders/knobs/sliders), gpu2d.zig (SDF shapes,
//! analytic shadows, atlas text, frosted glass). One cohesive live app — the
//! consolidation of everything proven in the demos. DAW-specific code; the
//! reusable engine pieces live in their own modules (see TOOLKIT.md).

const std = @import("std");
const mlog = std.log.scoped(.mixer); // logs control changes to the app console
const gpu2d = @import("gpu2d.zig");
const trellis = @import("trellis.zig");
const widgets = @import("widgets.zig");
const project = @import("project.zig");
const color = @import("color.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;
const px = trellis.px;
const grow = trellis.grow;
const groww = trellis.groww;

pub const WinAction = enum { none, close, minimize, maximize, move };
pub const State = struct {
    playing: bool = true,
    window_action: WinAction = .none,
    sel_track: i32 = 2,
    sel_clip: i32 = 0,
    master_gain: f32 = 0.8,
    nav_sel: i32 = 0,
    playhead: f32 = 0.34, // 0..1 position across the arrangement
    sends: [8][2]f32 = .{
        .{ 0.28, 0.10 }, .{ 0.40, 0.16 }, .{ 0.22, 0.30 }, .{ 0.34, 0.12 },
        .{ 0.18, 0.08 }, .{ 0.30, 0.20 }, .{ 0.26, 0.14 }, .{ 0.36, 0.18 },
    },
    mutes: [8]bool = [_]bool{false} ** 8,
    solos: [8]bool = [_]bool{false} ** 8,
    meters: [8]f32 = [_]f32{0.3} ** 8, // smoothed meter levels (VU ballistics)
    track_levels: [8]f32 = [_]f32{0} ** 8, // real per-track output level from the engine
    // real-audio drive: when the audio engine is running, it owns the playhead
    // and feeds the live master peak so meters bounce with the actual signal.
    audio_active: bool = false,
    audio_level: f32 = 0,
    // piano-roll / clip editor
    editing: bool = false,
    edit_track: usize = 0,
    edit_clip: usize = 0,
};

// ---- refined palette (design-identity pass) --------------------------------
// color math now lives in the toolkit (color.zig) so any component can reuse it.
const oklch = color.oklch;
const Cols = struct { pal: [5]Color, accent: Color, accent2: Color };
const C: Cols = blk: {
    @setEvalBranchQuota(1_000_000);
    break :blk .{
        // MUTED, low-chroma category tints (the "carefully balanced desaturated"
        // palette modern DAWs use — sophisticated, not neon). OKLCH so the steps
        // are perceptually even. chroma ~0.07 vs the old ~0.15 = far less cheesy.
        .pal = .{ oklch(0.665, 0.072, 38), oklch(0.685, 0.068, 156), oklch(0.675, 0.066, 232), oklch(0.640, 0.078, 292), oklch(0.660, 0.074, 350) },
        .accent = oklch(0.68, 0.150, 266), // electric indigo
        .accent2 = oklch(0.67, 0.165, 300),
    };
};
const pal = C.pal;
const accent = C.accent;

// modern dark surfaces — clean cool-dark with clear elevation steps so cards
// pop off panels (helped by the crisp rim lighting), subtle hairlines.
const bg_top = Color.rgb(18, 19, 25);
const bg_bot = Color.rgb(11, 12, 16);
const panel_t = Color.rgb(24, 26, 33);
const panel_b = Color.rgb(19, 21, 27);
const card_t = Color.rgb(37, 40, 50);
const card_b = Color.rgb(31, 34, 43);
const lane = Color.rgb(14, 15, 20);
const lane2 = Color.rgb(16, 18, 23);
const titlebar_t = Color.rgb(25, 27, 34);
const titlebar_b = Color.rgb(18, 19, 25);
// no gray border lines — surfaces separate by rim light + shadow + fill contrast
const bord = Color.rgba(255, 255, 255, 0);
const rim = Color.rgba(255, 255, 255, 20);
const grid = Color.rgba(255, 255, 255, 7);
// text — bumped contrast for APCA legibility (muted labels were too dark)
const txt = Color.rgb(237, 240, 247);
const dim = Color.rgb(160, 167, 182); // secondary content (APCA Lc ~60)
const faint = Color.rgb(124, 132, 147); // most-muted, still legible (Lc ~45)
const label_col = Color.rgb(150, 158, 173); // small-caps section labels
const amber = Color.rgb(238, 176, 80);
const green = Color.rgb(120, 208, 140);
const red = Color.rgb(236, 100, 100);
// meter colors + ballistics moved to the toolkit: widgets.Ui.meter(...)

// color utilities are toolkit-level now (color.zig) — alias for local brevity.
const mix = color.lerp;
const shade = color.shade;

// ---- demo project ----------------------------------------------------------
fn fillClip(c: *project.Clip, pitches: []const u8) !void {
    if (pitches.len == 0) return;
    const step = c.length / pitches.len;
    for (pitches, 0..) |pitch, i| try c.notes.append(.{ .start = @as(u64, i) * step, .len = step * 3 / 4, .pitch = pitch, .velocity = 100 });
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

// ---- GPU icons -------------------------------------------------------------
const icons = struct {
    fn play(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.tri(cx - s * 0.32, cy - s * 0.52, cx - s * 0.32, cy + s * 0.52, cx + s * 0.5, cy, c);
    }
    fn pause(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.40, cy - s * 0.5, s * 0.28, s, 1.5, c);
        g.rect(cx + s * 0.12, cy - s * 0.5, s * 0.28, s, 1.5, c);
    }
    fn stop(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.44, cy - s * 0.44, s * 0.88, s * 0.88, 2.5, c);
    }
    fn record(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.44, cy - s * 0.44, s * 0.88, s * 0.88, s * 0.44, c);
    }
    fn minimize(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.5, cy - 1, s, 2, 1, c);
    }
    fn maximize(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.stroke(cx - s * 0.5, cy - s * 0.5, s, s, 2.5, 1.5, c);
    }
    fn close(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.line(cx - s * 0.5, cy - s * 0.5, cx + s * 0.5, cy + s * 0.5, 1.7, c);
        g.line(cx - s * 0.5, cy + s * 0.5, cx + s * 0.5, cy - s * 0.5, 1.7, c);
    }
};

fn frac(x: f32) f32 {
    return x - @floor(x);
}
/// Live meter level — bouncing transients while playing, static (gain) when stopped.
fn meterLevel(ti: usize, ts: f32, gain: f32, playing: bool) f32 {
    if (!playing) return gain * 0.72;
    const t = ts * 2.0 + @as(f32, @floatFromInt(ti)) * 0.37; // ~2 beats/sec @120bpm
    const env = @exp(-frac(t) * 3.5);
    const n = frac(@sin((t + @as(f32, @floatFromInt(ti))) * 53.13) * 43758.5453);
    return std.math.clamp(gain * (0.45 + 0.6 * env * (0.55 + 0.45 * n)), 0.0, 1.0);
}

// ---- a small, consistent thin-stroke icon set (browser categories) ---------
const ico = struct {
    fn sound(g: *Gpu, cx: f32, cy: f32, c: Color) void { // level bars
        g.rect(cx - 4.6, cy - 2, 1.7, 4, 0.85, c);
        g.rect(cx - 1.6, cy - 5, 1.7, 10, 0.85, c);
        g.rect(cx + 1.4, cy - 3.5, 1.7, 7, 0.85, c);
        g.rect(cx + 4.4, cy - 1, 1.7, 2, 0.85, c);
    }
    fn drum(g: *Gpu, cx: f32, cy: f32, c: Color) void { // 2x2 pads
        g.stroke(cx - 5, cy - 5, 4.4, 4.4, 1.3, 1.2, c);
        g.stroke(cx + 0.7, cy - 5, 4.4, 4.4, 1.3, 1.2, c);
        g.stroke(cx - 5, cy + 0.7, 4.4, 4.4, 1.3, 1.2, c);
        g.stroke(cx + 0.7, cy + 0.7, 4.4, 4.4, 1.3, 1.2, c);
    }
    fn instrument(g: *Gpu, cx: f32, cy: f32, c: Color) void { // piano keys
        var k: f32 = 0;
        while (k < 4) : (k += 1) g.rect(cx - 5 + k * 2.7, cy - 5, 1.9, 10, 0.7, c);
    }
    fn fx(g: *Gpu, cx: f32, cy: f32, c: Color) void { // knob
        g.stroke(cx - 5, cy - 5, 10, 10, 5, 1.3, c);
        g.line(cx, cy, cx + 2.6, cy - 3.2, 1.4, c);
    }
    fn midi(g: *Gpu, cx: f32, cy: f32, c: Color) void { // note
        g.rect(cx - 4.2, cy + 1.4, 4.6, 3.6, 1.8, c);
        g.rect(cx - 0.1, cy - 5, 1.6, 7.6, 0, c);
        g.rect(cx - 0.1, cy - 5, 4.2, 1.6, 0, c);
    }
    fn sample(g: *Gpu, cx: f32, cy: f32, c: Color) void { // waveform
        g.rect(cx - 5, cy - 1.5, 1.5, 3, 0, c);
        g.rect(cx - 2.6, cy - 5, 1.5, 10, 0, c);
        g.rect(cx - 0.2, cy - 3, 1.5, 6, 0, c);
        g.rect(cx + 2.2, cy - 4.5, 1.5, 9, 0, c);
        g.rect(cx + 4.6, cy - 2, 1.5, 4, 0, c);
    }
    fn cat(g: *Gpu, kind: u8, cx: f32, cy: f32, c: Color) void {
        switch (kind) {
            0 => sound(g, cx, cy, c),
            1 => drum(g, cx, cy, c),
            2 => instrument(g, cx, cy, c),
            3 => fx(g, cx, cy, c),
            4 => midi(g, cx, cy, c),
            else => sample(g, cx, cy, c),
        }
    }
};
fn noise(i: u32) f32 {
    var x = i *% 2654435761;
    x ^= x >> 15;
    x *%= 2246822519;
    x ^= x >> 13;
    return @as(f32, @floatFromInt(x & 0xffffff)) / 16777215.0 * 2.0 - 1.0;
}
/// Synthesize a REAL 2-bar drum loop (kick + snare + hats) into a sample buffer.
/// The waveform display below is peak-analyzed from these actual samples.
pub fn synthDrumLoop(a: std.mem.Allocator, n: usize) ![]f32 {
    const buf = try a.alloc(f32, n);
    const sr: f32 = 48000.0;
    var i: usize = 0;
    while (i < n) : (i += 1) {
        const t = @as(f32, @floatFromInt(i)) / sr;
        const kp = @mod(t, 0.5); // kick every beat
        const kick = @sin(kp * 2.0 * std.math.pi * (52.0 + 45.0 * @exp(-kp * 28.0))) * @exp(-kp * 8.5);
        const hp = @mod(t, 0.25); // hats on 1/8
        const hat = noise(@intCast(i)) * @exp(-hp * 75.0) * 0.32;
        const snp = @mod(t - 0.5 + 1.0, 1.0); // snare on beats 2 & 4
        const snare = (noise(@as(u32, @intCast(i)) +% 99) * 0.7 + @sin(t * 2.0 * std.math.pi * 185.0) * 0.3) * @exp(-snp * 15.0) * 0.55;
        buf[i] = std.math.clamp(kick * 0.92 + hat + snare, -1.0, 1.0);
    }
    return buf;
}
/// Per-track mixer STEMS — distinct audio per channel so the mixer faders/mute/
/// solo audibly do something. Drums (the loop), Bass (low), Lead (mid melody),
/// Keys (chord), Pad (sustained) — distinct frequency content makes solo/mute
/// measurable (FFT) and the meters per-track. Returns `ntracks` mono buffers.
pub fn synthStems(a: std.mem.Allocator, sr_u: u32, n: usize, ntracks: usize) ![][]f32 {
    const sr: f32 = @floatFromInt(sr_u);
    const stems = try a.alloc([]f32, ntracks);
    for (stems, 0..) |*st, ti| {
        st.* = try a.alloc(f32, n);
        const buf = st.*;
        for (buf, 0..) |*v, i| {
            const t = @as(f32, @floatFromInt(i)) / sr;
            const beat = @mod(t, 0.5); // 120 BPM
            v.* = switch (ti) {
                0 => blk: { // Drums: kick + hats + snare (the loop)
                    const kick = @sin(beat * 2.0 * std.math.pi * (52.0 + 45.0 * @exp(-beat * 28.0))) * @exp(-beat * 8.5) * 0.9;
                    const hp = @mod(t, 0.25);
                    const hat = noise(@intCast(i)) * @exp(-hp * 75.0) * 0.28;
                    const snp = @mod(t - 0.5 + 1.0, 1.0);
                    const snare = (noise(@as(u32, @intCast(i)) +% 99) * 0.7) * @exp(-snp * 15.0) * 0.5;
                    break :blk std.math.clamp(kick + hat + snare, -1.0, 1.0);
                },
                1 => @sin(t * 2.0 * std.math.pi * 55.0) * 0.5 * @exp(-beat * 4.0), // Bass: A1 plucks
                2 => blk: { // Lead: a 4-note arpeggio (A4..)
                    const notes = [_]f32{ 440.0, 554.37, 659.25, 880.0 };
                    const step = @as(usize, @intFromFloat(@mod(t * 4.0, 4.0)));
                    break :blk @sin(t * 2.0 * std.math.pi * notes[step]) * 0.28 * @exp(-@mod(t, 0.25) * 6.0);
                },
                3 => (@sin(t * 2.0 * std.math.pi * 261.63) + @sin(t * 2.0 * std.math.pi * 329.63) + @sin(t * 2.0 * std.math.pi * 392.0)) * 0.12, // Keys: C-major triad
                else => (@sin(t * 2.0 * std.math.pi * 220.0) + @sin(t * 2.0 * std.math.pi * 277.18)) * 0.14, // Pad: sustained A+C#
            };
        }
    }
    return stems;
}

// waveform rendering moved to the toolkit: widgets.waveform(...)

fn drawClips(g: *Gpu, fb: *const Font, u: *widgets.Ui, p: *project.Project, ti: usize, r: [4]f32, bar: u64, state: *State, wave: []const f32) void {
    const bars: f32 = 4;
    const total: f64 = @floatFromInt(@as(i64, 4) * @as(i64, @intCast(bar)));
    const scale: f64 = @as(f64, r[2]) / total;
    // gridlines
    var gl: i32 = 1;
    while (gl < 4) : (gl += 1) g.rect(r[0] + r[2] * @as(f32, @floatFromInt(gl)) / bars, r[1] + 4, 1, r[3] - 8, 0, grid);
    const t = &p.tracks.items[ti];
    const col = pal[ti % pal.len];
    const muted = state.mutes[ti];
    for (t.clips.items, 0..) |clip, ci| {
        const cx = r[0] + @as(f32, @floatCast(@as(f64, @floatFromInt(clip.start)) * scale)) + 3;
        const cw = @max(@as(f32, @floatCast(@as(f64, @floatFromInt(clip.length)) * scale)) - 5, 12);
        const cy = r[1] + 4;
        const ch = r[3] - 8;
        const hovered = u.in.mx >= cx and u.in.mx < cx + cw and u.in.my >= cy and u.in.my < cy + ch;
        if (hovered and u.pressed) {
            state.sel_track = @intCast(ti);
            state.sel_clip = @intCast(ci);
        }
        const sel = state.sel_track == @as(i32, @intCast(ti)) and state.sel_clip == @as(i32, @intCast(ci));
        const base = if (muted) mix(col, Color.rgb(56, 60, 72), 0.74) else col;
        const cc = if (hovered) shade(base, 0.05) else base;
        if (sel) g.glow(cx + cw / 2, cy + ch / 2, @max(cw, ch) / 2 + 6, Color.rgba(108, 147, 244, 110)); // accent selection glow (toolkit effect)
        g.elevate(cx, cy, cw, ch, 7, .e2); // reusable elevation preset (two-layer depth)
        // SUBTLE smooth gradient: one muted color with a gentle vertical sheen
        // (top +5% / bottom -15%), dither via material elev to avoid banding.
        g.card(cx, cy, cw, ch, 7, shade(cc, 0.05), shade(cc, -0.15), 0, bord, 0.45);
        g.rect(cx + 1, cy + 1, cw - 2, 1, 0, Color.rgba(255, 255, 255, 36)); // crisp lit top edge
        // soft dark scrim under the label so the name reads on any clip color
        g.rectGrad(cx, cy, cw, 22, 7, Color.rgba(0, 0, 0, 60), Color.rgba(0, 0, 0, 0), 0, bord);
        // audio tracks show a waveform; MIDI tracks show note blocks
        if (t.instrument == .sampler) {
            widgets.waveform(g, cx + 3, cy + 18, cw - 6, ch - 22, wave, mix(cc, Color.rgb(255, 255, 255), 0.42));
        } else if (clip.length != 0) {
            const clen: f32 = @floatFromInt(clip.length);
            const nc = mix(cc, Color.rgb(255, 255, 255), 0.42);
            for (clip.notes.items) |note| {
                const nx = cx + @as(f32, @floatFromInt(note.start)) / clen * cw;
                const nw = @max(@as(f32, @floatFromInt(note.len)) / clen * cw, 2);
                const pn = std.math.clamp((@as(f32, @floatFromInt(note.pitch)) - 32) / 60, 0.0, 1.0);
                const ny = cy + ch - 4 - pn * @max(ch - 22, 1);
                g.rect(nx + 1, ny, nw - 1, 3, 1.5, nc);
            }
        }
        fb.text(g, cx + 8, cy + 2, clip.name.items, Color.rgb(244, 247, 251)); // light label on the scrim
        if (sel) g.stroke(cx, cy, cw, ch, 7, 1.5, accent);
    }
}

// ---- the view --------------------------------------------------------------
pub const View = struct {
    g: *Gpu,
    c: trellis.Ctx,
    u: widgets.Ui,
    fc: *const Font, // caption 12 (labels)
    fb: *const Font, // body 14
    fu: *const Font, // title 16
    fd: *const Font, // display 28
    tc_buf: [16]u8 = undefined, // live timecode string
    wave: []const f32 = &.{}, // real audio samples for waveform display
    cm_open: bool = false, // context menu
    cm_x: f32 = 0,
    cm_y: f32 = 0,
    cm_anim: f32 = 0,
    pr: @import("pianoroll.zig").PianoRoll = .{}, // the clip editor (when state.editing)

    pub fn init(g: *Gpu, fc: *const Font, fb: *const Font, fu: *const Font, fd: *const Font) View {
        return .{ .g = g, .c = trellis.Ctx.init(g, fb, fu, fd), .u = widgets.Ui.init(g), .fc = fc, .fb = fb, .fu = fu, .fd = fd };
    }
    /// A small-caps section label (caption font, letter-spaced) — a core modern
    /// pattern. Caller already uppercases the string.
    fn secLabel(self: *View, s: []const u8, h: f32) void {
        self.c.label(s, self.fc, label_col, .{ .h = px(h), .tracking = 1.4 });
    }

    pub fn frame(self: *View, p: *project.Project, bar: u64, state: *State, W: f32, H: f32, mx: f32, my: f32, down: bool, rclick: bool) WinAction {
        const g = self.g;
        const c = &self.c;
        const u = &self.u;
        state.window_action = .none;
        g.begin(@intFromFloat(W), @intFromFloat(H), bg_bot);
        g.rectGrad(0, 0, W, H, 0, bg_top, bg_bot, 0, bord);
        u.begin(.{ .mx = mx, .my = my, .mouse_down = down }, 0.016);

        // frosted-glass value tooltip shown while hovering a fader/knob
        var tip_show = false;
        var tip_val: f32 = 0;
        var tip_x: f32 = 0;
        var tip_y: f32 = 0;

        const ntr = p.tracks.items.len;
        const ts = state.playhead * 8.0; // 4 bars @120bpm ≈ 8s
        const tc = std.fmt.bufPrint(&self.tc_buf, "{d:0>2} : {d:0>2} : {d:0>2}", .{
            @as(u32, @intFromFloat(ts / 60)),
            @as(u32, @intFromFloat(@mod(ts, 60))),
            @as(u32, @intFromFloat(@mod(ts * 100, 100))),
        }) catch "00 : 00 : 00";
        const TBH: f32 = 58; // title bar height (a glass strip drawn over the body)
        // ---- BODY pass (rendered first, below the glass title bar) ----
        c.beginAt(0, TBH, W, H - TBH, mx, my, down, 0.016);
        {
            c.open(.{ .dir = .row, .w = grow(), .h = grow() });
            {
                // BROWSER
                c.open(.{ .dir = .col, .w = px(224), .h = grow(), .pad = 12, .gap = 4, .aligni = .stretch, .bg = panel_t, .bg2 = panel_b, .border = bord });
                {
                    self.secLabel("BROWSER", 24);
                    const navs = [_]struct { n: []const u8, col: Color }{
                        .{ .n = "Sounds", .col = accent },     .{ .n = "Drums", .col = pal[0] },
                        .{ .n = "Instruments", .col = pal[3] }, .{ .n = "Audio FX", .col = pal[1] },
                        .{ .n = "MIDI FX", .col = pal[4] },     .{ .n = "Samples", .col = C.accent2 },
                    };
                    for (navs, 0..) |nv, i| {
                        const sel = state.nav_sel == @as(i32, @intCast(i));
                        // quiet, neutral active state (Linear-style): a subtly lifted
                        // surface; the accent is carried by text/icon contrast, not a
                        // saturated fill. Subtle hover.
                        c.open(.{ .dir = .row, .h = px(32), .pad = 8, .gap = 9, .radius = 6, .aligni = .center, .id = 1000 + @as(u64, i), .bg = if (sel) Color.rgb(37, 40, 50) else panel_t, .bg2 = if (sel) Color.rgb(31, 34, 43) else panel_b, .hover_bg = Color.rgb(32, 35, 44), .border = null });
                        {
                            c.box(.{ .w = px(15), .h = px(15), .id = 1100 + @as(u64, i) });
                            c.label(nv.n, self.fb, if (sel) txt else dim, .{});
                        }
                        c.close();
                    }
                    c.box(.{ .w = grow(), .h = px(10) });
                    c.box(.{ .w = grow(), .h = px(1), .bg = bord });
                    self.secLabel("DEVICES", 28);
                    const devs = [_]struct { n: []const u8, t: []const u8, col: Color }{
                        .{ .n = "Operator", .t = "INST", .col = pal[3] }, .{ .n = "Analog", .t = "INST", .col = pal[3] },
                        .{ .n = "Reverb", .t = "FX", .col = pal[1] },     .{ .n = "EQ Eight", .t = "FX", .col = pal[1] },
                        .{ .n = "Compressor", .t = "FX", .col = pal[1] },
                    };
                    for (devs) |d| {
                        c.open(.{ .dir = .row, .h = px(28), .gap = 9, .aligni = .center, .pad = 2 });
                        {
                            c.box(.{ .w = px(6), .h = px(6), .radius = 2, .bg = d.col });
                            c.label(d.n, self.fb, dim, .{});
                            c.box(.{ .w = grow() });
                            c.open(.{ .dir = .row, .pad = 5, .radius = 5, .bg = Color.rgb(28, 31, 40), .aligni = .center });
                            c.label(d.t, self.fb, faint, .{});
                            c.close();
                        }
                        c.close();
                    }
                }
                c.close();

                // MAIN (arrangement + mixer)
                c.open(.{ .dir = .col, .w = grow(), .h = grow() });
                {
                    // ARRANGEMENT
                    c.open(.{ .dir = .col, .w = grow(), .h = grow(), .pad = 8, .gap = 5 });
                    {
                        self.secLabel("ARRANGEMENT", 20);
                        c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 14, .pad = 12, .gap = 8, .bg = panel_t, .bg2 = panel_b, .border = bord, .elev = 0.5, .shadow = 18 });
                        {
                            // ruler row
                            c.open(.{ .dir = .row, .w = grow(), .h = px(24), .gap = 8 });
                            {
                                c.box(.{ .w = px(180) });
                                c.box(.{ .w = grow(), .h = grow(), .id = 950 });
                            }
                            c.close();
                            // track rows
                            var ti: usize = 0;
                            while (ti < ntr) : (ti += 1) {
                                const tcol = pal[ti % pal.len];
                                c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 8 });
                                {
                                    // header
                                    c.open(.{ .dir = .row, .w = px(184), .h = grow(), .radius = 10, .pad = 10, .gap = 10, .aligni = .center, .bg = card_t, .bg2 = card_b, .border = bord, .elev = 1 });
                                    {
                                        c.box(.{ .w = px(4), .h = grow(), .radius = 2, .bg = tcol });
                                        c.open(.{ .dir = .col, .w = grow(), .gap = 2 });
                                        {
                                            c.label(p.tracks.items[ti].name.items, self.fu, txt, .{});
                                            c.label(if (p.tracks.items[ti].instrument == .sampler) "Sampler" else "Synth", self.fb, dim, .{});
                                        }
                                        c.close();
                                        c.open(.{ .dir = .row, .gap = 4 });
                                        {
                                            c.box(.{ .w = px(18), .h = px(16), .id = 340 + @as(u64, ti) });
                                            c.box(.{ .w = px(18), .h = px(16), .id = 360 + @as(u64, ti) });
                                        }
                                        c.close();
                                    }
                                    c.close();
                                    // lane
                                    c.box(.{ .w = grow(), .h = grow(), .radius = 8, .bg = if (ti % 2 == 0) lane else lane2, .id = 800 + @as(u64, ti) });
                                }
                                c.close();
                            }
                        }
                        c.close();
                    }
                    c.close();

                    // MIXER
                    const mix_h = std.math.clamp(H * 42 / 100, 210, 380);
                    c.open(.{ .dir = .col, .w = grow(), .h = px(mix_h), .pad = 8, .gap = 5 });
                    {
                        self.secLabel("MIXER", 20);
                        c.open(.{ .dir = .row, .w = grow(), .h = grow(), .radius = 14, .pad = 12, .gap = 8, .bg = panel_t, .bg2 = panel_b, .border = bord, .elev = 0.5, .shadow = 18 });
                        {
                            var ti: usize = 0;
                            while (ti <= ntr) : (ti += 1) {
                                const is_master = ti == ntr;
                                const scol = if (is_master) accent else pal[ti % pal.len];
                                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 12, .pad = 10, .gap = 8, .bg = card_t, .bg2 = card_b, .border = bord, .elev = 1 });
                                {
                                    c.box(.{ .w = grow(), .h = px(4), .radius = 2, .bg = scol });
                                    c.label(if (is_master) "Master" else p.tracks.items[ti].name.items, self.fu, txt, .{ .h = px(21) });
                                    if (!is_master) {
                                        c.open(.{ .dir = .row, .w = grow(), .h = px(16), .gap = 4, .aligni = .center });
                                        {
                                            c.box(.{ .w = px(22), .h = px(16), .id = 300 + @as(u64, ti) });
                                            c.box(.{ .w = px(22), .h = px(16), .id = 320 + @as(u64, ti) });
                                            c.box(.{ .w = grow() });
                                            c.label("PAN", self.fc, faint, .{ .tracking = 0.8 });
                                        }
                                        c.close();
                                        c.box(.{ .w = grow(), .h = px(6), .id = 400 + @as(u64, ti) });
                                        c.open(.{ .dir = .row, .w = grow(), .h = px(42), .gap = 6, .justify = .center });
                                        {
                                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                                            c.box(.{ .w = px(26), .h = px(26), .id = 600 + @as(u64, ti) });
                                            c.label("A", self.fc, faint, .{});
                                            c.close();
                                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                                            c.box(.{ .w = px(26), .h = px(26), .id = 700 + @as(u64, ti) });
                                            c.label("B", self.fc, faint, .{});
                                            c.close();
                                        }
                                        c.close();
                                    }
                                    c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 10, .justify = .center });
                                    {
                                        c.box(.{ .w = px(10), .h = grow(), .id = 100 + @as(u64, ti) });
                                        c.box(.{ .w = px(10), .h = grow(), .id = 500 + @as(u64, ti) });
                                    }
                                    c.close();
                                    c.box(.{ .w = grow(), .h = px(16) });
                                }
                                c.close();
                            }
                        }
                        c.close();
                    }
                    c.close();
                }
                c.close();
            }
            c.close();
        }
        c.end(); // body chrome computed

        // ---- WIDGETS / custom content into the solved rects ----------------
        // (transport + window controls are drawn over the glass title bar, below)
        // nav selection (Trellis hover handled in chrome; click via Trellis)
        if (c.click >= 1000 and c.click < 1010) state.nav_sel = @intCast(c.click - 1000);
        // browser category icons (thin-stroke, in category color)
        const navico = [_]struct { k: u8, col: Color }{
            .{ .k = 0, .col = accent }, .{ .k = 1, .col = pal[0] }, .{ .k = 2, .col = pal[3] },
            .{ .k = 3, .col = pal[1] }, .{ .k = 4, .col = pal[4] }, .{ .k = 5, .col = C.accent2 },
        };
        for (navico, 0..) |ni, i| {
            if (c.rectOf(1100 + @as(u64, i))) |r| ico.cat(g, ni.k, r[0] + r[2] / 2, r[1] + r[3] / 2, ni.col);
        }

        // ruler
        if (c.rectOf(950)) |r| ruler(g, self.fb, r);

        // arrangement: per-track M/S + clips
        var ti: usize = 0;
        while (ti < ntr) : (ti += 1) {
            if (c.rectOf(340 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(340 + ti), r, "M", state.mutes[ti], amber)) {
                state.mutes[ti] = !state.mutes[ti];
            };
            if (c.rectOf(360 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(360 + ti), r, "S", state.solos[ti], green)) {
                state.solos[ti] = !state.solos[ti];
            };
            if (c.rectOf(800 + @as(u64, ti))) |r| drawClips(g, self.fb, u, p, ti, r, bar, state, self.wave);
        }

        // playhead — a bright line across the timeline + a marker in the ruler.
        // With the audio engine running it owns the playhead (set from the actual
        // sample position); otherwise advance it on the UI clock.
        if (state.playing and !state.audio_active) state.playhead = frac(state.playhead + 0.0016);
        if (c.rectOf(800)) |l0| {
            if (c.rectOf(950)) |rl| {
                const phx = l0[0] + state.playhead * l0[2];
                const top = rl[1] + 2;
                var bot = l0[1] + l0[3];
                if (c.rectOf(800 + @as(u64, ntr - 1))) |lN| bot = lN[1] + lN[3];
                g.shadow(phx - 1.5, top, 3, bot - top, 1, 4, Color.rgba(108, 147, 244, 90));
                g.rect(phx - 0.75, top, 1.5, bot - top, 0, Color.rgb(150, 180, 255));
                g.tri(phx - 4, rl[1] + 3, phx + 4, rl[1] + 3, phx, rl[1] + 11, Color.rgb(150, 180, 255));
            }
        }

        // mixer widgets
        ti = 0;
        while (ti <= ntr) : (ti += 1) {
            const is_master = ti == ntr;
            const gain = if (is_master) &state.master_gain else &p.tracks.items[ti].gain;
            if (!is_master) {
                if (c.rectOf(300 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(300 + ti), r, "M", state.mutes[ti], amber)) {
                    state.mutes[ti] = !state.mutes[ti];
                    mlog.info("mute t{d} ({s}) -> {}", .{ ti, p.tracks.items[ti].name.items, state.mutes[ti] });
                };
                if (c.rectOf(320 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(320 + ti), r, "S", state.solos[ti], green)) {
                    state.solos[ti] = !state.solos[ti];
                    mlog.info("solo t{d} ({s}) -> {}", .{ ti, p.tracks.items[ti].name.items, state.solos[ti] });
                };
                if (c.rectOf(400 + @as(u64, ti))) |r| if (u.hSliderBipolar(@intCast(400 + ti), r[0], r[1], r[2], r[3], &p.tracks.items[ti].pan, -1.0, 1.0)) {
                    mlog.debug("pan t{d} -> {d:.2}", .{ ti, p.tracks.items[ti].pan });
                };
                if (c.rectOf(600 + @as(u64, ti))) |r| {
                    _ = u.knob(@intCast(600 + ti), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][0]);
                    if (u.hot == 600 + @as(u32, @intCast(ti))) {
                        tip_show = true;
                        tip_val = state.sends[ti][0] * 100;
                        tip_x = r[0] + r[2] / 2;
                        tip_y = r[1] - 6;
                    }
                }
                if (c.rectOf(700 + @as(u64, ti))) |r| {
                    _ = u.knob(@intCast(700 + ti), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][1]);
                    if (u.hot == 700 + @as(u32, @intCast(ti))) {
                        tip_show = true;
                        tip_val = state.sends[ti][1] * 100;
                        tip_x = r[0] + r[2] / 2;
                        tip_y = r[1] - 6;
                    }
                }
            }
            if (c.rectOf(100 + @as(u64, ti))) |r| {
                if (u.vFader(@intCast(100 + ti), r[0], r[1], r[2], r[3], gain)) {
                    mlog.debug("fader {s} -> {d:.0}%", .{ if (is_master) "Master" else p.tracks.items[ti].name.items, gain.* * 100 });
                }
                if (u.hot == 100 + @as(u32, @intCast(ti))) {
                    tip_show = true;
                    tip_val = gain.* * 100;
                    tip_x = r[0] + r[2] / 2;
                    tip_y = r[1] - 4;
                }
            }
            if (c.rectOf(500 + @as(u64, ti))) |r| {
                const muted = !is_master and state.mutes[ti];
                const target = if (muted) 0.0 else if (state.audio_active)
                    // REAL level from the engine: master peak for the master strip,
                    // the per-track post-fader output level for each channel.
                    (if (is_master) state.audio_level else state.track_levels[ti])
                else
                    meterLevel(ti, ts, gain.*, state.playing);
                _ = u.meter(@intCast(850 + ti), r[0], r[1], r[2], r[3], target); // toolkit VU meter (ballistics)
            }
            if (c.rectOf(100 + @as(u64, ti))) |r| {
                var vbuf: [8]u8 = undefined;
                const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
                self.fc.textNum(g, r[0] - 6, r[1] + r[3] + 6, vs, dim);
            }
        }

        // title-bar drag region (avoid the interactive clusters)
        if (state.window_action == .none and u.pressed and my < TBH and (mx < 150 or (mx > 300 and mx < W - 360))) state.window_action = .move;

        // ---- TRANSPORT BAR: a CLEAN flat bar (modern chrome). Liquid Glass is
        // reserved for floating overlays (menus/tooltips), not forced onto the main
        // bar — a glass strip with a bright border reads as dated '90s bevel.
        g.flush(); // render the body, then the chrome on top
        g.rectGrad(0, 0, W, TBH, 0, titlebar_t, titlebar_b, 0, bord);
        g.shadow(0, TBH - 6, W, 8, 0, 6, Color.rgba(0, 0, 0, 60)); // soft separation from the body
        g.rect(0, TBH - 1, W, 1, 0, Color.rgba(255, 255, 255, 12)); // crisp bottom hairline
        self.fd.text(g, 22, 15, "Zenith", accent);
        const tyy: f32 = 15;
        if (state.playing) glow(g, 170, tyy + 14, 22, Color.rgba(108, 147, 244, 150));
        const picol = if (state.playing) Color.rgb(14, 18, 22) else txt;
        if (u.iconSlot(1, 151, tyy, 38, 28, state.playing)) state.playing = !state.playing;
        if (state.playing) icons.pause(g, 170, tyy + 14, 13, picol) else icons.play(g, 170, tyy + 14, 14, picol);
        _ = u.iconSlot(2, 192, tyy, 38, 28, false);
        icons.stop(g, 211, tyy + 14, 12, dim);
        _ = u.iconSlot(3, 233, tyy, 38, 28, false);
        icons.record(g, 252, tyy + 14, 12, red);
        g.rect(300, 13, 1, 32, 0, Color.rgba(255, 255, 255, 24));
        self.fd.textNum(g, 316, 8, "120", txt);
        self.fc.text(g, 316, 39, "BPM  4 / 4", faint);
        const tcw = self.fd.textWidth(tc);
        self.fd.textNum(g, W / 2 - tcw / 2, 15, tc, txt);
        const dotc = if (state.playing) green else faint;
        g.rect(W - 338, 22, 7, 7, 3, dotc);
        self.fu.text(g, W - 324, 14, if (state.playing) "Playing" else "Stopped", if (state.playing) accent else dim);
        self.fc.text(g, W - 324, 36, "100% Zig", faint);
        if (u.iconSlot(900, W - 108, 16, 30, 24, false)) state.window_action = .minimize;
        icons.minimize(g, W - 93, 28, 11, dim);
        if (u.iconSlot(901, W - 74, 16, 30, 24, false)) state.window_action = .maximize;
        icons.maximize(g, W - 59, 28, 10, dim);
        if (u.iconSlot(902, W - 40, 16, 30, 24, false)) state.window_action = .close;
        icons.close(g, W - 25, 28, 10, if (u.hoverOf(902) > 0.1) Color.rgb(248, 120, 120) else dim);

        u.end();
        g.flush();

        // frosted-glass value tooltip over the blurred backdrop
        if (tip_show) {
            const tw: f32 = 46;
            const th: f32 = 22;
            const tx = std.math.clamp(tip_x - tw / 2, 2, W - tw - 2);
            const ty = @max(tip_y - th - 4, 2);
            g.captureBlur(@intFromFloat(W), @intFromFloat(H));
            g.glass(tx, ty, tw, th, 9, Color.rgba(86, 94, 116, 70), Color.rgba(255, 255, 255, 150));
            var buf: [8]u8 = undefined;
            const s = std.fmt.bufPrint(&buf, "{d:.0}", .{tip_val}) catch "";
            const sw = self.fb.textWidth(s);
            self.fb.textNum(g, tx + (tw - sw) / 2, ty + (th - 15) / 2, s, txt);
            g.flush();
        }

        // right-click context menu — frosted Liquid Glass over the content
        if (rclick) {
            self.cm_open = true;
            self.cm_x = mx;
            self.cm_y = my;
        }
        const cmt: f32 = if (self.cm_open) 1.0 else 0.0;
        self.cm_anim += (cmt - self.cm_anim) * 0.30;
        if (self.cm_anim > 0.01) {
            const items = [_][]const u8{ "Rename", "Duplicate", "Split", "Delete" };
            const iw: f32 = 172;
            const ih: f32 = 32;
            const fullh: f32 = ih * items.len + 12;
            const eased = self.cm_anim * self.cm_anim * (3.0 - 2.0 * self.cm_anim);
            const hh = fullh * eased;
            const cmx = std.math.clamp(self.cm_x, 4, W - iw - 4);
            const cmy = std.math.clamp(self.cm_y, 4, H - hh - 4);
            g.shadow(cmx, cmy, iw, hh, 12, 22, Color.rgba(0, 0, 0, @intFromFloat(170 * eased)));
            g.flush();
            g.captureBlur(@intFromFloat(W), @intFromFloat(H));
            g.glass(cmx, cmy, iw, hh, 12, Color.rgba(84, 92, 114, 76), Color.rgba(255, 255, 255, 150));
            var hit: i32 = -1;
            for (items, 0..) |it, i| {
                const iy = cmy + 6 + @as(f32, @floatFromInt(i)) * ih;
                if (iy + ih > cmy + hh - 2) continue;
                const hov = mx >= cmx + 6 and mx < cmx + iw - 6 and my >= iy and my < iy + ih;
                if (hov) {
                    hit = @intCast(i);
                    g.rect(cmx + 6, iy, iw - 12, ih, 7, Color.rgba(108, 147, 244, 60));
                }
                const ic = if (i == items.len - 1) red else txt;
                self.fb.text(g, cmx + 16, iy + (ih - 15) / 2, it, if (hov) ic else dim);
            }
            g.flush();
            if (u.pressed) {
                const inside = mx >= cmx and mx < cmx + iw and my >= cmy and my < cmy + hh;
                if (!inside or hit >= 0) self.cm_open = false;
            }
        }

        // ---- piano-roll / clip editor overlay (toggled with 'E' on a selected clip) ----
        if (state.editing and state.edit_track < p.tracks.items.len) {
            const tr = &p.tracks.items[state.edit_track];
            if (state.edit_clip < tr.clips.items.len) {
                g.rect(0, TBH, W, H - TBH, 0, Color.rgba(0, 0, 0, 200)); // dim backdrop
                const er = [4]f32{ 18, TBH + 30, W - 36, H - TBH - 48 };
                self.pr.bar_frames = bar;
                self.pr.update(g, self.fb, er, &tr.clips.items[state.edit_clip], mx, my, u.pressed, down, u.released);
                var hbuf: [64]u8 = undefined;
                const hs = std.fmt.bufPrint(&hbuf, "Piano Roll — {s}  (Esc / E to close)", .{tr.name.items}) catch "Piano Roll";
                self.fd.text(g, 28, TBH + 4, hs, accent);
                g.flush(); // render the overlay before main_daw's grain snapshots the frame
            }
        }
        return state.window_action;
    }
};

fn glow(g: *Gpu, cx: f32, cy: f32, r: f32, c: Color) void {
    g.glow(cx, cy, r, c); // toolkit effect (gpu2d.glow)
}
fn miniToggle(u: *widgets.Ui, g: *Gpu, fb: *const Font, id: u32, r: [4]f32, lbl: []const u8, on: bool, oncol: Color) bool {
    const clicked = u.iconSlot(id, r[0], r[1], r[2], r[3], false);
    if (on) g.rect(r[0], r[1], r[2], r[3], 5, oncol);
    const tw = fb.textWidth(lbl);
    fb.text(g, r[0] + (r[2] - tw) / 2, r[1] + (r[3] - 12) / 2, lbl, if (on) Color.rgb(18, 20, 26) else dim);
    return clicked;
}
fn ruler(g: *Gpu, fb: *const Font, r: [4]f32) void {
    g.rectGrad(r[0], r[1], r[2], r[3], 6, Color.rgb(29, 32, 41), Color.rgb(24, 26, 34), 1, bord);
    const bw = r[2] / 4;
    var i: i32 = 0;
    while (i < 4) : (i += 1) {
        const bx = r[0] + @as(f32, @floatFromInt(i)) * bw;
        if (i > 0) g.rect(bx, r[1] + 4, 1, r[3] - 8, 0, bord);
        var buf: [8]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{i + 1}) catch "";
        fb.text(g, bx + 8, r[1] + (r[3] - 12) / 2, s, dim);
        var be: i32 = 1;
        while (be < 4) : (be += 1) g.rect(bx + bw * @as(f32, @floatFromInt(be)) / 4, r[1] + r[3] - 7, 1, 4, 0, grid);
    }
}
