//! image.zig — a minimal, dependency-free PNG decoder producing straight-alpha
//! RGBA8 pixels. Supports 8-bit depth, color types 0/2/3/4/6 (gray, RGB, palette,
//! gray+alpha, RGBA), tRNS for palette/gray, and all five scanline filters.
//! Uses std's zlib only for IDAT inflate. Non-interlaced. Original Zig.
//!
//! PNG is the raster import path for the toolkit; pair Image.pixels with
//! gpu2d.GpuImage to upload + draw. A vector (SVG) rasterizer is the next step.

const std = @import("std");

pub const Image = struct {
    w: usize,
    h: usize,
    pixels: []u8, // RGBA8, row-major, top-left origin, straight alpha
    allocator: std.mem.Allocator,
    pub fn deinit(self: *Image) void {
        self.allocator.free(self.pixels);
    }
};

pub const Error = error{ BadSignature, Unsupported, Corrupt };

fn be32(b: []const u8) u32 {
    return (@as(u32, b[0]) << 24) | (@as(u32, b[1]) << 16) | (@as(u32, b[2]) << 8) | @as(u32, b[3]);
}

fn paeth(a: i32, b: i32, c: i32) i32 {
    const p = a + b - c;
    const pa = @abs(p - a);
    const pb = @abs(p - b);
    const pc = @abs(p - c);
    if (pa <= pb and pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

fn unfilter(filt: u8, src: []const u8, dst: []u8, prev: ?[]const u8, ch: usize) void {
    var i: usize = 0;
    while (i < src.len) : (i += 1) {
        const x: i32 = src[i];
        const a: i32 = if (i >= ch) dst[i - ch] else 0;
        const b: i32 = if (prev) |p| p[i] else 0;
        const c: i32 = if (prev != null and i >= ch) prev.?[i - ch] else 0;
        const v: i32 = switch (filt) {
            0 => x,
            1 => x + a,
            2 => x + b,
            3 => x + @divFloor(a + b, 2),
            4 => x + paeth(a, b, c),
            else => x,
        };
        dst[i] = @intCast(v & 0xff);
    }
}

/// Decode a PNG byte buffer into an owned RGBA8 Image. Caller deinits.
pub fn decodePng(a: std.mem.Allocator, bytes: []const u8) !Image {
    const sig = [_]u8{ 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
    if (bytes.len < 8 or !std.mem.eql(u8, bytes[0..8], &sig)) return Error.BadSignature;

    var pos: usize = 8;
    var width: usize = 0;
    var height: usize = 0;
    var bit_depth: u8 = 0;
    var color_type: u8 = 0;
    var pal_buf: [768]u8 = undefined;
    var pal_len: usize = 0;
    var trns_buf: [256]u8 = undefined;
    var trns_len: usize = 0;
    var idat = std.ArrayList(u8).init(a);
    defer idat.deinit();

    while (pos + 8 <= bytes.len) {
        const clen = be32(bytes[pos .. pos + 4]);
        const ctype = bytes[pos + 4 .. pos + 8];
        const cstart = pos + 8;
        if (cstart + clen + 4 > bytes.len) return Error.Corrupt;
        const cdata = bytes[cstart .. cstart + clen];
        if (std.mem.eql(u8, ctype, "IHDR")) {
            if (clen < 13) return Error.Corrupt;
            width = be32(cdata[0..4]);
            height = be32(cdata[4..8]);
            bit_depth = cdata[8];
            color_type = cdata[9];
            if (cdata[12] != 0) return Error.Unsupported; // interlaced
        } else if (std.mem.eql(u8, ctype, "PLTE")) {
            if (clen <= pal_buf.len) {
                @memcpy(pal_buf[0..clen], cdata);
                pal_len = clen;
            }
        } else if (std.mem.eql(u8, ctype, "tRNS")) {
            if (clen <= trns_buf.len) {
                @memcpy(trns_buf[0..clen], cdata);
                trns_len = clen;
            }
        } else if (std.mem.eql(u8, ctype, "IDAT")) {
            try idat.appendSlice(cdata);
        } else if (std.mem.eql(u8, ctype, "IEND")) {
            break;
        }
        pos = cstart + clen + 4; // data + 4-byte CRC
    }
    const palette = pal_buf[0..pal_len];
    const trns = trns_buf[0..trns_len];
    if (width == 0 or height == 0) return Error.Corrupt;
    if (bit_depth != 8) return Error.Unsupported; // 8-bit channels only

    const channels: usize = switch (color_type) {
        0 => 1, // grayscale
        2 => 3, // truecolor
        3 => 1, // palette index
        4 => 2, // gray + alpha
        6 => 4, // truecolor + alpha
        else => return Error.Unsupported,
    };
    const stride = width * channels;

    // inflate the concatenated IDAT (zlib stream) into raw filtered scanlines
    var raw = std.ArrayList(u8).init(a);
    defer raw.deinit();
    var in_stream = std.io.fixedBufferStream(idat.items);
    std.compress.zlib.decompress(in_stream.reader(), raw.writer()) catch return Error.Corrupt;
    if (raw.items.len < (stride + 1) * height) return Error.Corrupt;

    // reverse the per-scanline filters in place
    const unf = try a.alloc(u8, stride * height);
    defer a.free(unf);
    var y: usize = 0;
    while (y < height) : (y += 1) {
        const filt = raw.items[y * (stride + 1)];
        const src = raw.items[y * (stride + 1) + 1 .. y * (stride + 1) + 1 + stride];
        const dst = unf[y * stride .. y * stride + stride];
        const prev: ?[]const u8 = if (y == 0) null else unf[(y - 1) * stride .. (y - 1) * stride + stride];
        unfilter(filt, src, dst, prev, channels);
    }

    // expand whatever the source color type is into RGBA8
    const pixels = try a.alloc(u8, width * height * 4);
    var i: usize = 0;
    while (i < width * height) : (i += 1) {
        const s = unf[i * channels .. i * channels + channels];
        var r: u8 = 0;
        var g: u8 = 0;
        var b: u8 = 0;
        var al: u8 = 255;
        switch (color_type) {
            0 => {
                r = s[0];
                g = s[0];
                b = s[0];
            },
            2 => {
                r = s[0];
                g = s[1];
                b = s[2];
            },
            3 => {
                const idx = @as(usize, s[0]);
                if (idx * 3 + 2 < palette.len) {
                    r = palette[idx * 3];
                    g = palette[idx * 3 + 1];
                    b = palette[idx * 3 + 2];
                }
                if (idx < trns.len) al = trns[idx];
            },
            4 => {
                r = s[0];
                g = s[0];
                b = s[0];
                al = s[1];
            },
            6 => {
                r = s[0];
                g = s[1];
                b = s[2];
                al = s[3];
            },
            else => unreachable,
        }
        pixels[i * 4] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = al;
    }
    return .{ .w = width, .h = height, .pixels = pixels, .allocator = a };
}
