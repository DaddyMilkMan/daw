//! svg.zig — a compact SVG rasterizer. Parses the common SVG subset (the `<svg>`
//! viewBox; `<path>` with M/L/H/V/C/S/Q/T/Z; `<rect>`/`<circle>`/`<ellipse>`/
//! `<line>`/`<polyline>`/`<polygon>`; solid `fill`/`stroke` with opacity) and
//! scan-converts it (analytic-horizontal + 4× vertical AA, nonzero/even-odd) into
//! straight-alpha RGBA8 — the same `image.Image` the PNG path yields, so it drops
//! straight into gpu2d.GpuImage. Painter's-order compositing.
//!
//! Scope: solid fills + basic strokes. Elliptical arcs (A) are approximated as a
//! line to the endpoint; gradients/patterns/text are out of scope. Original Zig.

const std = @import("std");
const Image = @import("image.zig").Image;

pub const Error = error{ BadSvg, OutOfMemory };

const SS = 4; // vertical supersampling
const BEZ = 16; // bezier flattening segments

const Rgba = struct { r: u8, g: u8, b: u8, a: u8 };

const P = struct { x: f32, y: f32 };
const Edge = struct { x0: f32, y0: f32, x1: f32, y1: f32 };

// ---- tiny attribute / number helpers --------------------------------------
fn attr(el: []const u8, name: []const u8, buf: []u8) ?[]const u8 {
    // find `name="..."` (or name='...') within the element text
    var i: usize = 0;
    while (i + name.len + 2 < el.len) : (i += 1) {
        if (!std.mem.eql(u8, el[i .. i + name.len], name)) continue;
        // require a boundary before name and '=' after (with optional spaces)
        if (i > 0 and (std.ascii.isAlphanumeric(el[i - 1]) or el[i - 1] == '-' or el[i - 1] == ':')) continue;
        var j = i + name.len;
        while (j < el.len and el[j] == ' ') j += 1;
        if (j >= el.len or el[j] != '=') continue;
        j += 1;
        while (j < el.len and el[j] == ' ') j += 1;
        if (j >= el.len or (el[j] != '"' and el[j] != '\'')) continue;
        const q = el[j];
        j += 1;
        const start = j;
        while (j < el.len and el[j] != q) j += 1;
        const val = el[start..j];
        const n = @min(val.len, buf.len);
        @memcpy(buf[0..n], val[0..n]);
        return buf[0..n];
    }
    return null;
}

fn parseF(s: []const u8, idx: *usize) ?f32 {
    var i = idx.*;
    while (i < s.len and (s[i] == ' ' or s[i] == ',' or s[i] == '\n' or s[i] == '\r' or s[i] == '\t')) i += 1;
    const start = i;
    if (i < s.len and (s[i] == '+' or s[i] == '-')) i += 1;
    var any = false;
    while (i < s.len and std.ascii.isDigit(s[i])) {
        i += 1;
        any = true;
    }
    if (i < s.len and s[i] == '.') {
        i += 1;
        while (i < s.len and std.ascii.isDigit(s[i])) {
            i += 1;
            any = true;
        }
    }
    if (i < s.len and (s[i] == 'e' or s[i] == 'E')) {
        i += 1;
        if (i < s.len and (s[i] == '+' or s[i] == '-')) i += 1;
        while (i < s.len and std.ascii.isDigit(s[i])) i += 1;
    }
    if (!any) return null;
    idx.* = i;
    return std.fmt.parseFloat(f32, s[start..i]) catch null;
}

fn hex1(c: u8) u8 {
    return switch (c) {
        '0'...'9' => c - '0',
        'a'...'f' => c - 'a' + 10,
        'A'...'F' => c - 'A' + 10,
        else => 0,
    };
}

