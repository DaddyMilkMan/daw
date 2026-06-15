//! audio_engine.zig — the live DAW's real-time audio engine (output + input).
//!
//! Output thread: opens the OS output (ALSA -> PipeWire) and renders the project
//! loop PLUS a polyphonic synth fed by incoming MIDI, paced by the audio clock
//! (blocking writeBlock, no sleeps). Input thread: opens the default capture and
//! publishes a live input peak (so the engine "hears" the mic). UI <-> audio
//! communicate only through atomics + one lock-free SPSC note queue — no locks on
//! the audio path. The UI pushes playing/gain and MIDI notes; the engine
//! publishes the sample-accurate playhead, master peak, and input peak.

const std = @import("std");
const alsa = @import("audio_alsa.zig");
const synth = @import("synth.zig");
const audio_track = @import("audio_track.zig");
const effects = @import("effects.zig");
const dsp = @import("dsp.zig");
const project = @import("project.zig");
const alog = std.log.scoped(.audio);

const RING = 512; // note-event queue capacity (power-of-two not required)
// A lock-free engine event: notes + ordered per-channel/per-note (MPE) expression.
const EvKind = enum(u8) { note_on, note_off, ch_bend, ch_press, ch_timbre, note_bend, note_press, note_timbre };
const Ev = struct {
    kind: EvKind,
    channel: u4 = 0,
    note: u8 = 0,
    freq: f32 = 0,
    vel: f32 = 0,
    value: f32 = 0, // bend ratio / pressure 0..1 / timbre -1..1
};
pub const MAXTRACKS = 8;

/// Normalize a 7-bit MIDI 1.0 velocity (0..127) to 0..1.
pub fn vel7(v: u8) f32 {
    return @as(f32, @floatFromInt(v)) / 127.0;
}
/// Normalize a 16-bit MIDI 2.0 velocity (0..65535) to 0..1.
pub fn vel16(v: u32) f32 {
    return @as(f32, @floatFromInt(@min(v, 65535))) / 65535.0;
}

/// One scheduled synth event placed at a sample offset within the current block.
const SeqEvt = struct { off: u32, on: bool, freq: f32, vel: f32 };

/// If loop-frame `f` falls in the block [start, start+n) over a wrapping loop of
/// `len`, return its offset within the block (0..n); otherwise null.
fn offsetInBlock(f: u64, start: u64, n: u64, len: u64) ?u32 {
    const end = start + n;
    if (end <= len) {
        if (f >= start and f < end) return @intCast(f - start);
        return null;
    }
    if (f >= start) return @intCast(f - start); // [start, len)
    if (f < end - len) return @intCast(f + len - start); // [0, end-len) (wrapped)
    return null;
}

/// Render `out` while applying `evs` (sorted by offset) at their exact sample
/// offsets — so note onsets land sample-accurately, not quantized to the block.
/// Live note events already applied at offset 0 by the caller (drainNotes).
fn renderSegmented(s: *synth.Synth, out: []f32, evs: []const SeqEvt) void {
    var cur: usize = 0;
    for (evs) |e| {
        const off = @min(@as(usize, e.off), out.len);
        if (off > cur) {
            s.renderBlock(out[cur..off]);
            cur = off;
        }
        if (e.on) s.noteOn(e.freq, e.vel) else s.noteOff(e.freq);
    }
    if (cur < out.len) s.renderBlock(out[cur..]);
}

/// Stereo variant of renderSegmented — the synth renders L/R with sample-accurate
/// onsets, so the live engine gets the synth's stereo image.
fn renderSegmentedStereo(s: *synth.Synth, outL: []f32, outR: []f32, evs: []const SeqEvt) void {
    var cur: usize = 0;
    for (evs) |e| {
        const off = @min(@as(usize, e.off), outL.len);
        if (off > cur) {
            s.renderStereo(outL[cur..off], outR[cur..off]);
            cur = off;
        }
        if (e.on) s.noteOn(e.freq, e.vel) else s.noteOff(e.freq);
    }
    if (cur < outL.len) s.renderStereo(outL[cur..], outR[cur..]);
}

test "offsetInBlock handles the loop wrap" {
    try std.testing.expectEqual(@as(?u32, 0), offsetInBlock(100, 100, 256, 48000));
    try std.testing.expectEqual(@as(?u32, 56), offsetInBlock(156, 100, 256, 48000));
    try std.testing.expectEqual(@as(?u32, null), offsetInBlock(356, 100, 256, 48000));
    // block straddles the loop end: frames 990..1000 and 0..245 are in-block
    try std.testing.expectEqual(@as(?u32, 5), offsetInBlock(995, 990, 256, 1000));
    try std.testing.expectEqual(@as(?u32, 20), offsetInBlock(10, 990, 256, 1000));
    try std.testing.expectEqual(@as(?u32, null), offsetInBlock(500, 990, 256, 1000));
}

