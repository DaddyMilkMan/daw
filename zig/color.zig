//! color.zig — the shared 8-bit RGBA color type PLUS a small toolkit of reusable
//! color utilities (lerp, lighten/darken, tint, alpha, and OKLCH so palettes are
//! perceptually even and easy to keep muted/modern). Tiny and dependency-free, so
//! any Zenith component (GPU renderer, layout engine, widgets) can depend on this
//! alone. These are à-la-carte helpers — reach for them; they're not a template.
//! Original Zig.

const std = @import("std");

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
    pub const white = Color{ .r = 255, .g = 255, .b = 255, .a = 255 };
    pub const black = Color{ .r = 0, .g = 0, .b = 0, .a = 255 };

    /// Same RGB, new alpha. `c.alpha(128)` for a 50%-opaque copy.
    pub fn alpha(c: Color, a: u8) Color {
        return .{ .r = c.r, .g = c.g, .b = c.b, .a = a };
    }
    /// Lighten toward white by `amt` (0..1).
    pub fn lighten(c: Color, amt: f32) Color {
        return lerp(c, white, amt);
    }
    /// Darken toward near-black by `amt` (0..1) — keeps a hint of hue (not pure
    /// black), which reads better for SUBTLE gradients than mixing to #000.
    pub fn darken(c: Color, amt: f32) Color {
        return lerp(c, Color.rgb(9, 10, 13), amt);
    }
    /// Mix this color toward another by t — `fg.tint(bg, 0.1)` = an 8–10% tinted
    /// surface (the modern muted-surface technique).
    pub fn tint(c: Color, toward: Color, t: f32) Color {
        return lerp(c, toward, t);
    }
};

/// Linear interpolation between two colors in sRGB-byte space. t in 0..1.
pub fn lerp(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0.0, 1.0);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}
/// Lighten (amt>0, toward white) or darken (amt<0, toward near-black). A single
/// knob for the SUBTLE vertical-sheen gradients modern surfaces use.
pub fn shade(c: Color, amt: f32) Color {
    return if (amt >= 0) c.lighten(amt) else c.darken(-amt);
}

fn srgbEnc(x: f32) u8 {
    const c = std.math.clamp(x, 0.0, 1.0);
    const v = if (c <= 0.0031308) c * 12.92 else 1.055 * std.math.pow(f32, c, 1.0 / 2.4) - 0.055;
    return @intFromFloat(@round(std.math.clamp(v, 0.0, 1.0) * 255.0));
}
/// OKLCH → sRGB Color. Perceptually-uniform: fix hue+chroma and step lightness for
/// even tonal scales; keep **chroma low (~0.06–0.09)** for muted modern category
/// tints (high chroma reads as cheesy/2000s). L 0..1, C ~0..0.4, H in degrees.
/// Works at comptime (build palettes in a `comptime` block) and runtime.
pub fn oklch(L: f32, C: f32, Hdeg: f32) Color {
    const h = Hdeg * std.math.pi / 180.0;
    const a = C * @cos(h);
    const b = C * @sin(h);
    const l_ = L + 0.3963377774 * a + 0.2158037573 * b;
    const m_ = L - 0.1055613458 * a - 0.0638541728 * b;
    const s_ = L - 0.0894841775 * a - 1.2914855480 * b;
    const l = l_ * l_ * l_;
    const m = m_ * m_ * m_;
    const s = s_ * s_ * s_;
    return .{
        .r = srgbEnc(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
        .g = srgbEnc(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
        .b = srgbEnc(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s),
    };
}
