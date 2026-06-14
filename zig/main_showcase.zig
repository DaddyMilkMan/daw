//! main_showcase.zig — a gallery of everything the Zenith UI toolkit can do:
//! gradients & color blends, glass (refraction), layered shadows, the full
//! control set with live hover/press, and the icon set. `zig build showcase`.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const flex = @import("flex.zig");
const widgets = @import("widgets.zig");
const image = @import("image.zig");
const svg = @import("svg.zig");
const ttf = @import("ttf.zig");
const FontData = @import("font.zig").Font;
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const px = flex.px;
const grow = flex.grow;
const groww = flex.groww;

// palette (OKLCH-ish hand-picked vibrant set)
const bg0 = Color.rgb(15, 16, 21);
const bg1 = Color.rgb(10, 11, 15);
const card_t = Color.rgb(34, 37, 47);
const card_b = Color.rgb(27, 30, 38);
const txt = Color.rgb(236, 240, 247);
const dim = Color.rgb(150, 158, 173);
const faint = Color.rgb(96, 104, 120);
const accent = Color.rgb(108, 147, 244);
const clear = Color.rgba(0, 0, 0, 0);

fn lerp(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

// ---- expanded icon set (thin-stroke, ~s px) --------------------------------
const ico = struct {
    fn play(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.tri(x - s * 0.3, y - s * 0.45, x - s * 0.3, y + s * 0.45, x + s * 0.45, y, c);
    }
    fn pause(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.36, y - s * 0.45, s * 0.24, s * 0.9, 1.4, c);
        g.rect(x + s * 0.12, y - s * 0.45, s * 0.24, s * 0.9, 1.4, c);
    }
    fn stop(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.4, y - s * 0.4, s * 0.8, s * 0.8, 2.5, c);
    }
    fn record(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.4, y - s * 0.4, s * 0.8, s * 0.8, s * 0.4, c);
    }
    fn plus(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.line(x - s * 0.45, y, x + s * 0.45, y, 1.7, c);
        g.line(x, y - s * 0.45, x, y + s * 0.45, 1.7, c);
    }
    fn check(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.line(x - s * 0.4, y + s * 0.05, x - s * 0.08, y + s * 0.38, 1.8, c);
        g.line(x - s * 0.08, y + s * 0.38, x + s * 0.45, y - s * 0.38, 1.8, c);
    }
    fn close(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.line(x - s * 0.4, y - s * 0.4, x + s * 0.4, y + s * 0.4, 1.7, c);
        g.line(x - s * 0.4, y + s * 0.4, x + s * 0.4, y - s * 0.4, 1.7, c);
    }
    fn search(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.stroke(x - s * 0.42, y - s * 0.42, s * 0.62, s * 0.62, s * 0.31, 1.5, c);
        g.line(x + s * 0.12, y + s * 0.12, x + s * 0.45, y + s * 0.45, 1.7, c);
    }
    fn gear(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        var k: f32 = 0;
        while (k < 8) : (k += 1) {
            const a = k / 8.0 * std.math.tau;
            g.line(x + @cos(a) * s * 0.28, y + @sin(a) * s * 0.28, x + @cos(a) * s * 0.46, y + @sin(a) * s * 0.46, 1.6, c);
        }
        g.stroke(x - s * 0.2, y - s * 0.2, s * 0.4, s * 0.4, s * 0.2, 1.5, c);
    }
    fn folder(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.stroke(x - s * 0.45, y - s * 0.28, s * 0.9, s * 0.62, 2.5, 1.5, c);
        g.line(x - s * 0.45, y - s * 0.28, x - s * 0.1, y - s * 0.28, 1.5, c);
        g.line(x - s * 0.32, y - s * 0.4, x - s * 0.1, y - s * 0.28, 1.5, c);
    }
    fn save(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.stroke(x - s * 0.42, y - s * 0.42, s * 0.84, s * 0.84, 2.5, 1.5, c);
        g.rect(x - s * 0.2, y - s * 0.42, s * 0.4, s * 0.26, 1, c);
        g.stroke(x - s * 0.26, y + s * 0.02, s * 0.52, s * 0.4, 1.5, 1.4, c);
    }
    fn heart(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.24, y - s * 0.2, s * 0.26, s * 0.26, s * 0.13, c);
        g.rect(x - s * 0.02, y - s * 0.2, s * 0.26, s * 0.26, s * 0.13, c);
        g.tri(x - s * 0.34, y - s * 0.02, x + s * 0.34, y - s * 0.02, x, y + s * 0.42, c);
    }
    fn star(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        var k: f32 = 0;
        var px0: f32 = 0;
        var py0: f32 = 0;
        while (k <= 10) : (k += 1) {
            const a = -std.math.pi / 2.0 + k / 10.0 * std.math.tau;
            const rr = if (@mod(k, 2) == 0) s * 0.46 else s * 0.2;
            const xx = x + @cos(a) * rr;
            const yy = y + @sin(a) * rr;
            if (k > 0) g.line(px0, py0, xx, yy, 1.6, c);
            px0 = xx;
            py0 = yy;
        }
    }
    fn wave(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.45, y - s * 0.15, 1.6, s * 0.3, 0, c);
        g.rect(x - s * 0.22, y - s * 0.45, 1.6, s * 0.9, 0, c);
        g.rect(x + s * 0.0, y - s * 0.28, 1.6, s * 0.56, 0, c);
        g.rect(x + s * 0.22, y - s * 0.4, 1.6, s * 0.8, 0, c);
        g.rect(x + s * 0.44, y - s * 0.18, 1.6, s * 0.36, 0, c);
    }
    fn menu(g: *Gpu, x: f32, y: f32, s: f32, c: Color) void {
        g.rect(x - s * 0.45, y - s * 0.28, s * 0.9, 1.7, 1, c);
        g.rect(x - s * 0.45, y - 0.85, s * 0.9, 1.7, 1, c);
        g.rect(x - s * 0.45, y + s * 0.28 - 1.7, s * 0.9, 1.7, 1, c);
    }
};

