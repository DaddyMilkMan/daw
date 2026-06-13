//! midi_alsa.zig — MIDI input via the ALSA sequencer. Creates a "Zenith" input
//! port any MIDI source (keyboard, aplaymidi, another app) can connect to.
//! 100% Zig over libasound; no JUCE.

const std = @import("std");

const snd_seq_t = opaque {};

// snd_seq_open streams / modes
const SND_SEQ_OPEN_INPUT: c_int = 2;
const SND_SEQ_NONBLOCK: c_int = 1;

// port capabilities / types
const SND_SEQ_PORT_CAP_WRITE: c_uint = 1 << 1;
const SND_SEQ_PORT_CAP_SUBS_WRITE: c_uint = 1 << 6; // (1<<5 is SUBS_READ — wrong direction)
const SND_SEQ_PORT_TYPE_MIDI_GENERIC: c_uint = 1 << 1;
const SND_SEQ_PORT_TYPE_APPLICATION: c_uint = 1 << 20;

// event types we care about
const SND_SEQ_EVENT_NOTEON: u8 = 6;
const SND_SEQ_EVENT_NOTEOFF: u8 = 7;

/// Mirror of C `snd_seq_event_t` (32 bytes on 64-bit). We only read `type` and
/// `data.note`, but the full layout must match so the offsets are correct.
const SeqEvent = extern struct {
    type: u8,
    flags: u8,
    tag: u8,
    queue: u8,
    time: [8]u8, // snd_seq_timestamp_t (union, opaque to us)
    source: [2]u8, // snd_seq_addr_t
    dest: [2]u8, // snd_seq_addr_t
    data: extern union {
        note: extern struct {
            channel: u8,
            note: u8,
            velocity: u8,
            off_velocity: u8,
            duration: u32,
        },
        ext: extern struct { len: u32, ptr: ?*anyopaque }, // forces 16-byte, 8-aligned union
        raw: [16]u8,
    },
};

extern fn snd_seq_open(handle: *?*snd_seq_t, name: [*:0]const u8, streams: c_int, mode: c_int) c_int;
extern fn snd_seq_set_client_name(seq: *snd_seq_t, name: [*:0]const u8) c_int;
extern fn snd_seq_create_simple_port(seq: *snd_seq_t, name: [*:0]const u8, caps: c_uint, type: c_uint) c_int;
extern fn snd_seq_event_input(seq: *snd_seq_t, ev: *?*SeqEvent) c_int;
extern fn snd_seq_client_id(seq: *snd_seq_t) c_int;
extern fn snd_seq_close(seq: *snd_seq_t) c_int;

pub const MidiEvent = struct {
    pub const Kind = enum { note_on, note_off };
    kind: Kind,
    note: u8,
    velocity: u8,
};

pub const MidiError = error{ OpenFailed, PortFailed };

pub const MidiInput = struct {
    seq: *snd_seq_t,
    client: c_int,
    port: c_int,

    pub fn open(client_name: [*:0]const u8, port_name: [*:0]const u8) MidiError!MidiInput {
        var handle: ?*snd_seq_t = null;
        if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0)
            return MidiError.OpenFailed;
        const seq = handle.?;
        _ = snd_seq_set_client_name(seq, client_name);
        const port = snd_seq_create_simple_port(
            seq,
            port_name,
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION,
        );
        if (port < 0) {
            _ = snd_seq_close(seq);
            return MidiError.PortFailed;
        }
        return .{ .seq = seq, .client = snd_seq_client_id(seq), .port = port };
    }

    /// Drain all pending MIDI events into `out`; returns how many were written.
    /// Non-blocking: returns 0 immediately when nothing is queued.
    pub fn poll(self: *MidiInput, out: []MidiEvent) usize {
        var count: usize = 0;
        while (count < out.len) {
            var ev: ?*SeqEvent = null;
            if (snd_seq_event_input(self.seq, &ev) < 0) break; // -EAGAIN => drained
            const e = ev orelse break;
            switch (e.type) {
                SND_SEQ_EVENT_NOTEON => {
                    const n = e.data.note;
                    if (n.velocity > 0) {
                        out[count] = .{ .kind = .note_on, .note = n.note, .velocity = n.velocity };
                    } else {
                        out[count] = .{ .kind = .note_off, .note = n.note, .velocity = 0 };
                    }
                    count += 1;
                },
                SND_SEQ_EVENT_NOTEOFF => {
                    const n = e.data.note;
                    out[count] = .{ .kind = .note_off, .note = n.note, .velocity = n.off_velocity };
                    count += 1;
                },
                else => {}, // ignore CC/pitchbend/etc. for now
            }
        }
        return count;
    }

    pub fn close(self: *MidiInput) void {
        _ = snd_seq_close(self.seq);
    }
};

test "SeqEvent layout matches C ABI" {
    try std.testing.expectEqual(@as(usize, 32), @sizeOf(SeqEvent));
    try std.testing.expectEqual(@as(usize, 16), @offsetOf(SeqEvent, "data"));
}
