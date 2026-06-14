//! midi2.zig — MIDI 2.0 / Universal MIDI Packet (UMP), pure Zig.
//!
//! Implements the current MIDI 2.0 standard as published by AMEI / The MIDI
//! Association: **M2-104-UM "UMP Format & MIDI 2.0 Protocol" v1.1.2 (2023-10-27)**.
//! All eight defined UMP Message Types are covered — Utility, System Real
//! Time/Common, MIDI 1.0 Channel Voice, Data (SysEx7), MIDI 2.0 Channel Voice,
//! Data (SysEx8 / Mixed Data Set), Flex Data, and UMP Stream — with builders,
//! decoders, the spec's min-center-max value scaling, and the default MIDI 1.0
//! <-> 2.0 translation.
//!
//! A UMP is up to four big-endian 32-bit words; `Ump.len` is the word count.
//! Word 0 layout (channel-voice family): MT[31:28] group[27:24] status[23:16]
//! (= opcode[23:20] channel[19:16]) p1[15:8] p2[7:0].
//!
//! Original implementation. The packet layouts are the spec's; the code is ours.

const std = @import("std");

// ===========================================================================
// Core packet
// ===========================================================================

/// First nibble of word 0 — the UMP Message Type, which fixes the packet size.
pub const MessageType = enum(u4) {
    utility = 0x0, // 32-bit
    system = 0x1, // 32-bit  (System Real Time / Common)
    midi1_channel_voice = 0x2, // 32-bit  (legacy 7-bit protocol carried in a UMP)
    data_64 = 0x3, // 64-bit  (SysEx7)
    midi2_channel_voice = 0x4, // 64-bit  (the MIDI 2.0 Protocol)
    data_128 = 0x5, // 128-bit (SysEx8 / Mixed Data Set)
    flex_data = 0xD, // 128-bit (tempo, time-sig, key, chord, lyrics, ...)
    stream = 0xF, // 128-bit (UMP Stream — endpoint/function-block discovery)
    _,

    /// Number of 32-bit words this message type occupies (per the spec's size map).
    pub fn words(self: MessageType) u8 {
        return switch (@intFromEnum(self)) {
            0x0, 0x1, 0x2, 0x6, 0x7 => 1,
            0x3, 0x4, 0x8, 0x9, 0xA => 2,
            0xB, 0xC => 3,
            0x5, 0xD, 0xE, 0xF => 4,
        };
    }
};

/// A Universal MIDI Packet: up to four 32-bit words. `len` is the valid count.
pub const Ump = struct {
    words: [4]u32 = .{ 0, 0, 0, 0 },
    len: u8 = 1,

    pub fn messageType(self: Ump) MessageType {
        return @enumFromInt(@as(u4, @intCast(self.words[0] >> 28)));
    }
    pub fn group(self: Ump) u4 {
        return @intCast((self.words[0] >> 24) & 0xF);
    }
    /// Channel-voice opcode (high nibble of the status byte).
    pub fn opcode(self: Ump) u4 {
        return @intCast((self.words[0] >> 20) & 0xF);
    }
    pub fn channel(self: Ump) u4 {
        return @intCast((self.words[0] >> 16) & 0xF);
    }
    /// Full status byte (system / MIDI 1.0 CV use this directly).
    pub fn status(self: Ump) u8 {
        return @intCast((self.words[0] >> 16) & 0xFF);
    }
    /// Serialize the valid words big-endian into `out` (4*len bytes written).
    pub fn toBytes(self: Ump, out: []u8) usize {
        var i: usize = 0;
        while (i < self.len) : (i += 1) {
            std.mem.writeInt(u32, out[i * 4 ..][0..4], self.words[i], .big);
        }
        return self.len * 4;
    }
};

// ===========================================================================
// MIDI 2.0 Channel Voice opcodes (Message Type 0x4)
// ===========================================================================

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

