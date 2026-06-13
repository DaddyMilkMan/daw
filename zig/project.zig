//! project.zig — Zenith's project/document model + save/load + undo.
//!
//! Arrangement model: Project → Tracks → Clips → Notes. A Clip sits at a
//! timeline position (frames); its Notes are clip-relative. Pure portable Zig;
//! the on-disk format is explicit little-endian (loads identically on any OS).
//! Undo is snapshot-based, reusing the serializer.

const std = @import("std");

pub const MAGIC = "ZNPR";
pub const VERSION: u32 = 2; // v2: clips

pub const InstrumentKind = enum(u8) { synth = 0, sampler = 1 };

/// A note inside a clip (start/len are clip-relative, in frames).
pub const Note = struct {
    start: u64,
    len: u64,
    pitch: u8,
    velocity: u8,
};

/// A clip placed on a track's timeline at `start` (frames). Notes are relative.
pub const Clip = struct {
    name: std.ArrayList(u8),
    notes: std.ArrayList(Note),
    start: u64 = 0,
    length: u64 = 0,

    pub fn init(a: std.mem.Allocator) Clip {
        return .{ .name = std.ArrayList(u8).init(a), .notes = std.ArrayList(Note).init(a) };
    }
    pub fn deinit(self: *Clip) void {
        self.name.deinit();
        self.notes.deinit();
    }
};

pub const Track = struct {
    name: std.ArrayList(u8),
    sample_path: std.ArrayList(u8), // empty = none (for sampler tracks)
    clips: std.ArrayList(Clip),
    instrument: InstrumentKind = .synth,
    gain: f32 = 0.8,
    pan: f32 = 0.0,
    mute: bool = false,

    pub fn init(a: std.mem.Allocator) Track {
        return .{
            .name = std.ArrayList(u8).init(a),
            .sample_path = std.ArrayList(u8).init(a),
            .clips = std.ArrayList(Clip).init(a),
        };
    }
    pub fn deinit(self: *Track) void {
        self.name.deinit();
        self.sample_path.deinit();
        for (self.clips.items) |*c| c.deinit();
        self.clips.deinit();
    }
    pub fn addClip(self: *Track, name: []const u8, start: u64) !*Clip {
        var c = Clip.init(self.clips.allocator);
        c.start = start;
        try c.name.appendSlice(name);
        try self.clips.append(c);
        return &self.clips.items[self.clips.items.len - 1];
    }
};

pub const Project = struct {
    allocator: std.mem.Allocator,
    tracks: std.ArrayList(Track),
    tempo: f64 = 120.0,
    sample_rate: u32 = 48000,

    pub fn init(a: std.mem.Allocator) Project {
        return .{ .allocator = a, .tracks = std.ArrayList(Track).init(a) };
    }
    pub fn deinit(self: *Project) void {
        for (self.tracks.items) |*t| t.deinit();
        self.tracks.deinit();
    }

    pub fn addTrack(self: *Project, name: []const u8, instrument: InstrumentKind) !*Track {
        var t = Track.init(self.allocator);
        t.instrument = instrument;
        try t.name.appendSlice(name);
        try self.tracks.append(t);
        return &self.tracks.items[self.tracks.items.len - 1];
    }

    // --- serialization ---

    fn writeStr(w: anytype, s: []const u8) !void {
        try w.writeInt(u16, @intCast(s.len), .little);
        try w.writeAll(s);
    }
    fn readStrInto(r: anytype, list: *std.ArrayList(u8)) !void {
        const len = try r.readInt(u16, .little);
        try list.resize(len);
        try r.readNoEof(list.items);
    }

    pub fn serialize(self: *const Project, w: anytype) !void {
        try w.writeAll(MAGIC);
        try w.writeInt(u32, VERSION, .little);
        try w.writeInt(u64, @bitCast(self.tempo), .little);
        try w.writeInt(u32, self.sample_rate, .little);
        try w.writeInt(u32, @intCast(self.tracks.items.len), .little);
        for (self.tracks.items) |t| {
            try writeStr(w, t.name.items);
            try w.writeInt(u8, @intFromEnum(t.instrument), .little);
            try w.writeInt(u32, @bitCast(t.gain), .little);
            try w.writeInt(u32, @bitCast(t.pan), .little);
            try w.writeInt(u8, @as(u8, @intFromBool(t.mute)), .little);
            try writeStr(w, t.sample_path.items);
            try w.writeInt(u32, @intCast(t.clips.items.len), .little);
            for (t.clips.items) |c| {
                try writeStr(w, c.name.items);
                try w.writeInt(u64, c.start, .little);
                try w.writeInt(u64, c.length, .little);
                try w.writeInt(u32, @intCast(c.notes.items.len), .little);
                for (c.notes.items) |nt| {
                    try w.writeInt(u64, nt.start, .little);
                    try w.writeInt(u64, nt.len, .little);
                    try w.writeInt(u8, nt.pitch, .little);
                    try w.writeInt(u8, nt.velocity, .little);
                }
            }
        }
    }

    pub fn deserialize(allocator: std.mem.Allocator, r: anytype) !Project {
        var magic: [4]u8 = undefined;
        try r.readNoEof(&magic);
        if (!std.mem.eql(u8, &magic, MAGIC)) return error.BadMagic;
        _ = try r.readInt(u32, .little); // version

        var p = Project.init(allocator);
        errdefer p.deinit();
        p.tempo = @bitCast(try r.readInt(u64, .little));
        p.sample_rate = try r.readInt(u32, .little);

        const tcount = try r.readInt(u32, .little);
        var ti: u32 = 0;
        while (ti < tcount) : (ti += 1) {
            var t = Track.init(allocator);
            errdefer t.deinit();
            try readStrInto(r, &t.name);
            t.instrument = @enumFromInt(try r.readInt(u8, .little));
            t.gain = @bitCast(try r.readInt(u32, .little));
            t.pan = @bitCast(try r.readInt(u32, .little));
            t.mute = (try r.readInt(u8, .little)) != 0;
            try readStrInto(r, &t.sample_path);
            const ccount = try r.readInt(u32, .little);
            var ci: u32 = 0;
            while (ci < ccount) : (ci += 1) {
                var c = Clip.init(allocator);
                errdefer c.deinit();
                try readStrInto(r, &c.name);
                c.start = try r.readInt(u64, .little);
                c.length = try r.readInt(u64, .little);
                const ncount = try r.readInt(u32, .little);
                var ni: u32 = 0;
                while (ni < ncount) : (ni += 1) {
                    try c.notes.append(.{
                        .start = try r.readInt(u64, .little),
                        .len = try r.readInt(u64, .little),
                        .pitch = try r.readInt(u8, .little),
                        .velocity = try r.readInt(u8, .little),
                    });
                }
                try t.clips.append(c);
            }
            try p.tracks.append(t);
        }
        return p;
    }

    // --- file + snapshot helpers ---

    pub fn save(self: *const Project, path: []const u8) !void {
        var buf = std.ArrayList(u8).init(self.allocator);
        defer buf.deinit();
        try self.serialize(buf.writer());
        const file = try std.fs.cwd().createFile(path, .{});
        defer file.close();
        try file.writeAll(buf.items);
    }
    pub fn load(allocator: std.mem.Allocator, path: []const u8) !Project {
        const bytes = try std.fs.cwd().readFileAlloc(allocator, path, 1 << 28);
        defer allocator.free(bytes);
        var fbs = std.io.fixedBufferStream(bytes);
        return deserialize(allocator, fbs.reader());
    }
    pub fn toBytes(self: *const Project, allocator: std.mem.Allocator) ![]u8 {
        var buf = std.ArrayList(u8).init(allocator);
        errdefer buf.deinit();
        try self.serialize(buf.writer());
        return buf.toOwnedSlice();
    }
    pub fn fromBytes(allocator: std.mem.Allocator, bytes: []const u8) !Project {
        var fbs = std.io.fixedBufferStream(bytes);
        return deserialize(allocator, fbs.reader());
    }
};