fn parseColor(s: []const u8) ?Rgba {
    if (s.len == 0) return null;
    if (std.mem.eql(u8, s, "none")) return null;
    if (std.mem.eql(u8, s, "transparent")) return null;
    if (s[0] == '#') {
        if (s.len >= 7) return .{ .r = hex1(s[1]) * 16 + hex1(s[2]), .g = hex1(s[3]) * 16 + hex1(s[4]), .b = hex1(s[5]) * 16 + hex1(s[6]), .a = 255 };
        if (s.len >= 4) return .{ .r = hex1(s[1]) * 17, .g = hex1(s[2]) * 17, .b = hex1(s[3]) * 17, .a = 255 };
        return null;
    }
    if (std.mem.startsWith(u8, s, "rgb")) {
        var i: usize = 0;
        while (i < s.len and s[i] != '(') i += 1;
        i += 1;
        const r = parseF(s, &i) orelse return null;
        const g = parseF(s, &i) orelse return null;
        const b = parseF(s, &i) orelse return null;
        const a = parseF(s, &i) orelse 1.0;
        return .{ .r = clamp8(r), .g = clamp8(g), .b = clamp8(b), .a = if (a <= 1.0) clamp8(a * 255) else clamp8(a) };
    }
    // a small named-color table (the ones icons actually use)
    const named = [_]struct { n: []const u8, c: Rgba }{
        .{ .n = "black", .c = .{ .r = 0, .g = 0, .b = 0, .a = 255 } },
        .{ .n = "white", .c = .{ .r = 255, .g = 255, .b = 255, .a = 255 } },
        .{ .n = "red", .c = .{ .r = 255, .g = 0, .b = 0, .a = 255 } },
        .{ .n = "green", .c = .{ .r = 0, .g = 128, .b = 0, .a = 255 } },
        .{ .n = "blue", .c = .{ .r = 0, .g = 0, .b = 255, .a = 255 } },
        .{ .n = "gray", .c = .{ .r = 128, .g = 128, .b = 128, .a = 255 } },
        .{ .n = "grey", .c = .{ .r = 128, .g = 128, .b = 128, .a = 255 } },
        .{ .n = "orange", .c = .{ .r = 255, .g = 165, .b = 0, .a = 255 } },
        .{ .n = "currentColor", .c = .{ .r = 230, .g = 233, .b = 240, .a = 255 } },
    };
    for (named) |nm| if (std.mem.eql(u8, s, nm.n)) return nm.c;
    return null;
}
fn clamp8(v: f32) u8 {
    return @intFromFloat(std.math.clamp(v, 0, 255));
}

// ---- rasterizer ------------------------------------------------------------
const Canvas = struct {
    px: []u8, // RGBA8
    w: usize,
    h: usize,
    rowcov: []f32,

    fn fill(self: *Canvas, edges: []const Edge, col: Rgba, evenodd: bool) void {
        const inv: f32 = 1.0 / @as(f32, SS);
        var py: usize = 0;
        while (py < self.h) : (py += 1) {
            @memset(self.rowcov, 0);
            var sub: usize = 0;
            while (sub < SS) : (sub += 1) {
                const sy = @as(f32, @floatFromInt(py)) + (@as(f32, @floatFromInt(sub)) + 0.5) * inv;
                var xs: [128]struct { x: f32, dir: i2 } = undefined;
                var n: usize = 0;
                for (edges) |e| {
                    const ylo = @min(e.y0, e.y1);
                    const yhi = @max(e.y0, e.y1);
                    if (sy < ylo or sy >= yhi or e.y0 == e.y1) continue;
                    const t = (sy - e.y0) / (e.y1 - e.y0);
                    if (n < xs.len) {
                        xs[n] = .{ .x = e.x0 + t * (e.x1 - e.x0), .dir = if (e.y1 > e.y0) 1 else -1 };
                        n += 1;
                    }
                }
                if (n < 2) continue;
                std.sort.insertion(@TypeOf(xs[0]), xs[0..n], {}, struct {
                    fn lt(_: void, a: @TypeOf(xs[0]), b: @TypeOf(xs[0])) bool {
                        return a.x < b.x;
                    }
                }.lt);
                var wind: i32 = 0;
                var c: usize = 0;
                while (c + 1 < n) : (c += 1) {
                    wind += xs[c].dir;
                    const inside = if (evenodd) (@mod(c + 1, 2) == 1) else (wind != 0);
                    if (inside) self.addSpan(xs[c].x, xs[c + 1].x, inv);
                }
            }
            // composite this row's coverage with the fill color (straight-alpha over)
            const ca = @as(f32, @floatFromInt(col.a)) / 255.0;
            const base = py * self.w * 4;
            var x: usize = 0;
            while (x < self.w) : (x += 1) {
                const cov = self.rowcov[x];
                if (cov <= 0) continue;
                const a = std.math.clamp(cov, 0, 1) * ca;
                const o = base + x * 4;
                self.px[o] = overC(self.px[o], col.r, a);
                self.px[o + 1] = overC(self.px[o + 1], col.g, a);
                self.px[o + 2] = overC(self.px[o + 2], col.b, a);
                const da = @as(f32, @floatFromInt(self.px[o + 3])) / 255.0;
                self.px[o + 3] = clamp8((a + da * (1 - a)) * 255.0);
            }
        }
    }
    fn addSpan(self: *Canvas, xl_in: f32, xr_in: f32, weight: f32) void {
        const wf: f32 = @floatFromInt(self.w);
        const xl = std.math.clamp(xl_in, 0, wf);
        const xr = std.math.clamp(xr_in, 0, wf);
        if (xr <= xl) return;
        const ixl: usize = @intFromFloat(@floor(xl));
        const ixr: usize = @intFromFloat(@floor(xr));
        if (ixl == ixr) {
            if (ixl < self.w) self.rowcov[ixl] += (xr - xl) * weight;
            return;
        }
        if (ixl < self.w) self.rowcov[ixl] += (@as(f32, @floatFromInt(ixl + 1)) - xl) * weight;
        var ix = ixl + 1;
        while (ix < ixr and ix < self.w) : (ix += 1) self.rowcov[ix] += weight;
        if (ixr < self.w) self.rowcov[ixr] += (xr - @as(f32, @floatFromInt(ixr))) * weight;
    }
};
fn overC(dst: u8, src: u8, a: f32) u8 {
    const d: f32 = @floatFromInt(dst);
    const s: f32 = @floatFromInt(src);
    return clamp8(s * a + d * (1 - a));
}

