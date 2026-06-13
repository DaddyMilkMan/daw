//! render2d.zig — a tiny software 2D renderer (RGBA framebuffer). Pure Zig, no
//! deps, cross-platform. The drawing core every Zenith UI view builds on. Live
//! windowing (X11/Wayland/etc.) blits this buffer; here we write it to an image.

const std = @import("std");
const font = @import("font_data.zig");
const FontT = @import("font.zig").Font;

pub const Color = struct {
    r: u8,
    g: u8,
    b: u8,
    a: u8 = 255,
    pub fn rgb(r: u8, g: u8, b: u8) Color {
        return .{ .r = r, .g = g, .b = b };
    }
    pub fn rgba(r: u8, g: u8, b: u8, a: u8) Color {
        return .{ .r = r, .g = g, .b = b, .a = a };
    }
};

fn lerp(a: u8, b: u8, t: f32) u8 {
    return @intFromFloat(@as(f32, @floatFromInt(a)) * (1 - t) + @as(f32, @floatFromInt(b)) * t);
}

pub const Canvas = struct {
    pixels: []u8, // RGBA, row-major
    width: usize,
    height: usize,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator, w: usize, h: usize) !Canvas {
        const px = try a.alloc(u8, w * h * 4);
        @memset(px, 0);
        return .{ .pixels = px, .width = w, .height = h, .allocator = a };
    }
    pub fn deinit(self: *Canvas) void {
        self.allocator.free(self.pixels);
    }

    pub fn clear(self: *Canvas, c: Color) void {
        var i: usize = 0;
        while (i < self.width * self.height) : (i += 1) {
            self.pixels[i * 4] = c.r;
            self.pixels[i * 4 + 1] = c.g;
            self.pixels[i * 4 + 2] = c.b;
            self.pixels[i * 4 + 3] = 255;
        }
    }

    pub fn pset(self: *Canvas, x: i32, y: i32, c: Color) void {
        if (x < 0 or y < 0 or x >= @as(i32, @intCast(self.width)) or y >= @as(i32, @intCast(self.height))) return;
        const idx = (@as(usize, @intCast(y)) * self.width + @as(usize, @intCast(x))) * 4;
        if (c.a == 255) {
            self.pixels[idx] = c.r;
            self.pixels[idx + 1] = c.g;
            self.pixels[idx + 2] = c.b;
        } else {
            const a: u32 = c.a;
            const ia: u32 = 255 - a;
            self.pixels[idx] = @intCast((@as(u32, c.r) * a + @as(u32, self.pixels[idx]) * ia) / 255);
            self.pixels[idx + 1] = @intCast((@as(u32, c.g) * a + @as(u32, self.pixels[idx + 1]) * ia) / 255);
            self.pixels[idx + 2] = @intCast((@as(u32, c.b) * a + @as(u32, self.pixels[idx + 2]) * ia) / 255);
        }
    }

    pub fn fillRect(self: *Canvas, x: i32, y: i32, w: i32, h: i32, c: Color) void {
        var yy = y;
        while (yy < y + h) : (yy += 1) {
            var xx = x;
            while (xx < x + w) : (xx += 1) self.pset(xx, yy, c);
        }
    }

    pub fn outline(self: *Canvas, x: i32, y: i32, w: i32, h: i32, c: Color) void {
        self.fillRect(x, y, w, 1, c);
        self.fillRect(x, y + h - 1, w, 1, c);
        self.fillRect(x, y, 1, h, c);
        self.fillRect(x + w - 1, y, 1, h, c);
    }

    pub fn vGradient(self: *Canvas, x: i32, y: i32, w: i32, h: i32, top: Color, bot: Color) void {
        var row: i32 = 0;
        while (row < h) : (row += 1) {
            const t = @as(f32, @floatFromInt(row)) / @as(f32, @floatFromInt(@max(h - 1, 1)));
            self.fillRect(x, y + row, w, 1, .{ .r = lerp(top.r, bot.r, t), .g = lerp(top.g, bot.g, t), .b = lerp(top.b, bot.b, t) });
        }
    }

    pub fn text(self: *Canvas, x: i32, y: i32, s: []const u8, c: Color, scale: i32) void {
        var cx = x;
        for (s) |ch| {
            const code: usize = ch;
            if (code >= font.first_char and code < font.first_char + font.glyphs.len) {
                const g = font.glyphs[code - font.first_char];
                var row: usize = 0;
                while (row < font.glyph_h) : (row += 1) {
                    const bits = g[row];
                    var col: usize = 0;
                    while (col < font.glyph_w) : (col += 1) {
                        if ((bits >> @as(u3, @intCast(7 - col))) & 1 != 0)
                            self.fillRect(cx + @as(i32, @intCast(col)) * scale, y + @as(i32, @intCast(row)) * scale, scale, scale, c);
                    }
                }
            }
            cx += @as(i32, @intCast(font.glyph_w)) * scale;
        }
    }

    /// Per-channel alpha blend (for subpixel text). aR/aG/aB are 0..255.
    fn psetSub(self: *Canvas, x: i32, y: i32, r: u8, g: u8, b: u8, ar: u32, ag: u32, ab: u32) void {
        if (x < 0 or y < 0 or x >= @as(i32, @intCast(self.width)) or y >= @as(i32, @intCast(self.height))) return;
        const i = (@as(usize, @intCast(y)) * self.width + @as(usize, @intCast(x))) * 4;
        self.pixels[i] = @intCast((@as(u32, r) * ar + @as(u32, self.pixels[i]) * (255 - ar)) / 255);
        self.pixels[i + 1] = @intCast((@as(u32, g) * ag + @as(u32, self.pixels[i + 1]) * (255 - ag)) / 255);
        self.pixels[i + 2] = @intCast((@as(u32, b) * ab + @as(u32, self.pixels[i + 2]) * (255 - ab)) / 255);
    }

    /// Antialiased proportional text — grayscale or LCD subpixel coverage.
    pub fn textAA(self: *Canvas, x: i32, y: i32, s: []const u8, c: Color, f: *const FontT) void {
        var pen = x;
        for (s) |ch| {
            const code: usize = ch;
            if (code < f.first or code >= f.first + f.advance.len) {
                pen += 6;
                continue;
            }
            const gi = code - f.first;
            const w = f.width[gi];
            const off = f.offset[gi];
            var gy: usize = 0;
            while (gy < f.cell_h) : (gy += 1) {
                var gx: usize = 0;
                while (gx < w) : (gx += 1) {
                    if (f.subpixel) {
                        const base = off + gy * (w * 3) + gx * 3;
                        const cr = f.data[base];
                        const cg = f.data[base + 1];
                        const cb = f.data[base + 2];
                        if ((cr | cg | cb) == 0) continue;
                        const ca: u32 = c.a;
                        self.psetSub(pen + @as(i32, @intCast(gx)), y + @as(i32, @intCast(gy)), c.r, c.g, c.b, @as(u32, cr) * ca / 255, @as(u32, cg) * ca / 255, @as(u32, cb) * ca / 255);
                    } else {
                        const cov = f.data[off + gy * w + gx];
                        if (cov == 0) continue;
                        self.pset(pen + @as(i32, @intCast(gx)), y + @as(i32, @intCast(gy)), .{ .r = c.r, .g = c.g, .b = c.b, .a = @intCast((@as(u32, cov) * @as(u32, c.a)) / 255) });
                    }
                }
            }
            pen += f.advance[gi];
        }
    }

    /// Width in pixels of `s` in font `f`.
    pub fn textWidth(s: []const u8, f: *const FontT) i32 {
        var wsum: i32 = 0;
        for (s) |ch| {
            const code: usize = ch;
            wsum += if (code >= f.first and code < f.first + f.advance.len) @as(i32, f.advance[code - f.first]) else 6;
        }
        return wsum;
    }

    /// Antialiased filled rounded rectangle.
    pub fn fillRoundedRect(self: *Canvas, x: i32, y: i32, w: i32, h: i32, radius: i32, c: Color) void {
        const r: f32 = @floatFromInt(@min(radius, @min(@divTrunc(w, 2), @divTrunc(h, 2))));
        const wf: f32 = @floatFromInt(w);
        const hf: f32 = @floatFromInt(h);
        var yy: i32 = 0;
        while (yy < h) : (yy += 1) {
            var xx: i32 = 0;
            while (xx < w) : (xx += 1) {
                const cov = roundedCoverage(@floatFromInt(xx), @floatFromInt(yy), wf, hf, r);
                if (cov <= 0.003) continue;
                const a: u32 = @intFromFloat(cov * @as(f32, @floatFromInt(c.a)));
                self.pset(x + xx, y + yy, .{ .r = c.r, .g = c.g, .b = c.b, .a = @intCast(@min(a, 255)) });
            }
        }
    }

    /// Soft drop shadow behind a rounded rect (draw before the element).
    pub fn dropShadow(self: *Canvas, x: i32, y: i32, w: i32, h: i32, radius: i32, spread: i32) void {
        var s: i32 = spread;
        while (s >= 1) : (s -= 1) {
            const a: u8 = @intCast(@as(u32, 26) * @as(u32, @intCast(spread - s + 1)) / @as(u32, @intCast(spread)));
            self.fillRoundedRect(x - s, y - s + 4, w + 2 * s, h + 2 * s, radius + s, .{ .r = 0, .g = 0, .b = 0, .a = a });
        }
    }
};

fn roundedCoverage(px: f32, py: f32, w: f32, h: f32, r: f32) f32 {
    if (r <= 0) return 1.0;
    const corner_x = px < r or px > w - 1 - r;
    const corner_y = py < r or py > h - 1 - r;
    if (!(corner_x and corner_y)) return 1.0;
    const cx = if (px < r) r else w - 1 - r;
    const cy = if (py < r) r else h - 1 - r;
    const dx = px - cx;
    const dy = py - cy;
    const d = @sqrt(dx * dx + dy * dy);
    return std.math.clamp(r - d + 0.5, 0.0, 1.0);
}