fn cv2(grp: u4, op: Opcode, ch: u4, b2: u8, b3: u8, data: u32) Ump {
    const w0 = (@as(u32, 0x4) << 28) |
        (@as(u32, grp) << 24) |
        (@as(u32, @intFromEnum(op)) << 20) |
        (@as(u32, ch) << 16) |
        (@as(u32, b2) << 8) |
        @as(u32, b3);
    return .{ .words = .{ w0, data, 0, 0 }, .len = 2 };
}

/// Note On with 16-bit velocity. `attr_type`/`attr` = MIDI 2.0 note attribute
/// (0 none, 1 manufacturer, 2 profile, 3 Pitch 7.9).
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
/// Assignable Per-Note Controller.
pub fn assignablePerNoteController(grp: u4, ch: u4, note: u7, index: u8, value: u32) Ump {
    return cv2(grp, .assignable_per_note_controller, ch, note, index, value);
}
/// Per-Note Management (detach / reset controllers for a note). `flags` bit1 =
/// detach, bit0 = reset/set-to-default.
pub fn perNoteManagement(grp: u4, ch: u4, note: u7, flags: u8) Ump {
    return cv2(grp, .per_note_management, ch, note, flags, 0);
}

// ===========================================================================
// MIDI 1.0 Channel Voice in a UMP (Message Type 0x2)
// ===========================================================================

pub fn midi1(grp: u4, statusbyte: u8, d1: u7, d2: u7) Ump {
    const w0 = (@as(u32, 0x2) << 28) | (@as(u32, grp) << 24) |
        (@as(u32, statusbyte) << 16) | (@as(u32, d1) << 8) | @as(u32, d2);
    return .{ .words = .{ w0, 0, 0, 0 }, .len = 1 };
}

// ===========================================================================
// System Real Time / Common (Message Type 0x1)
// ===========================================================================

pub const System = struct {
    pub const timing_clock: u8 = 0xF8;
    pub const start: u8 = 0xFA;
    pub const cont: u8 = 0xFB;
    pub const stop: u8 = 0xFC;
    pub const active_sensing: u8 = 0xFE;
    pub const reset: u8 = 0xFF;
    pub const song_position: u8 = 0xF2;
    pub const song_select: u8 = 0xF3;
    pub const tune_request: u8 = 0xF6;
};
pub fn system(grp: u4, statusbyte: u8, d1: u8, d2: u8) Ump {
    const w0 = (@as(u32, 0x1) << 28) | (@as(u32, grp) << 24) |
        (@as(u32, statusbyte) << 16) | (@as(u32, d1) << 8) | @as(u32, d2);
    return .{ .words = .{ w0, 0, 0, 0 }, .len = 1 };
}

// ===========================================================================
// Utility messages (Message Type 0x0) — NOOP + Jitter-Reduction timestamps
// ===========================================================================

pub const Utility = enum(u4) {
    noop = 0x0,
    jr_clock = 0x1,
    jr_timestamp = 0x2,
    delta_clockstamp_tpqn = 0x3,
    delta_clockstamp = 0x4,
};
fn util(sub: Utility, data: u32) Ump {
    const w0 = (@as(u32, @intFromEnum(sub)) << 20) | (data & 0x000FFFFF);
    return .{ .words = .{ w0, 0, 0, 0 }, .len = 1 };
}
pub fn noop() Ump {
    return .{ .words = .{ 0, 0, 0, 0 }, .len = 1 };
}
/// JR Clock — a sender's 16-bit clock time (1/31250 s ticks) for jitter reduction.
pub fn jrClock(clock_time: u16) Ump {
    return util(.jr_clock, clock_time);
}
/// JR Timestamp — 16-bit timestamp applied to the following message(s).
pub fn jrTimestamp(timestamp: u16) Ump {
    return util(.jr_timestamp, timestamp);
}
/// Delta Clockstamp: ticks since the last Delta Clockstamp (20-bit).
pub fn deltaClockstamp(ticks: u20) Ump {
    return util(.delta_clockstamp, ticks);
}
/// Delta Clockstamp Ticks Per Quarter Note.
pub fn deltaClockstampTPQN(tpqn: u16) Ump {
    return util(.delta_clockstamp_tpqn, tpqn);
}

