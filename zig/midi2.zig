//! midi2.zig — MIDI 2.0 Universal MIDI Packet (UMP) + the MIDI 2.0 Protocol.
//!
//! This is the *current* MIDI standard (UMP, MMA/AMEI 2020+): 16 groups × 16
//! channels (256 channels), 16-bit velocity, 32-bit controllers & pitch bend,
//! per-note controllers and per-note pitch bend, and bank-aware program change.
//! Pure Zig, no dependencies — pack/unpack 32/64-bit packets, the spec's
//! min-center-max bit-scaling, and the default MIDI 1.0 → MIDI 2.0 translation.
//!
//! Original implementation. Verified against the spec's canonical test vectors
//! in the tests at the bottom (`zig test zig/midi2.zig`).

const std = @import("std");

/// First nibble of word 0 — the UMP Message Type, which fixes the packet size.
pub const MessageType = enum(u4) {
    utility = 0x0, // 32-bit
    system = 0x1, // 32-bit  (System Real Time / Common)
    midi1_channel_voice = 0x2, // 32-bit  (legacy 7-bit protocol in a UMP)
    data_64 = 0x3, // 64-bit  (SysEx7)
    midi2_channel_voice = 0x4, // 64-bit  (the MIDI 2.0 Protocol)
    data_128 = 0x5, // 128-bit (SysEx8 / Mixed Data Set)
    flex_data = 0xD, // 128-bit
    stream = 0xF, // 128-bit (UMP Stream / endpoint discovery)
    _,

    /// Number of 32-bit words this message type occupies.
    pub fn words(self: MessageType) u8 {
        return switch (@intFromEnum(self)) {
            0x0, 0x1, 0x2 => 1,
            0x3, 0x4 => 2,
            0x5, 0xD, 0xF => 4,
            else => 1,
        };
    }
};

/// MIDI 2.0 Channel Voice opcode (the high nibble of byte 1). All 16 are
/// defined by the protocol; 0x7 is reserved.
pub const Opcode = enum(u4) {
    registered_per_note_controller = 0x0,
    assignable_per_note_controller = 0x1,
    registered_controller = 0x2, // RPN
    assignable_controller = 0x3, // NRPN
    relative_registered_controller = 0x4,
    relative_assignable_controller = 0x5,
    per_note_pitch_bend = 0x6,
    reserved_7 = 0x7,
    note_off = 0x8,
    note_on = 0x9,
    poly_pressure = 0xA,
    control_change = 0xB,
    program_change = 0xC,
    channel_pressure = 0xD,
    pitch_bend = 0xE,
    per_note_management = 0xF,
};

/// A Universal MIDI Packet: up to four 32-bit words, big-endian word order.
/// `len` is the count of valid words (1, 2, or 4).
pub const Ump = struct {
    words: [4]u32 = .{ 0, 0, 0, 0 },
    len: u8 = 1,

    pub fn messageType(self: Ump) MessageType {
        return @enumFromInt(@as(u4, @intCast(self.words[0] >> 28)));
    }
    pub fn group(self: Ump) u4 {
        return @intCast((self.words[0] >> 24) & 0xF);
    }
    /// Channel Voice opcode (valid for MIDI 1.0/2.0 CV message types).
    pub fn opcode(self: Ump) u4 {
        return @intCast((self.words[0] >> 20) & 0xF);
    }
    pub fn channel(self: Ump) u4 {
        return @intCast((self.words[0] >> 16) & 0xF);
    }
};

// ---- MIDI 2.0 Channel Voice builders (Message Type 0x4, two words) ----------

fn cv2(grp: u4, op: Opcode, ch: u4, b2: u8, b3: u8, data: u32) Ump {
    const w0 = (@as(u32, 0x4) << 28) |
        (@as(u32, grp) << 24) |
        (@as(u32, @intFromEnum(op)) << 20) |
        (@as(u32, ch) << 16) |
        (@as(u32, b2) << 8) |
        @as(u32, b3);
    return .{ .words = .{ w0, data, 0, 0 }, .len = 2 };
}

