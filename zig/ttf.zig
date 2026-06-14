//! ttf.zig — a runtime TrueType rasterizer. Parses a `.ttf` (glyf outlines),
//! flattens the quadratic-bezier contours, and scan-converts them with analytic
//! horizontal coverage + 4× vertical supersampling into a grayscale `font.Font`
//! — the exact struct the GPU atlas (gpu2d.GpuFont) already consumes.
//!
//! This is what turns "ship a handful of baked fonts" into "load any of the
//! hundreds/thousands of fonts installed on the machine" at runtime. ASCII
//! 32..126, TrueType (glyf) outlines, cmap formats 0/4/6/12. CFF/OpenType-PS
//! (.otf) outlines are a separate path and return error.UnsupportedOutline.
//!
//! Original implementation.

const std = @import("std");
const Font = @import("font.zig").Font;

pub const Error = error{ BadFont, UnsupportedOutline, NoCmap, OutOfMemory };

const STEPS = 6; // bezier flattening segments
const SS = 4; // vertical supersampling

// ---- big-endian reads ------------------------------------------------------
fn rdU16(d: []const u8, o: usize) u16 {
    return (@as(u16, d[o]) << 8) | d[o + 1];
}
fn rdI16(d: []const u8, o: usize) i16 {
    return @bitCast(rdU16(d, o));
}
fn rdU32(d: []const u8, o: usize) u32 {
    return (@as(u32, d[o]) << 24) | (@as(u32, d[o + 1]) << 16) | (@as(u32, d[o + 2]) << 8) | d[o + 3];
}

const Tables = struct {
    head: usize = 0,
    maxp: usize = 0,
    hhea: usize = 0,
    hmtx: usize = 0,
    loca: usize = 0,
    glyf: usize = 0,
    cmap: usize = 0,
};

fn findTables(d: []const u8) !Tables {
    if (d.len < 12) return Error.BadFont;
    const sfnt = rdU32(d, 0);
    if (sfnt == 0x4F54544F) return Error.UnsupportedOutline; // 'OTTO' = CFF outlines
    if (sfnt != 0x00010000 and sfnt != 0x74727565) return Error.BadFont; // 1.0 or 'true'
    const num = rdU16(d, 4);
    var t = Tables{};
    var i: usize = 0;
    while (i < num) : (i += 1) {
        const rec = 12 + i * 16;
        if (rec + 16 > d.len) return Error.BadFont;
        const tag = d[rec .. rec + 4];
        const off = rdU32(d, rec + 8);
        if (std.mem.eql(u8, tag, "head")) t.head = off;
        if (std.mem.eql(u8, tag, "maxp")) t.maxp = off;
        if (std.mem.eql(u8, tag, "hhea")) t.hhea = off;
        if (std.mem.eql(u8, tag, "hmtx")) t.hmtx = off;
        if (std.mem.eql(u8, tag, "loca")) t.loca = off;
        if (std.mem.eql(u8, tag, "glyf")) t.glyf = off;
        if (std.mem.eql(u8, tag, "cmap")) t.cmap = off;
    }
    if (t.head == 0 or t.glyf == 0 or t.loca == 0 or t.cmap == 0 or t.hmtx == 0 or t.hhea == 0) return Error.BadFont;
    return t;
}