fn edgesFromPoly(poly: []const P, edges: *std.ArrayList(Edge)) !void {
    if (poly.len < 2) return;
    var i: usize = 0;
    while (i < poly.len) : (i += 1) {
        const p = poly[i];
        const q = poly[(i + 1) % poly.len];
        try edges.append(.{ .x0 = p.x, .y0 = p.y, .x1 = q.x, .y1 = q.y });
    }
}

// ---- path `d` flattening ---------------------------------------------------
const PathFlattener = struct {
    s: []const u8,
    i: usize = 0,
    sx: f32 = 0, // scale + translate (viewBox -> pixels)
    sy: f32 = 0,
    ox: f32 = 0,
    oy: f32 = 0,
    cx: f32 = 0, // current point (user units)
    cy: f32 = 0,
    startx: f32 = 0,
    starty: f32 = 0,
    px: f32 = 0, // last control reflection point
    py: f32 = 0,
    edges: *std.ArrayList(Edge),
    poly: *std.ArrayList(P),

    fn pt(self: *PathFlattener, ux: f32, uy: f32) P {
        return .{ .x = self.ox + ux * self.sx, .y = self.oy + uy * self.sy };
    }
    fn moveTo(self: *PathFlattener, x: f32, y: f32) !void {
        try self.closeSub();
        self.cx = x;
        self.cy = y;
        self.startx = x;
        self.starty = y;
        try self.poly.append(self.pt(x, y));
    }
    fn lineTo(self: *PathFlattener, x: f32, y: f32) !void {
        self.cx = x;
        self.cy = y;
        try self.poly.append(self.pt(x, y));
    }
    fn cubic(self: *PathFlattener, x1: f32, y1: f32, x2: f32, y2: f32, x: f32, y: f32) !void {
        const x0 = self.cx;
        const y0 = self.cy;
        var k: usize = 1;
        while (k <= BEZ) : (k += 1) {
            const t = @as(f32, @floatFromInt(k)) / BEZ;
            const mt = 1 - t;
            const a = mt * mt * mt;
            const b = 3 * mt * mt * t;
            const c = 3 * mt * t * t;
            const d = t * t * t;
            try self.poly.append(self.pt(a * x0 + b * x1 + c * x2 + d * x, a * y0 + b * y1 + c * y2 + d * y));
        }
        self.px = x2;
        self.py = y2;
        self.cx = x;
        self.cy = y;
    }
    fn quad(self: *PathFlattener, x1: f32, y1: f32, x: f32, y: f32) !void {
        const x0 = self.cx;
        const y0 = self.cy;
        var k: usize = 1;
        while (k <= BEZ) : (k += 1) {
            const t = @as(f32, @floatFromInt(k)) / BEZ;
            const mt = 1 - t;
            try self.poly.append(self.pt(mt * mt * x0 + 2 * mt * t * x1 + t * t * x, mt * mt * y0 + 2 * mt * t * y1 + t * t * y));
        }
        self.px = x1;
        self.py = y1;
        self.cx = x;
        self.cy = y;
    }
    fn closeSub(self: *PathFlattener) !void {
        if (self.poly.items.len >= 2) try edgesFromPoly(self.poly.items, self.edges);
        self.poly.clearRetainingCapacity();
    }

    fn run(self: *PathFlattener) !void {
        var cmd: u8 = 0;
        while (self.i < self.s.len) {
            // skip separators
            while (self.i < self.s.len and (self.s[self.i] == ' ' or self.s[self.i] == ',' or self.s[self.i] == '\n' or self.s[self.i] == '\r' or self.s[self.i] == '\t')) self.i += 1;
            if (self.i >= self.s.len) break;
            const ch = self.s[self.i];
            if (std.ascii.isAlphabetic(ch)) {
                cmd = ch;
                self.i += 1;
            }
            const rel = std.ascii.isLower(cmd);
            const C = std.ascii.toUpper(cmd);
            switch (C) {
                'M' => {
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    try self.moveTo(if (rel) self.cx + x else x, if (rel) self.cy + y else y);
                    cmd = if (rel) 'l' else 'L'; // subsequent pairs are lineTo
                },
                'L' => {
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    try self.lineTo(if (rel) self.cx + x else x, if (rel) self.cy + y else y);
                },
                'H' => {
                    const x = (parseF(self.s, &self.i)) orelse break;
                    try self.lineTo(if (rel) self.cx + x else x, self.cy);
                },
                'V' => {
                    const y = (parseF(self.s, &self.i)) orelse break;
                    try self.lineTo(self.cx, if (rel) self.cy + y else y);
                },
                'C' => {
                    const x1 = (parseF(self.s, &self.i)) orelse break;
                    const y1 = (parseF(self.s, &self.i)) orelse break;
                    const x2 = (parseF(self.s, &self.i)) orelse break;
                    const y2 = (parseF(self.s, &self.i)) orelse break;
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    if (rel) {
                        try self.cubic(self.cx + x1, self.cy + y1, self.cx + x2, self.cy + y2, self.cx + x, self.cy + y);
                    } else try self.cubic(x1, y1, x2, y2, x, y);
                },
                'S' => {
                    const x2 = (parseF(self.s, &self.i)) orelse break;
                    const y2 = (parseF(self.s, &self.i)) orelse break;
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    const rx = 2 * self.cx - self.px;
                    const ry = 2 * self.cy - self.py;
                    if (rel) {
                        try self.cubic(rx, ry, self.cx + x2, self.cy + y2, self.cx + x, self.cy + y);
                    } else try self.cubic(rx, ry, x2, y2, x, y);
                },
                'Q' => {
                    const x1 = (parseF(self.s, &self.i)) orelse break;
                    const y1 = (parseF(self.s, &self.i)) orelse break;
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    if (rel) {
                        try self.quad(self.cx + x1, self.cy + y1, self.cx + x, self.cy + y);
                    } else try self.quad(x1, y1, x, y);
                },
                'T' => {
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    const rx = 2 * self.cx - self.px;
                    const ry = 2 * self.cy - self.py;
                    if (rel) {
                        try self.quad(rx, ry, self.cx + x, self.cy + y);
                    } else try self.quad(rx, ry, x, y);
                },
                'A' => { // arc — approximate as a line to the endpoint (params skipped)
                    _ = parseF(self.s, &self.i);
                    _ = parseF(self.s, &self.i);
                    _ = parseF(self.s, &self.i);
                    _ = parseF(self.s, &self.i);
                    _ = parseF(self.s, &self.i);
                    const x = (parseF(self.s, &self.i)) orelse break;
                    const y = (parseF(self.s, &self.i)) orelse break;
                    try self.lineTo(if (rel) self.cx + x else x, if (rel) self.cy + y else y);
                },
                'Z' => {
                    try self.lineTo(self.startx, self.starty);
                    try self.closeSub();
                },
                else => self.i += 1, // unknown — skip a char to avoid a stall
            }
        }
        try self.closeSub();
    }
};

