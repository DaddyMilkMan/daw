//! ui_gpu.zig — the Zenith DAW view rendered entirely on the GPU toolkit
//! (gpu2d): SDF rounded rects, analytic shadows, gradient/material cards,
//! atlas text. Same docked layout as the CPU ui.zig, ported to f32 + gpu2d.
//! Immediate-mode widgets (faders/knobs/sliders/toggles) live here too.

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
const project = @import("project.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;

const uimod = @import("ui.zig");
pub const State = uimod.State;
pub const WinAction = uimod.WinAction;
pub const buildDemoProject = uimod.buildDemoProject;

// ---- color system (OKLCH-derived, matches the CPU path) --------------------
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
        .r = encSrgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        .g = encSrgb(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        .b = encSrgb(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s),
    };
}
fn encSrgb(c: f32) u8 {
    const x = std.math.clamp(c, 0.0, 1.0);
    const s = if (x <= 0.0031308) x * 12.92 else 1.055 * std.math.pow(f32, x, 1.0 / 2.4) - 0.055;
    return @intFromFloat(@round(std.math.clamp(s, 0.0, 1.0) * 255.0));
}
const Cols = struct { palette: [5]Color, accent: Color, accent2: Color };
const cols: Cols = blk: {
    @setEvalBranchQuota(1_000_000);
    break :blk .{
        .palette = .{
            oklch(0.77, 0.135, 58),
            oklch(0.80, 0.145, 152),
            oklch(0.78, 0.110, 233),
            oklch(0.73, 0.150, 295),
            oklch(0.75, 0.150, 358),
        },
        .accent = oklch(0.80, 0.115, 228),
        .accent2 = oklch(0.73, 0.150, 295),
    };
};
const palette = cols.palette;
const c_accent = cols.accent;
const c_accent2 = cols.accent2;

const c_bg_top = Color.rgb(26, 28, 36);
const c_bg_bot = Color.rgb(13, 14, 19);
const c_panel_top = Color.rgb(37, 40, 51);
const c_panel_bot = Color.rgb(27, 29, 38);
const c_card_top = Color.rgb(47, 51, 64);
const c_card_bot = Color.rgb(34, 37, 48);
const c_lane = Color.rgb(21, 23, 30);
const c_lane_alt = Color.rgb(24, 26, 34);
const c_border = Color.rgba(255, 255, 255, 16);
const c_grid = Color.rgba(255, 255, 255, 10);
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
fn clerp(a: Color, b: Color, t: f32) Color {
    return mix(a, b, t);
}

// ---- immediate-mode toolkit ------------------------------------------------
pub const Input = struct { mx: f32 = -1, my: f32 = -1, mouse_down: bool = false };
const Anim = struct { id: u32 = 0, used: bool = false, hover: f32 = 0, press: f32 = 0, extra: f32 = 0 };

fn ease(cur: f32, target: f32, dt: f32, speed: f32) f32 {
    return cur + (target - cur) * @min(1.0, dt * speed);
}