// ===========================================================================
// Data messages — SysEx7 (MT 0x3, 64-bit) and SysEx8 (MT 0x5, 128-bit)
// ===========================================================================

/// SysEx packet status (the 4-bit "format" field): how this packet sits in a
/// possibly multi-packet System Exclusive stream.
pub const SysExStatus = enum(u4) { complete = 0x0, start = 0x1, cont = 0x2, end = 0x3 };

/// One SysEx7 packet carrying up to 6 7-bit data bytes (a chunk of a larger
/// SysEx if `st` != complete).
pub fn sysex7(grp: u4, st: SysExStatus, data: []const u8) Ump {
    const n: u8 = @intCast(@min(data.len, 6));
    var b: [6]u8 = .{ 0, 0, 0, 0, 0, 0 };
    for (0..n) |i| b[i] = data[i] & 0x7F;
    const w0 = (@as(u32, 0x3) << 28) | (@as(u32, grp) << 24) |
        (@as(u32, @intFromEnum(st)) << 20) | (@as(u32, n) << 16) |
        (@as(u32, b[0]) << 8) | @as(u32, b[1]);
    const w1 = (@as(u32, b[2]) << 24) | (@as(u32, b[3]) << 16) |
        (@as(u32, b[4]) << 8) | @as(u32, b[5]);
    return .{ .words = .{ w0, w1, 0, 0 }, .len = 2 };
}

/// One SysEx8 packet: up to 13 full 8-bit data bytes + an 8-bit stream id,
/// across four words (the 8-bit-clean SysEx of MIDI 2.0).
pub fn sysex8(grp: u4, st: SysExStatus, stream_id: u8, data: []const u8) Ump {
    const n: u8 = @intCast(@min(data.len, 13));
    var bytes: [16]u8 = [_]u8{0} ** 16;
    bytes[0] = (@as(u8, 0x5) << 4) | @as(u8, grp);
    bytes[1] = (@as(u8, @intFromEnum(st)) << 4) | (n + 1); // numBytes incl. stream id
    bytes[2] = stream_id;
    for (0..n) |i| bytes[3 + i] = data[i];
    var w: [4]u32 = undefined;
    for (0..4) |i| w[i] = std.mem.readInt(u32, bytes[i * 4 ..][0..4], .big);
    return .{ .words = w, .len = 4 };
}

// ===========================================================================
// Flex Data (Message Type 0xD) — tempo, time signature, key, chord, metadata
// ===========================================================================

pub const FlexFormat = enum(u2) { complete = 0, start = 1, cont = 2, end = 3 };
pub const FlexAddress = enum(u2) { channel = 0, group = 1 };

fn flex(grp: u4, fmt: FlexFormat, addr: FlexAddress, ch: u4, bank: u8, st: u8, d1: u32, d2: u32, d3: u32) Ump {
    const w0 = (@as(u32, 0xD) << 28) | (@as(u32, grp) << 24) |
        (@as(u32, @intFromEnum(fmt)) << 22) | (@as(u32, @intFromEnum(addr)) << 20) |
        (@as(u32, ch) << 16) | (@as(u32, bank) << 8) | @as(u32, st);
    return .{ .words = .{ w0, d1, d2, d3 }, .len = 4 };
}

/// Set Tempo (Flex Data, bank 0x00 status 0x00). `ten_ns_per_qn` = number of
/// 10-nanosecond units per quarter note (the spec's tempo unit).
pub fn setTempo(grp: u4, ten_ns_per_qn: u32) Ump {
    return flex(grp, .complete, .group, 0, 0x00, 0x00, ten_ns_per_qn, 0, 0);
}
/// Set Time Signature (bank 0x00 status 0x01).
pub fn setTimeSignature(grp: u4, numerator: u8, denominator: u8, num_32nd_per_beat: u8) Ump {
    const d1 = (@as(u32, numerator) << 24) | (@as(u32, denominator) << 16) | (@as(u32, num_32nd_per_beat) << 8);
    return flex(grp, .complete, .group, 0, 0x00, 0x01, d1, 0, 0);
}
/// Set Key Signature (bank 0x00 status 0x05). `sharps_flats` is two's-complement
/// in a nibble (−7..+7), `tonic` 0..11.
pub fn setKeySignature(grp: u4, addr: FlexAddress, ch: u4, sharps_flats: i8, tonic: u8) Ump {
    const sf: u8 = @as(u8, @bitCast(sharps_flats)) & 0x0F;
    const d1 = (@as(u32, (sf << 4) | (tonic & 0x0F))) << 24;
    return flex(grp, .complete, addr, ch, 0x00, 0x05, d1, 0, 0);
}

