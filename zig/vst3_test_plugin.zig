//! vst3_test_plugin.zig — a minimal VST3 plugin (.so) that implements the COM
//! interfaces our host drives: IPluginFactory -> IComponent + IAudioProcessor.
//! It's a 440 Hz stereo tone generator (1 audio output, 0 inputs) so process()
//! produces verifiable non-silent audio. Both host and plugin share vst3_abi.zig.
//!
//! State lives in globals (a single instance is enough to verify the host path);
//! the COM `this` pointers are ignored, which is legal at the ABI level.

const std = @import("std");
const v = @import("vst3_abi.zig");

const class_cid = v.uid(0x5A454E49, 0x54485633, 0x54455354, 0x504C5547); // "ZENI THV3 TEST PLUG"

var g_sr: f64 = 48000.0;

// --- tiny poly sine synth driven by note events ---
const NVOICES = 16;
const Voice = struct { active: bool = false, key: i16 = 0, phase: f64 = 0, freq: f64 = 0, env: f32 = 0, releasing: bool = false };
var g_voices: [NVOICES]Voice = [_]Voice{.{}} ** NVOICES;

fn noteOn(key: i16) void {
    const freq = 440.0 * std.math.pow(f64, 2.0, @as(f64, @floatFromInt(key - 69)) / 12.0);
    for (&g_voices) |*vc| {
        if (!vc.active) {
            vc.* = .{ .active = true, .key = key, .phase = 0, .freq = freq, .env = 0, .releasing = false };
            return;
        }
    }
}
fn noteOff(key: i16) void {
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
    return s * 0.4;
}

fn iidEql(p: [*]const u8, target: v.TUID) bool {
    return std.mem.eql(u8, p[0..16], &target);
}
fn putUtf16(dst: []u16, s: []const u8) void {
    @memset(dst, 0);
    var i: usize = 0;
    while (i < s.len and i < dst.len - 1) : (i += 1) dst[i] = s[i];
}
fn putAscii(dst: []u8, s: []const u8) void {
    @memset(dst, 0);
    const n = @min(dst.len - 1, s.len);
    @memcpy(dst[0..n], s[0..n]);
}

