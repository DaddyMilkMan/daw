//! midi_alsa.zig — MIDI input via the ALSA sequencer. Creates a "Zenith" input
//! port any MIDI source (keyboard, aplaymidi, another app) can connect to.
//! 100% Zig over libasound; no JUCE.

const std = @import("std");
const midi2 = @import("midi2.zig");

const snd_seq_t = opaque {};
const snd_seq_client_info_t = opaque {};
const snd_seq_port_info_t = opaque {};

// snd_seq_open streams / modes
const SND_SEQ_OPEN_INPUT: c_int = 2;
const SND_SEQ_NONBLOCK: c_int = 1;
const SND_SEQ_CLIENT_SYSTEM: c_int = 0;
const SND_SEQ_PORT_SYSTEM_ANNOUNCE: c_int = 1;

// port capabilities / types
const SND_SEQ_PORT_CAP_READ: c_uint = 1 << 0;
const SND_SEQ_PORT_CAP_WRITE: c_uint = 1 << 1;
const SND_SEQ_PORT_CAP_SUBS_READ: c_uint = 1 << 5;
const SND_SEQ_PORT_CAP_SUBS_WRITE: c_uint = 1 << 6;
const SND_SEQ_PORT_CAP_NO_EXPORT: c_uint = 1 << 7;
const SND_SEQ_PORT_TYPE_MIDI_GENERIC: c_uint = 1 << 1;
const SND_SEQ_PORT_TYPE_APPLICATION: c_uint = 1 << 20;

// event types we care about
const SND_SEQ_EVENT_NOTEON: u8 = 6;
const SND_SEQ_EVENT_NOTEOFF: u8 = 7;
const EV_CLIENT_START: u8 = 60; // 60..65 = announce topology changes (hot-plug)
const EV_PORT_CHANGE: u8 = 65;

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
extern fn snd_seq_connect_from(seq: *snd_seq_t, my_port: c_int, src_client: c_int, src_port: c_int) c_int;
extern fn snd_seq_client_info_malloc(p: *?*snd_seq_client_info_t) c_int;
extern fn snd_seq_client_info_free(p: *snd_seq_client_info_t) void;
extern fn snd_seq_client_info_set_client(p: *snd_seq_client_info_t, c: c_int) void;
extern fn snd_seq_client_info_get_client(p: *const snd_seq_client_info_t) c_int;
extern fn snd_seq_query_next_client(seq: *snd_seq_t, p: *snd_seq_client_info_t) c_int;
extern fn snd_seq_port_info_malloc(p: *?*snd_seq_port_info_t) c_int;
extern fn snd_seq_port_info_free(p: *snd_seq_port_info_t) void;
extern fn snd_seq_port_info_set_client(p: *snd_seq_port_info_t, c: c_int) void;
extern fn snd_seq_port_info_set_port(p: *snd_seq_port_info_t, port: c_int) void;
extern fn snd_seq_port_info_get_port(p: *const snd_seq_port_info_t) c_int;
extern fn snd_seq_port_info_get_capability(p: *const snd_seq_port_info_t) c_uint;
extern fn snd_seq_query_next_port(seq: *snd_seq_t, p: *snd_seq_port_info_t) c_int;

pub const MidiEvent = struct {
    pub const Kind = enum { note_on, note_off };
    kind: Kind,
    note: u8,
    velocity: u8,

    /// Up-convert this 7-bit MIDI 1.0 event to a MIDI 2.0 UMP (16-bit velocity,
    /// scaled per the spec). `group` selects one of the 16 UMP groups.
    pub fn toUmp(self: MidiEvent, group: u4) midi2.Ump {
        const status: u8 = switch (self.kind) {
            .note_on => 0x90,
            .note_off => 0x80,
        };
        return midi2.fromMidi1(group, status, self.note, self.velocity).?;
    }
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
        var self = MidiInput{ .seq = seq, .client = snd_seq_client_id(seq), .port = port };
        _ = snd_seq_connect_from(seq, port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE); // hot-plug notices
        _ = self.connectAllSources();
        return self;
    }

    /// Subscribe our input to every readable MIDI source (skipping System and
    /// ourselves). Idempotent; returns the number of NEW connections made.
    pub fn connectAllSources(self: *MidiInput) usize {
        var cinfo: ?*snd_seq_client_info_t = null;
        if (snd_seq_client_info_malloc(&cinfo) < 0) return 0;
        defer snd_seq_client_info_free(cinfo.?);
        var pinfo: ?*snd_seq_port_info_t = null;
        if (snd_seq_port_info_malloc(&pinfo) < 0) return 0;
        defer snd_seq_port_info_free(pinfo.?);
        const need = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
        var made: usize = 0;
        snd_seq_client_info_set_client(cinfo.?, -1);
        while (snd_seq_query_next_client(self.seq, cinfo.?) >= 0) {
            const cl = snd_seq_client_info_get_client(cinfo.?);
            if (cl == SND_SEQ_CLIENT_SYSTEM or cl == self.client) continue;
            snd_seq_port_info_set_client(pinfo.?, cl);
            snd_seq_port_info_set_port(pinfo.?, -1);
            while (snd_seq_query_next_port(self.seq, pinfo.?) >= 0) {
                const caps = snd_seq_port_info_get_capability(pinfo.?);
                if (caps & need != need) continue;
                if (caps & SND_SEQ_PORT_CAP_NO_EXPORT != 0) continue;
                if (snd_seq_connect_from(self.seq, self.port, cl, snd_seq_port_info_get_port(pinfo.?)) >= 0) made += 1;
            }
        }
        return made;
    }

    /// Drain all pending MIDI events into `out`; returns how many were written.
    /// Non-blocking: returns 0 immediately when nothing is queued.
    pub fn poll(self: *MidiInput, out: []MidiEvent) usize {
        var count: usize = 0;
        var rescan = false;
        while (count < out.len) {
            var ev: ?*SeqEvent = null;
            if (snd_seq_event_input(self.seq, &ev) < 0) break; // -EAGAIN => drained
            const e = ev orelse break;
            if (e.type >= EV_CLIENT_START and e.type <= EV_PORT_CHANGE) {
                rescan = true; // device plugged/unplugged -> reconnect after draining
                continue;
            }
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
        if (rescan) _ = self.connectAllSources();
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
