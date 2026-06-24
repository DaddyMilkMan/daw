//! sysex.zig — System Exclusive: Universal SysEx classification (device identity,
//! GM on/off, master volume, tuning) + an Identity Reply builder, plus UMP SysEx7
//! (de)packetization so SysEx works on both the MIDI-1.0 and MIDI-2.0 paths.
//! Pure — no I/O; unit-tested without hardware.

const std = @import("std");
const midi2 = @import("midi2.zig");

pub const NON_REALTIME: u8 = 0x7E; // Universal SysEx, non-real-time
pub const REALTIME: u8 = 0x7F; // Universal SysEx, real-time
pub const ALL_DEVICES: u8 = 0x7F; // device id "broadcast"

// Zenith's manufacturer id: 0x7D is the SysEx id reserved for non-commercial /
// educational / development use — exactly right for an in-house DAW.
pub const ZENITH_MANUFACTURER: u8 = 0x7D;
pub const ZENITH_FAMILY: u16 = 0x0001;
pub const ZENITH_MODEL: u16 = 0x0001;
pub const ZENITH_VERSION = [4]u8{ 0, 1, 0, 0 };

pub const Kind = enum {
    identity_request, // F0 7E dd 06 01 F7
    identity_reply, // F0 7E dd 06 02 ... F7
    gm_system_on, // F0 7E dd 09 01 F7
    gm_system_off, // F0 7E dd 09 02 F7
    master_volume, // F0 7F dd 04 01 ll mm F7
    tuning, // F0 7E/7F dd 08 ...
    universal_other,
    manufacturer, // a vendor-specific message
    unknown,
};

pub const Message = struct {
    kind: Kind,
    device: u8 = ALL_DEVICES, // device id / channel (0x7F = all)
    realtime: bool = false, // 0x7F (real-time) vs 0x7E (non-real-time)
    manufacturer: u32 = 0, // vendor id (1-byte, or 3-byte extended with 0x00 prefix)
    value: u16 = 0, // small payload (e.g. master volume 14-bit)
    body: []const u8 = &.{}, // payload bytes (between sub-ids and F7), borrows the input
};

/// Classify a complete SysEx message. Accepts the bytes with or without the
/// surrounding F0 / F7 framing.
pub fn parse(raw: []const u8) Message {
    var b = raw;
    if (b.len > 0 and b[0] == 0xF0) b = b[1..];
    if (b.len > 0 and b[b.len - 1] == 0xF7) b = b[0 .. b.len - 1];
    if (b.len == 0) return .{ .kind = .unknown };

    const id = b[0];
    if (id == NON_REALTIME or id == REALTIME) {
        const rt = id == REALTIME;
        const device: u8 = if (b.len > 1) b[1] else ALL_DEVICES;
        const sub1: u8 = if (b.len > 2) b[2] else 0;
        const sub2: u8 = if (b.len > 3) b[3] else 0;
        const body: []const u8 = if (b.len > 4) b[4..] else &.{};
        const kind: Kind = switch (sub1) {
            0x06 => switch (sub2) { // General Information
                0x01 => .identity_request,
                0x02 => .identity_reply,
                else => .universal_other,
            },
            0x09 => switch (sub2) { // GM System
                0x01 => .gm_system_on,
                0x02 => .gm_system_off,
                else => .universal_other,
            },
            0x04 => if (sub2 == 0x01) .master_volume else .universal_other, // Device Control
            0x08 => .tuning, // MIDI Tuning Standard
            else => .universal_other,
        };
        var msg = Message{ .kind = kind, .device = device, .realtime = rt, .body = body };
        if (kind == .master_volume and b.len >= 6) {
            msg.value = @as(u16, b[4] & 0x7F) | (@as(u16, b[5] & 0x7F) << 7); // LSB, MSB
        }
        if (kind == .identity_reply and body.len >= 1) {
            msg.manufacturer = body[0];
        }
        return msg;
    }

    // Manufacturer-specific. 0x00 prefix => 3-byte extended id.
    if (id == 0x00 and b.len >= 3) {
        return .{ .kind = .manufacturer, .manufacturer = (@as(u32, b[1]) << 8) | b[2], .body = b[3..] };
    }
    return .{ .kind = .manufacturer, .manufacturer = id, .body = b[1..] };
}

/// Write Zenith's MIDI Identity Reply into `buf` (needs >= 15 bytes); returns the
/// written framed message (F0 ... F7). Reply to an `identity_request`.
pub fn identityReply(buf: []u8, device: u8) []u8 {
    const m = [_]u8{
        0xF0, NON_REALTIME, device, 0x06, 0x02, // General Info / Identity Reply
        ZENITH_MANUFACTURER,
        @intCast(ZENITH_FAMILY & 0x7F), @intCast((ZENITH_FAMILY >> 7) & 0x7F),
        @intCast(ZENITH_MODEL & 0x7F),  @intCast((ZENITH_MODEL >> 7) & 0x7F),
        ZENITH_VERSION[0], ZENITH_VERSION[1], ZENITH_VERSION[2], ZENITH_VERSION[3],
        0xF7,
    };
    @memcpy(buf[0..m.len], &m);
    return buf[0..m.len];
}