/// Rasterize SVG bytes into an `Image` of `tw`×`th` pixels (uniform fit to the
/// viewBox). Caller deinits the returned image.
pub fn rasterize(a: std.mem.Allocator, svg: []const u8, tw: usize, th: usize) !Image {
    // viewBox (preferred) or width/height
    var nbuf: [256]u8 = undefined;
    var vbx: f32 = 0;
    var vby: f32 = 0;
    var vbw: f32 = 0;
    var vbh: f32 = 0;
    if (std.mem.indexOf(u8, svg, "<svg")) |si| {
        const head = svg[si..@min(svg.len, si + 400)];
        if (attr(head, "viewBox", &nbuf)) |vb| {
            var idx: usize = 0;
            vbx = parseF(vb, &idx) orelse 0;
            vby = parseF(vb, &idx) orelse 0;
            vbw = parseF(vb, &idx) orelse 0;
            vbh = parseF(vb, &idx) orelse 0;
        }
        if (vbw == 0) {
            if (attr(head, "width", &nbuf)) |w| {
                var idx: usize = 0;
                vbw = parseF(w, &idx) orelse 0;
            }
            if (attr(head, "height", &nbuf)) |h| {
                var idx: usize = 0;
                vbh = parseF(h, &idx) orelse 0;
            }
        }
    }
    if (vbw == 0 or vbh == 0) return Error.BadSvg;

    const scale = @min(@as(f32, @floatFromInt(tw)) / vbw, @as(f32, @floatFromInt(th)) / vbh);
    const ox = -vbx * scale + (@as(f32, @floatFromInt(tw)) - vbw * scale) / 2;
    const oy = -vby * scale + (@as(f32, @floatFromInt(th)) - vbh * scale) / 2;

    const pixels = try a.alloc(u8, tw * th * 4);
    @memset(pixels, 0);
    const rowcov = try a.alloc(f32, tw);
    defer a.free(rowcov);
    var canvas = Canvas{ .px = pixels, .w = tw, .h = th, .rowcov = rowcov };

    var edges = std.ArrayList(Edge).init(a);
    defer edges.deinit();
    var poly = std.ArrayList(P).init(a);
    defer poly.deinit();

    // walk elements in document order (painter's algorithm)
    var pos: usize = 0;
    while (std.mem.indexOfScalarPos(u8, svg, pos, '<')) |lt| {
        const gt = std.mem.indexOfScalarPos(u8, svg, lt, '>') orelse break;
        const el = svg[lt + 1 .. gt];
        pos = gt + 1;
        if (el.len == 0 or el[0] == '/' or el[0] == '!' or el[0] == '?') continue;

        var cbuf: [64]u8 = undefined;
        const fill = if (attr(el, "fill", &cbuf)) |f| parseColor(f) else Rgba{ .r = 0, .g = 0, .b = 0, .a = 255 };
        var fcol = fill;
        var fbuf: [32]u8 = undefined;
        if (attr(el, "fill-opacity", &fbuf)) |fo| {
            var idx: usize = 0;
            if (parseF(fo, &idx)) |v| if (fcol) |*c| {
                c.a = clamp8(@as(f32, @floatFromInt(c.a)) * v);
            };
        }

        edges.clearRetainingCapacity();
        poly.clearRetainingCapacity();

        if (std.mem.startsWith(u8, el, "path")) {
            var dbuf: [8192]u8 = undefined;
            if (attr(el, "d", &dbuf)) |d| {
                var pf = PathFlattener{ .s = d, .sx = scale, .sy = scale, .ox = ox, .oy = oy, .edges = &edges, .poly = &poly };
                pf.run() catch {};
            }
        } else if (std.mem.startsWith(u8, el, "rect")) {
            const x = num(el, "x", &nbuf);
            const y = num(el, "y", &nbuf);
            const w = num(el, "width", &nbuf);
            const h = num(el, "height", &nbuf);
            var rx = num(el, "rx", &nbuf);
            var ry = num(el, "ry", &nbuf);
            if (rx == 0) rx = ry;
            if (ry == 0) ry = rx;
            rx = @min(rx, w / 2);
            ry = @min(ry, h / 2);
            if (rx > 0 and ry > 0) {
                // rounded rect — four corner arcs
                const corners = [_][3]f32{ // cx, cy, start-angle
                    .{ x + w - rx, y + ry, -std.math.pi / 2.0 }, // top-right
                    .{ x + w - rx, y + h - ry, 0 }, // bottom-right
                    .{ x + rx, y + h - ry, std.math.pi / 2.0 }, // bottom-left
                    .{ x + rx, y + ry, std.math.pi }, // top-left
                };
                for (corners) |cc| {
                    var k: usize = 0;
                    const seg = 10;
                    while (k <= seg) : (k += 1) {
                        const ang = cc[2] + @as(f32, @floatFromInt(k)) / seg * (std.math.pi / 2.0);
                        try poly.append(.{ .x = ox + (cc[0] + @cos(ang) * rx) * scale, .y = oy + (cc[1] + @sin(ang) * ry) * scale });
                    }
                }
                try edgesFromPoly(poly.items, &edges);
            } else {
                const quad = [_]P{
                    .{ .x = ox + x * scale, .y = oy + y * scale },
                    .{ .x = ox + (x + w) * scale, .y = oy + y * scale },
                    .{ .x = ox + (x + w) * scale, .y = oy + (y + h) * scale },
                    .{ .x = ox + x * scale, .y = oy + (y + h) * scale },
                };
                try edgesFromPoly(&quad, &edges);
            }
        } else if (std.mem.startsWith(u8, el, "circle") or std.mem.startsWith(u8, el, "ellipse")) {
            const cx = num(el, "cx", &nbuf);
            const cy = num(el, "cy", &nbuf);
            const r = num(el, "r", &nbuf);
            const rx = if (r != 0) r else num(el, "rx", &nbuf);
            const ry = if (r != 0) r else num(el, "ry", &nbuf);
            var k: usize = 0;
            const seg = 64;
            while (k < seg) : (k += 1) {
                const ang = @as(f32, @floatFromInt(k)) / seg * std.math.tau;
                try poly.append(.{ .x = ox + (cx + @cos(ang) * rx) * scale, .y = oy + (cy + @sin(ang) * ry) * scale });
            }
            try edgesFromPoly(poly.items, &edges);
        } else if (std.mem.startsWith(u8, el, "polygon") or std.mem.startsWith(u8, el, "polyline")) {
            var pbuf: [4096]u8 = undefined;
            if (attr(el, "points", &pbuf)) |pts| {
                var idx: usize = 0;
                while (idx < pts.len) {
                    const x = parseF(pts, &idx) orelse break;
                    const y = parseF(pts, &idx) orelse break;
                    try poly.append(.{ .x = ox + x * scale, .y = oy + y * scale });
                }
                try edgesFromPoly(poly.items, &edges);
            }
        } else continue;

        if (fcol) |col| if (col.a > 0 and edges.items.len > 0) canvas.fill(edges.items, col, false);
    }

    return .{ .w = tw, .h = th, .pixels = pixels, .allocator = a };
}

fn num(el: []const u8, name: []const u8, buf: []u8) f32 {
    if (attr(el, name, buf)) |v| {
        var idx: usize = 0;
        return parseF(v, &idx) orelse 0;
    }
    return 0;
}

test "rasterize a small svg" {
    const a = std.testing.allocator;
    const doc =
        \\<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
        \\<rect x="10" y="10" width="80" height="80" fill="#3a6cf4"/>
        \\<circle cx="50" cy="50" r="24" fill="#ffffff"/>
        \\<path d="M20 80 L50 30 L80 80 Z" fill="rgba(255,80,80,0.6)"/>
        \\</svg>
    ;
    var img = try rasterize(a, doc, 64, 64);
    defer img.deinit();
    try std.testing.expectEqual(@as(usize, 64), img.w);
    // center pixel should be inked (white circle over blue rect)
    const c = (32 * 64 + 32) * 4;
    try std.testing.expect(img.pixels[c + 3] > 200);
    // a corner outside the rect should be transparent
    try std.testing.expectEqual(@as(u8, 0), img.pixels[3]);
}
