//! font.zig — antialiased font descriptor. When `subpixel` is true the glyph
//! data has 3 horizontal coverage samples per output pixel (LCD R,G,B order).
pub const Font = struct {
    cell_h: usize,
    first: u8,
    subpixel: bool = false,
    advance: []const u8,
    width: []const u8,
    offset: []const u32,
    data: []const u8,
};
