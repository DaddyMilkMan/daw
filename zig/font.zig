//! font.zig — antialiased font descriptor (grayscale coverage glyph atlas).
pub const Font = struct {
    cell_h: usize,
    first: u8,
    advance: []const u8,
    width: []const u8,
    offset: []const u32,
    data: []const u8,
};
