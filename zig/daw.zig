//! daw.zig — the Zenith DAW view, assembled entirely from our own toolkit:
//! flex.zig (layout), widgets.zig (faders/knobs/sliders), gpu2d.zig (SDF shapes,
//! analytic shadows, atlas text, frosted glass). One cohesive live app — the
//! consolidation of everything proven in the demos. DAW-specific code; the
//! reusable engine pieces live in their own modules (see TOOLKIT.md).

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
const flex = @import("flex.zig");
const widgets = @import("widgets.zig");
const project = @import("project.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;
const px = flex.px;
const grow = flex.grow;
const groww = flex.groww;

pub const WinAction = enum { none, close, minimize, maximize, move };
pub const State = struct {
    playing: bool = false,
    window_action: WinAction = .none,
    sel_track: i32 = -1,
    sel_clip: i32 = -1,
    master_gain: f32 = 0.8,
    nav_sel: i32 = 0,
    sends: [8][2]f32 = .{
        .{ 0.28, 0.10 }, .{ 0.40, 0.16 }, .{ 0.22, 0.30 }, .{ 0.34, 0.12 },
        .{ 0.18, 0.08 }, .{ 0.30, 0.20 }, .{ 0.26, 0.14 }, .{ 0.36, 0.18 },
    },
    mutes: [8]bool = [_]bool{false} ** 8,
    solos: [8]bool = [_]bool{false} ** 8,
};

// ---- refined palette (design-identity pass) --------------------------------
fn oklch(light: f32, chroma: f32, Hdeg: f32) Color {
    const h = Hdeg * std.math.pi / 180.0;
    const a = chroma * @cos(h);
    const b = chroma * @sin(h);
    const l_ = light + 0.3963377774 * a + 0.2158037573 * b;
    const m_ = light - 0.1055613458 * a - 0.0638541728 * b;
    const s_ = light - 0.0894841775 * a - 1.2914855480 * b;
    const l = l_ * l_ * l_;
    const m = m_ * m_ * m_;
    const s = s_ * s_ * s_;
    return .{
        .r = enc(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        .g = enc(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        .b = enc(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s),
    };
}
fn enc(c: f32) u8 {
    const x = std.math.clamp(c, 0.0, 1.0);
    const v = if (x <= 0.0031308) x * 12.92 else 1.055 * std.math.pow(f32, x, 1.0 / 2.4) - 0.055;
    return @intFromFloat(@round(std.math.clamp(v, 0.0, 1.0) * 255.0));
}
const Cols = struct { pal: [5]Color, accent: Color, accent2: Color };
const C: Cols = blk: {
    @setEvalBranchQuota(1_000_000);
    break :blk .{
        // slightly desaturated for a more confident, less candy look
        .pal = .{ oklch(0.76, 0.115, 56), oklch(0.79, 0.120, 152), oklch(0.78, 0.095, 233), oklch(0.72, 0.125, 295), oklch(0.74, 0.130, 356) },
        .accent = oklch(0.80, 0.110, 228),
        .accent2 = oklch(0.72, 0.130, 295),
    };
};
const pal = C.pal;
const accent = C.accent;

// neutral surfaces — flatter gradients, crisp hairlines, strict rhythm
const bg_top = Color.rgb(23, 25, 32);
const bg_bot = Color.rgb(14, 15, 20);
const panel_t = Color.rgb(31, 34, 43);
const panel_b = Color.rgb(26, 28, 36);
const card_t = Color.rgb(42, 46, 57);
const card_b = Color.rgb(35, 38, 48);
const lane = Color.rgb(20, 22, 28);
const lane2 = Color.rgb(23, 25, 32);
const titlebar_t = Color.rgb(32, 35, 45);
const titlebar_b = Color.rgb(24, 26, 34);
const bord = Color.rgba(255, 255, 255, 14);
const rim = Color.rgba(255, 255, 255, 26);
const grid = Color.rgba(255, 255, 255, 9);
const txt = Color.rgb(232, 236, 243);
const dim = Color.rgb(138, 146, 162);
const faint = Color.rgb(92, 99, 115);
const amber = Color.rgb(238, 176, 80);
const green = Color.rgb(120, 208, 140);
const red = Color.rgb(236, 100, 100);

fn mix(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

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

fn drawClips(g: *Gpu, fb: *const Font, u: *widgets.Ui, p: *project.Project, ti: usize, r: [4]f32, bar: u64, state: *State) void {
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
        const cc = if (muted) mix(col, Color.rgb(72, 76, 88), 0.7) else col;
        g.shadow(cx, cy, cw, ch, 6, 4, Color.rgba(0, 0, 0, 110));
        g.card(cx, cy, cw, ch, 6, mix(cc, Color.rgb(255, 255, 255), if (hovered) 0.24 else 0.12), mix(cc, panel_b, 0.5), 1, bord, 1.0);
        g.rect(cx, cy, cw, 16, 6, mix(cc, Color.rgb(255, 255, 255), 0.2));
        // notes
        if (clip.length != 0) {
            const clen: f32 = @floatFromInt(clip.length);
            const nc = mix(cc, Color.rgb(255, 255, 255), 0.5);
            for (clip.notes.items) |note| {
                const nx = cx + @as(f32, @floatFromInt(note.start)) / clen * cw;
                const nw = @max(@as(f32, @floatFromInt(note.len)) / clen * cw, 2);
                const pn = std.math.clamp((@as(f32, @floatFromInt(note.pitch)) - 32) / 60, 0.0, 1.0);
                const ny = cy + ch - 4 - pn * @max(ch - 22, 1);
                g.rect(nx + 1, ny, nw - 1, 3, 1, nc);
            }
        }
        fb.text(g, cx + 7, cy + 1, clip.name.items, Color.rgb(14, 16, 22));
        if (sel) g.stroke(cx, cy, cw, ch, 6, 1.5, accent);
    }
}

// ---- the view --------------------------------------------------------------
pub const View = struct {
    g: *Gpu,
    c: flex.Ctx,
    u: widgets.Ui,
    fb: *const Font,
    fu: *const Font,
    fd: *const Font,

    pub fn init(g: *Gpu, fb: *const Font, fu: *const Font, fd: *const Font) View {
        return .{ .g = g, .c = flex.Ctx.init(g, fb, fu, fd), .u = widgets.Ui.init(g), .fb = fb, .fu = fu, .fd = fd };
    }

    pub fn frame(self: *View, p: *project.Project, bar: u64, state: *State, W: f32, H: f32, mx: f32, my: f32, down: bool) WinAction {
        const g = self.g;
        const c = &self.c;
        const u = &self.u;
        state.window_action = .none;
        g.begin(@intFromFloat(W), @intFromFloat(H), bg_bot);
        g.rectGrad(0, 0, W, H, 0, bg_top, bg_bot, 0, bord);
        u.begin(.{ .mx = mx, .my = my, .mouse_down = down }, 0.016);

        const ntr = p.tracks.items.len;
        c.begin(W, H, mx, my, down, 0.016);
        c.open(.{ .dir = .col, .w = px(W), .h = px(H) });
        {
            // ---- TITLE BAR ----
            c.open(.{ .dir = .row, .w = grow(), .h = px(56), .pad = 12, .gap = 14, .aligni = .center, .bg = titlebar_t, .bg2 = titlebar_b, .border = bord });
            {
                c.label("Zenith", self.fd, accent, .{});
                c.open(.{ .dir = .row, .h = px(34), .gap = 2, .pad = 3, .radius = 9, .bg = Color.rgb(28, 31, 40), .bg2 = Color.rgb(22, 24, 32), .border = bord, .aligni = .center });
                {
                    c.box(.{ .w = px(36), .h = px(28), .id = 1 });
                    c.box(.{ .w = px(36), .h = px(28), .id = 2 });
                    c.box(.{ .w = px(36), .h = px(28), .id = 3 });
                }
                c.close();
                c.open(.{ .dir = .col, .gap = 1 });
                {
                    c.label("120", self.fd, txt, .{ .h = px(28) });
                    c.label("BPM  4 / 4", self.fb, dim, .{});
                }
                c.close();
                c.box(.{ .w = grow() });
                c.label("00 : 00 : 04", self.fd, txt, .{});
                c.box(.{ .w = grow() });
                c.open(.{ .dir = .col, .gap = 1, .aligni = .end });
                {
                    c.label(if (state.playing) "Playing" else "Stopped", self.fu, if (state.playing) accent else dim, .{});
                    c.label("100% Zig", self.fb, faint, .{});
                }
                c.close();
                c.box(.{ .w = px(8) });
                c.box(.{ .w = px(30), .h = px(24), .id = 900 });
                c.box(.{ .w = px(30), .h = px(24), .id = 901 });
                c.box(.{ .w = px(30), .h = px(24), .id = 902 });
            }
            c.close();

            // ---- BODY ----
            c.open(.{ .dir = .row, .w = grow(), .h = grow() });
            {
                // BROWSER
                c.open(.{ .dir = .col, .w = px(224), .h = grow(), .pad = 12, .gap = 4, .aligni = .stretch, .bg = panel_t, .bg2 = panel_b, .border = bord });
                {
                    c.label("BROWSER", self.fb, faint, .{ .h = px(22) });
                    const navs = [_]struct { n: []const u8, col: Color }{
                        .{ .n = "Sounds", .col = accent },     .{ .n = "Drums", .col = pal[0] },
                        .{ .n = "Instruments", .col = pal[3] }, .{ .n = "Audio FX", .col = pal[1] },
                        .{ .n = "MIDI FX", .col = pal[4] },     .{ .n = "Samples", .col = C.accent2 },
                    };
                    for (navs, 0..) |nv, i| {
                        const sel = state.nav_sel == @as(i32, @intCast(i));
                        c.open(.{ .dir = .row, .h = px(32), .pad = 8, .gap = 9, .radius = 7, .aligni = .center, .id = 1000 + @as(u64, i), .bg = if (sel) Color.rgb(40, 52, 60) else panel_t, .bg2 = if (sel) Color.rgb(33, 43, 51) else panel_b, .hover_bg = Color.rgb(44, 49, 60), .border = if (sel) bord else null });
                        {
                            c.box(.{ .w = px(8), .h = px(8), .radius = 2, .bg = nv.col });
                            c.label(nv.n, self.fb, if (sel) txt else dim, .{});
                        }
                        c.close();
                    }
                    c.box(.{ .w = grow(), .h = px(10) });
                    c.box(.{ .w = grow(), .h = px(1), .bg = bord });
                    c.label("DEVICES", self.fb, faint, .{ .h = px(28) });
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
                        c.label("ARRANGEMENT", self.fb, faint, .{ .h = px(18) });
                        c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 12, .pad = 10, .gap = 5, .bg = panel_t, .bg2 = panel_b, .border = bord, .elev = 0.5, .shadow = 14 });
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
                                    c.open(.{ .dir = .row, .w = px(180), .h = grow(), .radius = 9, .pad = 9, .gap = 9, .aligni = .center, .bg = card_t, .bg2 = card_b, .border = bord, .elev = 1 });
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
                        c.label("MIXER", self.fb, faint, .{ .h = px(18) });
                        c.open(.{ .dir = .row, .w = grow(), .h = grow(), .radius = 12, .pad = 10, .gap = 9, .bg = panel_t, .bg2 = panel_b, .border = bord, .elev = 0.5, .shadow = 14 });
                        {
                            var ti: usize = 0;
                            while (ti <= ntr) : (ti += 1) {
                                const is_master = ti == ntr;
                                const scol = if (is_master) accent else pal[ti % pal.len];
                                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 10, .pad = 9, .gap = 6, .bg = card_t, .bg2 = card_b, .border = bord, .elev = 1 });
                                {
                                    c.box(.{ .w = grow(), .h = px(4), .radius = 2, .bg = scol });
                                    c.label(if (is_master) "Master" else p.tracks.items[ti].name.items, self.fu, txt, .{ .h = px(21) });
                                    if (!is_master) {
                                        c.open(.{ .dir = .row, .w = grow(), .h = px(16), .gap = 4, .aligni = .center });
                                        {
                                            c.box(.{ .w = px(22), .h = px(16), .id = 300 + @as(u64, ti) });
                                            c.box(.{ .w = px(22), .h = px(16), .id = 320 + @as(u64, ti) });
                                            c.box(.{ .w = grow() });
                                            c.label("PAN", self.fb, faint, .{});
                                        }
                                        c.close();
                                        c.box(.{ .w = grow(), .h = px(6), .id = 400 + @as(u64, ti) });
                                        c.open(.{ .dir = .row, .w = grow(), .h = px(42), .gap = 6, .justify = .center });
                                        {
                                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                                            c.box(.{ .w = px(26), .h = px(26), .id = 600 + @as(u64, ti) });
                                            c.label("A", self.fb, faint, .{});
                                            c.close();
                                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                                            c.box(.{ .w = px(26), .h = px(26), .id = 700 + @as(u64, ti) });
                                            c.label("B", self.fb, faint, .{});
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
        c.close();
        c.end(); // draws all chrome + computes rects

        // ---- WIDGETS / custom content into the solved rects ----------------
        // transport
        const picol = if (state.playing) Color.rgb(14, 18, 22) else txt;
        if (c.rectOf(1)) |r| {
            if (state.playing) glow(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 20, Color.rgba(96, 210, 235, 110));
            if (u.iconSlot(1, r[0], r[1], r[2], r[3], state.playing)) state.playing = !state.playing;
            if (state.playing) icons.pause(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 13, picol) else icons.play(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 14, picol);
        }
        if (c.rectOf(2)) |r| {
            _ = u.iconSlot(2, r[0], r[1], r[2], r[3], false);
            icons.stop(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 12, dim);
        }
        if (c.rectOf(3)) |r| {
            _ = u.iconSlot(3, r[0], r[1], r[2], r[3], false);
            icons.record(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 12, red);
        }
        // window buttons
        if (c.rectOf(900)) |r| {
            if (u.iconSlot(900, r[0], r[1], r[2], r[3], false)) state.window_action = .minimize;
            icons.minimize(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 11, dim);
        }
        if (c.rectOf(901)) |r| {
            if (u.iconSlot(901, r[0], r[1], r[2], r[3], false)) state.window_action = .maximize;
            icons.maximize(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 10, dim);
        }
        if (c.rectOf(902)) |r| {
            if (u.iconSlot(902, r[0], r[1], r[2], r[3], false)) state.window_action = .close;
            icons.close(g, r[0] + r[2] / 2, r[1] + r[3] / 2, 10, if (u.hoverOf(902) > 0.1) Color.rgb(248, 120, 120) else dim);
        }
        // nav selection (flex hover handled in chrome; click via flex)
        if (c.click >= 1000 and c.click < 1010) state.nav_sel = @intCast(c.click - 1000);

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
            if (c.rectOf(800 + @as(u64, ti))) |r| drawClips(g, self.fb, u, p, ti, r, bar, state);
        }

        // mixer widgets
        ti = 0;
        while (ti <= ntr) : (ti += 1) {
            const is_master = ti == ntr;
            const gain = if (is_master) &state.master_gain else &p.tracks.items[ti].gain;
            if (!is_master) {
                if (c.rectOf(300 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(300 + ti), r, "M", state.mutes[ti], amber)) {
                    state.mutes[ti] = !state.mutes[ti];
                };
                if (c.rectOf(320 + @as(u64, ti))) |r| if (miniToggle(u, g, self.fb, @intCast(320 + ti), r, "S", state.solos[ti], green)) {
                    state.solos[ti] = !state.solos[ti];
                };
                if (c.rectOf(400 + @as(u64, ti))) |r| _ = u.hSlider(@intCast(400 + ti), r[0], r[1], r[2], r[3], &p.tracks.items[ti].pan, -1.0, 1.0);
                if (c.rectOf(600 + @as(u64, ti))) |r| _ = u.knob(@intCast(600 + ti), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][0]);
                if (c.rectOf(700 + @as(u64, ti))) |r| _ = u.knob(@intCast(700 + ti), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][1]);
            }
            if (c.rectOf(100 + @as(u64, ti))) |r| _ = u.vFader(@intCast(100 + ti), r[0], r[1], r[2], r[3], gain);
            if (c.rectOf(500 + @as(u64, ti))) |r| {
                g.rect(r[0], r[1], r[2], r[3], 4, Color.rgb(15, 17, 22));
                const lvl = if (!is_master and state.mutes[ti]) 0.0 else gain.* * 0.92;
                const mh = lvl * r[3];
                if (mh > 1) g.rectGrad(r[0], r[1] + r[3] - mh, r[2], mh, 4, red, green, 0, bord);
            }
            if (c.rectOf(100 + @as(u64, ti))) |r| {
                var vbuf: [8]u8 = undefined;
                const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
                self.fb.text(g, r[0] - 6, r[1] + r[3] + 5, vs, dim);
            }
        }

        // title-bar drag region (avoid the interactive clusters)
        if (state.window_action == .none and u.pressed and my < 56 and (mx < 150 or (mx > 300 and mx < W - 360))) state.window_action = .move;

        u.end();
        g.flush();
        return state.window_action;
    }
};

fn glow(g: *Gpu, cx: f32, cy: f32, r: f32, c: Color) void {
    g.shadow(cx - r, cy - r, 2 * r, 2 * r, r, r * 0.55, c);
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
