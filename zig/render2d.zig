//! render2d.zig — a tiny software 2D renderer (RGBA framebuffer). Pure Zig, no
//! deps, cross-platform. The drawing core every Zenith UI view builds on. Live
//! windowing (X11/Wayland/etc.) blits this buffer; here we write it to an image.

const std = @import("std");
const font = @import("font_data.zig");

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
};