const Icon = struct { name: []const u8, f: *const fn (*Gpu, f32, f32, f32, Color) void };
const ICONS = [_]Icon{
    .{ .name = "play", .f = ico.play },     .{ .name = "pause", .f = ico.pause },
    .{ .name = "stop", .f = ico.stop },     .{ .name = "record", .f = ico.record },
    .{ .name = "plus", .f = ico.plus },     .{ .name = "check", .f = ico.check },
    .{ .name = "close", .f = ico.close },   .{ .name = "search", .f = ico.search },
    .{ .name = "gear", .f = ico.gear },     .{ .name = "folder", .f = ico.folder },
    .{ .name = "save", .f = ico.save },     .{ .name = "heart", .f = ico.heart },
    .{ .name = "star", .f = ico.star },     .{ .name = "wave", .f = ico.wave },
    .{ .name = "menu", .f = ico.menu },
};

// gradient swatch colors (color blends)
const GRADS = [_][2]Color{
    .{ Color.rgb(108, 147, 244), Color.rgb(166, 122, 234) }, // indigo->violet
    .{ Color.rgb(244, 127, 85), Color.rgb(236, 100, 140) }, // coral->pink
    .{ Color.rgb(72, 199, 176), Color.rgb(108, 147, 244) }, // teal->indigo
    .{ Color.rgb(250, 204, 90), Color.rgb(244, 127, 85) }, // amber->coral
    .{ Color.rgb(122, 211, 140), Color.rgb(72, 199, 176) }, // green->teal
    .{ Color.rgb(166, 122, 234), Color.rgb(236, 100, 140) }, // violet->pink
};

