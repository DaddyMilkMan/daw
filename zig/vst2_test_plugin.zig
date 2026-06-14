//! vst2_test_plugin.zig — a minimal VST2 plugin (.so): a 440 Hz stereo synth
//! (0 inputs, 2 outputs) so processReplacing() yields verifiable audio. Exports
//! VSTPluginMain. Shares the clean-room ABI in vst2_abi.zig with the host.

const std = @import("std");
const v = @import("vst2_abi.zig");

var g_host: ?v.HostCallback = null;
var g_sr: f64 = 48000.0;
var g_gain: f32 = 0.5;

// poly sine synth driven by MIDI note events
const NVOICES = 16;
const Voice = struct { active: bool = false, key: i32 = 0, phase: f64 = 0, freq: f64 = 0, env: f32 = 0, releasing: bool = false };
var g_voices: [NVOICES]Voice = [_]Voice{.{}} ** NVOICES;

fn noteOn(key: i32) void {
    const freq = 440.0 * std.math.pow(f64, 2.0, @as(f64, @floatFromInt(key - 69)) / 12.0);
    for (&g_voices) |*vc| if (!vc.active) {
        vc.* = .{ .active = true, .key = key, .phase = 0, .freq = freq, .env = 0, .releasing = false };
        return;
    };
}
fn noteOff(key: i32) void {
    for (&g_voices) |*vc| if (vc.active and vc.key == key) {
        vc.releasing = true;
    };
}
fn renderSample() f32 {
    var s: f32 = 0;
    for (&g_voices) |*vc| {
        if (!vc.active) continue;
        if (vc.releasing) {
            vc.env -= 0.0004;
            if (vc.env <= 0) {
                vc.active = false;
                continue;
            }
        } else if (vc.env < 0.7) vc.env += 0.003;
        s += @as(f32, @floatCast(@sin(vc.phase))) * vc.env;
        vc.phase += 2.0 * std.math.pi * vc.freq / g_sr;
        if (vc.phase > 2.0 * std.math.pi) vc.phase -= 2.0 * std.math.pi;
    }
    return s * g_gain;
}

fn dispatcher(_: *v.AEffect, opcode: i32, _: i32, _: isize, ptr: ?*anyopaque, opt: f32) callconv(.c) isize {
    switch (opcode) {
        v.effOpen, v.effClose, v.effMainsChanged => return 0,
        v.effSetSampleRate => {
            g_sr = opt;
            g_voices = [_]Voice{.{}} ** NVOICES;
            return 0;
        },
        v.effSetBlockSize => return 0,
        v.effProcessEvents => {
            const p = ptr orelse return 0;
            const evs: *const v.VstEvents = @ptrCast(@alignCast(p));
            var i: usize = 0;
            const count: usize = @intCast(@max(evs.num_events, 0));
            while (i < count and i < evs.events.len) : (i += 1) {
                const ev = evs.events[i] orelse continue;
                if (ev.type != v.kVstMidiType) continue;
                const me: *const v.VstMidiEvent = @ptrCast(ev);
                const status = me.midi_data[0] & 0xF0;
                const note: i32 = me.midi_data[1];
                const vel = me.midi_data[2];
                if (status == 0x90 and vel > 0) noteOn(note) else if (status == 0x80 or (status == 0x90 and vel == 0)) noteOff(note);
            }
            return 1;
        },
        v.effGetEffectName, v.effGetProductString => {
            if (ptr) |p| {
                const buf: [*]u8 = @ptrCast(p);
                const name = "Zenith VST2 Test";
                @memcpy(buf[0..name.len], name);
                buf[name.len] = 0;
            }
            return 1;
        },
        v.effGetVendorString => {
            if (ptr) |p| {
                const buf: [*]u8 = @ptrCast(p);
                const s = "Zenith";
                @memcpy(buf[0..s.len], s);
                buf[s.len] = 0;
            }
            return 1;
        },
        v.effGetVstVersion => return 2400,
        else => return 0,
    }
}

fn setParameter(_: *v.AEffect, index: i32, value: f32) callconv(.c) void {
    if (index == 0) g_gain = value;
}
fn getParameter(_: *v.AEffect, index: i32) callconv(.c) f32 {
    return if (index == 0) g_gain else 0;
}

fn processReplacing(effect: *v.AEffect, _: [*]const [*]f32, outputs: [*]const [*]f32, frames: i32) callconv(.c) void {
    const nch: usize = @intCast(effect.numOutputs);
    const n: usize = @intCast(frames);
    var i: usize = 0;
    while (i < n) : (i += 1) {
        const s = renderSample();
        var c: usize = 0;
        while (c < nch) : (c += 1) outputs[c][i] = s;
    }
}

var g_effect = v.AEffect{
    .magic = v.kEffectMagic,
    .dispatcher = dispatcher,
    .process = processReplacing,
    .setParameter = setParameter,
    .getParameter = getParameter,
    .numPrograms = 0,
    .numParams = 1,
    .numInputs = 0,
    .numOutputs = 2,
    .flags = v.effFlagsCanReplacing | v.effFlagsIsSynth,
    .resvd1 = 0,
    .resvd2 = 0,
    .initialDelay = 0,
    .realQualities = 0,
    .offQualities = 0,
    .ioRatio = 1.0,
    .object = null,
    .user = null,
    .uniqueID = 0x5A546B32, // 'ZTk2'
    .version = 1000,
    .processReplacing = processReplacing,
    .processDoubleReplacing = null,
};

export fn VSTPluginMain(host: v.HostCallback) callconv(.c) ?*v.AEffect {
    g_host = host;
    return &g_effect;
}