// ===========================================================================
// UMP Stream (Message Type 0xF) — endpoint & function-block discovery
// ===========================================================================

pub const StreamStatus = enum(u10) {
    endpoint_discovery = 0x00,
    endpoint_info_notification = 0x01,
    device_identity_notification = 0x02,
    endpoint_name_notification = 0x03,
    product_instance_id_notification = 0x04,
    stream_configuration_request = 0x05,
    stream_configuration_notification = 0x06,
    function_block_discovery = 0x10,
    function_block_info_notification = 0x11,
    function_block_name_notification = 0x12,
    start_of_clip = 0x20,
    end_of_clip = 0x21,
    _,
};
fn streamPkt(fmt: u2, st: StreamStatus, w0lo: u16, d1: u32, d2: u32, d3: u32) Ump {
    const w0 = (@as(u32, 0xF) << 28) | (@as(u32, fmt) << 26) |
        (@as(u32, @intFromEnum(st)) << 16) | @as(u32, w0lo);
    return .{ .words = .{ w0, d1, d2, d3 }, .len = 4 };
}
/// Endpoint Discovery — ask an endpoint to identify itself. `filter` selects the
/// notifications requested (bit0 endpoint info, bit1 device id, bit2 name, ...).
pub fn endpointDiscovery(ump_major: u8, ump_minor: u8, filter: u8) Ump {
    const lo = (@as(u16, ump_major) << 8) | ump_minor;
    return streamPkt(0, .endpoint_discovery, lo, filter, 0, 0);
}
/// Function Block Discovery — query the endpoint's function blocks.
pub fn functionBlockDiscovery(block: u8, filter: u8) Ump {
    const lo = (@as(u16, block) << 8) | filter;
    return streamPkt(0, .function_block_discovery, lo, 0, 0, 0);
}
/// Start / End of a UMP Clip (used by the MIDI Clip File format).
pub fn startOfClip() Ump {
    return streamPkt(0, .start_of_clip, 0, 0, 0, 0);
}
pub fn endOfClip() Ump {
    return streamPkt(0, .end_of_clip, 0, 0, 0, 0);
}

// ===========================================================================
// Decode — MIDI 2.0 Channel Voice into a tagged union
// ===========================================================================

pub const Note = struct { group: u4, channel: u4, note: u7, velocity: u16, attribute_type: u8, attribute: u16 };
pub const Controller = struct { group: u4, channel: u4, index: u32, value: u32 };
pub const PerNote = struct { group: u4, channel: u4, note: u7, value: u32 };
/// A MIDI 2.0 Registered/Assignable Per-Note Controller — a 32-bit controller
/// scoped to ONE sounding note (the native per-note "third dimension"). `index`
/// reuses the CC numbering (74 = brightness/timbre, 71 = resonance, ...).
pub const PerNoteController = struct { group: u4, channel: u4, note: u7, index: u8, value: u32, registered: bool };

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
    per_note_controller: PerNoteController, // registered/assignable per-note CC (MPE timbre etc.)
    system: SystemMsg, // System Real Time / Common (clock, start, stop, song position, ...)
    other: void,
};

/// A decoded System message (UMP type 0x1). `status` is the full status byte —
/// compare against the `System.*` constants (timing_clock/start/cont/stop/...);
/// for song position, data1/data2 = LSB/MSB.
pub const SystemMsg = struct {
    group: u4 = 0,
    status: u8,
    data1: u8 = 0,
    data2: u8 = 0,
};