// ---- cmap: char -> glyph index --------------------------------------------
const Cmap = struct {
    sub: usize, // offset of the chosen subtable
    fn lookup(self: Cmap, d: []const u8, ch: u21) u16 {
        const o = self.sub;
        const fmt = rdU16(d, o);
        return switch (fmt) {
            0 => if (ch < 256 and o + 6 + ch < d.len) d[o + 6 + ch] else 0,
            6 => blk: {
                const first = rdU16(d, o + 6);
                const count = rdU16(d, o + 8);
                if (ch < first or ch >= first + count) break :blk 0;
                break :blk rdU16(d, o + 10 + (@as(usize, @intCast(ch)) - first) * 2);
            },
            4 => self.lookup4(d, ch),
            12 => self.lookup12(d, ch),
            else => 0,
        };
    }
    fn lookup4(self: Cmap, d: []const u8, ch: u21) u16 {
        if (ch > 0xFFFF) return 0;
        const o = self.sub;
        const segX2 = rdU16(d, o + 6);
        const segCount = segX2 / 2;
        const endO = o + 14;
        const startO = endO + segX2 + 2; // skip endCodes + reservedPad
        const deltaO = startO + segX2;
        const rangeO = deltaO + segX2;
        var i: usize = 0;
        while (i < segCount) : (i += 1) {
            const end = rdU16(d, endO + i * 2);
            if (ch <= end) {
                const start = rdU16(d, startO + i * 2);
                if (ch < start) return 0;
                const delta = rdU16(d, deltaO + i * 2);
                const ro = rdU16(d, rangeO + i * 2);
                if (ro == 0) return @truncate(ch +% delta);
                const gi_addr = rangeO + i * 2 + ro + (@as(usize, @intCast(ch)) - start) * 2;
                if (gi_addr + 1 >= d.len) return 0;
                const g = rdU16(d, gi_addr);
                if (g == 0) return 0;
                return @truncate(g +% delta);
            }
        }
        return 0;
    }
    fn lookup12(self: Cmap, d: []const u8, ch: u21) u16 {
        const o = self.sub;
        const ngroups = rdU32(d, o + 12);
        var i: usize = 0;
        while (i < ngroups) : (i += 1) {
            const g = o + 16 + i * 12;
            const startc = rdU32(d, g);
            const endc = rdU32(d, g + 4);
            if (ch >= startc and ch <= endc) {
                const startg = rdU32(d, g + 8);
                return @truncate(startg + (@as(u32, @intCast(ch)) - startc));
            }
        }
        return 0;
    }
};

fn chooseCmap(d: []const u8, cmap_off: usize) !Cmap {
    const ntab = rdU16(d, cmap_off + 2);
    var best: ?usize = null;
    var best_score: i32 = -1;
    var i: usize = 0;
    while (i < ntab) : (i += 1) {
        const rec = cmap_off + 4 + i * 8;
        const plat = rdU16(d, rec);
        const enc = rdU16(d, rec + 2);
        const sub = cmap_off + rdU32(d, rec + 4);
        // prefer Windows Unicode BMP / full, then Unicode, then anything
        const score: i32 = if (plat == 3 and enc == 10) 5 else if (plat == 3 and enc == 1) 4 else if (plat == 0) 3 else if (plat == 3 and enc == 0) 2 else 1;
        if (score > best_score) {
            best_score = score;
            best = sub;
        }
    }
    return .{ .sub = best orelse return Error.NoCmap };
}

// ---- outline extraction + rasterization -----------------------------------
const Pt = struct { x: f32, y: f32, on: bool };

fn glyphRange(d: []const u8, t: Tables, long_loca: bool, gi: usize) ?[2]usize {
    if (long_loca) {
        const a = rdU32(d, t.loca + gi * 4);
        const b = rdU32(d, t.loca + (gi + 1) * 4);
        if (b <= a) return null;
        return .{ t.glyf + a, t.glyf + b };
    } else {
        const a = @as(usize, rdU16(d, t.loca + gi * 2)) * 2;
        const b = @as(usize, rdU16(d, t.loca + (gi + 1) * 2)) * 2;
        if (b <= a) return null;
        return .{ t.glyf + a, t.glyf + b };
    }
}

const Raster = struct {
    cov: []f32, // gw*cell_h accumulation
    gw: usize,
    cell_h: usize,

    fn addSpan(self: *Raster, row: usize, xl_in: f32, xr_in: f32, weight: f32) void {
        const w: f32 = @floatFromInt(self.gw);
        const xl = std.math.clamp(xl_in, 0, w);
        const xr = std.math.clamp(xr_in, 0, w);
        if (xr <= xl) return;
        const base = row * self.gw;
        const ixl: usize = @intFromFloat(@floor(xl));
        const ixr: usize = @intFromFloat(@floor(xr));
        if (ixl == ixr) {
            if (ixl < self.gw) self.cov[base + ixl] += (xr - xl) * weight;
            return;
        }
        if (ixl < self.gw) self.cov[base + ixl] += (@as(f32, @floatFromInt(ixl + 1)) - xl) * weight;
        var ix = ixl + 1;
        while (ix < ixr and ix < self.gw) : (ix += 1) self.cov[base + ix] += weight;
        if (ixr < self.gw) self.cov[base + ixr] += (xr - @as(f32, @floatFromInt(ixr))) * weight;
    }
};

