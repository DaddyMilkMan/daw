//! pianoroll.zig — a MIDI clip editor over a project.Clip.
//!
//! Grid (key×time) + a velocity lane. Edits:
//!   • click empty cell  -> add a grid-quantized note (and drag to place it)
//!   • click a note       -> delete it
//!   • drag a note body   -> move (pitch + time)
//!   • drag a note's right edge -> resize (length)
//!   • drag in the velocity lane -> set the note's velocity
//! An optional `audition` hook previews notes (note-on on grab, note-off on release)
//! so edits are audible. Pure gpu2d toolkit — drops into the DAW or runs standalone.

const std = @import("std");
const gpu2d = @import("gpu2d.zig");
const project = @import("project.zig");
const Color = gpu2d.Color;
const Gpu = gpu2d.Gpu;
const Font = gpu2d.GpuFont;

const bg = Color.rgb(20, 21, 26);
const lane = Color.rgb(30, 32, 39);
const lane_dark = Color.rgb(25, 26, 32);
const key_white = Color.rgb(222, 226, 233);
const key_black = Color.rgb(40, 43, 52);
const key_label = Color.rgb(120, 126, 138);
const beat_line = Color.rgba(255, 255, 255, 14);
const bar_line = Color.rgba(255, 255, 255, 40);
const note_lo = Color.rgb(60, 84, 150); // low velocity
const note_hi = Color.rgb(132, 168, 255); // high velocity
const vel_bg = Color.rgb(16, 17, 21);

fn isBlack(note: u8) bool {
    return switch (note % 12) {
        1, 3, 6, 8, 10 => true,
        else => false,
    };
}
fn lerpC(a: Color, b: Color, t: f32) Color {
    const tc = std.math.clamp(t, 0, 1);
    return .{
        .r = @intFromFloat(@as(f32, @floatFromInt(a.r)) * (1 - tc) + @as(f32, @floatFromInt(b.r)) * tc),
        .g = @intFromFloat(@as(f32, @floatFromInt(a.g)) * (1 - tc) + @as(f32, @floatFromInt(b.g)) * tc),
        .b = @intFromFloat(@as(f32, @floatFromInt(a.b)) * (1 - tc) + @as(f32, @floatFromInt(b.b)) * tc),
    };
}

pub const AuditionFn = *const fn (ctx: ?*anyopaque, pitch: u8, on: bool) void;