/// Snapshot-based undo history.
pub const History = struct {
    allocator: std.mem.Allocator,
    undo_stack: std.ArrayList([]u8),

    pub fn init(a: std.mem.Allocator) History {
        return .{ .allocator = a, .undo_stack = std.ArrayList([]u8).init(a) };
    }
    pub fn deinit(self: *History) void {
        for (self.undo_stack.items) |snap| self.allocator.free(snap);
        self.undo_stack.deinit();
    }
    pub fn checkpoint(self: *History, p: *const Project) !void {
        try self.undo_stack.append(try p.toBytes(self.allocator));
    }
    pub fn undo(self: *History, p: *Project) !bool {
        const snap = self.undo_stack.pop() orelse return false;
        defer self.allocator.free(snap);
        const restored = try Project.fromBytes(self.allocator, snap);
        p.deinit();
        p.* = restored;
        return true;
    }
};

test "project save/load round-trip (clips)" {
    const a = std.testing.allocator;
    var p = Project.init(a);
    defer p.deinit();
    p.tempo = 128.0;
    p.sample_rate = 44100;
    const t = try p.addTrack("Lead", .synth);
    t.pan = -0.3;
    const c = try t.addClip("verse", 48000);
    try c.notes.append(.{ .start = 0, .len = 1000, .pitch = 60, .velocity = 100 });
    try c.notes.append(.{ .start = 1000, .len = 1000, .pitch = 64, .velocity = 90 });

    try p.save("test_proj.zpr");
    defer std.fs.cwd().deleteFile("test_proj.zpr") catch {};
    var q = try Project.load(a, "test_proj.zpr");
    defer q.deinit();

    try std.testing.expectEqual(@as(f64, 128.0), q.tempo);
    try std.testing.expectEqual(@as(usize, 1), q.tracks.items.len);
    try std.testing.expectEqual(@as(usize, 1), q.tracks.items[0].clips.items.len);
    try std.testing.expectEqual(@as(u64, 48000), q.tracks.items[0].clips.items[0].start);
    try std.testing.expectEqualStrings("verse", q.tracks.items[0].clips.items[0].name.items);
    try std.testing.expectEqual(@as(u8, 64), q.tracks.items[0].clips.items[0].notes.items[1].pitch);
    try std.testing.expectApproxEqAbs(@as(f32, -0.3), q.tracks.items[0].pan, 1e-6);
}

test "undo restores prior state" {
    const a = std.testing.allocator;
    var p = Project.init(a);
    defer p.deinit();
    _ = try p.addTrack("A", .synth);
    var hist = History.init(a);
    defer hist.deinit();
    try hist.checkpoint(&p);
    _ = try p.addTrack("B", .sampler);
    try std.testing.expectEqual(@as(usize, 2), p.tracks.items.len);
    try std.testing.expect(try hist.undo(&p));
    try std.testing.expectEqual(@as(usize, 1), p.tracks.items.len);
    try std.testing.expectEqualStrings("A", p.tracks.items[0].name.items);
}
