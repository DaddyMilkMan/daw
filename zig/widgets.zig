//! widgets.zig — reusable GPU immediate-mode widgets (faders, sliders, knobs,
//! icon slots) drawn through gpu2d, with per-id hover/press animation. No DAW or
//! layout dependency — pair it with flex.zig (positions) or call with raw rects.
//! Original Zig.

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;

const accent = Color.rgb(108, 147, 244); // electric indigo
const accent_hi = Color.rgb(190, 205, 255);
const accent_lo = Color.rgb(82, 116, 210);
const border = Color.rgba(255, 255, 255, 0); // no border lines on widgets
const track_bg = Color.rgb(24, 26, 33);
const c_on = Color.rgb(236, 239, 246);
const c_off = Color.rgb(150, 158, 173);

pub const Input = struct { mx: f32 = -1, my: f32 = -1, mouse_down: bool = false };

/// Column alignment for the table widget.
pub const Align = enum { left, right, center };
/// One table column: heading, pixel width, alignment, and whether its cells are
/// numeric (rendered with tabular figures so digits line up).
pub const Col = struct { title: []const u8, w: f32, al: Align = .left, num: bool = false };

fn tableCell(g: *Gpu, f: *const Font, cx: f32, ty: f32, col: Col, s: []const u8, color: Color, header: bool) void {
    const pad: f32 = 14;
    const tw = f.textWidth(s);
    const tx = switch (col.al) {
        .left => cx + pad,
        .right => cx + col.w - pad - tw,
        .center => cx + (col.w - tw) / 2,
    };
    if (header) {
        f.textTracked(g, tx, ty, s, color, 1.1);
    } else if (col.num) {
        f.textNum(g, tx, ty, s, color);
    } else {
        f.text(g, tx, ty, s, color);
    }
}
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
        return self.hSliderEx(id, x, y, w, h, value, lo, hi, false);
    }
    /// Bipolar slider — fill grows from the CENTER and a center detent tick is
    /// drawn (good for pan / balance). The thumb is balanced (round, centered).
    pub fn hSliderBipolar(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32, lo: f32, hi: f32) bool {
        return self.hSliderEx(id, x, y, w, h, value, lo, hi, true);
    }
    fn hSliderEx(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, value: *f32, lo: f32, hi: f32, bipolar: bool) bool {
        const tr = h * 0.5 + 4; // thumb radius (kept inside the track ends)
        const hov = self.inside(x - tr, y - tr, w + 2 * tr, h + 2 * tr);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        var changed = false;
        if (self.active == id and self.in.mouse_down) {
            const t = std.math.clamp((self.in.mx - (x + tr)) / @max(w - 2 * tr, 1), 0.0, 1.0);
            const nv = lo + t * (hi - lo);
            if (nv != value.*) {
                value.* = nv;
                changed = true;
            }
        }
        const a = self.anim(id);
        a.hover = ease(a.hover, if (hov or self.active == id) 1 else 0, self.dt, 14);
        const cy = y + h * 0.5;
        const t = (value.* - lo) / (hi - lo);
        const thx = x + tr + t * (w - 2 * tr); // thumb center — balanced
        const fill = lerp(Color.rgb(96, 120, 200), accent, a.hover);
        // track
        self.g.rect(x, y, w, h, h * 0.5, track_bg);
        if (bipolar) {
            const mid = x + w * 0.5;
            const from = @min(mid, thx);
            self.g.rect(from, y, @abs(thx - mid), h, h * 0.5, fill);
            self.g.rect(mid - 0.75, y - 2, 1.5, h + 4, 0, Color.rgba(255, 255, 255, 45)); // center detent
        } else {
            self.g.rect(x, y, thx - x, h, h * 0.5, fill);
        }
        // round thumb (the balanced dial), centered on the value
        const rr = tr + a.hover * 2;
        self.g.shadow(thx - rr, cy - rr + 2, 2 * rr, 2 * rr, rr, 4, Color.rgba(0, 0, 0, 130));
        self.g.card(thx - rr, cy - rr, 2 * rr, 2 * rr, rr, lerp(Color.rgb(244, 246, 250), Color.rgb(255, 255, 255), a.hover), Color.rgb(214, 219, 228), 0, border, 1.0);
        return changed;
    }

    /// Animated toggle switch — a pill track + a knob that SLIDES between off
    /// (left) and on (right) with eased motion; track lerps gray -> accent.
    pub fn toggle(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, on: *bool) bool {
        const hov = self.inside(x, y, w, h);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;
        if (clicked) on.* = !on.*;
        const a = self.anim(id);
        a.press = ease(a.press, if (on.*) 1 else 0, self.dt, 16); // slide position
        a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 14);
        const track = lerp(Color.rgb(54, 58, 70), accent, a.press);
        self.g.rect(x, y, w, h, h * 0.5, track);
        const kd = h - 6; // knob diameter
        const kx = x + 3 + a.press * (w - kd - 6);
        self.g.shadow(kx, y + 3 + 1, kd, kd, kd * 0.5, 3, Color.rgba(0, 0, 0, 120));
        self.g.card(kx, y + 3, kd, kd, kd * 0.5, Color.rgb(252, 253, 255), Color.rgb(228, 232, 240), 0, border, 1.0);
        return clicked;
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

    /// Checkbox with an animated check mark. Returns true the frame it's toggled.
    pub fn checkbox(self: *Ui, id: u32, x: f32, y: f32, s: f32, checked: *bool) bool {
        const hov = self.inside(x - 3, y - 3, s + 6, s + 6);
        if (hov) self.hot = id;
        if (hov and self.pressed) self.active = id;
        const clicked = self.active == id and self.released and hov;
        if (clicked) checked.* = !checked.*;
        const a = self.anim(id);
        a.press = ease(a.press, if (checked.*) 1 else 0, self.dt, 18);
        a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 14);
        const box = lerp(lerp(Color.rgb(44, 48, 60), Color.rgb(56, 61, 76), a.hover), accent, a.press);
        self.g.rect(x, y, s, s, s * 0.28, box);
        if (a.press > 0.05) { // check mark scales/fades in
            const p = a.press;
            const cx = x + s * 0.5;
            const cy = y + s * 0.5;
            self.g.line(cx - s * 0.22 * p, cy + s * 0.02 * p, cx - s * 0.04 * p, cy + s * 0.2 * p, 1.9, Color.rgb(255, 255, 255));
            self.g.line(cx - s * 0.04 * p, cy + s * 0.2 * p, cx + s * 0.26 * p, cy - s * 0.2 * p, 1.9, Color.rgb(255, 255, 255));
        }
        return clicked;
    }

    /// Segmented control / tab bar — a pill track with a SLIDING accent indicator
    /// behind the selected segment, and labels. Returns true when selection changes.
    pub fn segmented(self: *Ui, id: u32, x: f32, y: f32, w: f32, h: f32, sel: *usize, labels: []const []const u8, fb: *const Font) bool {
        const n = labels.len;
        if (n == 0) return false;
        self.g.rect(x, y, w, h, h * 0.5, track_bg);
        const segw = w / @as(f32, @floatFromInt(n));
        var changed = false;
        for (0..n) |i| {
            const sx = x + @as(f32, @floatFromInt(i)) * segw;
            if (self.inside(sx, y, segw, h)) {
                self.hot = id;
                if (self.pressed and sel.* != i) {
                    sel.* = i;
                    changed = true;
                }
            }
        }
        const a = self.anim(id);
        a.extra = ease(a.extra, @floatFromInt(sel.*), self.dt, 18); // sliding indicator
        const ix = x + a.extra * segw + 3;
        self.g.shadow(ix, y + 3 + 1, segw - 6, h - 6, (h - 6) * 0.5, 4, Color.rgba(0, 0, 0, 90));
        self.g.card(ix, y + 3, segw - 6, h - 6, (h - 6) * 0.5, accent, accent_lo, 0, border, 0.7);
        for (labels, 0..) |lbl, i| {
            const sx = x + @as(f32, @floatFromInt(i)) * segw;
            const tw = fb.textWidth(lbl);
            fb.text(self.g, sx + (segw - tw) / 2, y + (h - fb.cell_h) / 2, lbl, if (i == sel.*) Color.rgb(16, 18, 26) else c_off);
        }
        return changed;
    }

    /// Determinate progress bar (non-interactive). value 0..1.
    pub fn progress(self: *Ui, x: f32, y: f32, w: f32, h: f32, value: f32) void {
        self.g.rect(x, y, w, h, h * 0.5, track_bg);
        const fw = std.math.clamp(value, 0, 1) * w;
        if (fw > h) self.g.rectGrad(x, y, fw, h, h * 0.5, accent_hi, accent, 0, border);
    }

    /// Rendered data table — a rounded surface with a borderless tracked header,
    /// a separator hairline, zebra-striped body rows, hover highlight, and an
    /// optional selected row (accent wash + left bar). Numeric columns get
    /// tabular figures and right alignment so digits stay in line. `sel` may be
    /// null for a static (non-selectable) table. Returns the total height drawn.
    pub fn table(self: *Ui, id: u32, x: f32, y: f32, cols: []const Col, rows: []const []const []const u8, sel: ?*usize, fh: *const Font, fb: *const Font) f32 {
        var tw: f32 = 0;
        for (cols) |col| tw += col.w;
        const rowh: f32 = fb.cell_h + 13;
        const headh: f32 = fh.cell_h + 14;
        const total = headh + @as(f32, @floatFromInt(rows.len)) * rowh;
        // surface
        self.g.shadow(x, y + 4, tw, total, 12, 14, Color.rgba(0, 0, 0, 70));
        self.g.card(x, y, tw, total, 12, Color.rgb(34, 37, 47), Color.rgb(27, 30, 38), 0, border, 1.0);
        // header (borderless, tracked small-caps style) + hairline separator
        var hx = x;
        const hty = y + (headh - fh.cell_h) / 2;
        for (cols) |col| {
            tableCell(self.g, fh, hx, hty, col, col.title, Color.rgb(160, 168, 184), true);
            hx += col.w;
        }
        self.g.rect(x + 1, y + headh - 1, tw - 2, 1, 0, Color.rgba(255, 255, 255, 22));
        // body rows
        for (rows, 0..) |row, ri| {
            const ry = y + headh + @as(f32, @floatFromInt(ri)) * rowh;
            const hov = self.inside(x, ry, tw, rowh);
            if (hov) {
                self.hot = id;
                if (self.pressed) if (sel) |s| {
                    s.* = ri;
                };
            }
            const is_sel = if (sel) |s| s.* == ri else false;
            if (is_sel) {
                self.g.rect(x + 1, ry, tw - 2, rowh, 0, Color.rgba(108, 147, 244, 38));
                self.g.rect(x + 1, ry, 2.5, rowh, 0, accent);
            } else if (hov) {
                self.g.rect(x + 1, ry, tw - 2, rowh, 0, Color.rgba(255, 255, 255, 11));
            } else if (ri % 2 == 1) {
                self.g.rect(x + 1, ry, tw - 2, rowh, 0, Color.rgba(255, 255, 255, 6));
            }
            var cx = x;
            const ty = ry + (rowh - fb.cell_h) / 2;
            for (cols, 0..) |col, ci| {
                const cell = if (ci < row.len) row[ci] else "";
                const col_color = if (ci == 0) c_on else c_off;
                tableCell(self.g, fb, cx, ty, col, cell, col_color, false);
                cx += col.w;
            }
        }
        return total;
    }
};