/// Note On with 16-bit velocity. `attr_type`/`attr` are the MIDI 2.0 note
/// attribute (0 = none; 1 = manufacturer; 2 = profile; 3 = Pitch 7.9).
pub fn noteOn(grp: u4, ch: u4, note: u7, velocity: u16, attr_type: u8, attr: u16) Ump {
    return cv2(grp, .note_on, ch, note, attr_type, (@as(u32, velocity) << 16) | attr);
}
pub fn noteOff(grp: u4, ch: u4, note: u7, velocity: u16, attr_type: u8, attr: u16) Ump {
    return cv2(grp, .note_off, ch, note, attr_type, (@as(u32, velocity) << 16) | attr);
}
/// Control Change with a full 32-bit value (vs MIDI 1.0's 7 bits).
pub fn controlChange(grp: u4, ch: u4, index: u7, value: u32) Ump {
    return cv2(grp, .control_change, ch, index, 0, value);
}
pub fn channelPressure(grp: u4, ch: u4, value: u32) Ump {
    return cv2(grp, .channel_pressure, ch, 0, 0, value);
}
pub fn polyPressure(grp: u4, ch: u4, note: u7, value: u32) Ump {
    return cv2(grp, .poly_pressure, ch, note, 0, value);
}
/// Channel Pitch Bend, 32-bit (center = 0x8000_0000).
pub fn pitchBend(grp: u4, ch: u4, value: u32) Ump {
    return cv2(grp, .pitch_bend, ch, 0, 0, value);
}
/// Per-Note Pitch Bend — a distinct bend per sounding note (MPE-style, native).
pub fn perNotePitchBend(grp: u4, ch: u4, note: u7, value: u32) Ump {
    return cv2(grp, .per_note_pitch_bend, ch, note, 0, value);
}
/// Program Change; if `bank` is given the Bank Valid option flag is set.
pub fn programChange(grp: u4, ch: u4, program: u7, bank: ?u14) Ump {
    const opt: u8 = if (bank != null) 0x01 else 0x00; // bit0 = Bank Valid
    const b: u14 = bank orelse 0;
    const data = (@as(u32, program) << 24) |
        (@as(u32, (b >> 7) & 0x7F) << 8) |
        @as(u32, b & 0x7F);
    return cv2(grp, .program_change, ch, 0, opt, data);
}
/// Registered Controller (RPN): bank/index select, 32-bit value.
pub fn registeredController(grp: u4, ch: u4, bank: u7, index: u7, value: u32) Ump {
    return cv2(grp, .registered_controller, ch, bank, index, value);
}
/// Assignable Controller (NRPN): bank/index select, 32-bit value.
pub fn assignableController(grp: u4, ch: u4, bank: u7, index: u7, value: u32) Ump {
    return cv2(grp, .assignable_controller, ch, bank, index, value);
}
/// Registered Per-Note Controller — a controller scoped to one sounding note.
pub fn registeredPerNoteController(grp: u4, ch: u4, note: u7, index: u8, value: u32) Ump {
    return cv2(grp, .registered_per_note_controller, ch, note, index, value);
}

// ---- decode ----------------------------------------------------------------

pub const Note = struct {
    group: u4,
    channel: u4,
    note: u7,
    velocity: u16,
    attribute_type: u8,
    attribute: u16,
};
pub const Controller = struct { group: u4, channel: u4, index: u32, value: u32 };
pub const PerNote = struct { group: u4, channel: u4, note: u7, value: u32 };

/// A decoded MIDI 2.0 Channel Voice message. `other` covers opcodes we don't
/// surface as a struct yet (still fully representable as a raw Ump).
pub const Message = union(enum) {
    note_on: Note,
    note_off: Note,
    poly_pressure: PerNote,
    control_change: Controller,
    channel_pressure: struct { group: u4, channel: u4, value: u32 },
    pitch_bend: struct { group: u4, channel: u4, value: u32 },
    per_note_pitch_bend: PerNote,
    program_change: struct { group: u4, channel: u4, program: u7, bank: ?u14 },
    registered_controller: Controller,
    assignable_controller: Controller,
    other: void,
};

