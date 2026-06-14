//! crop.zig — crop + nearest-neighbour upscale a PNG so screenshot regions are
//! readable when inspecting the app. Run: zig run tools/control/crop.zig -- \
//!   <in.png> <x> <y> <w> <h> <scale> <out.png>

const std = @import("std");
const image = @import("image.zig");
const png = @import("png.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();
    const argv = try std.process.argsAlloc(a);
    defer std.process.argsFree(a, argv);
    if (argv.len < 8) {
        std.debug.print("usage: crop <in.png> <x> <y> <w> <h> <scale> <out.png>\n", .{});
        return;
    }
    const x = try std.fmt.parseInt(usize, argv[2], 10);
    const y = try std.fmt.parseInt(usize, argv[3], 10);
    const cw = try std.fmt.parseInt(usize, argv[4], 10);
    const ch = try std.fmt.parseInt(usize, argv[5], 10);
    const scale = try std.fmt.parseInt(usize, argv[6], 10);

    const bytes = try std.fs.cwd().readFileAlloc(a, argv[1], 1 << 26);
    defer a.free(bytes);
    var img = try image.decodePng(a, bytes);
    defer img.deinit();

    const ow = cw * scale;
    const oh = ch * scale;
    const out = try a.alloc(u8, ow * oh * 4);
    defer a.free(out);
    for (0..oh) |oy| {
        const sy = @min(y + oy / scale, img.h - 1);
        for (0..ow) |ox| {
            const sx = @min(x + ox / scale, img.w - 1);
            const s = (sy * img.w + sx) * 4;
            const d = (oy * ow + ox) * 4;
            @memcpy(out[d .. d + 4], img.pixels[s .. s + 4]);
        }
    }
    try png.write(a, argv[7], out, ow, oh);
    std.debug.print("wrote {s} ({d}x{d})\n", .{ argv[7], ow, oh });
}
