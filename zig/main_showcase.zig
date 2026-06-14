//! main_showcase.zig — a gallery of everything the Zenith UI toolkit can do:
//! gradients & color blends, glass (refraction), layered shadows, the full
//! control set with live hover/press, and the icon set. `zig build showcase`.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const flex = @import("flex.zig");
const widgets = @import("widgets.zig");
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
};
fn frac(x: f32) f32 {
    return x - @floor(x);
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1200;
    var H: usize = 760;

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
    var c = flex.Ctx.init(&g, &fb, &fu, &fd);
    var u = widgets.Ui.init(&g);
    var v = Vals{};
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

        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