const Vals = struct {
    knob: [3]f32 = .{ 0.35, 0.6, 0.85 },
    fader: [3]f32 = .{ 0.7, 0.45, 0.9 },
    slider: [2]f32 = .{ 0.6, 0.0 }, // standard (60%) + bipolar (centered/balanced)
    toggle: [2]bool = .{ true, false },
    meter: [4]f32 = .{ 0.3, 0.5, 0.7, 0.4 },
    check: [2]bool = .{ true, false },
    seg: usize = 0,
    prog: f32 = 0.35,
    trow: usize = 1, // selected table row
    // draggable floating chip (web drag-and-drop feel)
    drag_x: f32 = 980,
    drag_y: f32 = 92,
    grab_dx: f32 = 0,
    grab_dy: f32 = 0,
    dragging: bool = false,
    drag_lift: f32 = 0,
};

// demo table — a mixer/track list with tabular numeric columns
const TCOLS = [_]widgets.Col{
    .{ .title = "#", .w = 46, .al = .center, .num = true },
    .{ .title = "TRACK", .w = 196 },
    .{ .title = "TYPE", .w = 116 },
    .{ .title = "LEVEL", .w = 96, .al = .right, .num = true },
    .{ .title = "PAN", .w = 86, .al = .right, .num = true },
    .{ .title = "STATE", .w = 110 },
};
const TROWS = [_][]const []const u8{
    &.{ "1", "Kick Bus", "Audio", "-3.2 dB", "C", "armed" },
    &.{ "2", "Snare Top", "Audio", "-6.0 dB", "12L", "solo" },
    &.{ "3", "Bass Synth", "Instrument", "-4.8 dB", "C", "active" },
    &.{ "4", "Lead Pluck", "Instrument", "-9.1 dB", "24R", "active" },
    &.{ "5", "Vocal Chop", "Audio", "-12.4 dB", "8L", "muted" },
    &.{ "6", "Reverb Send", "FX Return", "-18.0 dB", "C", "active" },
};
fn frac(x: f32) f32 {
    return x - @floor(x);
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1200;
    var H: usize = 1356;

    var window = try win.NativeWindow.open(a, W, H, "Zenith Toolkit");
    defer window.close();
    window.makeCurrent();
    var g = try gpu2d.Gpu.init(a, win.NativeWindow.glProc);
    defer g.deinit();
    var fc = try gpu2d.GpuFont.init(a, &@import("font_caption.zig").font);
    defer fc.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    var fu = try gpu2d.GpuFont.init(a, &@import("font_ui.zig").font);
    defer fu.deinit();
    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();
    // variety typefaces
    var fserif = try gpu2d.GpuFont.init(a, &@import("font_serif.zig").font);
    defer fserif.deinit();
    var fmono = try gpu2d.GpuFont.init(a, &@import("font_mono.zig").font);
    defer fmono.deinit();
    var fcond = try gpu2d.GpuFont.init(a, &@import("font_cond.zig").font);
    defer fcond.deinit();
    var falt = try gpu2d.GpuFont.init(a, &@import("font_alt.zig").font);
    defer falt.deinit();
    var fround = try gpu2d.GpuFont.init(a, &@import("font_round.zig").font);
    defer fround.deinit();
    // runtime TTF loader — rasterize installed system .ttf fonts on the fly into
    // the same atlas pipeline the baked fonts use (this is the path to "thousands
    // of fonts": any of the machine's installed TrueType faces, loaded at runtime).
    const RtFont = struct { gf: gpu2d.GpuFont, data: FontData, name: []const u8 };
    var rt_fonts = std.ArrayList(RtFont).init(a);
    defer {
        for (rt_fonts.items) |*rf| {
            rf.gf.deinit();
            ttf.freeFont(a, rf.data);
        }
        rt_fonts.deinit();
    }
    const rt_specs = [_]struct { path: []const u8, name: []const u8 }{
        .{ .path = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", .name = "DejaVu Serif" },
        .{ .path = "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", .name = "Ubuntu" },
        .{ .path = "/usr/share/fonts/truetype/freefont/FreeSerif.ttf", .name = "FreeSerif" },
    };
    for (rt_specs) |sp| {
        const bytes = std.fs.cwd().readFileAlloc(a, sp.path, 8 << 20) catch continue;
        defer a.free(bytes);
        const fnt = ttf.rasterizeAscii(a, bytes, 17) catch continue;
        const gf = gpu2d.GpuFont.init(a, &fnt) catch {
            ttf.freeFont(a, fnt);
            continue;
        };
        rt_fonts.append(.{ .gf = gf, .data = fnt, .name = sp.name }) catch {};
    }

    // typeface display: 3 baked faces (left column) + 3 runtime .ttf (right column)
    const Face = struct { f: *const gpu2d.GpuFont, n: []const u8, rt: bool };
    var faces = [_]Face{
        .{ .f = &fu, .n = "Fira Sans", .rt = false },
        .{ .f = &fserif, .n = "Noto Serif", .rt = false },
        .{ .f = &fmono, .n = "Fira Mono", .rt = false },
        .{ .f = &fcond, .n = "Fira Condensed", .rt = false },
        .{ .f = &falt, .n = "Open Sans", .rt = false },
        .{ .f = &fround, .n = "Cantarell", .rt = false },
    };
    for (rt_fonts.items, 0..) |*rf, i| {
        if (i < 3) faces[3 + i] = .{ .f = &rf.gf, .n = rf.name, .rt = true };
    }

    var c = flex.Ctx.init(&g, &fb, &fu, &fd);
    var u = widgets.Ui.init(&g);
    var v = Vals{};

    // image import — decode an embedded PNG (our own decoder) and upload it
    var sample_img: ?gpu2d.GpuImage = null;
    if (image.decodePng(a, @embedFile("assets/sample.png"))) |decoded| {
        var d = decoded;
        sample_img = gpu2d.GpuImage.init(d.pixels, d.w, d.h);
        d.deinit(); // texture now owns the pixels GPU-side
    } else |e| std.debug.print("png decode failed: {any}\n", .{e});
    defer if (sample_img) |*im| im.deinit();

    // vector import — rasterize an embedded SVG (our own rasterizer) to a texture
    var svg_img: ?gpu2d.GpuImage = null;
    if (svg.rasterize(a, @embedFile("assets/sample.svg"), 256, 256)) |decoded| {
        var d = decoded;
        svg_img = gpu2d.GpuImage.init(d.pixels, d.w, d.h);
        d.deinit();
    } else |e| std.debug.print("svg raster failed: {any}\n", .{e});
    defer if (svg_img) |*im| im.deinit();

    std.debug.print("Zenith toolkit showcase\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |val| {
            defer a.free(val);
            break :blk std.fmt.parseFloat(f64, val) catch 1.0e12;
        } else |_| break :blk 1.0e12;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var menu_open = true;
    var menu_anim: f32 = 1;
    var prev_down = false;
    var elapsed: f64 = 0;

    while (elapsed < secs) {
        while (true) {
            switch (window.poll()) {
                .none => break,
                .close => {
                    elapsed = secs;
                    break;
                },
                .resize => |r| {
                    window.resize(r.w, r.h);
                    W = r.w;
                    H = r.h;
                },
                .mouse_move => |m| {
                    mx = m.x;
                    my = m.y;
                },
                .mouse_down => |m| {
                    mx = m.x;
                    my = m.y;
                    down = true;
                },
                .mouse_up => down = false,
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                .expose => {},
            }
        }
        const fmx: f32 = @floatFromInt(mx);
        const fmy: f32 = @floatFromInt(my);
        const Wf: f32 = @floatFromInt(W);
        const Hf: f32 = @floatFromInt(H);

        g.begin(W, H, bg1);
        g.rectGrad(0, 0, Wf, Hf, 0, bg0, bg1, 0, clear);
        u.begin(.{ .mx = fmx, .my = fmy, .mouse_down = down }, 0.016);

        c.begin(Wf, Hf, fmx, fmy, down, 0.016);
        c.open(.{ .dir = .col, .w = px(Wf), .h = px(Hf), .pad = 28, .gap = 18 });
        {
            // header
            c.open(.{ .dir = .row, .w = grow(), .aligni = .center, .gap = 12 });
            {
                c.label("Zenith", &fd, accent, .{});
                c.label("UI Toolkit", &fd, txt, .{});
                c.box(.{ .w = grow() });
                c.label("GPU - SDF - GLASS - FLEX", &fc, faint, .{ .tracking = 2 });
            }
            c.close();

            // typeface variety
            c.label("TYPEFACES  ( baked + runtime .ttf loader — 563 installed )", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.box(.{ .w = grow(), .h = px(118), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 14, .id = 950 });

            // gradients / color blends row
            c.label("GRADIENTS & COLOR BLENDS", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.open(.{ .dir = .row, .w = grow(), .h = px(64), .gap = 12 });
            {
                for (GRADS, 0..) |_, i| c.box(.{ .w = grow(), .h = grow(), .radius = 12, .id = 700 + @as(u64, i) });
            }
            c.close();

            // controls row
            c.label("CONTROLS  ( drag them )", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.open(.{ .dir = .row, .w = grow(), .h = px(190), .gap = 14 });
            {
                // knobs card
                c.open(.{ .dir = .row, .w = grow(), .h = grow(), .radius = 14, .pad = 16, .gap = 14, .justify = .center, .aligni = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.box(.{ .w = px(46), .h = px(46), .id = 100 });
                    c.box(.{ .w = px(46), .h = px(46), .id = 101 });
                    c.box(.{ .w = px(46), .h = px(46), .id = 102 });
                }
                c.close();
                // faders card
                c.open(.{ .dir = .row, .w = grow(), .h = grow(), .radius = 14, .pad = 16, .gap = 20, .justify = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.box(.{ .w = px(10), .h = grow(), .id = 110 });
                    c.box(.{ .w = px(10), .h = grow(), .id = 111 });
                    c.box(.{ .w = px(10), .h = grow(), .id = 112 });
                }
                c.close();
                // sliders + toggles card
                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 14, .pad = 16, .gap = 16, .justify = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.box(.{ .w = grow(), .h = px(8), .id = 120 });
                    c.box(.{ .w = grow(), .h = px(8), .id = 121 });
                    c.open(.{ .dir = .row, .gap = 14, .aligni = .center });
                    {
                        c.box(.{ .w = px(44), .h = px(24), .id = 130 });
                        c.box(.{ .w = px(44), .h = px(24), .id = 131 });
                    }
                    c.close();
                }
                c.close();
            }
            c.close();

            // components row (checkbox, tabs/segmented, progress)
            c.label("COMPONENTS", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.open(.{ .dir = .row, .w = grow(), .h = px(86), .gap = 14 });
            {
                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 14, .pad = 16, .gap = 12, .justify = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.open(.{ .dir = .row, .gap = 12, .aligni = .center });
                    {
                        c.box(.{ .w = px(20), .h = px(20), .id = 140 });
                        c.label("Reverb send", &fb, txt, .{});
                    }
                    c.close();
                    c.open(.{ .dir = .row, .gap = 12, .aligni = .center });
                    {
                        c.box(.{ .w = px(20), .h = px(20), .id = 141 });
                        c.label("Sidechain", &fb, dim, .{});
                    }
                    c.close();
                }
                c.close();
                c.open(.{ .dir = .col, .w = groww(2), .h = grow(), .radius = 14, .pad = 16, .gap = 16, .justify = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.box(.{ .w = grow(), .h = px(34), .id = 150 }); // segmented / tabs
                    c.box(.{ .w = grow(), .h = px(8), .id = 160 }); // progress
                }
                c.close();
            }
            c.close();

            // rendered data table
            c.label("RENDERED TABLE  ( click a row )", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.box(.{ .w = grow(), .h = px(238), .id = 960 });

            // image import (PNG raster decode + SVG vector rasterize, both ours)
            c.label("IMAGE IMPORT  ( PNG raster + SVG vector — alpha )", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
            c.open(.{ .dir = .row, .w = grow(), .h = px(184), .gap = 14 });
            {
                c.box(.{ .w = px(296), .h = grow(), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16, .id = 970 });
                c.box(.{ .w = px(296), .h = grow(), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16, .id = 971 });
                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .radius = 14, .pad = 18, .gap = 8, .justify = .center, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 16 });
                {
                    c.label("Image pipeline", &fu, txt, .{});
                    c.label("PNG raster + SVG vector -> sRGB texture", &fb, dim, .{});
                    c.label("PNG: gray/RGB/palette/RGBA, all filters", &fc, faint, .{});
                    c.label("SVG: paths, shapes, fills, AA", &fc, faint, .{});
                    c.label("tint + opacity, premultiplied", &fc, faint, .{});
                }
                c.close();
            }
            c.close();

            // shadows / elevation + icons
            c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 14 });
            {
                // elevation card
                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .gap = 8 });
                {
                    c.label("ELEVATION", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
                    c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 14, .aligni = .center, .justify = .center });
                    {
                        c.box(.{ .w = px(64), .h = px(64), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 6 });
                        c.box(.{ .w = px(64), .h = px(64), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 14 });
                        c.box(.{ .w = px(64), .h = px(64), .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 24 });
                    }
                    c.close();
                }
                c.close();
                // icons
                c.open(.{ .dir = .col, .w = groww(2), .h = grow(), .gap = 8 });
                {
                    c.label("ICONS", &fc, faint, .{ .h = px(20), .tracking = 1.4 });
                    c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 10, .aligni = .center, .pad = 8, .radius = 14, .bg = card_t, .bg2 = card_b, .elev = 1, .shadow = 14 });
                    {
                        for (ICONS, 0..) |_, i| c.box(.{ .w = px(30), .h = px(30), .id = 800 + @as(u64, i) });
                    }
                    c.close();
                }
                c.close();
            }
            c.close();
        }
        c.close();
        c.end();

        // ---- widgets into the solved rects --------------------------------
        for (0..3) |i| if (c.rectOf(100 + @as(u64, i))) |r| {
            _ = u.knob(@intCast(100 + i), r[0] + r[2] / 2, r[1] + r[3] / 2, r[2] / 2, &v.knob[i]);
        };
        for (0..3) |i| if (c.rectOf(110 + @as(u64, i))) |r| {
            _ = u.vFader(@intCast(110 + i), r[0], r[1], r[2], r[3], &v.fader[i]);
        };
        if (c.rectOf(120)) |r| _ = u.hSlider(120, r[0], r[1], r[2], r[3], &v.slider[0], 0, 1); // standard
        if (c.rectOf(121)) |r| _ = u.hSliderBipolar(121, r[0], r[1], r[2], r[3], &v.slider[1], -1, 1); // bipolar (centered)
        for (0..2) |i| if (c.rectOf(130 + @as(u64, i))) |r| {
            _ = u.toggle(@intCast(130 + i), r[0], r[1], r[2], r[3], &v.toggle[i]);
        };
        // components: checkboxes, segmented/tabs, progress
        if (c.rectOf(140)) |r| _ = u.checkbox(140, r[0], r[1], r[2], &v.check[0]);
        if (c.rectOf(141)) |r| _ = u.checkbox(141, r[0], r[1], r[2], &v.check[1]);
        if (c.rectOf(150)) |r| _ = u.segmented(150, r[0], r[1], r[2], r[3], &v.seg, &[_][]const u8{ "Arrange", "Mix", "Edit" }, &fb);
        if (c.rectOf(160)) |r| {
            v.prog = frac(v.prog + 0.003); // animate progress
            u.progress(r[0], r[1], r[2], r[3], v.prog);
        }
        // gradient swatches
        for (GRADS, 0..) |gr, i| if (c.rectOf(700 + @as(u64, i))) |r| {
            g.rectGrad(r[0], r[1], r[2], r[3], 12, gr[0], gr[1], 0, clear);
        };
        // icons
        for (ICONS, 0..) |icn, i| if (c.rectOf(800 + @as(u64, i))) |r| {
            const hov = fmx >= r[0] and fmx < r[0] + r[2] and fmy >= r[1] and fmy < r[1] + r[3];
            if (hov) g.rect(r[0], r[1], r[2], r[3], 7, Color.rgba(108, 147, 244, 50));
            icn.f(&g, r[0] + r[2] / 2, r[1] + r[3] / 2, 16, if (hov) txt else dim);
        };
        // rendered data table
        if (c.rectOf(960)) |r| {
            _ = u.table(960, r[0], r[1], &TCOLS, &TROWS, &v.trow, &fc, &fb);
        }
        // imported images — fit (contain) inside the slot, preserving aspect:
        // slot 970 = decoded PNG (raster), slot 971 = rasterized SVG (vector).
        const drawn = [_]struct { id: u64, im: ?*gpu2d.GpuImage }{
            .{ .id = 970, .im = if (sample_img) |*p| p else null },
            .{ .id = 971, .im = if (svg_img) |*p| p else null },
        };
        for (drawn) |dd| {
            const im = dd.im orelse continue;
            if (c.rectOf(dd.id)) |r| {
                const imw: f32 = @floatFromInt(im.w);
                const imh: f32 = @floatFromInt(im.h);
                const pad: f32 = 14;
                const s = @min((r[2] - 2 * pad) / imw, (r[3] - 2 * pad) / imh);
                const dw = imw * s;
                const dh = imh * s;
                g.image(im, r[0] + (r[2] - dw) / 2, r[1] + (r[3] - dh) / 2, dw, dh, Color.white);
            }
        }
        // typeface variety — one sample line per face, 2 columns x 3 rows.
        // left column = baked fonts, right column = runtime-rasterized .ttf.
        if (c.rectOf(950)) |r| {
            const colw = (r[2] - 32) / 2;
            for (faces, 0..) |fe, i| {
                const col: f32 = @floatFromInt(i / 3);
                const row: f32 = @floatFromInt(i % 3);
                const fx = r[0] + 18 + col * (colw + 4);
                const fy = r[1] + 12 + row * 34;
                fc.text(&g, fx, fy, fe.n, if (fe.rt) Color.rgb(120, 210, 170) else accent);
                if (fe.rt) fc.text(&g, fx + fc.textWidth(fe.n) + 8, fy, "runtime .ttf", faint);
                fe.f.text(&g, fx, fy + 13, "The quick brown fox 0123", txt);
            }
        }

        u.end();
        g.flush();

        // glass panel over the gradients (refraction demo)
        if (c.rectOf(700)) |r0| {
            if (c.rectOf(705)) |r5| {
                const gw: f32 = 240;
                const gh: f32 = 56;
                const gx = (r0[0] + r5[0] + r5[2]) / 2 - gw / 2;
                const gy = r0[1] - 4;
                g.captureBlur(W, H);
                g.glass(gx, gy, gw, gh, 14, Color.rgba(84, 92, 114, 70), Color.rgba(255, 255, 255, 150));
                fu.text(&g, gx + 18, gy + (gh - 16) / 2, "Liquid Glass", txt);
                g.flush();
            }
        }

        // animated glass dropdown (top-right)
        menu_anim += (@as(f32, if (menu_open) 1 else 0) - menu_anim) * 0.28;
        const released = !down and prev_down;
        prev_down = down;
        if (menu_anim > 0.01) {
            const items = [_][]const u8{ "Hover me", "Frosted glass", "Drag the knobs", "Esc to quit" };
            const iw: f32 = 196;
            const ih: f32 = 34;
            const eased = menu_anim * menu_anim * (3.0 - 2.0 * menu_anim);
            const hh = (ih * items.len + 12) * eased;
            const mxp = Wf - iw - 28;
            const myp: f32 = 62;
            g.shadow(mxp, myp, iw, hh, 14, 22, Color.rgba(0, 0, 0, @intFromFloat(150 * eased)));
            g.flush();
            g.captureBlur(W, H);
            g.glass(mxp, myp, iw, hh, 14, Color.rgba(84, 92, 114, 76), Color.rgba(255, 255, 255, 150));
            for (items, 0..) |it, i| {
                const iy = myp + 6 + @as(f32, @floatFromInt(i)) * ih;
                if (iy + ih > myp + hh - 2) continue;
                const hov = fmx >= mxp + 6 and fmx < mxp + iw - 6 and fmy >= iy and fmy < iy + ih;
                if (hov) {
                    g.rect(mxp + 6, iy, iw - 12, ih, 8, Color.rgba(108, 147, 244, 55));
                    if (released) menu_open = false;
                }
                fb.text(&g, mxp + 16, iy + (ih - 15) / 2, it, if (hov) txt else dim);
            }
            g.flush();
        }

        // draggable floating chip — pick it up and it follows the cursor with a
        // lift (growing shadow + slight scale), the web drag-and-drop affordance.
        const chip_hover = blk: {
            const cw: f32 = 138;
            const ch: f32 = 42;
            const hov = fmx >= v.drag_x and fmx < v.drag_x + cw and fmy >= v.drag_y and fmy < v.drag_y + ch;
            if (hov and u.pressed) {
                v.dragging = true;
                v.grab_dx = fmx - v.drag_x;
                v.grab_dy = fmy - v.drag_y;
            }
            if (!down) v.dragging = false;
            if (v.dragging) {
                v.drag_x = std.math.clamp(fmx - v.grab_dx, 0, Wf - cw);
                v.drag_y = std.math.clamp(fmy - v.grab_dy, 0, Hf - ch);
            }
            v.drag_lift += ((if (v.dragging) @as(f32, 1) else 0) - v.drag_lift) * 0.25;
            const L = v.drag_lift;
            const x = v.drag_x - L * 2;
            const y = v.drag_y - L * 3;
            const w = cw + L * 4;
            const h = ch + L * 4;
            g.shadow(x, y + 5 + L * 8, w, h, 13, 9 + L * 16, Color.rgba(0, 0, 0, @intFromFloat(70 + L * 70)));
            g.card(x, y, w, h, 13, lerp(Color.rgb(58, 64, 82), Color.rgb(74, 84, 110), L), lerp(Color.rgb(44, 49, 64), Color.rgb(58, 66, 88), L), 0, clear, 1.0 + L);
            // grip dots
            var gd: usize = 0;
            while (gd < 6) : (gd += 1) {
                const col: f32 = @floatFromInt(gd % 2);
                const rowd: f32 = @floatFromInt(gd / 2);
                g.rect(x + 16 + col * 6, y + h / 2 - 7 + rowd * 6, 3, 3, 1.5, Color.rgba(255, 255, 255, 120));
            }
            fu.text(&g, x + 36, y + (h - 16) / 2, "Drag me", txt);
            g.flush();
            break :blk hov;
        };

        // custom cursors: grabbing while dragging, hand when hovering a control/chip
        window.setCursor(if (v.dragging or u.active != 0) .grabbing else if (chip_hover or u.hot != 0) .hand else .default);

        g.grain(W, H, 0.014); // subtle film grain finish
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
