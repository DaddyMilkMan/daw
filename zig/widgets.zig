//! widgets.zig — reusable GPU immediate-mode widgets (faders, sliders, knobs,
//! icon slots) drawn through gpu2d, with per-id hover/press animation. No DAW or
//! layout dependency — pair it with flex.zig (positions) or call with raw rects.
//! Original Zig.

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;

const accent = Color.rgb(108, 147, 244); // electric indigo
const accent_hi = Color.rgb(190, 205, 255);
const border = Color.rgba(255, 255, 255, 0); // no border lines on widgets
const track_bg = Color.rgb(24, 26, 33);

pub const Input = struct { mx: f32 = -1, my: f32 = -1, mouse_down: bool = false };
const Anim = struct { id: u32 = 0, used: bool = false, hover: f32 = 0, press: f32 = 0, extra: f32 = 0 };

fn ease(cur: f32, target: f32, dt: f32, speed: f32) f32 {
    return cur + (target - cur) * @min(1.0, dt * speed);
}
fn lerp(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

pub const Ui = struct {
    g: *Gpu,
    in: Input = .{},
    prev_down: bool = false,
    pressed: bool = false,
    released: bool = false,
    active: u32 = 0,
    hot: u32 = 0,
    dt: f32 = 0.016,
    anims: [256]Anim = [_]Anim{.{}} ** 256,

    pub fn init(g: *Gpu) Ui {
        return .{ .g = g };
    }
    pub fn begin(self: *Ui, in: Input, dt: f32) void {
        self.pressed = in.mouse_down and !self.prev_down;
        self.released = !in.mouse_down and self.prev_down;
        self.in = in;
        self.dt = dt;
        self.hot = 0;
    }
    pub fn end(self: *Ui) void {
        self.prev_down = self.in.mouse_down;
        if (self.released) self.active = 0;
    }
    fn inside(self: *Ui, x: f32, y: f32, w: f32, h: f32) bool {
        return self.in.mx >= x and self.in.mx < x + w and self.in.my >= y and self.in.my < y + h;
    }
    fn anim(self: *Ui, id: u32) *Anim {
        for (&self.anims) |*a| if (a.used and a.id == id) return a;
        for (&self.anims) |*a| if (!a.used) {
            a.* = .{ .id = id, .used = true };
            return a;
        };
        return &self.anims[0];
    }
    pub fn hoverOf(self: *Ui, id: u32) f32 {
        return self.anim(id).hover;
    }

    /// Animated empty button background (caller draws an icon/label on top).
    pub fn iconSlot(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, active_: bool) bool {
        const hov = self.inside(x, y, w, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 14);
        a.press = ease(a.press, if (self.active == id) 1 else 0, self.dt, 24);
        const base = if (active_) accent else lerp(Color.rgb(38, 42, 52), Color.rgb(60, 66, 80), a.hover);
        self.g.rect(x, y, w, h, 7, lerp(base, Color.rgb(255, 255, 255), a.press * 0.14));
        return clicked;
    }

    pub fn vFader(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32) bool {
        const hov = self.inside(x - 8, y, w + 16, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const nv = std.math.clamp(1.0 - (self.in.my - y) / @max(h, 1), 0.0, 1.0);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);
        self.g.rect(x, y, w, h, w / 2, track_bg);
        const fill = value.* * h;
        self.g.rect(x, y + h - fill, w, fill, w / 2, lerp(Color.rgb(66, 84, 150), accent, a.hover));
        const grow = a.hover * 3;
        const knob_y = y + h - value.* * (h - 16) - 16;
        self.g.shadow(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, 5, Color.rgba(0, 0, 0, 150));
        self.g.card(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, lerp(Color.rgb(220, 225, 234), Color.rgb(255, 255, 255), a.hover), Color.rgb(196, 202, 214), 0, border, 1.0);
        return changed;
    }

    pub fn hSlider(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32, lo: f32, hi: f32) bool {
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
        self.g.rect(x, y, w, h, h / 2, track_bg);
        const t = (value.* - lo) / (hi - lo);
        const kx = x + t * (w - 6);
        const grow = a.hover * 2;
        self.g.card(kx - grow, y - 3 - grow, 6 + 2 * grow, h + 6 + 2 * grow, 4, lerp(Color.rgb(124, 144, 214), accent, a.hover), lerp(Color.rgb(92, 110, 188), accent, a.hover), 0, border, 1.0);
        return changed;
    }

    pub fn knob(self: *Ui, id: u32, cx: f32, cy: f32, radius: f32, value: *f32) bool {
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
        self.g.card(cx - r, cy - r, 2 * r, 2 * r, r, Color.rgb(38, 42, 52), Color.rgb(28, 31, 39), 0, border, 0.6);
        // solid arc (modern) — connected segments, track + accent progress
        const start = std.math.pi * 0.75;
        const sweep = std.math.pi * 1.5;
        const ar = r - 2.5;
        const steps: usize = 30;
        var prevx: f32 = cx + @cos(start) * ar;
        var prevy: f32 = cy + @sin(start) * ar;
        var i: usize = 1;
        while (i <= steps) : (i += 1) {
            const t = @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(steps));
            const ang = start + t * sweep;
            const cxp = cx + @cos(ang) * ar;
            const cyp = cy + @sin(ang) * ar;
            const on = t <= value.*;
            const col = if (on) lerp(accent, accent_hi, a.hover * 0.5) else Color.rgb(44, 48, 58);
            self.g.line(prevx, prevy, cxp, cyp, 2.6, col);
            prevx = cxp;
            prevy = cyp;
        }
        // indicator
        const ang = start + value.* * sweep;
        self.g.line(cx + @cos(ang) * (ar - 6), cy + @sin(ang) * (ar - 6), cx + @cos(ang) * ar, cy + @sin(ang) * ar, 2.4, Color.rgb(240, 243, 250));
        return changed;
    }
};