pub const UiG = struct {
    g: *Gpu,
    fb: *const Font,
    fu: *const Font,
    fd: *const Font,
    in: Input = .{},
    prev_down: bool = false,
    pressed: bool = false,
    released: bool = false,
    active: u32 = 0,
    hot: u32 = 0,
    dt: f32 = 0.016,
    anims: [256]Anim = [_]Anim{.{}} ** 256,

    pub fn init(g: *Gpu, fb: *const Font, fu: *const Font, fd: *const Font) UiG {
        return .{ .g = g, .fb = fb, .fu = fu, .fd = fd };
    }
    pub fn begin(self: *UiG, in: Input, dt: f32) void {
        self.pressed = in.mouse_down and !self.prev_down;
        self.released = !in.mouse_down and self.prev_down;
        self.in = in;
        self.dt = dt;
        self.hot = 0;
    }
    pub fn end(self: *UiG) void {
        self.prev_down = self.in.mouse_down;
        if (self.released) self.active = 0;
    }
    fn inside(self: *UiG, x: f32, y: f32, w: f32, h: f32) bool {
        return self.in.mx >= x and self.in.mx < x + w and self.in.my >= y and self.in.my < y + h;
    }
    fn anim(self: *UiG, id: u32) *Anim {
        for (&self.anims) |*a| if (a.used and a.id == id) return a;
        for (&self.anims) |*a| if (!a.used) {
            a.* = .{ .id = id, .used = true };
            return a;
        };
        return &self.anims[0];
    }
    pub fn hoverOf(self: *UiG, id: u32) f32 {
        return self.anim(id).hover;
    }

    pub fn iconSlot(self: *UiG, id: u32, x: f32, y: f32, w: f32, h: f32, active_: bool) bool {
        const hov = self.inside(x, y, w, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 14);
        a.press = ease(a.press, if (self.active == id) 1 else 0, self.dt, 24);
        const base = if (active_) c_accent else clerp(Color.rgb(38, 42, 52), Color.rgb(60, 66, 80), a.hover);
        self.g.rect(x, y, w, h, 7, clerp(base, Color.rgb(255, 255, 255), a.press * 0.14));
        return clicked;
    }

    pub fn vFader(self: *UiG, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32) bool {
        const hov = self.inside(x - 8, y, w + 16, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const rel = (self.in.my - y) / @max(h, 1);
            const nv = std.math.clamp(1.0 - rel, 0.0, 1.0);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);
        self.g.rect(x, y, w, h, w / 2, Color.rgb(28, 31, 39));
        const fill = value.* * h;
        self.g.rect(x, y + h - fill, w, fill, w / 2, clerp(Color.rgb(70, 120, 150), c_accent, a.hover));
        const grow = a.hover * 3;
        const knob_y = y + h - value.* * (h - 16) - 16;
        self.g.shadow(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, 5, Color.rgba(0, 0, 0, 150));
        self.g.card(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, clerp(Color.rgb(220, 225, 234), Color.rgb(255, 255, 255), a.hover), Color.rgb(196, 202, 214), 0, c_border, 1.0);
        return changed;
    }

    pub fn hSlider(self: *UiG, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32, lo: f32, hi: f32) bool {
        const hov = self.inside(x, y - 5, w, h + 10);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const t = std.math.clamp((self.in.mx - x) / @max(w, 1), 0.0, 1.0);
            const nv = lo + t * (hi - lo);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);
        self.g.rect(x, y, w, h, h / 2, Color.rgb(28, 31, 39));
        const t = (value.* - lo) / (hi - lo);
        const kx = x + t * (w - 6);
        const grow = a.hover * 2;
        self.g.card(kx - grow, y - 3 - grow, 6 + 2 * grow, h + 6 + 2 * grow, 4, clerp(Color.rgb(150, 195, 220), c_accent, a.hover), clerp(Color.rgb(110, 160, 190), c_accent, a.hover), 0, c_border, 1.0);
        return changed;
    }

    pub fn knob(self: *UiG, id: u32, cx: f32, cy: f32, radius: f32, value: *f32) bool {
        const dx = self.in.mx - cx;
        const dy = self.in.my - cy;
        const within = (dx * dx + dy * dy) <= (radius + 8) * (radius + 8);
        if (within) self.hot = id;
        const a = self.anim(id);
        if (within and self.pressed) {
            self.active = id;
            a.extra = self.in.my;
        }
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const d = a.extra - self.in.my;
            const nv = std.math.clamp(value.* + d * 0.006, 0.0, 1.0);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
            a.extra = self.in.my;
        }
        a.hover = ease(a.hover, if (within or self.active == id) 1 else 0, self.dt, 14);

        const r = radius;
        self.g.card(cx - r, cy - r, 2 * r, 2 * r, r, Color.rgb(42, 46, 58), Color.rgb(30, 33, 42), 1, c_border, 1.0);
        const start = std.math.pi * 0.75;
        const sweep = std.math.pi * 1.5;
        var i: usize = 0;
        const steps: usize = 36;
        while (i <= steps) : (i += 1) {
            const t = @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(steps));
            const ang = start + t * sweep;
            const px = cx + @cos(ang) * (r - 3);
            const py = cy + @sin(ang) * (r - 3);
            const col = if (t <= value.*) clerp(c_accent, Color.rgb(210, 245, 255), a.hover * 0.5) else Color.rgb(52, 56, 68);
            self.g.rect(px - 1.6, py - 1.6, 3.2, 3.2, 1.6, col);
        }
        const ang = start + value.* * sweep;
        self.g.rect(cx + @cos(ang) * (r - 7) - 1.6, cy + @sin(ang) * (r - 7) - 1.6, 3.2, 3.2, 1.6, c_text);
        return changed;
    }
};