test "renderSegmented places an onset sample-accurately" {
    var s = synth.Synth{};
    var buf: [256]f32 = [_]f32{0} ** 256;
    const evs = [_]SeqEvt{.{ .off = 128, .on = true, .freq = 440.0, .vel = 1.0 }};
    renderSegmented(&s, &buf, &evs);
    var pre: f32 = 0;
    for (buf[0..128]) |x| pre = @max(pre, @abs(x));
    var post: f32 = 0;
    for (buf[128..]) |x| post = @max(post, @abs(x));
    try std.testing.expectEqual(@as(f32, 0), pre); // silent before the onset
    try std.testing.expect(post > 0.001); // sounding after
}

/// Mix `ntracks` mono stems at loop frame `pos` into a stereo block, applying
/// per-track gain + constant-power pan + mute/solo, then master gain. Reports each
/// track's block peak (post-fader). Pure — the live engine reads atomics into
/// plain arrays and calls this, and it's unit-tested directly.
pub fn mixBlocks(
    blocks: []const []const f32, // per-track post-FX mono blocks (each `n` samples)
    ntracks: usize,
    n: usize,
    gains: []const f32,
    pans: []const f32,
    mutes: []const bool,
    solos: []const bool,
    sends: []const f32, // per-track reverb send amount (0..1)
    out_l: []f32,
    out_r: []f32,
    send_bus: []f32, // mono: sum of post-fader signal * send
    track_peak: []f32,
) void {
    @memset(out_l[0..n], 0);
    @memset(out_r[0..n], 0);
    @memset(send_bus[0..n], 0);
    var any_solo = false;
    for (0..ntracks) |ti| {
        if (solos[ti]) any_solo = true;
    }
    for (0..ntracks) |ti| {
        track_peak[ti] = 0;
        const active = !mutes[ti] and !(any_solo and !solos[ti]);
        if (!active or ti >= blocks.len) continue;
        const ang = (std.math.clamp(pans[ti], -1.0, 1.0) * 0.5 + 0.5) * (std.math.pi / 2.0);
        const lg = @cos(ang) * gains[ti];
        const rg = @sin(ang) * gains[ti];
        var f: usize = 0;
        while (f < n) : (f += 1) {
            const s = blocks[ti][f];
            out_l[f] += s * lg;
            out_r[f] += s * rg;
            const post = s * gains[ti];
            send_bus[f] += post * sends[ti];
            const pk = @abs(post);
            if (pk > track_peak[ti]) track_peak[ti] = pk;
        }
    }
}

/// One track's serial FX chain: high-pass (remove rumble) -> compressor.
pub const TrackFx = struct {
    hp: effects.Biquad = .{},
    comp: dsp.Compressor = undefined,

    pub fn init(sr: f32) TrackFx {
        var c = dsp.Compressor.init(sr, 5.0, 80.0);
        c.threshold_db = -18;
        c.ratio = 3;
        c.knee_db = 6;
        return .{ .hp = effects.Biquad.highpass(sr, 30.0, 0.707), .comp = c };
    }
    pub fn process(self: *TrackFx, x: f32) f32 {
        return self.comp.process(self.hp.process(x));
    }
};

/// Stereo-linked brickwall master limiter: instant attack (so output never exceeds
/// `ceiling`), smooth ~80 ms release. A final clamp is the absolute safety.
pub const MasterLimiter = struct {
    ceiling: f32 = 0.98,
    rel: f32 = 0.99974, // per-sample release coefficient
    gain: f32 = 1,

    pub fn init(sr: f32) MasterLimiter {
        return .{ .rel = @exp(-1.0 / (0.08 * sr)) };
    }
    pub fn process(self: *MasterLimiter, l: *f32, r: *f32) void {
        const m = @max(@abs(l.*), @abs(r.*));
        // release toward unity first, then clamp the gain to what THIS sample needs
        // (instant attack) — so the output is always <= ceiling.
        self.gain += (1.0 - self.gain) * (1.0 - self.rel);
        const tgt: f32 = if (m > self.ceiling) self.ceiling / m else 1.0;
        if (tgt < self.gain) self.gain = tgt;
        l.* = std.math.clamp(l.* * self.gain, -1.0, 1.0);
        r.* = std.math.clamp(r.* * self.gain, -1.0, 1.0);
    }
    pub fn grDb(self: MasterLimiter) f32 {
        return 20.0 * std.math.log10(@max(self.gain, 1e-4));
    }
};

// Try these output/input devices in order, so audio works on any system — a
// PipeWire/Pulse desktop or bare ALSA hardware of any age. `default` first
// (whatever the user's chosen device is), then explicit servers, then raw HW.
const DEVICE_FALLBACKS = [_][*:0]const u8{ "default", "pipewire", "pulse", "plughw:0,0", "hw:0,0" };

