//! bmp.zig — write an RGBA framebuffer to a 24-bit BMP (universally viewable,
//! pure Zig, no compression deps).

const std = @import("std");

pub fn write(path: []const u8, pixels_rgba: []const u8, w: usize, h: usize) !void {
    const row_size = ((w * 3 + 3) / 4) * 4;
    const data_size = row_size * h;
    const file_size = 54 + data_size;

    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();
    var bw = std.io.bufferedWriter(file.writer());
    const o = bw.writer();

    // BITMAPFILEHEADER
    try o.writeAll("BM");
    try o.writeInt(u32, @intCast(file_size), .little);
    try o.writeInt(u16, 0, .little);
    try o.writeInt(u16, 0, .little);
    try o.writeInt(u32, 54, .little); // pixel data offset
    // BITMAPINFOHEADER
    try o.writeInt(u32, 40, .little);
    try o.writeInt(i32, @intCast(w), .little);
    try o.writeInt(i32, @intCast(h), .little);
    try o.writeInt(u16, 1, .little); // planes
    try o.writeInt(u16, 24, .little); // bits per pixel
    try o.writeInt(u32, 0, .little); // BI_RGB
    try o.writeInt(u32, @intCast(data_size), .little);
    try o.writeInt(i32, 2835, .little); // 72 dpi
    try o.writeInt(i32, 2835, .little);
    try o.writeInt(u32, 0, .little);
    try o.writeInt(u32, 0, .little);

    // pixel rows, bottom-up, BGR
    const pad = row_size - w * 3;
    var y: usize = h;
    while (y > 0) {
        y -= 1;
        var x: usize = 0;
        while (x < w) : (x += 1) {
            const i = (y * w + x) * 4;
            try o.writeByte(pixels_rgba[i + 2]); // B
            try o.writeByte(pixels_rgba[i + 1]); // G
            try o.writeByte(pixels_rgba[i + 0]); // R
        }
        var p: usize = 0;
        while (p < pad) : (p += 1) try o.writeByte(0);
    }
    try bw.flush();
}