/// Decode a MIDI 2.0 Channel Voice UMP. Returns `.other` for non-CV packets.
pub fn decode(u: Ump) Message {
    if (u.messageType() == .system) {
        return .{ .system = .{
            .group = u.group(),
            .status = @intCast((u.words[0] >> 16) & 0xFF),
            .data1 = @intCast((u.words[0] >> 8) & 0x7F),
            .data2 = @intCast(u.words[0] & 0x7F),
        } };
    }
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
        .registered_per_note_controller, .assignable_per_note_controller => .{ .per_note_controller = .{
            .group = grp,
            .channel = ch,
            .note = @intCast(b2 & 0x7F),
            .index = b3,
            .value = data,
            .registered = u.opcode() == @intFromEnum(Opcode.registered_per_note_controller),
        } },
        else => .other,
    };
}

// ===========================================================================
// MIDI 2.0 min-center-max value scaling (the spec's algorithm)
// ===========================================================================

/// Widen `src` from `src_bits` to `dst_bits` so min→min, center→center, max→max
/// exactly (not a plain shift, which would leave max short of full-scale).
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

// ===========================================================================
// Default MIDI 1.0 -> MIDI 2.0 translation
// ===========================================================================

/// Translate a classic 3-byte (or 2-byte) MIDI 1.0 Channel Voice message into
/// the equivalent MIDI 2.0 UMP, scaling resolutions per the spec. `status` is
/// the full status byte (opcode | channel). Returns null for non-CV status.
pub fn fromMidi1(grp: u4, status_byte: u8, d1: u8, d2: u8) ?Ump {
    const op: u4 = @intCast(status_byte >> 4);
    const ch: u4 = @intCast(status_byte & 0x0F);
    return switch (op) {
        0x9 => if (d2 == 0)
            noteOff(grp, ch, @intCast(d1 & 0x7F), 0, 0, 0)
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

// ===========================================================================
// Tests — round-trips + the spec's canonical scaling vectors
// ===========================================================================

test "system real-time messages round-trip" {
    const u = system(0, System.start, 0, 0);
    try std.testing.expectEqual(MessageType.system, u.messageType());
    const m = decode(u);
    try std.testing.expect(m == .system);
    try std.testing.expectEqual(@as(u8, System.start), m.system.status);
    // song position with data
    const sp = decode(system(0, System.song_position, 12, 34));
    try std.testing.expectEqual(@as(u8, 12), sp.system.data1);
    try std.testing.expectEqual(@as(u8, 34), sp.system.data2);
}

test "registered per-note controller round-trips (per-note timbre)" {
    const u = registeredPerNoteController(0, 2, 64, 74, 0x8000_0000);
    const m = decode(u);
    try std.testing.expect(m == .per_note_controller);
    try std.testing.expectEqual(@as(u7, 64), m.per_note_controller.note);
    try std.testing.expectEqual(@as(u8, 74), m.per_note_controller.index);
    try std.testing.expect(m.per_note_controller.registered);
    try std.testing.expectEqual(@as(u32, 0x8000_0000), m.per_note_controller.value);
}

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
    const u = noteOn(0xF, 0xF, 127, 0xFFFF, 0, 0);
    try std.testing.expectEqual(@as(u4, 0xF), u.group());
    try std.testing.expectEqual(@as(u4, 0xF), u.channel());
}

test "scaleUp hits min, center, and max exactly" {
    try std.testing.expectEqual(@as(u32, 0x0000), scaleUp(0x00, 7, 16));
    try std.testing.expectEqual(@as(u32, 0x8000), scaleUp(0x40, 7, 16));
    try std.testing.expectEqual(@as(u32, 0xFFFF), scaleUp(0x7F, 7, 16));
    try std.testing.expectEqual(@as(u32, 0x00000000), scaleUp(0x0000, 14, 32));
    try std.testing.expectEqual(@as(u32, 0x80000000), scaleUp(0x2000, 14, 32));
    try std.testing.expectEqual(@as(u32, 0xFFFFFFFF), scaleUp(0x3FFF, 14, 32));
    try std.testing.expectEqual(@as(u32, 0xFFFFFFFF), scaleUp(0x7F, 7, 32));
    try std.testing.expectEqual(@as(u32, 0x80000000), scaleUp(0x40, 7, 32));
}

test "midi 1.0 note on translates with scaled velocity" {
    const u = fromMidi1(0, 0x92, 60, 0x7F).?;
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
    const u = fromMidi1(0, 0xE0, 0x00, 0x40).?;
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
    try std.testing.expectEqual(@as(u8, 4), MessageType.flex_data.words());
    try std.testing.expectEqual(@as(u8, 2), MessageType.data_64.words());
}

test "utility jr timestamp + noop layout" {
    const u = jrTimestamp(0x1234);
    try std.testing.expectEqual(MessageType.utility, u.messageType());
    try std.testing.expectEqual(@as(u32, (0x2 << 20) | 0x1234), u.words[0]);
    try std.testing.expectEqual(@as(u32, 0), noop().words[0]);
}

test "system real-time start" {
    const u = system(0, System.start, 0, 0);
    try std.testing.expectEqual(MessageType.system, u.messageType());
    try std.testing.expectEqual(@as(u8, 0xFA), u.status());
}

test "sysex7 packs length + bytes" {
    const u = sysex7(0, .complete, &.{ 0x7E, 0x7F, 0x06, 0x01 });
    try std.testing.expectEqual(MessageType.data_64, u.messageType());
    try std.testing.expectEqual(@as(u32, 4), (u.words[0] >> 16) & 0xF); // numBytes
    try std.testing.expectEqual(@as(u8, 0x7E), @as(u8, @intCast((u.words[0] >> 8) & 0xFF)));
    try std.testing.expectEqual(@as(u8, 0x06), @as(u8, @intCast((u.words[1] >> 24) & 0xFF)));
}

test "sysex8 carries stream id and 8-bit data" {
    const u = sysex8(0, .complete, 0x42, &.{ 0x80, 0xFF, 0x01 });
    try std.testing.expectEqual(MessageType.data_128, u.messageType());
    try std.testing.expectEqual(@as(u8, 4), u.len);
    try std.testing.expectEqual(@as(u8, 0x42), @as(u8, @intCast((u.words[0] >> 8) & 0xFF))); // stream id
    try std.testing.expectEqual(@as(u8, 0x80), @as(u8, @intCast(u.words[0] & 0xFF))); // first 8-bit-clean data byte (high bit set)
    try std.testing.expectEqual(@as(u8, 0xFF), @as(u8, @intCast((u.words[1] >> 24) & 0xFF))); // second
}

test "flex data set tempo" {
    const u = setTempo(0, 500_000); // 5 ms per qn in 10ns units = 120 BPM
    try std.testing.expectEqual(MessageType.flex_data, u.messageType());
    try std.testing.expectEqual(@as(u8, 0x00), @as(u8, @intCast(u.words[0] & 0xFF))); // status
    try std.testing.expectEqual(@as(u32, 500_000), u.words[1]);
}

test "ump stream endpoint discovery + clip markers" {
    const u = endpointDiscovery(1, 1, 0x1F);
    try std.testing.expectEqual(MessageType.stream, u.messageType());
    try std.testing.expectEqual(@as(u32, 0x1F), u.words[1]); // filter bitmap
    try std.testing.expectEqual(@as(u10, @intFromEnum(StreamStatus.start_of_clip)), @as(u10, @intCast((startOfClip().words[0] >> 16) & 0x3FF)));
}

test "ump serializes big-endian" {
    const u = noteOn(1, 2, 60, 0x8000, 0, 0);
    var buf: [16]u8 = undefined;
    const n = u.toBytes(&buf);
    try std.testing.expectEqual(@as(usize, 8), n);
    try std.testing.expectEqual(@as(u8, 0x41), buf[0]); // MT=4, group=1
    try std.testing.expectEqual(@as(u8, 0x92), buf[1]); // note-on, channel 2
}