// ---------------- IComponent ----------------
fn cQI(_: *anyopaque, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (iidEql(iid, v.IComponent_iid) or iidEql(iid, v.IPluginBase_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_component;
        return v.kResultOk;
    }
    if (iidEql(iid, v.IAudioProcessor_iid)) {
        obj.* = &g_processor;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}
fn refOne(_: *anyopaque) callconv(.c) u32 {
    return 1;
}
fn cInitialize(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cTerminate(_: *anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetCtrlId(_: *anyopaque, _: [*]u8) callconv(.c) v.tresult {
    return v.kNotImplemented;
}
fn cSetIoMode(_: *anyopaque, _: i32) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetBusCount(_: *anyopaque, media: i32, dir: i32) callconv(.c) i32 {
    if (media == v.kAudio and dir == v.kOutput) return 1; // one stereo output
    if (media == v.kEvent and dir == v.kInput) return 1; // one event (MIDI) input
    return 0;
}
fn cGetBusInfo(_: *anyopaque, media: i32, dir: i32, index: i32, info: *v.BusInfo) callconv(.c) v.tresult {
    if (media == v.kAudio and dir == v.kOutput and index == 0) {
        info.* = .{ .media_type = v.kAudio, .direction = v.kOutput, .channel_count = 2, .name = undefined, .bus_type = v.kMain, .flags = v.kDefaultActive };
        putUtf16(&info.name, "Stereo Out");
        return v.kResultOk;
    }
    if (media == v.kEvent and dir == v.kInput and index == 0) {
        info.* = .{ .media_type = v.kEvent, .direction = v.kInput, .channel_count = 16, .name = undefined, .bus_type = v.kMain, .flags = v.kDefaultActive };
        putUtf16(&info.name, "Event In");
        return v.kResultOk;
    }
    return v.kInvalidArgument;
}
fn cGetRouting(_: *anyopaque, _: *anyopaque, _: *anyopaque) callconv(.c) v.tresult {
    return v.kNotImplemented;
}
fn cActivateBus(_: *anyopaque, _: i32, _: i32, _: i32, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cSetActive(_: *anyopaque, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cSetState(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetState(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}

const component_vtbl = v.ComponentVtbl{
    .queryInterface = cQI,
    .addRef = refOne,
    .release = refOne,
    .initialize = cInitialize,
    .terminate = cTerminate,
    .getControllerClassId = cGetCtrlId,
    .setIoMode = cSetIoMode,
    .getBusCount = cGetBusCount,
    .getBusInfo = cGetBusInfo,
    .getRoutingInfo = cGetRouting,
    .activateBus = cActivateBus,
    .setActive = cSetActive,
    .setState = cSetState,
    .getState = cGetState,
};
var g_component = v.Component{ .vtbl = &component_vtbl };

// ---------------- IAudioProcessor ----------------
fn pSetArr(_: *anyopaque, _: ?[*]u64, _: i32, _: ?[*]u64, _: i32) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn pGetArr(_: *anyopaque, _: i32, _: i32, arr: *u64) callconv(.c) v.tresult {
    arr.* = 0x3; // kStereo (L|R)
    return v.kResultOk;
}
fn pCanSize(_: *anyopaque, size: i32) callconv(.c) v.tresult {
    return if (size == v.kSample32) v.kResultOk else v.kResultFalse;
}
fn pLatency(_: *anyopaque) callconv(.c) u32 {
    return 0;
}
fn pSetup(_: *anyopaque, setup: *v.ProcessSetup) callconv(.c) v.tresult {
    g_sr = setup.sample_rate;
    g_voices = [_]Voice{.{}} ** NVOICES;
    return v.kResultOk;
}
fn pSetProcessing(_: *anyopaque, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn pProcess(_: *anyopaque, data: *v.ProcessData) callconv(.c) v.tresult {
    if (data.symbolic_sample_size != v.kSample32) return v.kResultOk;
    const outs = data.outputs orelse return v.kResultOk;
    if (data.num_outputs < 1) return v.kResultOk;
    const bus = &outs[0];
    const chans = bus.channel_buffers_32 orelse return v.kResultOk;
    const n: usize = @intCast(data.num_samples);

    // pull note events from the host's input event list
    var ev_count: i32 = 0;
    var ev_idx: i32 = 0;
    var el: ?*v.EventList = null;
    if (data.input_events) |raw| {
        el = @ptrCast(@alignCast(raw));
        ev_count = el.?.vtbl.getEventCount(el.?);
    }

    var i: usize = 0;
    while (i < n) : (i += 1) {
        // apply any events scheduled at or before this frame (sample-accurate)
        while (ev_idx < ev_count) {
            var e: v.Event = undefined;
            if (el.?.vtbl.getEvent(el.?, ev_idx, &e) != v.kResultOk) break;
            if (@as(usize, @intCast(e.sample_offset)) > i) break;
            if (e.type == v.kNoteOnEvent) noteOn(e.data.note_on.pitch) else if (e.type == v.kNoteOffEvent) noteOff(e.data.note_off.pitch);
            ev_idx += 1;
        }
        const s = renderSample();
        var c: usize = 0;
        while (c < @as(usize, @intCast(bus.num_channels))) : (c += 1) {
            if (chans[c]) |ch| ch[i] = s;
        }
    }
    bus.silence_flags = 0;
    return v.kResultOk;
}
fn pTail(_: *anyopaque) callconv(.c) u32 {
    return 0;
}

const processor_vtbl = v.ProcessorVtbl{
    .queryInterface = cQI, // same dispatcher: knows IComponent + IAudioProcessor
    .addRef = refOne,
    .release = refOne,
    .setBusArrangements = pSetArr,
    .getBusArrangement = pGetArr,
    .canProcessSampleSize = pCanSize,
    .getLatencySamples = pLatency,
    .setupProcessing = pSetup,
    .setProcessing = pSetProcessing,
    .process = pProcess,
    .getTailSamples = pTail,
};
var g_processor = v.Processor{ .vtbl = &processor_vtbl };

// ---------------- IPluginFactory ----------------
fn fQI(_: *anyopaque, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (iidEql(iid, v.IPluginFactory_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_factory;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}
fn fGetInfo(_: *anyopaque, info: *v.PFactoryInfo) callconv(.c) v.tresult {
    info.* = std.mem.zeroes(v.PFactoryInfo);
    putAscii(&info.vendor, "Zenith");
    putAscii(&info.url, "https://zenith.audio");
    info.flags = 0x10; // kUnicode
    return v.kResultOk;
}
fn fCount(_: *anyopaque) callconv(.c) i32 {
    return 1;
}
fn fGetClass(_: *anyopaque, index: i32, info: *v.PClassInfo) callconv(.c) v.tresult {
    if (index != 0) return v.kInvalidArgument;
    info.* = std.mem.zeroes(v.PClassInfo);
    info.cid = class_cid;
    info.cardinality = v.kManyInstances;
    putAscii(&info.category, "Audio Module Class");
    putAscii(&info.name, "Zenith VST3 Test");
    return v.kResultOk;
}
fn fCreate(_: *anyopaque, cid: [*]const u8, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (!iidEql(cid, class_cid)) {
        obj.* = null;
        return v.kNoInterface;
    }
    if (iidEql(iid, v.IComponent_iid) or iidEql(iid, v.IPluginBase_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_component;
        return v.kResultOk;
    }
    if (iidEql(iid, v.IAudioProcessor_iid)) {
        obj.* = &g_processor;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}

const factory_vtbl = v.FactoryVtbl{
    .queryInterface = fQI,
    .addRef = refOne,
    .release = refOne,
    .getFactoryInfo = fGetInfo,
    .countClasses = fCount,
    .getClassInfo = fGetClass,
    .createInstance = fCreate,
};
var g_factory = v.Factory{ .vtbl = &factory_vtbl };

// ---------------- module entry points ----------------
export fn GetPluginFactory() callconv(.c) ?*v.Factory {
    return &g_factory;
}
export fn ModuleEntry(_: ?*anyopaque) callconv(.c) bool {
    return true;
}
export fn ModuleExit() callconv(.c) bool {
    return true;
}