const OpenedOut = struct { stream: alsa.StreamOut, name: [*:0]const u8 };
fn openOut(preferred: [*:0]const u8, rate: u32, channels: u16) ?OpenedOut {
    if (alsa.StreamOut.open(preferred, rate, channels, 40_000)) |s| return .{ .stream = s, .name = preferred } else |_| {}
    for (DEVICE_FALLBACKS) |dev| {
        if (alsa.StreamOut.open(dev, rate, channels, 40_000)) |s| return .{ .stream = s, .name = dev } else |_| {}
    }
    return null;
}
fn openIn(preferred: [*:0]const u8, rate: u32) ?alsa.StreamIn {
    if (alsa.StreamIn.open(preferred, rate, 1, 60_000)) |s| return s else |_| {}
    for (DEVICE_FALLBACKS) |dev| {
        if (alsa.StreamIn.open(dev, rate, 1, 60_000)) |s| return s else |_| {}
    }
    return null;
}

pub const Engine = struct {
    samples: []const f32, // mono source loop (legacy single-loop fallback)
    audio_tracks: []const audio_track.AudioTrack = &.{}, // linear-timeline audio clips
    // per-track mixer stems + controls (the live mixer drives these, lock-free)
    stems: []const []const f32 = &.{},
    ntracks: usize = 0,
    tgain_bits: [MAXTRACKS]u32 = [_]u32{@bitCast(@as(f32, 0.8))} ** MAXTRACKS,
    tpan_bits: [MAXTRACKS]u32 = [_]u32{0} ** MAXTRACKS,
    tmute: [MAXTRACKS]bool = [_]bool{false} ** MAXTRACKS,
    tsolo: [MAXTRACKS]bool = [_]bool{false} ** MAXTRACKS,
    tlevel_bits: [MAXTRACKS]u32 = [_]u32{0} ** MAXTRACKS, // published per-track peak
    // per-track FX chains + a shared reverb send bus
    track_fx: [MAXTRACKS]TrackFx = undefined, // initialized in start()
    reverb: ?*effects.Reverb = null, // shared aux reverb (allocated by the host)
    rsend_bits: [MAXTRACKS]u32 = [_]u32{0} ** MAXTRACKS, // per-track reverb send
    rlevel_bits: u32 = 0, // published reverb-bus output level
    master_lim: MasterLimiter = .{}, // master brickwall limiter (audio-thread only)
    grdb_bits: u32 = 0, // published master limiter gain-reduction (dB, <=0)
    // sequenced clip notes -> synth (double-buffered: UI writes inactive, flips index)
    seq_bufs: [2][256]project.Note = undefined,
    seq_counts: [2]usize = .{ 0, 0 },
    seq_active: u32 = 0,
    seq_len: u64 = 0,
    seq_pos: u64 = 0,
    slevel_bits: u32 = 0, // published synth (MIDI + sequence) output level
    rate: u32 = 48000,
    channels: u16 = 2,
    device: [*:0]const u8 = "default",
    device_opened: [*:0]const u8 = "?", // the device the output thread actually opened
    capture: bool = true, // also open the input device and meter it
    tl_pos: u64 = 0, // linear timeline playhead (frames) for audio tracks

    // --- shared, accessed via atomics from both threads ---
    playing: bool = false,
    quit: bool = false,
    started: bool = false,
    gain_bits: u32 = @bitCast(@as(f32, 0.8)),
    pos: u64 = 0, // current frame index into `samples`
    peak_bits: u32 = 0, // master output peak (bitcast f32)
    in_peak_bits: u32 = 0, // live input peak (bitcast f32)
    // high-res MIDI 2.0 controllers (latest-value-wins, lock-free)
    ctl_cutoff_bits: u32 = @bitCast(@as(f32, 2200.0)), // CC74 -> filter cutoff Hz
    ctl_res_bits: u32 = @bitCast(@as(f32, 0.25)), // CC71 -> resonance 0..1
    ctl_bend_bits: u32 = @bitCast(@as(f32, 1.0)), // pitch bend -> freq multiplier
    ctl_press_bits: u32 = @bitCast(@as(f32, 0.0)), // channel pressure -> brightness
    ctl_mod_bits: u32 = @bitCast(@as(f32, 0.0)), // CC1 mod wheel -> vibrato depth
    ctl_expr_bits: u32 = @bitCast(@as(f32, 1.0)), // CC7*CC11 volume/expression -> gain
    ctl_sustain: bool = false, // CC64 sustain pedal
    prev_sustain: bool = false, // audio-thread edge detector for the pedal

    // --- SPSC note queue: UI/MIDI thread produces, audio thread consumes ---
    events: [RING]Ev = undefined,
    ev_head: usize = 0, // producer (next write)
    ev_tail: usize = 0, // consumer (next read)

    synth: synth.Synth = .{},
    out_thread: ?std.Thread = null,
    in_thread: ?std.Thread = null,

    pub fn start(self: *Engine) void {
        self.synth.sample_rate = @floatFromInt(self.rate);
        for (&self.track_fx) |*fx| fx.* = TrackFx.init(@floatFromInt(self.rate));
        self.master_lim = MasterLimiter.init(@floatFromInt(self.rate));
        self.out_thread = std.Thread.spawn(.{}, runOut, .{self}) catch null;
        if (self.capture) self.in_thread = std.Thread.spawn(.{}, runIn, .{self}) catch null;
    }
    pub fn stop(self: *Engine) void {
        @atomicStore(bool, &self.quit, true, .seq_cst);
        if (self.out_thread) |t| t.join();
        if (self.in_thread) |t| t.join();
        self.out_thread = null;
        self.in_thread = null;
    }

    pub fn setPlaying(self: *Engine, p: bool) void {
        @atomicStore(bool, &self.playing, p, .monotonic);
    }
    pub fn setGain(self: *Engine, g: f32) void {
        @atomicStore(u32, &self.gain_bits, @bitCast(g), .monotonic);
    }
    // per-track mixer controls (called from the UI thread; lock-free)
    pub fn setTrackGain(self: *Engine, i: usize, g: f32) void {
        if (i < MAXTRACKS) @atomicStore(u32, &self.tgain_bits[i], @bitCast(g), .monotonic);
    }
    pub fn setTrackPan(self: *Engine, i: usize, p: f32) void {
        if (i < MAXTRACKS) @atomicStore(u32, &self.tpan_bits[i], @bitCast(p), .monotonic);
    }
    pub fn setTrackMute(self: *Engine, i: usize, m: bool) void {
        if (i < MAXTRACKS) @atomicStore(bool, &self.tmute[i], m, .monotonic);
    }
    pub fn setTrackSolo(self: *Engine, i: usize, s: bool) void {
        if (i < MAXTRACKS) @atomicStore(bool, &self.tsolo[i], s, .monotonic);
    }
    pub fn getTrackLevel(self: *Engine, i: usize) f32 {
        return if (i < MAXTRACKS) @bitCast(@atomicLoad(u32, &self.tlevel_bits[i], .monotonic)) else 0;
    }
    pub fn setTrackSend(self: *Engine, i: usize, amt: f32) void {
        if (i < MAXTRACKS) @atomicStore(u32, &self.rsend_bits[i], @bitCast(amt), .monotonic);
    }
    pub fn getReverbLevel(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.rlevel_bits, .monotonic));
    }
    pub fn getLimiterGrDb(self: *Engine) f32 { // master limiter gain reduction (dB, <=0)
        return @bitCast(@atomicLoad(u32, &self.grdb_bits, .monotonic));
    }
    pub fn getSynthLevel(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.slevel_bits, .monotonic));
    }
    /// Publish a clip's notes for the synth to play (RT-safe: writes the inactive
    /// buffer, then flips the active index). `loop_len` = the loop length in frames.
    pub fn setSequence(self: *Engine, notes: []const project.Note, loop_len: u64) void {
        const inactive: u32 = 1 - @atomicLoad(u32, &self.seq_active, .monotonic);
        const n = @min(notes.len, self.seq_bufs[inactive].len);
        @memcpy(self.seq_bufs[inactive][0..n], notes[0..n]);
        self.seq_counts[inactive] = n;
        self.seq_len = loop_len;
        @atomicStore(u32, &self.seq_active, inactive, .release);
    }
    pub fn isLive(self: *Engine) bool {
        return @atomicLoad(bool, &self.started, .monotonic);
    }
    pub fn playheadNorm(self: *Engine) f32 {
        if (self.samples.len == 0) return 0;
        const p = @atomicLoad(u64, &self.pos, .monotonic);
        return @as(f32, @floatFromInt(p % self.samples.len)) / @as(f32, @floatFromInt(self.samples.len));
    }
    pub fn getPeak(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
    }
    pub fn getInputPeak(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.in_peak_bits, .monotonic));
    }

    // high-res controller setters (called from the UI/MIDI thread)
    pub fn setCutoff(self: *Engine, hz: f32) void {
        @atomicStore(u32, &self.ctl_cutoff_bits, @bitCast(hz), .monotonic);
    }
    pub fn setResonance(self: *Engine, r: f32) void {
        @atomicStore(u32, &self.ctl_res_bits, @bitCast(r), .monotonic);
    }
    pub fn setBend(self: *Engine, ratio: f32) void {
        @atomicStore(u32, &self.ctl_bend_bits, @bitCast(ratio), .monotonic);
    }
    pub fn setPressure(self: *Engine, v: f32) void {
        @atomicStore(u32, &self.ctl_press_bits, @bitCast(v), .monotonic);
    }
    pub fn setMod(self: *Engine, v: f32) void { // CC1 mod wheel -> vibrato depth
        @atomicStore(u32, &self.ctl_mod_bits, @bitCast(v), .monotonic);
    }
    pub fn setExpression(self: *Engine, v: f32) void { // CC7/CC11 -> synth gain
        @atomicStore(u32, &self.ctl_expr_bits, @bitCast(v), .monotonic);
    }
    pub fn setSustain(self: *Engine, on: bool) void { // CC64 sustain pedal
        @atomicStore(bool, &self.ctl_sustain, on, .monotonic);
    }
    pub fn setMacro(self: *Engine, i: usize, v: f32) void { // synth macro knob -> mod sources
        self.synth.setMacro(i, v);
    }
    pub fn setSynthPatch(self: *Engine, p: synth.Patch) void { // push an edited patch (incl. mod routes)
        self.synth.setPatch(p);
    }

    /// Push one event onto the lock-free ring (UI/MIDI thread -> audio thread).
    fn pushEv(self: *Engine, ev: Ev) void {
        const h = @atomicLoad(usize, &self.ev_head, .monotonic);
        const next = (h + 1) % RING;
        if (next == @atomicLoad(usize, &self.ev_tail, .acquire)) return; // full
        self.events[h] = ev;
        @atomicStore(usize, &self.ev_head, next, .release);
    }
    /// Queue a note on/off with velocity (0..1) on channel 0. Velocity is ignored
    /// for note-offs.
    pub fn pushNote(self: *Engine, on: bool, freq: f32, vel: f32) void {
        self.pushNoteCh(on, freq, vel, 0);
    }
    /// Queue a note on a specific MIDI channel (MPE: one note per channel).
    pub fn pushNoteCh(self: *Engine, on: bool, freq: f32, vel: f32, channel: u4) void {
        self.pushEv(.{ .kind = if (on) .note_on else .note_off, .freq = freq, .vel = vel, .channel = channel });
    }
    // --- MPE expression (ordered with notes through the same queue) ---
    pub fn chanBend(self: *Engine, channel: u4, ratio: f32) void {
        self.pushEv(.{ .kind = .ch_bend, .channel = channel, .value = ratio });
    }
    pub fn chanPressure(self: *Engine, channel: u4, v: f32) void {
        self.pushEv(.{ .kind = .ch_press, .channel = channel, .value = v });
    }
    pub fn chanTimbre(self: *Engine, channel: u4, v: f32) void {
        self.pushEv(.{ .kind = .ch_timbre, .channel = channel, .value = v });
    }
    pub fn noteBend(self: *Engine, note: u8, ratio: f32) void {
        self.pushEv(.{ .kind = .note_bend, .note = note, .value = ratio });
    }
    pub fn notePressure(self: *Engine, note: u8, v: f32) void {
        self.pushEv(.{ .kind = .note_press, .note = note, .value = v });
    }
    pub fn noteTimbre(self: *Engine, note: u8, v: f32) void { // MPE per-note CC74
        self.pushEv(.{ .kind = .note_timbre, .note = note, .value = v });
    }
    fn drainNotes(self: *Engine) void {
        while (true) {
            const t = @atomicLoad(usize, &self.ev_tail, .monotonic);
            if (t == @atomicLoad(usize, &self.ev_head, .acquire)) break;
            const ev = self.events[t];
            @atomicStore(usize, &self.ev_tail, (t + 1) % RING, .release);
            switch (ev.kind) {
                .note_on => self.synth.noteOnMpe(ev.freq, ev.vel, ev.channel),
                .note_off => self.synth.noteOff(ev.freq),
                .ch_bend => self.synth.setChannelBend(ev.channel, ev.value),
                .ch_press => self.synth.setChannelPressure(ev.channel, ev.value),
                .ch_timbre => self.synth.setChannelTimbre(ev.channel, ev.value),
                .note_bend => self.synth.setNoteBend(ev.note, ev.value),
                .note_press => self.synth.setNotePressure(ev.note, ev.value),
                .note_timbre => self.synth.setNoteTimbre(ev.note, ev.value),
            }
        }
    }

    fn runOut(self: *Engine) void {
        const opened = openOut(self.device, self.rate, self.channels) orelse {
            alog.err("output: no usable audio device (tried default/pipewire/pulse/hw)", .{});
            return;
        };
        var out = opened.stream;
        self.device_opened = opened.name;
        defer out.close();
        alog.info("output: device '{s}' opened @ {d} Hz, {d}ch", .{ opened.name, self.rate, self.channels });
        @atomicStore(bool, &self.started, true, .monotonic);

        const N = 256;
        var buf: [N * 2]i16 = undefined;
        var sbL: [N]f32 = undefined; // synth stereo output (L/R)
        var sbR: [N]f32 = undefined;
        var pos: usize = 0;
        const n = self.samples.len;

        while (!@atomicLoad(bool, &self.quit, .monotonic)) {
            // apply high-res MIDI 2.0 controllers to the synth for this block
            // (latest-value-wins atomics). Sustain is edge-detected so pedal-up
            // releases held voices exactly once, and is applied BEFORE draining the
            // note queue so a queued note-off correctly defers while the pedal is down.
            // Channel-wide (non-MPE) controllers are latest-value-wins atomics; the
            // per-channel/per-note (MPE) expression arrives in-order via the queue.
            const base_cut: f32 = @bitCast(@atomicLoad(u32, &self.ctl_cutoff_bits, .monotonic));
            self.synth.cutoff = std.math.clamp(base_cut, 40.0, 18000.0);
            self.synth.resonance = @bitCast(@atomicLoad(u32, &self.ctl_res_bits, .monotonic));
            self.synth.bend = @bitCast(@atomicLoad(u32, &self.ctl_bend_bits, .monotonic));
            self.synth.mod = @bitCast(@atomicLoad(u32, &self.ctl_mod_bits, .monotonic));
            self.synth.pressure = @bitCast(@atomicLoad(u32, &self.ctl_press_bits, .monotonic));
            self.synth.expression = @bitCast(@atomicLoad(u32, &self.ctl_expr_bits, .monotonic));
            const sus = @atomicLoad(bool, &self.ctl_sustain, .monotonic);
            if (sus != self.prev_sustain) {
                self.synth.setSustain(sus);
                self.prev_sustain = sus;
            }
            self.drainNotes(); // live MIDI notes apply at the block start (offset 0)
            const playing = @atomicLoad(bool, &self.playing, .monotonic);
            // Collect the clip's note on/offs that fall in THIS block, each at its
            // exact sample offset, so onsets are sample-accurate (not quantized to N).
            var segev: [512]SeqEvt = undefined;
            var nseg: usize = 0;
            if (playing and self.seq_len > 0) {
                const ai = @atomicLoad(u32, &self.seq_active, .acquire);
                const sn = self.seq_bufs[ai][0..self.seq_counts[ai]];
                for (sn) |note| {
                    if (nseg + 2 > segev.len) break;
                    const freq = noteToFreq(@intCast(note.pitch));
                    if (offsetInBlock(note.start % self.seq_len, self.seq_pos, N, self.seq_len)) |off| {
                        segev[nseg] = .{ .off = off, .on = true, .freq = freq, .vel = vel7(@intCast(note.velocity)) };
                        nseg += 1;
                    }
                    if (offsetInBlock((note.start + note.len) % self.seq_len, self.seq_pos, N, self.seq_len)) |off| {
                        segev[nseg] = .{ .off = off, .on = false, .freq = freq, .vel = 0 };
                        nseg += 1;
                    }
                }
                self.seq_pos = (self.seq_pos + N) % self.seq_len;
                // insertion sort by offset (tiny n; allocation-free on the RT path)
                var i: usize = 1;
                while (i < nseg) : (i += 1) {
                    const key = segev[i];
                    var j = i;
                    while (j > 0 and segev[j - 1].off > key.off) : (j -= 1) segev[j] = segev[j - 1];
                    segev[j] = key;
                }
            }
            renderSegmentedStereo(&self.synth, sbL[0..N], sbR[0..N], segev[0..nseg]); // MIDI- + sequence-driven voices
            var spk: f32 = 0;
            for (0..N) |i| spk = @max(spk, @max(@abs(sbL[i]), @abs(sbR[i])));
            @atomicStore(u32, &self.slevel_bits, @bitCast(spk), .monotonic);
            const gain: f32 = @bitCast(@atomicLoad(u32, &self.gain_bits, .monotonic));

            // mix the linear-timeline audio tracks (stereo) for this block
            var aL: [N]f32 = [_]f32{0} ** N;
            var aR: [N]f32 = [_]f32{0} ** N;
            if (playing and self.audio_tracks.len > 0) {
                audio_track.mixTracks(self.audio_tracks, aL[0..N], aR[0..N], self.tl_pos);
            }

            // mixer playback: per-track stems (the live mixer) OR the legacy loop
            var pL: [N]f32 = [_]f32{0} ** N;
            var pR: [N]f32 = [_]f32{0} ** N;
            if (playing and self.ntracks > 0) {
                var tg: [MAXTRACKS]f32 = undefined;
                var tp: [MAXTRACKS]f32 = undefined;
                var tm: [MAXTRACKS]bool = undefined;
                var ts: [MAXTRACKS]bool = undefined;
                var tsend: [MAXTRACKS]f32 = undefined;
                var tpk: [MAXTRACKS]f32 = undefined;
                for (0..self.ntracks) |ti| {
                    tg[ti] = @bitCast(@atomicLoad(u32, &self.tgain_bits[ti], .monotonic));
                    tp[ti] = @bitCast(@atomicLoad(u32, &self.tpan_bits[ti], .monotonic));
                    tm[ti] = @atomicLoad(bool, &self.tmute[ti], .monotonic);
                    ts[ti] = @atomicLoad(bool, &self.tsolo[ti], .monotonic);
                    tsend[ti] = @bitCast(@atomicLoad(u32, &self.rsend_bits[ti], .monotonic));
                }
                // render each stem block through its FX chain (HP -> compressor)
                var fxblk: [MAXTRACKS][N]f32 = undefined;
                var fxptr: [MAXTRACKS][]const f32 = undefined;
                const loop_len = self.stems[0].len;
                for (0..self.ntracks) |ti| {
                    const stem = self.stems[ti];
                    var f0: usize = 0;
                    while (f0 < N) : (f0 += 1) {
                        const raw = if (loop_len > 0) stem[(pos + f0) % loop_len] else 0;
                        fxblk[ti][f0] = self.track_fx[ti].process(raw);
                    }
                    fxptr[ti] = fxblk[ti][0..N];
                }
                // dry mix + reverb send bus
                var send_bus: [N]f32 = undefined;
                mixBlocks(fxptr[0..self.ntracks], self.ntracks, N, &tg, &tp, &tm, &ts, &tsend, &pL, &pR, &send_bus, &tpk);
                for (0..self.ntracks) |ti| @atomicStore(u32, &self.tlevel_bits[ti], @bitCast(tpk[ti]), .monotonic);
                // reverb return (mono -> both channels), then master gain on everything
                var rpk: f32 = 0;
                var f1: usize = 0;
                while (f1 < N) : (f1 += 1) {
                    const wet = if (self.reverb) |rv| rv.process(send_bus[f1]) else 0;
                    if (@abs(wet) > rpk) rpk = @abs(wet);
                    pL[f1] = (pL[f1] + wet) * gain;
                    pR[f1] = (pR[f1] + wet) * gain;
                }
                @atomicStore(u32, &self.rlevel_bits, @bitCast(rpk), .monotonic);
                if (loop_len > 0) pos = (pos + N) % loop_len;
            } else if (playing and n > 0) {
                var f0: usize = 0;
                while (f0 < N) : (f0 += 1) {
                    pL[f0] = self.samples[pos] * gain;
                    pR[f0] = pL[f0];
                    pos += 1;
                    if (pos >= n) pos = 0;
                }
            } else {
                for (0..self.ntracks) |ti| @atomicStore(u32, &self.tlevel_bits[ti], @bitCast(@as(f32, 0)), .monotonic);
            }

            // master bus: a stereo-linked brickwall limiter prevents harsh clipping
            var pk: f32 = 0;
            var f: usize = 0;
            while (f < N) : (f += 1) {
                var l = sbL[f] + aL[f] + pL[f]; // synth is stereo + note-gated
                var r = sbR[f] + aR[f] + pR[f];
                self.master_lim.process(&l, &r);
                pk = @max(pk, @max(@abs(l), @abs(r)));
                buf[f * 2] = @intFromFloat(l * 32767.0);
                buf[f * 2 + 1] = @intFromFloat(r * 32767.0);
            }
            @atomicStore(u32, &self.grdb_bits, @bitCast(self.master_lim.grDb()), .monotonic);
            out.writeBlock(&buf) catch {};
            if (playing) self.tl_pos += N; // advance the timeline playhead
            @atomicStore(u64, &self.pos, pos, .monotonic);
            const prev: f32 = @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
            const nx = if (pk > prev) pk else prev * 0.82 + pk * 0.18;
            @atomicStore(u32, &self.peak_bits, @bitCast(nx), .monotonic);
        }
    }

    fn runIn(self: *Engine) void {
        var in = openIn(self.device, self.rate) orelse return;
        defer in.close();
        const N = 512;
        var buf: [N]i16 = undefined;
        while (!@atomicLoad(bool, &self.quit, .monotonic)) {
            const got = in.readBlock(&buf) catch break;
            var pk: f32 = 0;
            for (buf[0..got]) |s| {
                const a = @abs(@as(f32, @floatFromInt(s)) / 32768.0);
                if (a > pk) pk = a;
            }
            const prev: f32 = @bitCast(@atomicLoad(u32, &self.in_peak_bits, .monotonic));
            const nx = if (pk > prev) pk else prev * 0.80 + pk * 0.20;
            @atomicStore(u32, &self.in_peak_bits, @bitCast(nx), .monotonic);
        }
    }
};