const Edge = struct { x0: f32, y0: f32, x1: f32, y1: f32 };

fn flattenQuad(poly: *std.ArrayList(Pt), p0: Pt, c: Pt, p1: Pt) !void {
    var k: usize = 1;
    while (k <= STEPS) : (k += 1) {
        const t = @as(f32, @floatFromInt(k)) / STEPS;
        const mt = 1 - t;
        const x = mt * mt * p0.x + 2 * mt * t * c.x + t * t * p1.x;
        const y = mt * mt * p0.y + 2 * mt * t * c.y + t * t * p1.y;
        try poly.append(.{ .x = x, .y = y, .on = true });
    }
}

/// Rasterize ASCII 32..126 of `ttf_bytes` at `pixel_size` px into an owned
/// `font.Font`. Free it later with `freeFont`.
pub fn rasterizeAscii(a: std.mem.Allocator, ttf_bytes: []const u8, pixel_size: f32) !Font {
    const d = ttf_bytes;
    const t = try findTables(d);
    const units_per_em: f32 = @floatFromInt(rdU16(d, t.head + 18));
    const long_loca = rdI16(d, t.head + 50) == 1;
    const num_hmetrics = rdU16(d, t.hhea + 34);
    const ascender: f32 = @floatFromInt(rdI16(d, t.hhea + 4));
    const descender: f32 = @floatFromInt(rdI16(d, t.hhea + 6));
    const scale = pixel_size / units_per_em;
    const cmap = try chooseCmap(d, t.cmap);

    const baseline = ascender * scale;
    const cell_h: usize = @max(1, @as(usize, @intFromFloat(@round((ascender - descender) * scale))) + 1);

    const first: u8 = 32;
    const last: u8 = 126;
    const nchars: usize = last - first + 1;

    var advances = try a.alloc(u8, nchars);
    var widths = try a.alloc(u8, nchars);
    var offsets = try a.alloc(u32, nchars);
    var data = std.ArrayList(u8).init(a);
    errdefer {
        a.free(advances);
        a.free(widths);
        a.free(offsets);
        data.deinit();
    }

    var poly = std.ArrayList(Pt).init(a);
    defer poly.deinit();
    var pts = std.ArrayList(Pt).init(a);
    defer pts.deinit();
    var edges = std.ArrayList(Edge).init(a);
    defer edges.deinit();

    var ci: usize = 0;
    while (ci < nchars) : (ci += 1) {
        const ch: u8 = first + @as(u8, @intCast(ci));
        const gi = cmap.lookup(d, ch);

        // advance width from hmtx
        const adv_units: f32 = if (gi < num_hmetrics)
            @floatFromInt(rdU16(d, t.hmtx + gi * 4))
        else
            @floatFromInt(rdU16(d, t.hmtx + (num_hmetrics - 1) * 4));
        const adv_px = adv_units * scale;
        advances[ci] = @intFromFloat(std.math.clamp(@round(adv_px), 1, 255));

        offsets[ci] = @intCast(data.items.len);
        edges.clearRetainingCapacity();

        var gw: usize = 1;
        if (glyphRange(d, t, long_loca, gi)) |rng| {
            const g0 = rng[0];
            const ncont = rdI16(d, g0);
            if (ncont > 0) {
                const xmax: f32 = @floatFromInt(rdI16(d, g0 + 6));
                gw = @max(1, @as(usize, @intFromFloat(@ceil(xmax * scale))) + 1);
                gw = @min(gw, @as(usize, @intFromFloat(pixel_size)) * 3 + 4);
                try buildEdges(d, g0, @intCast(ncont), scale, baseline, &pts, &poly, &edges);
            }
        }
        widths[ci] = @intCast(@min(gw, 255));

        // scan-convert the accumulated edges into a coverage cell
        const cov = try a.alloc(f32, gw * cell_h);
        defer a.free(cov);
        @memset(cov, 0);
        var rs = Raster{ .cov = cov, .gw = gw, .cell_h = cell_h };
        rasterize(&rs, edges.items);
        // emit grayscale bytes
        for (cov) |c| try data.append(@intFromFloat(std.math.clamp(c * 255.0, 0, 255)));
    }

    return Font{
        .cell_h = cell_h,
        .first = first,
        .subpixel = false,
        .advance = advances,
        .width = widths,
        .offset = offsets,
        .data = try data.toOwnedSlice(),
    };
}