// ---- vector icons (GPU) ----------------------------------------------------
const icons = struct {
    fn play(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.tri(cx - s * 0.34, cy - s * 0.52, cx - s * 0.34, cy + s * 0.52, cx + s * 0.5, cy, c);
    }
    fn pause(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.42, cy - s * 0.5, s * 0.3, s, 1.5, c);
        g.rect(cx + s * 0.12, cy - s * 0.5, s * 0.3, s, 1.5, c);
    }
    fn stop(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.46, cy - s * 0.46, s * 0.92, s * 0.92, 2.5, c);
    }
    fn record(g: *Gpu, cx: f32, cy: f32, s: f32, c: Color) void {
        g.rect(cx - s * 0.46, cy - s * 0.46, s * 0.92, s * 0.92, s * 0.46, c);
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

// ---- view helpers ----------------------------------------------------------
fn cardBox(g: *Gpu, x: f32, y: f32, w: f32, h: f32, r: f32, top: Color, bot: Color, elev: f32) void {
    g.card(x, y, w, h, r, top, bot, 1, c_border, elev);
    g.rect(x + r, y + 1, w - 2 * r, 1, 0, Color.rgba(255, 255, 255, 28)); // top rim highlight
}
fn glow(g: *Gpu, cx: f32, cy: f32, r: f32, c: Color) void {
    g.shadow(cx - r, cy - r, 2 * r, 2 * r, r, r * 0.55, c);
}

fn miniToggle(ui: *UiG, id: u32, x: f32, y: f32, w: f32, h: f32, lbl: []const u8, on: bool, oncol: Color) bool {
    const clicked = ui.iconSlot(id, x, y, w, h, false);
    if (on) ui.g.rect(x, y, w, h, 5, oncol);
    const tw = ui.fb.textWidth(lbl);
    ui.fb.text(ui.g, x + (w - tw) / 2, y + (h - 12) / 2, lbl, if (on) Color.rgb(18, 20, 26) else c_dim);
    return clicked;
}

fn drawClipNotes(g: *Gpu, clip: project.Clip, x: f32, y: f32, w: f32, h: f32, nc: Color) void {
    if (clip.length == 0) return;
    const lo: f32 = 32;
    const hi: f32 = 92;
    const clen: f32 = @floatFromInt(clip.length);
    for (clip.notes.items) |note| {
        const nx = x + @as(f32, @floatFromInt(note.start)) / clen * w;
        const nw = @max(@as(f32, @floatFromInt(note.len)) / clen * w, 2);
        const pn = std.math.clamp((@as(f32, @floatFromInt(note.pitch)) - lo) / (hi - lo), 0.0, 1.0);
        const ny = y + h - 3 - pn * @max(h - 6, 1);
        g.rect(nx + 1, ny, nw - 1, 3, 1, nc);
    }
}

fn titleBar(ui: *UiG, W: f32, h: f32, state: *State) void {
    const g = ui.g;
    g.rectGrad(0, 0, W, h, 0, Color.rgb(34, 37, 47), Color.rgb(24, 26, 34), 0, c_border);
    g.rect(0, 0, W, 1, 0, Color.rgba(255, 255, 255, 28));
    g.rect(0, h - 1, W, 1, 0, Color.rgb(9, 10, 14));
    ui.fd.text(g, 22, 13, "Zenith", c_accent);

    cardBox(g, 146, 12, 138, 34, 9, Color.rgb(30, 33, 42), Color.rgb(23, 25, 33), 0);
    const ty: f32 = 15;
    if (state.playing) glow(g, 170, ty + 14, 22, Color.rgba(96, 210, 235, 110));
    const picol = if (state.playing) Color.rgb(14, 18, 22) else c_text;
    if (ui.iconSlot(1, 151, ty, 38, 28, state.playing)) state.playing = !state.playing;
    if (state.playing) icons.pause(g, 170, ty + 14, 13, picol) else icons.play(g, 170, ty + 14, 14, picol);
    _ = ui.iconSlot(2, 192, ty, 38, 28, false);
    icons.stop(g, 211, ty + 14, 12, c_dim);
    _ = ui.iconSlot(3, 233, ty, 38, 28, false);
    icons.record(g, 252, ty + 14, 12, c_red);

    g.rect(300, 12, 1, 34, 0, c_border);
    ui.fd.text(g, 316, 8, "120", c_text);
    ui.fb.text(g, 316, 38, "BPM   4 / 4", c_dim);
    g.rect(412, 12, 1, 34, 0, c_border);
    ui.fd.text(g, W / 2 - 64, 12, "00 : 00 : 04", c_text);

    const dotc = if (state.playing) c_green else c_faint;
    g.rect(W - 336, ty + 3, 7, 7, 3, dotc);
    if (state.playing) glow(g, W - 333, ty + 6, 8, Color.rgba(122, 211, 140, 150));
    ui.fu.text(g, W - 322, 12, if (state.playing) "Playing" else "Stopped", if (state.playing) c_accent else c_dim);
    ui.fb.text(g, W - 322, 34, "100% Zig  -  CLAP ready", c_faint);

    if (ui.iconSlot(900, W - 108, 16, 30, 24, false)) state.window_action = .minimize;
    icons.minimize(g, W - 93, 28, 11, c_dim);
    if (ui.iconSlot(901, W - 74, 16, 30, 24, false)) state.window_action = .maximize;
    icons.maximize(g, W - 59, 28, 10, c_dim);
    if (ui.iconSlot(902, W - 40, 16, 30, 24, false)) state.window_action = .close;
    icons.close(g, W - 25, 28, 10, if (ui.hoverOf(902) > 0.1) Color.rgb(248, 120, 120) else c_dim);

    if (state.window_action == .none and ui.pressed and ui.in.my < h and
        (ui.in.mx < 144 or (ui.in.mx > 286 and ui.in.mx < W - 346)))
        state.window_action = .move;
}

fn browser(ui: *UiG, x: f32, y: f32, w: f32, h: f32) void {
    const g = ui.g;
    g.rectGrad(x, y, w, h, 0, c_panel_top, c_panel_bot, 0, c_border);
    g.rect(x + w - 1, y, 1, h, 0, Color.rgb(9, 10, 14));
    g.rect(x, y, w, 1, 0, Color.rgba(255, 255, 255, 28));

    var yy = y + 16;
    ui.fb.text(g, x + 18, yy, "BROWSER", c_faint);
    yy += 26;
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
        const rh: f32 = 30;
        const ry = yy + @as(f32, @floatFromInt(i)) * rh;
        const sel = (i == 0);
        if (sel) {
            g.rectGrad(x + 8, ry, w - 18, rh - 4, 7, Color.rgb(45, 60, 70), Color.rgb(36, 47, 56), 1, c_border);
            g.rect(x + 8, ry + 5, 3, rh - 14, 1, c_accent);
        }
        g.rect(x + 22, ry + 9, 8, 8, 2, cat.col);
        ui.fb.text(g, x + 40, ry + 5, cat.name, if (sel) c_text else c_dim);
    }
    yy += @as(f32, @floatFromInt(cats.len)) * 30 + 14;
    g.rect(x + 16, yy, w - 34, 1, 0, c_border);
    yy += 14;
    ui.fb.text(g, x + 18, yy, "DEVICES", c_faint);
    yy += 26;
    const devs = [_]struct { name: []const u8, tag: []const u8, col: Color }{
        .{ .name = "Operator", .tag = "INST", .col = palette[3] },
        .{ .name = "Analog", .tag = "INST", .col = palette[3] },
        .{ .name = "Reverb", .tag = "FX", .col = palette[1] },
        .{ .name = "EQ Eight", .tag = "FX", .col = palette[1] },
        .{ .name = "Compressor", .tag = "FX", .col = palette[1] },
        .{ .name = "Saturator", .tag = "FX", .col = palette[1] },
    };
    for (devs, 0..) |dev, i| {
        const rh: f32 = 28;
        const ry = yy + @as(f32, @floatFromInt(i)) * rh;
        if (ry + rh > y + h - 8) break;
        g.rect(x + 22, ry + 9, 6, 6, 2, dev.col);
        ui.fb.text(g, x + 38, ry + 4, dev.name, c_dim);
        const tw = ui.fb.textWidth(dev.tag);
        g.rect(x + w - 22 - tw - 10, ry + 4, tw + 10, 18, 5, Color.rgb(30, 33, 42));
        ui.fb.text(g, x + w - 22 - tw - 5, ry + 4, dev.tag, c_faint);
    }
}