test "mixBlocks: fader/mute/solo/pan + reverb send" {
    const b0 = [_]f32{1.0};
    const b1 = [_]f32{1.0};
    const blocks = [_][]const f32{ &b0, &b1 };
    var l: [1]f32 = undefined;
    var r: [1]f32 = undefined;
    var sb: [1]f32 = undefined;
    var pk: [MAXTRACKS]f32 = undefined;
    const expectApprox = std.testing.expectApproxEqAbs;
    const noSend = [_]f32{ 0, 0 };

    // both gain 1, center pan -> 0.7071 each per channel; no send
    mixBlocks(&blocks, 2, 1, &.{ 1, 1 }, &.{ 0, 0 }, &.{ false, false }, &.{ false, false }, &noSend, &l, &r, &sb, &pk);
    try expectApprox(@as(f32, 2 * 0.7071), l[0], 1e-3);
    try expectApprox(@as(f32, 0), sb[0], 1e-6);

    // mute track 0 -> only track 1; muted track reports 0 level
    mixBlocks(&blocks, 2, 1, &.{ 1, 1 }, &.{ 0, 0 }, &.{ true, false }, &.{ false, false }, &noSend, &l, &r, &sb, &pk);
    try expectApprox(@as(f32, 0.7071), l[0], 1e-3);
    try expectApprox(@as(f32, 0), pk[0], 1e-6);

    // solo track 1 -> track 0 dropped though not muted
    mixBlocks(&blocks, 2, 1, &.{ 1, 1 }, &.{ 0, 0 }, &.{ false, false }, &.{ false, true }, &noSend, &l, &r, &sb, &pk);
    try expectApprox(@as(f32, 0), pk[0], 1e-6);
    try expectApprox(@as(f32, 1.0), pk[1], 1e-6);

    // hard-left pan on track 0 (solo it) -> all in L, none in R
    mixBlocks(&blocks, 2, 1, &.{ 1, 1 }, &.{ -1, 0 }, &.{ false, false }, &.{ true, false }, &noSend, &l, &r, &sb, &pk);
    try std.testing.expect(l[0] > 0.9);
    try expectApprox(@as(f32, 0), r[0], 1e-6);

    // reverb send: track 0 gain 1, send 0.5 -> send bus = 0.5
    mixBlocks(&blocks, 2, 1, &.{ 1, 1 }, &.{ 0, 0 }, &.{ false, true }, &.{ false, false }, &.{ 0.5, 0 }, &l, &r, &sb, &pk);
    try expectApprox(@as(f32, 0.5), sb[0], 1e-6);
}

