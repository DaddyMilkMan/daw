//! demo.zig — the shared "song": a short phrase so both the WAV render and the
//! live ALSA path play exactly the same thing. Arpeggiate C-E-G-C, then hold a
//! C-major chord. Proves polyphony + envelope + filter end to end.

const std = @import("std");
const synth = @import("synth.zig");

pub fn midiToFreq(note: f32) f32 {
    return 440.0 * std.math.pow(f32, 2.0, (note - 69.0) / 12.0);
}

pub fn renderDemo(s: *synth.Synth, buf: []f32, sr: u32) void {
    const srf: f32 = @floatFromInt(sr);
    const total = buf.len;

    const arp = [_]f32{ 60, 64, 67, 72 }; // C4 E4 G4 C5
    const step: usize = @intFromFloat(0.35 * srf);
    const block: usize = 256;

    var i: usize = 0;
    var next_event: usize = 0;
    var arp_idx: usize = 0;
    var prev_freq: f32 = -1.0;
    var chord_started = false;

    while (i < total) {
        if (i >= next_event) {
            if (arp_idx < arp.len) {
                if (prev_freq > 0) s.noteOff(prev_freq);
                const fr = midiToFreq(arp[arp_idx]);
                s.noteOn(fr, 1.0);
                prev_freq = fr;
                arp_idx += 1;
                next_event = i + step;
            } else if (!chord_started) {
                if (prev_freq > 0) s.noteOff(prev_freq);
                s.noteOn(midiToFreq(60), 1.0);
                s.noteOn(midiToFreq(64), 1.0);
                s.noteOn(midiToFreq(67), 1.0);
                s.noteOn(midiToFreq(72), 1.0);
                chord_started = true;
                next_event = total;
            }
        }
        const n = @min(block, total - i);
        s.renderBlock(buf[i .. i + n]);
        i += n;
    }
}