fn ruler(ui: *UiG, x: f32, y: f32, w: f32, h: f32, bars: i32) void {
    const g = ui.g;
    g.rectGrad(x, y, w, h, 6, Color.rgb(30, 33, 42), Color.rgb(25, 27, 35), 1, c_border);
    const bw = w / @as(f32, @floatFromInt(bars));
    var i: i32 = 0;
    while (i < bars) : (i += 1) {
        const bx = x + @as(f32, @floatFromInt(i)) * bw;
        if (i > 0) g.rect(bx, y + 4, 1, h - 8, 0, c_border);
        var buf: [8]u8 = undefined;
        const s = std.fmt.bufPrint(&buf, "{d}", .{i + 1}) catch "";
        ui.fb.text(g, bx + 8, y + (h - 12) / 2, s, c_dim);
        var be: i32 = 1;
        while (be < 4) : (be += 1) {
            const tx = bx + bw * @as(f32, @floatFromInt(be)) / 4;
            g.rect(tx, y + h - 7, 1, 4, 0, c_grid);
        }
    }
}

fn arrangement(ui: *UiG, p: *project.Project, bar: u64, rx: f32, ry0: f32, rw: f32, rh0: f32, state: *State) void {
    const g = ui.g;
    ui.fb.text(g, rx + 8, ry0 + 4, "ARRANGEMENT", c_faint);
    const ay = ry0 + 24;
    const cx = rx + 8;
    const cy = ay;
    const cw = rw - 16;
    const ch = (rh0 - 24) - 8;
    g.shadow(cx, cy, cw, ch, 12, 16, Color.rgba(0, 0, 0, 110));
    cardBox(g, cx, cy, cw, ch, 12, c_panel_top, c_panel_bot, 0.6);

    const ix = cx + 12;
    const iy = cy + 12;
    const iw = cw - 24;
    const ih = ch - 24;
    const ntracks: f32 = @floatFromInt(p.tracks.items.len);
    const hdr_w: f32 = 178;
    const tl_x = ix + hdr_w + 8;
    const tl_w = iw - hdr_w - 8;
    const bars: i32 = 4;
    const total: f64 = @floatFromInt(@as(i64, bars) * @as(i64, @intCast(bar)));
    const scale: f64 = @as(f64, tl_w) / total;
    const bar_px = tl_w / @as(f32, @floatFromInt(bars));

    const ruler_h: f32 = 24;
    ruler(ui, tl_x, iy, tl_w, ruler_h, bars);

    const grid_top = iy + ruler_h + 6;
    const rows_h = ih - ruler_h - 6;
    const gap: f32 = 6;
    const row_h = std.math.clamp((rows_h - (ntracks - 1) * gap) / ntracks, 28, 96);

    for (p.tracks.items, 0..) |t, ti| {
        const rry = grid_top + @as(f32, @floatFromInt(ti)) * (row_h + gap);
        const col = palette[ti % palette.len];

        cardBox(g, ix, rry, hdr_w, row_h, 9, c_card_top, c_card_bot, 1.0);
        g.rect(ix + 8, rry + 9, 4, row_h - 18, 2, col);
        const name_y = if (row_h >= 46) rry + 9 else rry + (row_h - 16) / 2;
        ui.fu.text(g, ix + 22, name_y, t.name.items, c_text);
        if (row_h >= 46) ui.fb.text(g, ix + 22, rry + 31, if (t.instrument == .sampler) "Sampler" else "Synth", c_dim);
        if (miniToggle(ui, 340 + @as(u32, @intCast(ti)), ix + hdr_w - 46, rry + 9, 18, 15, "M", state.mutes[ti], c_amber)) state.mutes[ti] = !state.mutes[ti];
        if (miniToggle(ui, 360 + @as(u32, @intCast(ti)), ix + hdr_w - 25, rry + 9, 18, 15, "S", state.solos[ti], c_green)) state.solos[ti] = !state.solos[ti];

        const lane_bg = if (ti % 2 == 0) c_lane else c_lane_alt;
        g.rect(tl_x, rry, tl_w, row_h, 8, lane_bg);
        var gl: i32 = 1;
        while (gl < bars) : (gl += 1) g.rect(tl_x + @as(f32, @floatFromInt(gl)) * bar_px, rry + 4, 1, row_h - 8, 0, c_grid);

        const muted = state.mutes[ti];
        for (t.clips.items, 0..) |clip, ci| {
            const clx = tl_x + @as(f32, @floatCast(@as(f64, @floatFromInt(clip.start)) * scale)) + 4;
            const clw = @max(@as(f32, @floatCast(@as(f64, @floatFromInt(clip.length)) * scale)) - 6, 12);
            const cby = rry + 5;
            const cbh = row_h - 10;
            const hovered = ui.in.mx >= clx and ui.in.mx < clx + clw and ui.in.my >= cby and ui.in.my < cby + cbh;
            if (hovered and ui.pressed) {
                state.sel_track = @intCast(ti);
                state.sel_clip = @intCast(ci);
            }
            const selected = state.sel_track == @as(i32, @intCast(ti)) and state.sel_clip == @as(i32, @intCast(ci));
            const cc = if (muted) mix(col, Color.rgb(70, 74, 86), 0.7) else col;
            g.shadow(clx, cby, clw, cbh, 7, 4, Color.rgba(0, 0, 0, 120));
            g.card(clx, cby, clw, cbh, 7, mix(cc, Color.rgb(255, 255, 255), if (hovered) 0.26 else 0.12), mix(cc, c_panel_bot, 0.5), 1, c_border, 1.0);
            g.rect(clx, cby, clw, 17, 7, mix(cc, Color.rgb(255, 255, 255), 0.22));
            drawClipNotes(g, clip, clx, cby + 18, clw, cbh - 21, mix(cc, Color.rgb(255, 255, 255), 0.45));
            ui.fb.text(g, clx + 8, cby + 1, clip.name.items, Color.rgb(14, 16, 22));
            if (selected) g.stroke(clx, cby, clw, cbh, 7, 1.5, c_accent);
        }
    }
}