test "MasterLimiter never exceeds the ceiling on a hot signal" {
    var lim = MasterLimiter.init(48000);
    var prng = std.Random.DefaultPrng.init(7);
    const rnd = prng.random();
    var max_out: f32 = 0;
    for (0..20000) |_| {
        var l = (rnd.float(f32) * 2.0 - 1.0) * 3.0; // up to +/-3.0 (way over)
        var r = (rnd.float(f32) * 2.0 - 1.0) * 3.0;
        lim.process(&l, &r);
        max_out = @max(max_out, @max(@abs(l), @abs(r)));
    }
    try std.testing.expect(max_out <= 0.98 + 1e-4); // capped at the ceiling
    try std.testing.expect(lim.grDb() < 0); // gain reduction is active
}

test "TrackFx: high-pass removes DC, compressor tames a loud signal" {
    var fx = TrackFx.init(48000);
    // DC -> high-pass should drive the output toward 0
    var dc: f32 = 0;
    for (0..4000) |_| dc = fx.process(1.0);
    try std.testing.expect(@abs(dc) < 0.2);

    // a loud constant tone gets compressed below its input level (measure after
    // the attack settles — the first samples pass before the compressor engages)
    var fx2 = TrackFx.init(48000);
    var peak: f32 = 0;
    var sign: f32 = 1;
    for (0..20000) |i| {
        const y = fx2.process(sign * 0.9);
        sign = -sign;
        if (i > 10000) peak = @max(peak, @abs(y));
    }
    try std.testing.expect(peak < 0.8); // gain reduction applied (input was 0.9)
}

