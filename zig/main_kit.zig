//! main_kit.zig — a synth device panel built with the layout engine and the new
//! widgets (knobs, toggles). No hand-placed coordinates: everything is laid out
//! by carving the region with layout.Box.

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const uikit = @import("uikit.zig");
const L = @import("layout.zig").Box;
const Color = r2d.Color;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

fn centered(cv: *r2d.Canvas, box: L, baseline_y: i32, s: []const u8, c: Color) void {
    const tw = r2d.Canvas.textWidth(s, fb);
    cv.textAA(box.cxi() - @divTrunc(tw, 2), baseline_y, s, c, fb);
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 760;
    const H: usize = 440;

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();
    var u = uikit.Ui.init(&cv);

    const accent = Color.rgb(96, 210, 235);
    const text = Color.rgb(232, 236, 244);
    const dim = Color.rgb(140, 148, 164);
    const panel = Color.rgb(30, 33, 41);
    const card = Color.rgb(38, 42, 52);

    var cutoff: f32 = 0.72;
    var reso: f32 = 0.34;
    var drive: f32 = 0.55;
    var mix: f32 = 0.66;
    var atk: f32 = 0.18;
    var rel: f32 = 0.48;
    var sync = true;
    var mono = false;
    var glide = true;

    u.begin(.{}, 1.0);
    cv.vGradient(0, 0, @intCast(W), @intCast(H), Color.rgb(20, 22, 28), Color.rgb(14, 15, 20));

    var root = L{ .x = 20, .y = 20, .w = @as(i32, @intCast(W)) - 40, .h = @as(i32, @intCast(H)) - 40 };
    cv.dropShadow(root.x, root.y, root.w, root.h, 16, 5);
    cv.fillRoundedRect(root.x, root.y, root.w, root.h, 16, panel);
    var inner = root.shrink(22);

    const title = inner.cutTop(34);
    cv.textAA(title.x, title.y - 4, "Zenith Synth", accent, fd);
    cv.textAA(title.x + 226, title.y + 8, "polyphonic", dim, fb);
    inner.gapTop(16);

    // knob row (6 columns, laid out)
    const knrow = inner.cutTop(132);
    var kb: [6]L = undefined;
    knrow.cols(6, 14, &kb);
    const knames = [_][]const u8{ "CUTOFF", "RESO", "DRIVE", "MIX", "ATTACK", "RELEASE" };
    const kvals = [_]*f32{ &cutoff, &reso, &drive, &mix, &atk, &rel };
    for (kb, 0..) |box, i| {
        cv.fillRoundedRect(box.x, box.y, box.w, box.h, 12, card);
        var b = box.shrink(10);
        const lbl = b.cutTop(16);
        centered(&cv, lbl, lbl.y, knames[i], dim);
        const valbox = b.cutBottom(16);
        _ = u.knob(@intCast(10 + i), b.cxi(), b.cyi(), 30, kvals[i]);
        var vbuf: [8]u8 = undefined;
        const vs = std.fmt.bufPrint(&vbuf, "{d:.0}%", .{kvals[i].* * 100}) catch "";
        centered(&cv, valbox, valbox.y + 2, vs, text);
    }
    inner.gapTop(16);

    // toggle row (3 columns)
    const tgrow = inner.cutTop(56);
    var tb: [3]L = undefined;
    tgrow.cols(3, 14, &tb);
    const tnames = [_][]const u8{ "Sync", "Mono", "Glide" };
    const tvals = [_]*bool{ &sync, &mono, &glide };
    for (tb, 0..) |box, i| {
        cv.fillRoundedRect(box.x, box.y, box.w, box.h, 10, card);
        const b = box.shrink(14);
        cv.textAA(b.x, b.y + 8, tnames[i], text, fb);
        _ = u.toggle(@intCast(30 + i), b.x + b.w - 50, b.y + 4, 46, 24, tvals[i]);
    }

    u.end();
    try bmp.write("kit_ui.bmp", cv.pixels, W, H);
    std.debug.print("rendered widget/layout demo -> kit_ui.bmp ({d}x{d})\n", .{ W, H });
}