/// Decode a MIDI 2.0 Channel Voice UMP. Returns `.other` for non-CV packets.
pub fn decode(u: Ump) Message {
    if (u.messageType() != .midi2_channel_voice) return .other;
    const grp = u.group();
    const ch = u.channel();
    const b2: u8 = @intCast((u.words[0] >> 8) & 0xFF);
    const b3: u8 = @intCast(u.words[0] & 0xFF);
    const data = u.words[1];
    const note = (Note){
        .group = grp,
        .channel = ch,
        .note = @intCast(b2 & 0x7F),
        .velocity = @intCast(data >> 16),
        .attribute_type = b3,
        .attribute = @intCast(data & 0xFFFF),
    };
    return switch (@as(Opcode, @enumFromInt(u.opcode()))) {
        .note_on => .{ .note_on = note },
        .note_off => .{ .note_off = note },
        .poly_pressure => .{ .poly_pressure = .{ .group = grp, .channel = ch, .note = @intCast(b2 & 0x7F), .value = data } },
        .control_change => .{ .control_change = .{ .group = grp, .channel = ch, .index = b2, .value = data } },
        .channel_pressure => .{ .channel_pressure = .{ .group = grp, .channel = ch, .value = data } },
        .pitch_bend => .{ .pitch_bend = .{ .group = grp, .channel = ch, .value = data } },
        .per_note_pitch_bend => .{ .per_note_pitch_bend = .{ .group = grp, .channel = ch, .note = @intCast(b2 & 0x7F), .value = data } },
        .program_change => .{ .program_change = .{
            .group = grp,
            .channel = ch,
            .program = @intCast((data >> 24) & 0x7F),
            .bank = if (b3 & 0x01 != 0) @as(u14, @intCast(((data >> 8) & 0x7F) << 7 | (data & 0x7F))) else null,
        } },
        .registered_controller => .{ .registered_controller = .{ .group = grp, .channel = ch, .index = (@as(u32, b2) << 8) | b3, .value = data } },
        .assignable_controller => .{ .assignable_controller = .{ .group = grp, .channel = ch, .index = (@as(u32, b2) << 8) | b3, .value = data } },
        else => .other,
    };
}

// ---- MIDI 2.0 min-center-max scaling --------------------------------------

/// The MIDI 2.0 Protocol's value up-scaling: widen `src` from `src_bits` to
/// `dst_bits` so that min→min, center→center, and max→max exactly (not a plain
/// shift, which would leave max short of full-scale). This is the spec's
/// canonical algorithm.
pub fn scaleUp(src: u32, src_bits: u6, dst_bits: u6) u32 {
    if (dst_bits <= src_bits) return src;
    const scale_bits: u6 = dst_bits - src_bits;
    var shifted = src << @intCast(scale_bits);
    const src_center = @as(u32, 1) << @intCast(src_bits - 1);
    if (src <= src_center) return shifted;
    const repeat_bits: u6 = src_bits - 1;
    const repeat_mask = (@as(u32, 1) << @intCast(repeat_bits)) - 1;
    var repeat = src & repeat_mask;
    if (scale_bits > repeat_bits) {
        repeat <<= @intCast(scale_bits - repeat_bits);
    } else {
        repeat >>= @intCast(repeat_bits - scale_bits);
    }
    while (repeat != 0) {
        shifted |= repeat;
        repeat >>= @intCast(repeat_bits);
    }
    return shifted;
}

pub fn vel7to16(v: u7) u16 {
    return @intCast(scaleUp(v, 7, 16));
}
pub fn cc7to32(v: u7) u32 {
    return scaleUp(v, 7, 32);
}
pub fn bend14to32(v: u14) u32 {
    return scaleUp(v, 14, 32);
}

// ---- default MIDI 1.0 -> MIDI 2.0 translation ------------------------------

/// Translate a classic 3-byte (or 2-byte) MIDI 1.0 Channel Voice message into
/// the equivalent MIDI 2.0 UMP, scaling resolutions per the spec. `status` is
/// the full status byte (opcode | channel). Returns null for non-CV status.
pub fn fromMidi1(grp: u4, status: u8, d1: u8, d2: u8) ?Ump {
    const op: u4 = @intCast(status >> 4);
    const ch: u4 = @intCast(status & 0x0F);
    return switch (op) {
        0x9 => if (d2 == 0)
            noteOff(grp, ch, @intCast(d1 & 0x7F), 0, 0, 0) // running-status note-off
        else
            noteOn(grp, ch, @intCast(d1 & 0x7F), vel7to16(@intCast(d2 & 0x7F)), 0, 0),
        0x8 => noteOff(grp, ch, @intCast(d1 & 0x7F), vel7to16(@intCast(d2 & 0x7F)), 0, 0),
        0xA => polyPressure(grp, ch, @intCast(d1 & 0x7F), cc7to32(@intCast(d2 & 0x7F))),
        0xB => controlChange(grp, ch, @intCast(d1 & 0x7F), cc7to32(@intCast(d2 & 0x7F))),
        0xC => programChange(grp, ch, @intCast(d1 & 0x7F), null),
        0xD => channelPressure(grp, ch, cc7to32(@intCast(d1 & 0x7F))),
        0xE => pitchBend(grp, ch, bend14to32(@intCast((@as(u14, d2 & 0x7F) << 7) | (d1 & 0x7F)))),
        else => null,
    };
}