test "MPE events route through the queue to the synth voices" {
    var e = Engine{ .samples = &.{} };
    e.synth.sample_rate = 48000;
    // two notes on two channels, each with its own bend (MPE)
    e.pushNoteCh(true, noteToFreq(60), 1.0, 0);
    e.pushNoteCh(true, noteToFreq(64), 1.0, 1);
    e.chanBend(1, 1.05); // bend only channel 1
    e.notePressure(64, 0.5); // per-note pressure on note 64
    e.drainNotes();
    // a voice on each channel
    var ch0 = false;
    var ch1 = false;
    for (e.synth.voices) |v| {
        if (v.in_use and v.channel == 0) ch0 = true;
        if (v.in_use and v.channel == 1) ch1 = true;
    }
    try std.testing.expect(ch0 and ch1);
    try std.testing.expectEqual(@as(f32, 1.0), e.synth.chan_bend[0]); // channel 0 unbent
    try std.testing.expectEqual(@as(f32, 1.05), e.synth.chan_bend[1]); // channel 1 bent only
}

/// MIDI note number -> frequency (A4 = 69 = 440 Hz).
pub fn noteToFreq(note: u7) f32 {
    return 440.0 * std.math.pow(f32, 2.0, (@as(f32, @floatFromInt(note)) - 69.0) / 12.0);
}

// ---- MIDI 2.0 high-res controller mapping (32-bit values) -----------------
fn unit32(v: u32) f32 {
    return @as(f32, @floatFromInt(v)) / 4294967295.0; // 0..1
}
/// CC 32-bit value -> filter cutoff Hz (exponential, ~120 Hz .. 12 kHz).
pub fn ccToCutoff(v: u32) f32 {
    return 120.0 * std.math.pow(f32, 12000.0 / 120.0, unit32(v));
}
/// CC 32-bit value -> 0..1 (resonance, etc.).
pub fn ccToUnit(v: u32) f32 {
    return unit32(v);
}
/// CC 32-bit value -> -1..1 (bipolar, e.g. MPE timbre around center).
pub fn ccToBipolar(v: u32) f32 {
    return unit32(v) * 2.0 - 1.0;
}
/// 32-bit pitch bend (center 0x8000_0000) -> frequency multiplier (±`semis`).
pub fn bendToRatio(v: u32, semis: f32) f32 {
    const n = (unit32(v) - 0.5) * 2.0; // -1..+1
    return std.math.pow(f32, 2.0, n * semis / 12.0);
}
