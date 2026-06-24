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
const snd_seq_client_info_t = opaque {};
const snd_seq_port_info_t = opaque {};

const SND_SEQ_OPEN_INPUT: c_int = 2;
const SND_SEQ_OPEN_OUTPUT: c_int = 1;
const SND_SEQ_NONBLOCK: c_int = 1;
const SND_SEQ_CLIENT_UMP_MIDI_2_0: c_int = 2;
const SND_SEQ_ADDRESS_SUBSCRIBERS: u8 = 254;
const SND_SEQ_QUEUE_DIRECT: u8 = 253;
const SND_SEQ_CLIENT_SYSTEM: c_int = 0;
const SND_SEQ_PORT_SYSTEM_ANNOUNCE: c_int = 1;

const SND_SEQ_PORT_CAP_READ: c_uint = 1 << 0;
const SND_SEQ_PORT_CAP_WRITE: c_uint = 1 << 1;
const SND_SEQ_PORT_CAP_SUBS_READ: c_uint = 1 << 5;
const SND_SEQ_PORT_CAP_SUBS_WRITE: c_uint = 1 << 6;
const SND_SEQ_PORT_CAP_NO_EXPORT: c_uint = 1 << 7;
const SND_SEQ_PORT_TYPE_MIDI_GENERIC: c_uint = 1 << 1;
const SND_SEQ_PORT_TYPE_MIDI_UMP: c_uint = 1 << 7;
const SND_SEQ_PORT_TYPE_APPLICATION: c_uint = 1 << 20;

const SND_SEQ_EVENT_UMP: u8 = 1 << 5; // flags bit: this event carries a UMP packet
// System Announce event types (topology changes -> hot-plug rescan):
const EV_CLIENT_START: u8 = 60;
const EV_PORT_CHANGE: u8 = 65; // 60..65 = client/port start/exit/change

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
extern fn snd_seq_ump_event_output_direct(seq: *snd_seq_t, ev: *SeqUmpEvent) c_int;
extern fn snd_seq_client_id(seq: *snd_seq_t) c_int;
extern fn snd_seq_close(seq: *snd_seq_t) c_int;
extern fn snd_seq_connect_from(seq: *snd_seq_t, my_port: c_int, src_client: c_int, src_port: c_int) c_int;
extern fn snd_seq_disconnect_from(seq: *snd_seq_t, my_port: c_int, src_client: c_int, src_port: c_int) c_int;
extern fn snd_seq_client_info_get_name(p: *const snd_seq_client_info_t) [*:0]const u8;
extern fn snd_seq_port_info_get_name(p: *const snd_seq_port_info_t) [*:0]const u8;
// client/port enumeration (auto-connect every source)
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

pub const MidiError = error{ OpenFailed, VersionFailed, PortFailed };