/// Reassembles UMP SysEx7 packets (MT 0x3) into one contiguous byte stream.
/// Feed each `data_64` UMP; returns the full payload bytes when complete.
pub const Sysex7Assembler = struct {
    buf: [512]u8 = undefined,
    len: usize = 0,
    active: bool = false,

    pub fn reset(self: *Sysex7Assembler) void {
        self.len = 0;
        self.active = false;
    }

    /// Feed one UMP. Returns the assembled bytes (without F0/F7) on completion.
    pub fn feed(self: *Sysex7Assembler, u: midi2.Ump) ?[]const u8 {
        if (u.messageType() != .data_64) return null;
        const st: midi2.SysExStatus = @enumFromInt(@as(u4, @intCast((u.words[0] >> 20) & 0xF)));
        const nbytes: usize = @intCast((u.words[0] >> 16) & 0xF); // byte count (bits 16..19)
        // 6 data bytes spread across the two words after the 16-bit header
        const all = [6]u8{
            @intCast((u.words[0] >> 8) & 0x7F), @intCast(u.words[0] & 0x7F),
            @intCast((u.words[1] >> 24) & 0x7F), @intCast((u.words[1] >> 16) & 0x7F),
            @intCast((u.words[1] >> 8) & 0x7F),  @intCast(u.words[1] & 0x7F),
        };
        if (st == .complete or st == .start) self.reset();
        self.active = true;
        const take = @min(nbytes, all.len);
        for (all[0..take]) |byte| {
            if (self.len < self.buf.len) {
                self.buf[self.len] = byte;
                self.len += 1;
            }
        }
        if (st == .complete or st == .end) {
            self.active = false;
            return self.buf[0..self.len];
        }
        return null;
    }
};

/// Split a SysEx byte payload (without F0/F7) into UMP SysEx7 packets written to
/// `out`; returns the packet count. Handles the complete/start/continue/end framing.
pub fn toSysex7(group: u4, payload: []const u8, out: []midi2.Ump) usize {
    if (payload.len == 0 or out.len == 0) return 0;
    if (payload.len <= 6) {
        out[0] = midi2.sysex7(group, .complete, payload);
        return 1;
    }
    var i: usize = 0;
    var k: usize = 0;
    while (i < payload.len and k < out.len) {
        const n = @min(@as(usize, 6), payload.len - i);
        const st: midi2.SysExStatus = if (i == 0) .start else if (i + n >= payload.len) .end else .cont;
        out[k] = midi2.sysex7(group, st, payload[i .. i + n]);
        k += 1;
        i += n;
    }
    return k;
}

test "parse identity request" {
    const m = parse(&.{ 0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7 });
    try std.testing.expectEqual(Kind.identity_request, m.kind);
    try std.testing.expectEqual(@as(u8, 0x7F), m.device);
}

test "identity reply round-trips" {
    var buf: [32]u8 = undefined;
    const reply = identityReply(&buf, 0x00);
    const m = parse(reply);
    try std.testing.expectEqual(Kind.identity_reply, m.kind);
    try std.testing.expectEqual(@as(u32, ZENITH_MANUFACTURER), m.manufacturer);
}

test "parse GM on / master volume" {
    try std.testing.expectEqual(Kind.gm_system_on, parse(&.{ 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 }).kind);
    const mv = parse(&.{ 0xF0, 0x7F, 0x7F, 0x04, 0x01, 0x00, 0x40, 0xF7 }); // volume MSB 0x40
    try std.testing.expectEqual(Kind.master_volume, mv.kind);
    try std.testing.expect(mv.realtime);
    try std.testing.expectEqual(@as(u16, 0x40 << 7), mv.value);
}

test "manufacturer id (1-byte and 3-byte)" {
    try std.testing.expectEqual(@as(u32, 0x41), parse(&.{ 0xF0, 0x41, 0x10, 0xF7 }).manufacturer); // Roland
    const ext = parse(&.{ 0xF0, 0x00, 0x20, 0x33, 0x01, 0xF7 }); // 3-byte id
    try std.testing.expectEqual(@as(u32, 0x2033), ext.manufacturer);
}

test "sysex7 packetize + reassemble round-trip" {
    const payload = [_]u8{ 0x7E, 0x7F, 0x06, 0x02, 0x7D, 0x01, 0x00, 0x01, 0x00 }; // 9 bytes -> 2 packets
    var pkts: [8]midi2.Ump = undefined;
    const n = toSysex7(0, &payload, &pkts);
    try std.testing.expectEqual(@as(usize, 2), n);
    var assembler = Sysex7Assembler{};
    var out: ?[]const u8 = null;
    for (pkts[0..n]) |u| {
        if (assembler.feed(u)) |bytes| out = bytes;
    }
    try std.testing.expect(out != null);
    try std.testing.expectEqualSlices(u8, &payload, out.?);
}
