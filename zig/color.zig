//! color.zig — the shared 8-bit RGBA color type. Tiny, dependency-free, so any
//! reusable Zenith component (the GPU renderer, the CPU renderer, the layout
//! engine) can depend on this alone rather than on each other. Original Zig.

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
    /// Untinted white (use as the default tint when drawing images).
    pub const white = Color{ .r = 255, .g = 255, .b = 255, .a = 255 };
};