pub const Midi2Input = struct {
    seq: *snd_seq_t,
    client: c_int,
    port: c_int,
    skip_client: c_int = -1, // never auto-connect this client (e.g. our own output -> no MIDI loop)

    /// Open a native UMP MIDI 2.0 input client + port. `exclude_client` is a seq
    /// client never to auto-connect (pass our own output's client id to avoid a
    /// feedback loop). Fails (so the caller can fall back to legacy) if the
    /// kernel/alsa-lib lacks UMP support.
    pub fn open(client_name: [*:0]const u8, port_name: [*:0]const u8, exclude_client: c_int) MidiError!Midi2Input {
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
        var self = Midi2Input{ .seq = seq, .client = snd_seq_client_id(seq), .port = port, .skip_client = exclude_client };
        // hot-plug: receive topology notifications from the System Announce port
        _ = snd_seq_connect_from(seq, port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
        // auto-connect every MIDI source already present (any age — the kernel
        // up-converts each to MIDI 2.0 UMP for us)
        _ = self.connectAllSources();
        return self;
    }

    /// Subscribe our input to every readable MIDI source on the system (skipping
    /// the System client and ourselves). Idempotent — already-connected sources
    /// just fail harmlessly. Returns the number of NEW connections made.
    pub fn connectAllSources(self: *Midi2Input) usize {
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
            if (cl == SND_SEQ_CLIENT_SYSTEM or cl == self.client or cl == self.skip_client) continue;
            snd_seq_port_info_set_client(pinfo.?, cl);
            snd_seq_port_info_set_port(pinfo.?, -1);
            while (snd_seq_query_next_port(self.seq, pinfo.?) >= 0) {
                const caps = snd_seq_port_info_get_capability(pinfo.?);
                if (caps & need != need) continue; // not a connectable source
                if (caps & SND_SEQ_PORT_CAP_NO_EXPORT != 0) continue;
                const pt = snd_seq_port_info_get_port(pinfo.?);
                if (snd_seq_connect_from(self.seq, self.port, cl, pt) >= 0) made += 1;
            }
        }
        return made;
    }

    /// A connectable MIDI source: its seq address + a "Client: Port" label.
    pub const SourceInfo = struct {
        client: c_int,
        port: c_int,
        name: [80]u8 = [_]u8{0} ** 80,
        name_len: usize = 0,
        pub fn label(self: *const SourceInfo) []const u8 {
            return self.name[0..self.name_len];
        }
    };

    /// Enumerate every connectable MIDI source on the system into `out` (for a
    /// device picker). Returns how many were found.
    pub fn listSources(self: *Midi2Input, out: []SourceInfo) usize {
        var cinfo: ?*snd_seq_client_info_t = null;
        if (snd_seq_client_info_malloc(&cinfo) < 0) return 0;
        defer snd_seq_client_info_free(cinfo.?);
        var pinfo: ?*snd_seq_port_info_t = null;
        if (snd_seq_port_info_malloc(&pinfo) < 0) return 0;
        defer snd_seq_port_info_free(pinfo.?);
        const need = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
        var k: usize = 0;
        snd_seq_client_info_set_client(cinfo.?, -1);
        while (snd_seq_query_next_client(self.seq, cinfo.?) >= 0 and k < out.len) {
            const cl = snd_seq_client_info_get_client(cinfo.?);
            if (cl == SND_SEQ_CLIENT_SYSTEM or cl == self.client or cl == self.skip_client) continue;
            const cname = std.mem.span(snd_seq_client_info_get_name(cinfo.?));
            snd_seq_port_info_set_client(pinfo.?, cl);
            snd_seq_port_info_set_port(pinfo.?, -1);
            while (snd_seq_query_next_port(self.seq, pinfo.?) >= 0 and k < out.len) {
                const caps = snd_seq_port_info_get_capability(pinfo.?);
                if (caps & need != need) continue;
                if (caps & SND_SEQ_PORT_CAP_NO_EXPORT != 0) continue;
                const pname = std.mem.span(snd_seq_port_info_get_name(pinfo.?));
                out[k] = .{ .client = cl, .port = snd_seq_port_info_get_port(pinfo.?) };
                const s = std.fmt.bufPrint(&out[k].name, "{s}: {s}", .{ cname, pname }) catch out[k].name[0..0];
                out[k].name_len = s.len;
                k += 1;
            }
        }
        return k;
    }

    /// Subscribe ONLY to sources whose "Client: Port" label contains `match`
    /// (case-sensitive substring); disconnect the rest. A simple device picker.
    /// Returns the number connected.
    pub fn connectOnlyMatching(self: *Midi2Input, match: []const u8) usize {
        var src: [64]SourceInfo = undefined;
        const n = self.listSources(&src);
        var connected: usize = 0;
        for (src[0..n]) |s| {
            if (std.mem.indexOf(u8, s.label(), match) != null) {
                if (snd_seq_connect_from(self.seq, self.port, s.client, s.port) >= 0) connected += 1;
            } else {
                _ = snd_seq_disconnect_from(self.seq, self.port, s.client, s.port);
            }
        }
        return connected;
    }

    /// Drain all pending UMP packets into `out`; returns how many were written.
    /// Each entry is a genuine Universal MIDI Packet (MIDI 2.0 protocol).
    pub fn poll(self: *Midi2Input, out: []midi2.Ump) usize {
        var count: usize = 0;
        var rescan = false;
        while (count < out.len) {
            var ev: ?*SeqUmpEvent = null;
            if (snd_seq_ump_event_input(self.seq, &ev) < 0) break; // -EAGAIN => drained
            const e = ev orelse break;
            if (e.flags & SND_SEQ_EVENT_UMP == 0) {
                // a System Announce topology event (device plugged/unplugged) ->
                // reconnect after draining so new hardware just works
                if (e.type >= EV_CLIENT_START and e.type <= EV_PORT_CHANGE) rescan = true;
                continue;
            }
            var u = midi2.Ump{ .words = e.ump, .len = 1 };
            u.len = u.messageType().words();
            out[count] = u;
            count += 1;
        }
        if (rescan) _ = self.connectAllSources();
        return count;
    }

    pub fn close(self: *Midi2Input) void {
        _ = snd_seq_close(self.seq);
    }
};

/// Native MIDI 2.0 UMP OUTPUT — a source port that emits genuine Universal MIDI
/// Packets (16-bit velocity, 32-bit controllers) to any subscriber. Other apps
/// connect to it and receive native MIDI 2.0; legacy clients get the kernel's
/// down-conversion automatically.
pub const Midi2Output = struct {
    seq: *snd_seq_t,
    client: c_int,
    port: c_int,

    pub fn open(client_name: [*:0]const u8, port_name: [*:0]const u8) MidiError!Midi2Output {
        var handle: ?*snd_seq_t = null;
        if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0)
            return MidiError.OpenFailed;
        const seq = handle.?;
        _ = snd_seq_set_client_name(seq, client_name);
        if (snd_seq_set_client_midi_version(seq, SND_SEQ_CLIENT_UMP_MIDI_2_0) < 0) {
            _ = snd_seq_close(seq);
            return MidiError.VersionFailed;
        }
        const port = snd_seq_create_simple_port(
            seq,
            port_name,
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_MIDI_UMP | SND_SEQ_PORT_TYPE_APPLICATION,
        );
        if (port < 0) {
            _ = snd_seq_close(seq);
            return MidiError.PortFailed;
        }
        return .{ .seq = seq, .client = snd_seq_client_id(seq), .port = port };
    }

    /// Emit one Universal MIDI Packet to all subscribers (native MIDI 2.0).
    pub fn send(self: *Midi2Output, u: midi2.Ump) void {
        var e = SeqUmpEvent{
            .type = 0,
            .flags = SND_SEQ_EVENT_UMP,
            .tag = 0,
            .queue = SND_SEQ_QUEUE_DIRECT,
            .time = [_]u8{0} ** 8,
            .source = .{ 0, @intCast(self.port) },
            .dest = .{ SND_SEQ_ADDRESS_SUBSCRIBERS, 0 },
            .ump = u.words,
        };
        _ = snd_seq_ump_event_output_direct(self.seq, &e);
    }

    pub fn close(self: *Midi2Output) void {
        _ = snd_seq_close(self.seq);
    }
};