pub const PianoRoll = struct {
    lo: u8 = 48,
    hi: u8 = 83,
    bars: u32 = 4,
    bar_frames: u64 = 48000,
    key_w: f32 = 46,
    div: u32 = 8,
    vel_h: f32 = 64,
    last_edit: enum { none, added, deleted, moved, resized, velocity } = .none,
    drag: ?Drag = null,
    aud_pitch: ?u8 = null,
    audition: ?AuditionFn = null,
    audition_ctx: ?*anyopaque = null,

    const Mode = enum { move, resize, vel };
    const Drag = struct { idx: usize, mode: Mode, col_off: i64 = 0, was_existing: bool = false, moved: bool = false };

    fn cf(self: PianoRoll) u64 {
        return self.bar_frames / self.div;
    }
    fn preview(self: *PianoRoll, pitch: u8, on: bool) void {
        if (self.audition) |f| f(self.audition_ctx, pitch, on);
    }

    pub fn update(self: *PianoRoll, g: *Gpu, fb: *const Font, rect: [4]f32, clip: *project.Clip, mx: f32, my: f32, pressed: bool, down: bool, released: bool) void {
        const x = rect[0];
        const y = rect[1];
        const w = rect[2];
        const ht = rect[3];
        const gx = x + self.key_w;
        const gw = w - self.key_w;
        const grid_h = ht - self.vel_h - 4;
        const vel_y = y + grid_h + 4;
        const nrows: usize = self.hi - self.lo + 1;
        const rh = grid_h / @as(f32, @floatFromInt(nrows));
        const cols = self.bars * self.div;
        const cw = gw / @as(f32, @floatFromInt(cols));
        const cfr = self.cf();

        const colAt = struct {
            fn f(m: f32, gx_: f32, cw_: f32) i64 {
                return @intFromFloat(@floor((m - gx_) / cw_));
            }
        }.f;

        // ---------- interaction (apply before drawing) ----------
        self.last_edit = .none;
        const in_grid = mx >= gx and mx < x + w and my >= y and my < y + grid_h;
        const in_vel = mx >= gx and mx < x + w and my >= vel_y and my < y + ht;

        if (pressed) {
            if (in_vel) {
                // grab the note whose time-span sits under the cursor
                const mframe: f32 = (mx - gx) / cw * @as(f32, @floatFromInt(cfr));
                var found: ?usize = null;
                for (clip.notes.items, 0..) |n, i| {
                    if (@as(f32, @floatFromInt(n.start)) <= mframe and mframe < @as(f32, @floatFromInt(n.start + n.len))) found = i;
                }
                if (found) |i| self.drag = .{ .idx = i, .mode = .vel };
            } else if (in_grid) {
                const col = colAt(mx, gx, cw);
                const row: i64 = @intFromFloat(@floor((my - y) / rh));
                const pitch: u8 = @intCast(std.math.clamp(@as(i64, self.hi) - row, self.lo, self.hi));
                const mframe: f32 = (mx - gx) / cw * @as(f32, @floatFromInt(cfr));
                var hit: ?usize = null;
                for (clip.notes.items, 0..) |n, i| {
                    if (n.pitch == pitch and @as(f32, @floatFromInt(n.start)) <= mframe and mframe < @as(f32, @floatFromInt(n.start + n.len))) hit = i;
                }
                if (hit) |i| {
                    const n = clip.notes.items[i];
                    const end_x = gx + @as(f32, @floatFromInt(n.start + n.len)) / @as(f32, @floatFromInt(cfr)) * cw;
                    if (mx > end_x - 7) {
                        self.drag = .{ .idx = i, .mode = .resize };
                    } else {
                        const ncol: i64 = @intCast(n.start / cfr);
                        self.drag = .{ .idx = i, .mode = .move, .col_off = ncol - col, .was_existing = true };
                    }
                    self.preview(n.pitch, true);
                    self.aud_pitch = n.pitch;
                } else if (col >= 0 and col < cols) {
                    clip.notes.append(.{ .start = @as(u64, @intCast(col)) * cfr, .len = cfr, .pitch = pitch, .velocity = 100 }) catch return;
                    self.drag = .{ .idx = clip.notes.items.len - 1, .mode = .move, .col_off = 0 };
                    self.last_edit = .added;
                    self.preview(pitch, true);
                    self.aud_pitch = pitch;
                }
            }
        }

        if (down) if (self.drag) |*d| {
            if (d.idx < clip.notes.items.len) switch (d.mode) {
                .move => {
                    const col = std.math.clamp(colAt(mx, gx, cw) + d.col_off, 0, @as(i64, cols) - 1);
                    const row: i64 = std.math.clamp(@as(i64, @intFromFloat(@floor((my - y) / rh))), 0, @as(i64, @intCast(nrows)) - 1);
                    const nn = &clip.notes.items[d.idx];
                    const ns: u64 = @as(u64, @intCast(col)) * cfr;
                    const np: u8 = @intCast(@as(i64, self.hi) - row);
                    if (ns != nn.start or np != nn.pitch) {
                        d.moved = true;
                        if (np != nn.pitch) {
                            self.preview(np, true); // retrigger on pitch change
                            self.aud_pitch = np;
                        }
                    }
                    nn.start = ns;
                    nn.pitch = np;
                    if (d.moved) self.last_edit = .moved;
                },
                .resize => {
                    const nn = &clip.notes.items[d.idx];
                    const end_f: f32 = (mx - gx) / cw * @as(f32, @floatFromInt(cfr));
                    const span = @as(i64, @intFromFloat(end_f)) - @as(i64, @intCast(nn.start));
                    const cells = @max(@as(i64, 1), @divFloor(span, @as(i64, @intCast(cfr))) + 1);
                    nn.len = @as(u64, @intCast(cells)) * cfr;
                    d.moved = true;
                    self.last_edit = .resized;
                },
                .vel => {
                    const v = std.math.clamp((vel_y + self.vel_h - my) / self.vel_h, 0, 1) * 127.0;
                    clip.notes.items[d.idx].velocity = @intFromFloat(v);
                    self.last_edit = .velocity;
                },
            };
        };

        if (released) {
            if (self.drag) |d| {
                if (d.mode == .move and !d.moved and d.was_existing and d.idx < clip.notes.items.len) {
                    _ = clip.notes.orderedRemove(d.idx);
                    self.last_edit = .deleted;
                }
            }
            if (self.aud_pitch) |p| self.preview(p, false);
            self.aud_pitch = null;
            self.drag = null;
        }

        // ---------- draw ----------
        g.rect(x, y, w, ht, 0, bg);
        for (0..nrows) |row| {
            const note: u8 = @intCast(@as(usize, self.hi) - row);
            const ry = y + @as(f32, @floatFromInt(row)) * rh;
            const black = isBlack(note);
            g.rect(gx, ry, gw, rh - 1, 0, if (black) lane_dark else lane);
            g.rect(x, ry, self.key_w - 2, rh - 1, 0, if (black) key_black else key_white);
            if (note % 12 == 0) {
                var lab: [8]u8 = undefined;
                const s = std.fmt.bufPrint(&lab, "C{d}", .{note / 12 - 1}) catch "C";
                fb.text(g, x + 5, ry + (rh - fb.cell_h) / 2, s, key_label);
            }
        }
        for (0..cols + 1) |col| {
            const cx = gx + @as(f32, @floatFromInt(col)) * cw;
            const is_bar = col % self.div == 0;
            g.rect(cx, y, if (is_bar) 1.5 else 1.0, grid_h, 0, if (is_bar) bar_line else beat_line);
        }
        for (clip.notes.items) |note| {
            if (note.pitch < self.lo or note.pitch > self.hi) continue;
            const nrow: usize = self.hi - note.pitch;
            const ny = y + @as(f32, @floatFromInt(nrow)) * rh;
            const nx = gx + @as(f32, @floatFromInt(note.start)) / @as(f32, @floatFromInt(cfr)) * cw;
            const nw = @max(@as(f32, @floatFromInt(note.len)) / @as(f32, @floatFromInt(cfr)) * cw, 4);
            const col = lerpC(note_lo, note_hi, @as(f32, @floatFromInt(note.velocity)) / 127.0);
            g.rect(nx + 1, ny + 1, nw - 2, rh - 3, 3, col);
        }
        // velocity lane: a bar per note
        g.rect(gx, vel_y, gw, self.vel_h, 0, vel_bg);
        for (clip.notes.items) |note| {
            const nx = gx + @as(f32, @floatFromInt(note.start)) / @as(f32, @floatFromInt(cfr)) * cw;
            const bh = @as(f32, @floatFromInt(note.velocity)) / 127.0 * (self.vel_h - 4);
            const col = lerpC(note_lo, note_hi, @as(f32, @floatFromInt(note.velocity)) / 127.0);
            g.rect(nx + 2, vel_y + self.vel_h - bh - 2, @max(cw - 4, 3), bh, 1, col);
        }
    }
};

test "piano-roll cell<->note mapping" {
    const a = std.testing.allocator;
    var clip = project.Clip.init(a);
    defer clip.deinit();
    const pr = PianoRoll{ .bar_frames = 48000 };
    const cfr = pr.bar_frames / pr.div;
    try clip.notes.append(.{ .start = 3 * cfr, .len = cfr, .pitch = pr.hi, .velocity = 100 });
    try std.testing.expectEqual(@as(u64, 3 * cfr), clip.notes.items[0].start);
    _ = clip.notes.orderedRemove(0);
    try std.testing.expectEqual(@as(usize, 0), clip.notes.items.len);
}
