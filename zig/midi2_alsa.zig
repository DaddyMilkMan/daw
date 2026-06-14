//! midi2_alsa.zig — NATIVE MIDI 2.0 input via the ALSA sequencer's UMP API.
//!
//! Unlike midi_alsa.zig (a legacy MIDI 1.0 client we up-convert ourselves), this
//! registers the client as a real **UMP MIDI 2.0** endpoint
//! (`snd_seq_set_client_midi_version(SND_SEQ_CLIENT_UMP_MIDI_2_0)`), so the kernel
//! delivers genuine Universal MIDI Packets — and transparently translates any
//! legacy MIDI 1.0 sender to MIDI 2.0 UMP (16-bit velocity, 32-bit controllers)
//! per the spec. We read the raw 32-bit UMP words straight into `midi2.Ump`.
//!
//! Requires alsa-lib >= 1.2.10 (UMP seq API) and Linux >= 6.5. Pure Zig, no JUCE.

const std = @import("std");
const midi2 = @import("midi2.zig");

const snd_seq_t = opaque {};

const SND_SEQ_OPEN_INPUT: c_int = 2;
const SND_SEQ_NONBLOCK: c_int = 1;
const SND_SEQ_CLIENT_UMP_MIDI_2_0: c_int = 2;

const SND_SEQ_PORT_CAP_WRITE: c_uint = 1 << 1;
const SND_SEQ_PORT_CAP_SUBS_WRITE: c_uint = 1 << 6;
const SND_SEQ_PORT_TYPE_MIDI_GENERIC: c_uint = 1 << 1;
const SND_SEQ_PORT_TYPE_MIDI_UMP: c_uint = 1 << 7;
const SND_SEQ_PORT_TYPE_APPLICATION: c_uint = 1 << 20;

const SND_SEQ_EVENT_UMP: u8 = 1 << 5; // flags bit: this event carries a UMP packet

/// Mirror of C `snd_seq_ump_event_t` (32 bytes; the UMP words live at offset 16,
/// overlaying the legacy 16-byte data union).
const SeqUmpEvent = extern struct {
    type: u8,
    flags: u8,
    tag: u8,
    queue: u8,
    time: [8]u8, // snd_seq_timestamp_t (union)
    source: [2]u8, // snd_seq_addr_t
    dest: [2]u8, // snd_seq_addr_t
    ump: [4]u32, // the Universal MIDI Packet words
};

extern fn snd_seq_open(handle: *?*snd_seq_t, name: [*:0]const u8, streams: c_int, mode: c_int) c_int;
extern fn snd_seq_set_client_name(seq: *snd_seq_t, name: [*:0]const u8) c_int;
extern fn snd_seq_set_client_midi_version(seq: *snd_seq_t, version: c_int) c_int;
extern fn snd_seq_create_simple_port(seq: *snd_seq_t, name: [*:0]const u8, caps: c_uint, type: c_uint) c_int;
extern fn snd_seq_ump_event_input(seq: *snd_seq_t, ev: *?*SeqUmpEvent) c_int;
extern fn snd_seq_client_id(seq: *snd_seq_t) c_int;
extern fn snd_seq_close(seq: *snd_seq_t) c_int;

pub const MidiError = error{ OpenFailed, VersionFailed, PortFailed };

pub const Midi2Input = struct {
    seq: *snd_seq_t,
    client: c_int,
    port: c_int,

    /// Open a native UMP MIDI 2.0 input client + port. Fails (so the caller can
    /// fall back to legacy) if the kernel/alsa-lib lacks UMP support.
    pub fn open(client_name: [*:0]const u8, port_name: [*:0]const u8) MidiError!Midi2Input {
        var handle: ?*snd_seq_t = null;
        if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0)
            return MidiError.OpenFailed;
        const seq = handle.?;
        _ = snd_seq_set_client_name(seq, client_name);
        // negotiate as a MIDI 2.0 endpoint BEFORE creating ports
        if (snd_seq_set_client_midi_version(seq, SND_SEQ_CLIENT_UMP_MIDI_2_0) < 0) {
            _ = snd_seq_close(seq);
            return MidiError.VersionFailed;
        }
        const port = snd_seq_create_simple_port(
            seq,
            port_name,
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_MIDI_UMP | SND_SEQ_PORT_TYPE_APPLICATION,
        );
        if (port < 0) {
            _ = snd_seq_close(seq);
            return MidiError.PortFailed;
        }
        return .{ .seq = seq, .client = snd_seq_client_id(seq), .port = port };
    }

    /// Drain all pending UMP packets into `out`; returns how many were written.
    /// Each entry is a genuine Universal MIDI Packet (MIDI 2.0 protocol).
    pub fn poll(self: *Midi2Input, out: []midi2.Ump) usize {
        var count: usize = 0;
        while (count < out.len) {
            var ev: ?*SeqUmpEvent = null;
            if (snd_seq_ump_event_input(self.seq, &ev) < 0) break; // -EAGAIN => drained
            const e = ev orelse break;
            if (e.flags & SND_SEQ_EVENT_UMP == 0) continue; // not a UMP event (e.g. subscription notice)
            var u = midi2.Ump{ .words = e.ump, .len = 1 };
            u.len = u.messageType().words();
            out[count] = u;
            count += 1;
        }
        return count;
    }

    pub fn close(self: *Midi2Input) void {
        _ = snd_seq_close(self.seq);
    }
};
