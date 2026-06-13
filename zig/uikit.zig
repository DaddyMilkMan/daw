//! uikit.zig — immediate-mode UI toolkit with animated micro-interactions.
//! Each widget keeps persistent per-id animation state (hover fade, press feel)
//! eased over time (dt), the way good web UIs feel. Pure Zig.

const std = @import("std");
const r2d = @import("render2d.zig");
const Canvas = r2d.Canvas;
const Color = r2d.Color;

pub const Input = struct {
    mx: i32 = -1,
    my: i32 = -1,
    mouse_down: bool = false,
};

const Anim = struct {
    id: u32 = 0,
    used: bool = false,
    hover: f32 = 0,
    press: f32 = 0,
};

fn lerp(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}
fn ease(cur: f32, target: f32, dt: f32, speed: f32) f32 {
    return cur + (target - cur) * @min(1.0, dt * speed);
}

pub const Ui = struct {
    cv: *Canvas,
    in: Input = .{},
    prev_down: bool = false,
    pressed: bool = false,
    released: bool = false,
    active: u32 = 0,
    hot: u32 = 0,
    dt: f32 = 0.016,
    anims: [128]Anim = [_]Anim{.{}} ** 128,

    pub fn init(cv: *Canvas) Ui {
        return .{ .cv = cv };
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

    fn inside(self: *Ui, x: i32, y: i32, w: i32, h: i32) bool {
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
    /// hover progress 0..1 of the most recent widget (for app-level effects).
    pub fn hoverOf(self: *Ui, id: u32) f32 {
        return self.anim(id).hover;
    }

    pub fn button(self: *Ui, id: u32, x: i32, y: i32, w: i32, h: i32, label: []const u8, on: bool) bool {
        const hov = self.inside(x, y, w, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;

        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 14);
        a.press = ease(a.press, if (self.active == id) 1 else 0, self.dt, 24);

        const base = if (on) Color.rgb(96, 210, 235) else lerp(Color.rgb(40, 44, 54), Color.rgb(66, 72, 88), a.hover);
        const col = lerp(base, Color.rgb(255, 255, 255), a.press * 0.14);
        // tiny press "squish"
        const inset: i32 = @intFromFloat(a.press * 1.5);
        self.cv.fillRoundedRect(x + inset, y + inset, w - 2 * inset, h - 2 * inset, 7, col);
        if (a.hover > 0.02 and !on) self.cv.fillRoundedRect(x, y, w, 1, 7, .{ .r = 96, .g = 210, .b = 235, .a = @intFromFloat(a.hover * 120) });
        self.cv.text(x + 7, y + @divTrunc(h - 8, 2), label, if (on) Color.rgb(16, 20, 24) else Color.rgb(232, 236, 244), 1);
        return clicked;
    }

    pub fn vFader(self: *Ui, id: u32, x: i32, y: i32, w: i32, h: i32, value: *f32) bool {
        const hov = self.inside(x - 8, y, w + 16, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const rel = @as(f32, @floatFromInt(self.in.my - y)) / @as(f32, @floatFromInt(@max(h, 1)));
            const nv = std.math.clamp(1.0 - rel, 0.0, 1.0);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);

        self.cv.fillRoundedRect(x, y, w, h, @divTrunc(w, 2), Color.rgb(34, 38, 47));
        const fill = @as(i32, @intFromFloat(value.* * @as(f32, @floatFromInt(h))));
        self.cv.fillRoundedRect(x, y + h - fill, w, fill, @divTrunc(w, 2), lerp(Color.rgb(70, 120, 150), Color.rgb(96, 210, 235), a.hover));
        const knob_y = y + h - @as(i32, @intFromFloat(value.* * @as(f32, @floatFromInt(h - 16)))) - 16;
        const grow: i32 = @intFromFloat(a.hover * 3);
        self.cv.dropShadow(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, 2);
        self.cv.fillRoundedRect(x - 11 - grow, knob_y, 24 + 2 * grow, 16, 6, lerp(Color.rgb(214, 219, 228), Color.rgb(255, 255, 255), a.hover));
        return changed;
    }

    pub fn hSlider(self: *Ui, id: u32, x: i32, y: i32, w: i32, h: i32, value: *f32, lo: f32, hi: f32) bool {
        const hov = self.inside(x, y - 5, w, h + 10);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const t = std.math.clamp(@as(f32, @floatFromInt(self.in.mx - x)) / @as(f32, @floatFromInt(@max(w, 1))), 0.0, 1.0);
            const nv = lo + t * (hi - lo);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);
        self.cv.fillRoundedRect(x, y, w, h, @divTrunc(h, 2), Color.rgb(34, 38, 47));
        const t = (value.* - lo) / (hi - lo);
        const kx = x + @as(i32, @intFromFloat(t * @as(f32, @floatFromInt(w - 6))));
        const grow: i32 = @intFromFloat(a.hover * 2);
        self.cv.fillRoundedRect(kx - grow, y - 3 - grow, 6 + 2 * grow, h + 6 + 2 * grow, 4, lerp(Color.rgb(120, 170, 200), Color.rgb(96, 210, 235), a.hover));
        return changed;
    }
};
