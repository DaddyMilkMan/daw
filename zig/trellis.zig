//! trellis.zig — the Trellis layout engine (CSS-flexbox-style, declarative) over gpu2d.
//!
//! WHY: hand-computing pixel coordinates (cy += 22, x + w/2 - 18) is what makes
//! custom UIs drift and look "off", and what makes UI hard to author correctly.
//! Web UI is reliable because layout is DECLARATIVE: you state structure (a row
//! with a gap, a child that grows) and the engine computes exact positions.
//! This brings that model to Zenith — so positions are pixel-accurate and UI is
//! easy for a human or an AI to write the way you'd write HTML/CSS.
//!
//! Usage (reads like flexbox):
//!   ui.begin(W, H, input, dt);
//!   ui.open(.{ .dir = .col, .w = grow(), .h = grow(), .pad = 16, .gap = 12 });
//!     ui.box(.{ .h = px(48), .w = grow(), .bg = ..., .radius = 10 });   // a bar
//!     ui.open(.{ .dir = .row, .gap = 8, .h = px(40) });
//!       _ = ui.button("Play", 1, .{ .w = px(90) });
//!       _ = ui.button("Stop", 2, .{ .w = px(90) });
//!     ui.close();
//!   ui.close();
//!   ui.end();   // computes layout + renders

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
pub const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;

// ---- design tokens (Geist-style 4px grid) -----------------------------------
// Use these named tokens for spacing/radii instead of ad-hoc pixel values, so
// density stays consistent (the thing that makes modern UIs feel coherent).
pub const sp = struct {
    pub const @"1": f32 = 4;
    pub const @"2": f32 = 8;
    pub const @"3": f32 = 12;
    pub const @"4": f32 = 16;
    pub const @"5": f32 = 20;
    pub const @"6": f32 = 24;
    pub const @"8": f32 = 32;
    pub const @"10": f32 = 40;
};
pub const radius = struct {
    pub const xs: f32 = 4;
    pub const sm: f32 = 6;
    pub const md: f32 = 8;
    pub const lg: f32 = 10;
    pub const xl: f32 = 14;
    pub const pill: f32 = 9999;
};
// ---- easing (motion) — modern UIs animate state changes on a short ease-out --
pub fn easeOutCubic(t: f32) f32 {
    const u = 1.0 - std.math.clamp(t, 0.0, 1.0);
    return 1.0 - u * u * u;
}
pub fn easeInOut(t: f32) f32 {
    const x = std.math.clamp(t, 0.0, 1.0);
    return if (x < 0.5) 4.0 * x * x * x else 1.0 - std.math.pow(f32, -2.0 * x + 2.0, 3.0) / 2.0;
}
/// Frame-rate-independent critically-damped approach toward `target` (a smooth
/// spring without overshoot). `speed` ~10–20 = snappy-but-soft. Use for hover/
/// press/selection so transitions read as eased, not linear.
pub fn approach(cur: f32, target: f32, dt: f32, speed: f32) f32 {
    return cur + (target - cur) * (1.0 - std.math.exp(-speed * dt));
}

// ---- sizing (like CSS: fixed px, flex-grow, percent, fit-content) -----------
pub const Size = union(enum) { fix: f32, grow: f32, pct: f32, fit: void };
pub fn px(v: f32) Size {
    return .{ .fix = v };
}
pub fn grow() Size {
    return .{ .grow = 1 };
}
pub fn groww(w: f32) Size {
    return .{ .grow = w };
}
pub fn pct(v: f32) Size {
    return .{ .pct = v };
}
pub const fit: Size = .fit;
fn isFit(s: Size) bool {
    return switch (s) {
        .fit => true,
        else => false,
    };
}

pub const Dir = enum { row, col };
pub const Justify = enum { start, center, end, between };
pub const AlignI = enum { start, center, end, stretch };

pub const Style = struct {
    dir: Dir = .row,
    w: Size = fit,
    h: Size = fit,
    pad: f32 = 0,
    gap: f32 = 0,
    justify: Justify = .start,
    aligni: AlignI = .start,
    // visuals
    radius: f32 = 0,
    bg: ?Color = null,
    bg2: ?Color = null, // gradient bottom (defaults to bg)
    border: ?Color = null,
    border_w: f32 = 1,
    elev: f32 = 0, // material depth on the bg
    shadow: f32 = 0, // shadow sigma (0 = none)
    tracking: f32 = 0, // letter-spacing for text nodes
    tabular: bool = false, // render numbers with fixed-width (tabular) figures
    // interaction
    id: u64 = 0, // nonzero = interactive (hover/press/click)
    hover_bg: ?Color = null, // bg lerps toward this on hover
};