// ---- tests -----------------------------------------------------------------

test "note on packs and round-trips" {
    const u = noteOn(0, 3, 60, 0xC000, 0, 0);
    try std.testing.expectEqual(MessageType.midi2_channel_voice, u.messageType());
    try std.testing.expectEqual(@as(u8, 2), u.len);
    try std.testing.expectEqual(@as(u4, 3), u.channel());
    const m = decode(u);
    try std.testing.expect(m == .note_on);
    try std.testing.expectEqual(@as(u7, 60), m.note_on.note);
    try std.testing.expectEqual(@as(u16, 0xC000), m.note_on.velocity);
}

test "group and 256-channel addressing" {
    const u = noteOn(0xF, 0xF, 127, 0xFFFF, 0, 0); // group 15, channel 15
    try std.testing.expectEqual(@as(u4, 0xF), u.group());
    try std.testing.expectEqual(@as(u4, 0xF), u.channel());
}

test "scaleUp hits min, center, and max exactly" {
    // 7 -> 16
    try std.testing.expectEqual(@as(u32, 0x0000), scaleUp(0x00, 7, 16));
    try std.testing.expectEqual(@as(u32, 0x8000), scaleUp(0x40, 7, 16)); // center
    try std.testing.expectEqual(@as(u32, 0xFFFF), scaleUp(0x7F, 7, 16)); // max
    // 14 -> 32 (pitch bend)
    try std.testing.expectEqual(@as(u32, 0x00000000), scaleUp(0x0000, 14, 32));
    try std.testing.expectEqual(@as(u32, 0x80000000), scaleUp(0x2000, 14, 32)); // center
    try std.testing.expectEqual(@as(u32, 0xFFFFFFFF), scaleUp(0x3FFF, 14, 32)); // max
    // 7 -> 32 (control change)
    try std.testing.expectEqual(@as(u32, 0xFFFFFFFF), scaleUp(0x7F, 7, 32));
    try std.testing.expectEqual(@as(u32, 0x80000000), scaleUp(0x40, 7, 32));
}

test "midi 1.0 note on translates with scaled velocity" {
    const u = fromMidi1(0, 0x92, 60, 0x7F).?; // ch2 note on, max vel
    const m = decode(u);
    try std.testing.expect(m == .note_on);
    try std.testing.expectEqual(@as(u4, 2), m.note_on.channel);
    try std.testing.expectEqual(@as(u16, 0xFFFF), m.note_on.velocity);
}

test "midi 1.0 note on velocity 0 becomes note off" {
    const u = fromMidi1(0, 0x90, 64, 0).?;
    try std.testing.expect(decode(u) == .note_off);
}

test "midi 1.0 pitch bend center maps to 32-bit center" {
    const u = fromMidi1(0, 0xE0, 0x00, 0x40).?; // LSB=0, MSB=0x40 -> 0x2000 center
    const m = decode(u);
    try std.testing.expect(m == .pitch_bend);
    try std.testing.expectEqual(@as(u32, 0x80000000), m.pitch_bend.value);
}

test "program change with bank sets bank-valid and round-trips" {
    const u = programChange(0, 5, 42, 1234);
    const m = decode(u);
    try std.testing.expect(m == .program_change);
    try std.testing.expectEqual(@as(u7, 42), m.program_change.program);
    try std.testing.expectEqual(@as(?u14, 1234), m.program_change.bank);
}

test "control change carries full 32-bit value" {
    const u = controlChange(0, 0, 74, 0x12345678);
    const m = decode(u);
    try std.testing.expect(m == .control_change);
    try std.testing.expectEqual(@as(u32, 0x12345678), m.control_change.value);
    try std.testing.expectEqual(@as(u32, 74), m.control_change.index);
}

test "message type word counts" {
    try std.testing.expectEqual(@as(u8, 2), MessageType.midi2_channel_voice.words());
    try std.testing.expectEqual(@as(u8, 1), MessageType.midi1_channel_voice.words());
    try std.testing.expectEqual(@as(u8, 4), MessageType.stream.words());
}