fn buildEdges(d: []const u8, g0: usize, ncont: usize, scale: f32, baseline: f32, pts: *std.ArrayList(Pt), poly: *std.ArrayList(Pt), edges: *std.ArrayList(Edge)) !void {
    // end points of contours
    const endsO = g0 + 10;
    const npts = @as(usize, rdU16(d, endsO + (ncont - 1) * 2)) + 1;
    const insLenO = endsO + ncont * 2;
    const insLen = rdU16(d, insLenO);
    var cur = insLenO + 2 + insLen;

    // read flags (with repeat)
    var flags = std.ArrayList(u8).init(pts.allocator);
    defer flags.deinit();
    while (flags.items.len < npts) {
        const f = d[cur];
        cur += 1;
        try flags.append(f);
        if (f & 0x08 != 0) { // repeat
            var rep = d[cur];
            cur += 1;
            while (rep > 0) : (rep -= 1) try flags.append(f);
        }
    }
    // x coords (delta encoded)
    var xs = std.ArrayList(i32).init(pts.allocator);
    defer xs.deinit();
    var x: i32 = 0;
    for (flags.items) |f| {
        if (f & 0x02 != 0) { // x-short (1 byte)
            const dx: i32 = d[cur];
            cur += 1;
            x += if (f & 0x10 != 0) dx else -dx;
        } else if (f & 0x10 == 0) { // not same -> i16 delta
            x += rdI16(d, cur);
            cur += 2;
        }
        try xs.append(x);
    }
    // y coords
    var ys = std.ArrayList(i32).init(pts.allocator);
    defer ys.deinit();
    var y: i32 = 0;
    for (flags.items) |f| {
        if (f & 0x04 != 0) {
            const dy: i32 = d[cur];
            cur += 1;
            y += if (f & 0x20 != 0) dy else -dy;
        } else if (f & 0x20 == 0) {
            y += rdI16(d, cur);
            cur += 2;
        }
        try ys.append(y);
    }

    // walk each contour, insert implied on-curve midpoints, flatten, emit edges
    var start: usize = 0;
    var c: usize = 0;
    while (c < ncont) : (c += 1) {
        const end = @as(usize, rdU16(d, endsO + c * 2));
        const cnt = end - start + 1;
        if (cnt < 2) {
            start = end + 1;
            continue;
        }
        // raw contour points in pixel space
        pts.clearRetainingCapacity();
        var i: usize = 0;
        while (i < cnt) : (i += 1) {
            const idx = start + i;
            try pts.append(.{
                .x = @as(f32, @floatFromInt(xs.items[idx])) * scale,
                .y = baseline - @as(f32, @floatFromInt(ys.items[idx])) * scale,
                .on = (flags.items[idx] & 0x01) != 0,
            });
        }
        try emitContour(pts.items, poly, edges);
        start = end + 1;
    }
}

