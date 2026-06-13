//! uikit.zig — a tiny immediate-mode UI toolkit. Widgets both draw and handle
//! interaction inline using a hot/active item model (imgui-style). Pure Zig.

const std = @import("std");
const r2d = @import("render2d.zig");
const Canvas = r2d.Canvas;
const Color = r2d.Color;

pub const Input = struct {
    mx: i32 = -1,
    my: i32 = -1,
    mouse_down: bool = false,
};

pub const Ui = struct {
    cv: *Canvas,
    in: Input = .{},
    prev_down: bool = false,
    pressed: bool = false, // mouse went down this frame
    released: bool = false, // mouse went up this frame
    active: u32 = 0, // widget currently being dragged
    hot: u32 = 0, // widget hovered this frame

    pub fn init(cv: *Canvas) Ui {
        return .{ .cv = cv };
    }

    pub fn begin(self: *Ui, in: Input) void {
        self.pressed = in.mouse_down and !self.prev_down;
        self.released = !in.mouse_down and self.prev_down;
        self.in = in;
        self.hot = 0;
    }
    pub fn end(self: *Ui) void {
        self.prev_down = self.in.mouse_down;
        if (self.released) self.active = 0;
    }

    fn inside(self: *Ui, x: i32, y: i32, w: i32, h: i32) bool {
        return self.in.mx >= x and self.in.mx < x + w and self.in.my >= y and self.in.my < y + h;
    }

    /// Returns true on the frame the button is clicked (press+release inside).
    pub fn button(self: *Ui, id: u32, x: i32, y: i32, w: i32, h: i32, label: []const u8, on: bool) bool {
        const hov = self.inside(x, y, w, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;
        const base = if (on) Color.rgb(90, 210, 230) else if (hov) Color.rgb(54, 60, 74) else Color.rgb(40, 44, 54);
        self.cv.fillRect(x, y, w, h, base);
        self.cv.text(x + 6, y + @divTrunc(h - 8, 2), label, if (on) Color.rgb(20, 22, 27) else Color.rgb(232, 236, 244), 1);
        return clicked;
    }

    /// Vertical fader, `value` in 0..1 (top = 1). Returns true if dragged.
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
        self.cv.fillRect(x, y, w, h, Color.rgb(40, 44, 54));
        const knob_y = y + h - @as(i32, @intFromFloat(value.* * @as(f32, @floatFromInt(h - 12)))) - 12;
        const kc = if (self.active == id) Color.rgb(90, 210, 230) else Color.rgb(86, 92, 108);
        self.cv.fillRect(x - 8, knob_y, w + 16, 12, kc);
        self.cv.fillRect(x - 8, knob_y + 5, w + 16, 2, Color.rgb(20, 22, 27));
        return changed;
    }

    /// Horizontal slider, `value` mapped over [lo,hi]. Returns true if dragged.
    pub fn hSlider(self: *Ui, id: u32, x: i32, y: i32, w: i32, h: i32, value: *f32, lo: f32, hi: f32) bool {
        const hov = self.inside(x, y - 4, w, h + 8);
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
        self.cv.fillRect(x, y, w, h, Color.rgb(40, 44, 54));
        const t = (value.* - lo) / (hi - lo);
        const kx = x + @as(i32, @intFromFloat(t * @as(f32, @floatFromInt(w - 5))));
        self.cv.fillRect(kx, y - 3, 5, h + 6, if (self.active == id) Color.rgb(90, 210, 230) else Color.rgb(86, 92, 108));
        return changed;
    }
};
