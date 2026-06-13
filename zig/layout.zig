//! layout.zig — composable cut-based layout (à la "rectcut"). Carve strips off a
//! region from any side; split into equal columns/rows. Replaces hand-placed
//! coordinates with a real layout primitive.

pub const Box = struct {
    x: i32,
    y: i32,
    w: i32,
    h: i32,

    pub fn cutTop(self: *Box, amount: i32) Box {
        const r = Box{ .x = self.x, .y = self.y, .w = self.w, .h = amount };
        self.y += amount;
        self.h -= amount;
        return r;
    }
    pub fn cutBottom(self: *Box, amount: i32) Box {
        self.h -= amount;
        return .{ .x = self.x, .y = self.y + self.h, .w = self.w, .h = amount };
    }
    pub fn cutLeft(self: *Box, amount: i32) Box {
        const r = Box{ .x = self.x, .y = self.y, .w = amount, .h = self.h };
        self.x += amount;
        self.w -= amount;
        return r;
    }
    pub fn cutRight(self: *Box, amount: i32) Box {
        self.w -= amount;
        return .{ .x = self.x + self.w, .y = self.y, .w = amount, .h = self.h };
    }

    pub fn shrink(self: Box, p: i32) Box {
        return .{ .x = self.x + p, .y = self.y + p, .w = self.w - 2 * p, .h = self.h - 2 * p };
    }
    pub fn gapTop(self: *Box, g: i32) void {
        self.y += g;
        self.h -= g;
    }
    pub fn gapLeft(self: *Box, g: i32) void {
        self.x += g;
        self.w -= g;
    }

    /// Split into `n` equal-width columns with `gap` between; writes `out[0..n]`.
    pub fn cols(self: Box, n: i32, gap: i32, out: []Box) void {
        const cw = @divTrunc(self.w - gap * (n - 1), n);
        var i: i32 = 0;
        while (i < n) : (i += 1) out[@intCast(i)] = .{ .x = self.x + i * (cw + gap), .y = self.y, .w = cw, .h = self.h };
    }
    /// Split into `n` equal-height rows with `gap`; writes `out[0..n]`.
    pub fn rows(self: Box, n: i32, gap: i32, out: []Box) void {
        const rh = @divTrunc(self.h - gap * (n - 1), n);
        var i: i32 = 0;
        while (i < n) : (i += 1) out[@intCast(i)] = .{ .x = self.x, .y = self.y + i * (rh + gap), .w = self.w, .h = rh };
    }

    pub fn cxi(self: Box) i32 {
        return self.x + @divTrunc(self.w, 2);
    }
    pub fn cyi(self: Box) i32 {
        return self.y + @divTrunc(self.h, 2);
    }
};