fn emitContour(raw: []const Pt, poly: *std.ArrayList(Pt), edges: *std.ArrayList(Edge)) !void {
    const n = raw.len;
    // expand: insert implied on-curve midpoints between consecutive off points
    var exp = std.ArrayList(Pt).init(poly.allocator);
    defer exp.deinit();
    var i: usize = 0;
    while (i < n) : (i += 1) {
        const p = raw[i];
        try exp.append(p);
        const nx = raw[(i + 1) % n];
        if (!p.on and !nx.on) try exp.append(.{ .x = (p.x + nx.x) / 2, .y = (p.y + nx.y) / 2, .on = true });
    }
    // rotate so we start on an on-curve point
    var startIdx: ?usize = null;
    for (exp.items, 0..) |p, k| {
        if (p.on) {
            startIdx = k;
            break;
        }
    }
    const s = startIdx orelse return; // degenerate (no on-curve) — skip
    const m = exp.items.len;

    poly.clearRetainingCapacity();
    try poly.append(exp.items[s]);
    var k: usize = 1;
    while (k <= m) {
        const a = exp.items[(s + k) % m];
        if (a.on) {
            try poly.append(a);
            k += 1;
        } else {
            const b = exp.items[(s + k + 1) % m]; // guaranteed on-curve after expansion
            const cur_on = poly.items[poly.items.len - 1];
            try flattenQuad(poly, cur_on, a, b);
            k += 2;
        }
    }
    // emit closed edge loop
    const pn = poly.items.len;
    if (pn < 2) return;
    var j: usize = 0;
    while (j < pn) : (j += 1) {
        const p = poly.items[j];
        const q = poly.items[(j + 1) % pn];
        try edges.append(.{ .x0 = p.x, .y0 = p.y, .x1 = q.x, .y1 = q.y });
    }
}

fn rasterize(rs: *Raster, edges: []const Edge) void {
    const inv: f32 = 1.0 / @as(f32, SS);
    var xbuf: [256]struct { x: f32, dir: i2 } = undefined;
    var py: usize = 0;
    while (py < rs.cell_h) : (py += 1) {
        var sub: usize = 0;
        while (sub < SS) : (sub += 1) {
            const sy = @as(f32, @floatFromInt(py)) + (@as(f32, @floatFromInt(sub)) + 0.5) * inv;
            var nx: usize = 0;
            for (edges) |e| {
                const ylo = @min(e.y0, e.y1);
                const yhi = @max(e.y0, e.y1);
                if (sy < ylo or sy >= yhi) continue;
                if (e.y1 == e.y0) continue;
                const tt = (sy - e.y0) / (e.y1 - e.y0);
                const xx = e.x0 + tt * (e.x1 - e.x0);
                if (nx < xbuf.len) {
                    xbuf[nx] = .{ .x = xx, .dir = if (e.y1 > e.y0) 1 else -1 };
                    nx += 1;
                }
            }
            if (nx < 2) continue;
            std.sort.insertion(@TypeOf(xbuf[0]), xbuf[0..nx], {}, struct {
                fn lt(_: void, p: @TypeOf(xbuf[0]), q: @TypeOf(xbuf[0])) bool {
                    return p.x < q.x;
                }
            }.lt);
            var wind: i32 = 0;
            var c: usize = 0;
            while (c + 1 < nx) : (c += 1) {
                wind += xbuf[c].dir;
                if (wind != 0) rs.addSpan(py, xbuf[c].x, xbuf[c + 1].x, inv);
            }
        }
    }
}

/// Free a Font produced by `rasterizeAscii`.
pub fn freeFont(a: std.mem.Allocator, f: Font) void {
    a.free(f.advance);
    a.free(f.width);
    a.free(f.offset);
    a.free(f.data);
}

test "rasterize a system font if present" {
    const a = std.testing.allocator;
    const path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    const bytes = std.fs.cwd().readFileAlloc(a, path, 4 << 20) catch return; // skip if absent
    defer a.free(bytes);
    var f = try rasterizeAscii(a, bytes, 18);
    defer freeFont(a, f);
    try std.testing.expect(f.cell_h > 8 and f.cell_h < 64);
    try std.testing.expectEqual(@as(usize, 95), f.advance.len);
    // 'A' (index 33) should have some ink
    const gi = 'A' - 32;
    var ink: u32 = 0;
    const off = f.offset[gi];
    const len = @as(usize, f.width[gi]) * f.cell_h;
    for (f.data[off .. off + len]) |p| ink += p;
    try std.testing.expect(ink > 0);
}