fn channelStrip(ui: *UiG, bx: f32, by: f32, bw: f32, bh: f32, name: []const u8, col: Color, gain: *f32, pan: ?*f32, id: u32, ti: usize, state: *State, is_master: bool) void {
    const g = ui.g;
    g.shadow(bx, by, bw, bh, 11, 5, Color.rgba(0, 0, 0, 130));
    cardBox(g, bx, by, bw, bh, 11, c_card_top, c_card_bot, 1.0);
    g.rect(bx + 11, by + 9, bw - 22, 4, 2, col);
    if (is_master) glow(g, bx + bw - 16, by + 11, 10, Color.rgba(96, 210, 235, 90));
    ui.fu.text(g, bx + 12, by + 18, name, c_text);

    const bottom = by + bh - 22;
    var cy = by + 40;
    if (!is_master) {
        if (miniToggle(ui, 300 + @as(u32, @intCast(ti)), bx + 12, cy, 22, 16, "M", state.mutes[ti], c_amber)) state.mutes[ti] = !state.mutes[ti];
        if (miniToggle(ui, 320 + @as(u32, @intCast(ti)), bx + 38, cy, 22, 16, "S", state.solos[ti], c_green)) state.solos[ti] = !state.solos[ti];
        if (pan != null) ui.fb.text(g, bx + bw - 36, cy + 2, "PAN", c_faint);
        cy += 22;
        if (pan) |pp| {
            _ = ui.hSlider(id + 1000, bx + 12, cy, bw - 24, 6, pp, -1.0, 1.0);
            cy += 16;
        }
        if (bottom - (cy + 6) > 96) {
            const ay = cy + 12;
            const ax = bx + bw / 2 - 26;
            const bx2 = bx + bw / 2 + 26;
            _ = ui.knob(200 + @as(u32, @intCast(ti)) * 2, ax, ay, 12, &state.sends[ti][0]);
            _ = ui.knob(201 + @as(u32, @intCast(ti)) * 2, bx2, ay, 12, &state.sends[ti][1]);
            ui.fb.text(g, ax - 4, ay + 18, "A", c_faint);
            ui.fb.text(g, bx2 - 4, ay + 18, "B", c_faint);
            cy += 40;
        }
    } else {
        cy = by + 46;
    }

    const fy = cy + 6;
    const fader_h = @max(bottom - fy, 24);
    const fx = bx + bw / 2 - 18;
    _ = ui.vFader(id, fx, fy, 8, fader_h, gain);
    const mx2 = bx + bw / 2 + 12;
    g.rect(mx2, fy, 10, fader_h, 4, Color.rgb(15, 17, 22));
    const lvl = if (!is_master and state.mutes[ti]) 0.0 else gain.* * 0.92;
    const mh = lvl * fader_h;
    if (mh > 1) g.rectGrad(mx2, fy + fader_h - mh, 10, mh, 4, c_red, c_green, 0, c_border);
    var vbuf: [8]u8 = undefined;
    const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
    ui.fb.text(g, bx + 12, by + bh - 20, vs, c_dim);
}

