//! png.zig — minimal PNG encoder (RGB8) so screenshots are viewable anywhere.
//! Stored (uncompressed) zlib/deflate blocks + CRC32 + Adler32 — no compression
//! deps, valid PNG. Pairs with the PNG *decoder* in image.zig.

const std = @import("std");

fn adler32(data: []const u8) u32 {
    var s1: u32 = 1;
    var s2: u32 = 0;
    for (data) |b| {
        s1 = (s1 + b) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

fn writeChunk(w: anytype, tag: []const u8, data: []const u8) !void {
    try w.writeInt(u32, @intCast(data.len), .big);
    var crc = std.hash.crc.Crc32.init();
    crc.update(tag);
    crc.update(data);
    try w.writeAll(tag);
    try w.writeAll(data);
    try w.writeInt(u32, crc.final(), .big);
}

/// Write an RGBA framebuffer (row-major, 4 bytes/pixel) as an RGB PNG.
pub fn write(a: std.mem.Allocator, path: []const u8, pixels_rgba: []const u8, w: usize, h: usize) !void {
    // raw image: each row prefixed with filter byte 0, then RGB triples
    const row_bytes = w * 3 + 1;
    const raw = try a.alloc(u8, row_bytes * h);
    defer a.free(raw);
    for (0..h) |y| {
        raw[y * row_bytes] = 0; // filter: none
        for (0..w) |x| {
            const src = (y * w + x) * 4;
            const dst = y * row_bytes + 1 + x * 3;
            raw[dst] = pixels_rgba[src];
            raw[dst + 1] = pixels_rgba[src + 1];
            raw[dst + 2] = pixels_rgba[src + 2];
        }
    }

    // zlib stream: header + stored deflate blocks + adler32
    var idat = std.ArrayList(u8).init(a);
    defer idat.deinit();
    try idat.append(0x78); // CMF
    try idat.append(0x01); // FLG
    var off: usize = 0;
    while (off < raw.len) {
        const len: u16 = @intCast(@min(@as(usize, 65535), raw.len - off));
        const final: u8 = if (off + len >= raw.len) 1 else 0;
        try idat.append(final); // BFINAL + BTYPE=00 (stored)
        try idat.append(@truncate(len));
        try idat.append(@truncate(len >> 8));
        const nlen = ~len;
        try idat.append(@truncate(nlen));
        try idat.append(@truncate(nlen >> 8));
        try idat.appendSlice(raw[off .. off + len]);
        off += len;
    }
    const adler = adler32(raw);
    try idat.append(@truncate(adler >> 24));
    try idat.append(@truncate(adler >> 16));
    try idat.append(@truncate(adler >> 8));
    try idat.append(@truncate(adler));

    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const o = bw.writer();
    try o.writeAll(&[_]u8{ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A });

    var ihdr: [13]u8 = undefined;
    std.mem.writeInt(u32, ihdr[0..4], @intCast(w), .big);
    std.mem.writeInt(u32, ihdr[4..8], @intCast(h), .big);
    ihdr[8] = 8; // bit depth
    ihdr[9] = 2; // color type: truecolor RGB
    ihdr[10] = 0; // compression
    ihdr[11] = 0; // filter
    ihdr[12] = 0; // interlace
    try writeChunk(o, "IHDR", &ihdr);
    try writeChunk(o, "IDAT", idat.items);
    try writeChunk(o, "IEND", &.{});
    try bw.flush();
}

test "png writes a decodable file" {
    const a = std.testing.allocator;
    const image = @import("image.zig");
    // a 4x2 RGBA gradient
    var px: [4 * 2 * 4]u8 = undefined;
    for (0..8) |i| {
        px[i * 4] = @intCast(i * 30);
        px[i * 4 + 1] = 100;
        px[i * 4 + 2] = 200;
        px[i * 4 + 3] = 255;
    }
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrint(&nb, "test_png.{d}.png", .{std.os.linux.getpid()}) catch "test.png";
    try write(a, path, &px, 4, 2);
    defer std.fs.cwd().deleteFile(path) catch {};

    // round-trip through our own decoder
    const bytes = try std.fs.cwd().readFileAlloc(a, path, 1 << 20);
    defer a.free(bytes);
    var img = try image.decodePng(a, bytes);
    defer img.deinit();
    try std.testing.expectEqual(@as(usize, 4), img.w);
    try std.testing.expectEqual(@as(usize, 2), img.h);
    try std.testing.expectEqual(@as(u8, 100), img.pixels[1]); // green channel preserved
}