const Node = struct {
    style: Style = .{},
    parent: i32 = -1,
    first: i32 = -1,
    last: i32 = -1,
    next: i32 = -1,
    x: f32 = 0,
    y: f32 = 0,
    w: f32 = 0,
    h: f32 = 0,
    mw: f32 = 0, // measured intrinsic content size
    mh: f32 = 0,
    text: ?[]const u8 = null,
    font: ?*const Font = null,
    text_col: Color = Color.rgb(236, 239, 246),
};

const Anim = struct { id: u64 = 0, used: bool = false, hover: f32 = 0, press: f32 = 0 };

fn lerp(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}
fn ease(cur: f32, target: f32, dt: f32, speed: f32) f32 {
    return approach(cur, target, dt, speed); // smooth, frame-rate-independent
}

pub const Ctx = struct {
    g: *Gpu,
    nodes: [2048]Node = undefined,
    n: u32 = 0,
    stack: [64]i32 = undefined,
    sp: u32 = 0,
    W: f32 = 0,
    H: f32 = 0,
    ox: f32 = 0, // origin — lay the tree out anywhere (sub-regions, overlays)
    oy: f32 = 0,
    mx: f32 = -1,
    my: f32 = -1,
    mdown: bool = false,
    prevdown: bool = false,
    pressed: bool = false,
    released: bool = false,
    dt: f32 = 0.016,
    click: u64 = 0,
    anims: [512]Anim = [_]Anim{.{}} ** 512,
    // default fonts for label/button text
    fb: *const Font,
    fu: *const Font,
    fd: *const Font,

    pub fn init(g: *Gpu, fb: *const Font, fu: *const Font, fd: *const Font) Ctx {
        return .{ .g = g, .fb = fb, .fu = fu, .fd = fd };
    }

    pub fn begin(self: *Ctx, w: f32, h: f32, mx: f32, my: f32, mdown: bool, dt: f32) void {
        self.beginAt(0, 0, w, h, mx, my, mdown, dt);
    }
    /// Begin a layout whose root is placed at (ox,oy) — for sub-regions/overlays.
    pub fn beginAt(self: *Ctx, ox: f32, oy: f32, w: f32, h: f32, mx: f32, my: f32, mdown: bool, dt: f32) void {
        self.pressed = mdown and !self.prevdown;
        self.released = !mdown and self.prevdown;
        self.ox = ox;
        self.oy = oy;
        self.W = w;
        self.H = h;
        self.mx = mx;
        self.my = my;
        self.mdown = mdown;
        self.dt = dt;
        self.n = 0;
        self.sp = 0;
        self.click = 0;
        // root fills the window
        self.open(.{ .w = px(w), .h = px(h), .dir = .col });
    }

    fn add(self: *Ctx, style: Style) i32 {
        const i: i32 = @intCast(self.n);
        self.nodes[self.n] = .{ .style = style };
        self.n += 1;
        if (self.sp > 0) {
            const p = self.stack[self.sp - 1];
            self.nodes[@intCast(i)].parent = p;
            const pn = &self.nodes[@intCast(p)];
            if (pn.first < 0) {
                pn.first = i;
                pn.last = i;
            } else {
                self.nodes[@intCast(pn.last)].next = i;
                pn.last = i;
            }
        }
        return i;
    }
    /// Open a container; children declared until close().
    pub fn open(self: *Ctx, style: Style) void {
        const i = self.add(style);
        self.stack[self.sp] = i;
        self.sp += 1;
    }
    pub fn close(self: *Ctx) void {
        if (self.sp > 0) self.sp -= 1;
    }
    /// A leaf box (rectangle/spacer).
    pub fn box(self: *Ctx, style: Style) void {
        _ = self.add(style);
    }
    /// A text leaf. Sizes to the text.
    pub fn label(self: *Ctx, s: []const u8, font: *const Font, col: Color, style: Style) void {
        const i = self.add(style);
        const nn = &self.nodes[@intCast(i)];
        nn.text = s;
        nn.font = font;
        nn.text_col = col;
    }
    /// A clickable button with centered label. Returns true the frame it's clicked.
    pub fn button(self: *Ctx, text: []const u8, id: u64, style: Style) bool {
        var st = style;
        st.id = id;
        // FLAT, modern button. The old default (elev=1 material gloss + a bright
        // white rim border) read as a glossy '90s bevel — removed from the toolkit.
        // Now: a clean near-solid fill (tiny top->bottom delta), subtle hover, no
        // rim, no specular. Depth comes from hover + an optional caller shadow.
        if (st.bg == null) st.bg = Color.rgb(46, 50, 62);
        if (st.bg2 == null) st.bg2 = Color.rgb(41, 45, 56);
        if (st.hover_bg == null) st.hover_bg = Color.rgb(57, 62, 77);
        if (st.border == null) st.border = Color.rgba(255, 255, 255, 0); // no rim
        if (st.radius == 0) st.radius = 7;
        st.elev = 0; // no material gloss
        st.justify = .center;
        st.aligni = .center;
        self.open(st);
        self.label(text, self.fb, Color.rgb(236, 239, 246), .{});
        self.close();
        return self.click == id;
    }

    fn anim(self: *Ctx, id: u64) *Anim {
        for (&self.anims) |*a| if (a.used and a.id == id) return a;
        for (&self.anims) |*a| if (!a.used) {
            a.* = .{ .id = id, .used = true };
            return a;
        };
        return &self.anims[0];
    }

    pub fn end(self: *Ctx) void {
        self.close(); // root
        self.measure(0);
        self.arrange(0, self.ox, self.oy, self.W, self.H);
        self.render(0);
        self.prevdown = self.mdown;
    }

    fn measure(self: *Ctx, i: i32) void {
        const nn = &self.nodes[@intCast(i)];
        var c = nn.first;
        while (c >= 0) : (c = self.nodes[@intCast(c)].next) self.measure(c);
        if (nn.text) |t| {
            if (nn.font) |f| {
                nn.mw = f.textWidth(t) + nn.style.tracking * @as(f32, @floatFromInt(t.len));
                nn.mh = f.cell_h;
            }
            return;
        }
        const row = nn.style.dir == .row;
        var main: f32 = 0;
        var cross: f32 = 0;
        var cnt: u32 = 0;
        c = nn.first;
        while (c >= 0) : (c = self.nodes[@intCast(c)].next) {
            const cn = &self.nodes[@intCast(c)];
            const cm = if (row) cn.mw else cn.mh; // intrinsic main
            const cc = if (row) cn.mh else cn.mw; // intrinsic cross
            const sMain = if (row) cn.style.w else cn.style.h;
            const sCross = if (row) cn.style.h else cn.style.w;
            main += switch (sMain) {
                .fix => |v| v,
                else => cm,
            };
            const ccr = switch (sCross) {
                .fix => |v| v,
                else => cc,
            };
            if (ccr > cross) cross = ccr;
            cnt += 1;
        }
        if (cnt > 0) main += nn.style.gap * @as(f32, @floatFromInt(cnt - 1));
        main += nn.style.pad * 2;
        cross += nn.style.pad * 2;
        nn.mw = if (row) main else cross;
        nn.mh = if (row) cross else main;
    }

    fn arrange(self: *Ctx, i: i32, x: f32, y: f32, w: f32, h: f32) void {
        const nn = &self.nodes[@intCast(i)];
        nn.x = x;
        nn.y = y;
        nn.w = w;
        nn.h = h;
        const row = nn.style.dir == .row;
        const pad = nn.style.pad;
        const inx = x + pad;
        const iny = y + pad;
        const inw = w - 2 * pad;
        const inh = h - 2 * pad;
        const innerMain = if (row) inw else inh;
        const innerCross = if (row) inh else inw;

        var cnt: u32 = 0;
        var fixedMain: f32 = 0;
        var growSum: f32 = 0;
        var c = nn.first;
        while (c >= 0) : (c = self.nodes[@intCast(c)].next) {
            const cn = &self.nodes[@intCast(c)];
            const sMain = if (row) cn.style.w else cn.style.h;
            switch (sMain) {
                .grow => |g| growSum += g,
                .pct => |p| fixedMain += p / 100.0 * innerMain,
                .fix => |v| fixedMain += v,
                .fit => fixedMain += if (row) cn.mw else cn.mh,
            }
            cnt += 1;
        }
        const gaps = if (cnt > 0) nn.style.gap * @as(f32, @floatFromInt(cnt - 1)) else 0;
        var avail = innerMain - fixedMain - gaps;
        if (avail < 0) avail = 0;

        var cursor = if (row) inx else iny;
        var spacing = nn.style.gap;
        if (growSum == 0) {
            const extra = innerMain - fixedMain - gaps;
            switch (nn.style.justify) {
                .start => {},
                .center => cursor += extra / 2,
                .end => cursor += extra,
                .between => if (cnt > 1) {
                    spacing += extra / @as(f32, @floatFromInt(cnt - 1));
                },
            }
        }

        c = nn.first;
        while (c >= 0) : (c = self.nodes[@intCast(c)].next) {
            const cn = &self.nodes[@intCast(c)];
            const sMain = if (row) cn.style.w else cn.style.h;
            const sCross = if (row) cn.style.h else cn.style.w;
            const cmain: f32 = switch (sMain) {
                .fix => |v| v,
                .fit => if (row) cn.mw else cn.mh,
                .pct => |p| p / 100.0 * innerMain,
                .grow => |g| if (growSum > 0) avail * g / growSum else 0,
            };
            var ccross: f32 = switch (sCross) {
                .fix => |v| v,
                .fit => if (row) cn.mh else cn.mw,
                .pct => |p| p / 100.0 * innerCross,
                .grow => innerCross,
            };
            if (nn.style.aligni == .stretch and isFit(sCross)) ccross = innerCross;
            if (ccross > innerCross) ccross = innerCross;
            var crossOff: f32 = 0;
            switch (nn.style.aligni) {
                .start, .stretch => {},
                .center => crossOff = (innerCross - ccross) / 2,
                .end => crossOff = innerCross - ccross,
            }
            if (row) {
                self.arrange(c, cursor, iny + crossOff, cmain, ccross);
                cursor += cmain + spacing;
            } else {
                self.arrange(c, inx + crossOff, cursor, ccross, cmain);
                cursor += cmain + spacing;
            }
        }
    }

    fn render(self: *Ctx, i: i32) void {
        const nn = &self.nodes[@intCast(i)];
        const s = nn.style;
        var bg = s.bg;
        if (s.id != 0) {
            @import("uireg.zig").put(s.id, nn.x, nn.y, nn.w, nn.h); // publish for by-id automation
            const hov = self.mx >= nn.x and self.mx < nn.x + nn.w and self.my >= nn.y and self.my < nn.y + nn.h;
            const a = self.anim(s.id);
            a.hover = ease(a.hover, if (hov) 1 else 0, self.dt, 16);
            a.press = ease(a.press, if (hov and self.mdown) 1 else 0, self.dt, 26);
            if (hov and self.released) self.click = s.id;
            if (s.hover_bg) |hb| {
                if (bg) |b| bg = lerp(b, hb, a.hover);
            }
            // subtle press feedback: darken a touch
            if (bg) |b| bg = lerp(b, Color.rgb(12, 14, 18), a.press * 0.12);
        }
        if (s.shadow > 0) {
            // layered directional drop shadow (light from above): a soft wide
            // ambient offset down + a tighter contact. Low opacity = modern.
            self.g.shadow(nn.x, nn.y + s.shadow * 0.55, nn.w, nn.h, s.radius, s.shadow * 1.35, Color.rgba(0, 0, 0, 52));
            self.g.shadow(nn.x, nn.y + 3, nn.w, nn.h, s.radius, s.shadow * 0.42, Color.rgba(0, 0, 0, 70));
        }
        if (bg) |b| {
            const b2 = s.bg2 orelse b;
            const bw: f32 = if (s.border != null and s.border.?.a > 0) s.border_w else 0;
            const bc = s.border orelse b;
            self.g.card(nn.x, nn.y, nn.w, nn.h, s.radius, b, b2, bw, bc, s.elev);
        } else if (s.border) |bd| {
            self.g.stroke(nn.x, nn.y, nn.w, nn.h, s.radius, s.border_w, bd);
        }
        if (nn.text) |t| {
            if (nn.font) |f| {
                if (s.tabular) f.textNum(self.g, nn.x, nn.y, t, nn.text_col) else f.textTracked(self.g, nn.x, nn.y, t, nn.text_col, s.tracking);
            }
        }
        var c = nn.first;
        while (c >= 0) : (c = self.nodes[@intCast(c)].next) self.render(c);
    }

    /// Rect of an interactive node from this frame (after end()), for overlays.
    pub fn rectOf(self: *Ctx, id: u64) ?[4]f32 {
        var i: u32 = 0;
        while (i < self.n) : (i += 1) {
            if (self.nodes[i].style.id == id) return .{ self.nodes[i].x, self.nodes[i].y, self.nodes[i].w, self.nodes[i].h };
        }
        return null;
    }
};