fn mixer(ui: *UiG, p: *project.Project, rx: f32, ry0: f32, rw: f32, rh0: f32, state: *State) void {
    const g = ui.g;
    ui.fb.text(g, rx + 8, ry0 + 4, "MIXER", c_faint);
    const cx = rx + 8;
    const cy = ry0 + 24;
    const cw = rw - 16;
    const ch = (rh0 - 24) - 10;
    g.shadow(cx, cy, cw, ch, 12, 16, Color.rgba(0, 0, 0, 110));
    cardBox(g, cx, cy, cw, ch, 12, c_panel_top, c_panel_bot, 0.6);
    const ix = cx + 12;
    const iy = cy + 12;
    const iw = cw - 24;
    const ih = ch - 24;

    const n: f32 = @floatFromInt(p.tracks.items.len);
    const gap: f32 = 10;
    const strip_w = (iw - n * gap) / (n + 1);
    for (p.tracks.items, 0..) |*t, ti| {
        const sx = ix + @as(f32, @floatFromInt(ti)) * (strip_w + gap);
        channelStrip(ui, sx, iy, strip_w, ih, t.name.items, palette[ti % palette.len], &t.gain, &t.pan, @intCast(100 + ti), ti, state, false);
    }
    const msx = ix + n * (strip_w + gap);
    channelStrip(ui, msx, iy, ix + iw - msx, ih, "Master", c_accent, &state.master_gain, null, 800, 0, state, true);
}

pub fn frame(ui: *UiG, p: *project.Project, bar: u64, state: *State, W: f32, H: f32) void {
    const g = ui.g;
    state.window_action = .none;
    g.begin(@intFromFloat(W), @intFromFloat(H), c_bg_bot);
    // full-window background gradient
    g.rectGrad(0, 0, W, H, 0, c_bg_top, c_bg_bot, 0, c_border);

    const tb_h: f32 = 58;
    const browser_w = std.math.clamp(W * 20 / 100, 170, 230);
    const mix_h = std.math.clamp(H * 44 / 100, 220, 400);

    const content_y = tb_h;
    const content_h = H - tb_h;
    const arr_x = browser_w;
    const arr_w = W - browser_w;
    const arr_h = content_h - mix_h;

    browser(ui, 0, content_y, browser_w, content_h);
    arrangement(ui, p, bar, arr_x, content_y, arr_w, arr_h, state);
    mixer(ui, p, arr_x, content_y + arr_h, arr_w, mix_h, state);
    titleBar(ui, W, tb_h, state);

    g.flush();
}
