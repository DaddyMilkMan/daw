//! main_vst3.zig — host a VST3 *instrument* end-to-end: instantiate IComponent +
//! IAudioProcessor, feed it a note via a host-implemented IEventList, run
//! process(), and verify (by FFT) that the plugin synthesized the correct pitch.

const std = @import("std");
const v = @import("vst3_abi.zig");
const host = @import("vst3_host.zig");
const wav = @import("wav.zig");
const dsp = @import("dsp.zig");

// --- host-side IEventList the plugin reads its notes from ---
var g_events: [16]v.Event = undefined;
var g_event_count: i32 = 0;

fn elQI(self: *anyopaque, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (std.mem.eql(u8, iid[0..16], &v.IEventList_iid) or std.mem.eql(u8, iid[0..16], &v.FUnknown_iid)) {
        obj.* = self;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}
fn elRef(_: *anyopaque) callconv(.c) u32 {
    return 1;
}
fn elCount(_: *anyopaque) callconv(.c) i32 {
    return g_event_count;
}
fn elGet(_: *anyopaque, index: i32, out: *v.Event) callconv(.c) v.tresult {
    if (index < 0 or index >= g_event_count) return v.kInvalidArgument;
    out.* = g_events[@intCast(index)];
    return v.kResultOk;
}
fn elAdd(_: *anyopaque, _: *v.Event) callconv(.c) v.tresult {
    return v.kResultOk;
}
const el_vtbl = v.EventListVtbl{ .queryInterface = elQI, .addRef = elRef, .release = elRef, .getEventCount = elCount, .getEvent = elGet, .addEvent = elAdd };
var g_eventlist = v.EventList{ .vtbl = &el_vtbl };

fn noteOnEvent(pitch: i16, offset: i32) v.Event {
    var e = std.mem.zeroes(v.Event);
    e.bus_index = 0;
    e.sample_offset = offset;
    e.type = v.kNoteOnEvent;
    e.data.note_on = .{ .channel = 0, .pitch = pitch, .tuning = 0, .velocity = 0.9, .length = 0, .note_id = -1 };
    return e;
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();

    var args = std.process.args();
    _ = args.next();
    const path = args.next() orelse "zig-out/lib/libzenith_vst3_test.so";

    const sr: u32 = 48000;
    const block: u32 = 256;

    var mod = try host.Module.open(a, path);
    defer mod.close();
    const fi = mod.factoryInfo();
    std.debug.print("module '{s}': vendor='{s}' classes={d}\n", .{ path, host.sliceZ(&fi.vendor), mod.classCount() });

    const component = try mod.createComponent();
    if (component.vtbl.initialize(component, null) != v.kResultOk) return error.InitFailed;
    defer _ = component.vtbl.terminate(component);

    const processor = host.getProcessor(component) orelse return error.NoProcessor;

    const n_audio_out = component.vtbl.getBusCount(component, v.kAudio, v.kOutput);
    const n_event_in = component.vtbl.getBusCount(component, v.kEvent, v.kInput);
    std.debug.print("buses: audio-out={d} event-in={d} (instrument={})\n", .{ n_audio_out, n_event_in, n_event_in > 0 });

    var setup = v.ProcessSetup{ .process_mode = v.kRealtime, .symbolic_sample_size = v.kSample32, .max_samples_per_block = @intCast(block), .sample_rate = @floatFromInt(sr) };
    if (processor.vtbl.setupProcessing(processor, &setup) != v.kResultOk) return error.SetupFailed;

    var binfo: v.BusInfo = std.mem.zeroes(v.BusInfo);
    _ = component.vtbl.getBusInfo(component, v.kAudio, v.kOutput, 0, &binfo);
    const nch: usize = @intCast(binfo.channel_count);

    _ = component.vtbl.activateBus(component, v.kAudio, v.kOutput, 0, 1);
    if (n_event_in > 0) _ = component.vtbl.activateBus(component, v.kEvent, v.kInput, 0, 1);
    if (component.vtbl.setActive(component, 1) != v.kResultOk) return error.SetActiveFailed;
    defer _ = component.vtbl.setActive(component, 0);
    _ = processor.vtbl.setProcessing(processor, 1);
    defer _ = processor.vtbl.setProcessing(processor, 0);

    const chbufs = try a.alloc([]f32, nch);
    defer {
        for (chbufs) |c| a.free(c);
        a.free(chbufs);
    }
    const chptrs = try a.alloc(?[*]f32, nch);
    defer a.free(chptrs);
    for (chbufs, 0..) |*cb, i| {
        cb.* = try a.alloc(f32, block);
        chptrs[i] = cb.*.ptr;
    }
    var out_bus = v.AudioBusBuffers{ .num_channels = @intCast(nch), .silence_flags = 0, .channel_buffers_32 = chptrs.ptr };

    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    // Play A4 (MIDI 69 -> 440 Hz): note-on in the first block only.
    const total: usize = @intFromFloat(0.5 * @as(f64, @floatFromInt(sr)));
    var done: usize = 0;
    var first = true;
    while (done < total) {
        const n: u32 = @intCast(@min(@as(usize, block), total - done));
        if (first) {
            g_events[0] = noteOnEvent(69, 0);
            g_event_count = 1;
            first = false;
        } else g_event_count = 0;
        var data = v.ProcessData{
            .process_mode = v.kRealtime,
            .symbolic_sample_size = v.kSample32,
            .num_samples = @intCast(n),
            .num_inputs = 0,
            .num_outputs = 1,
            .inputs = null,
            .outputs = @ptrCast(&out_bus),
            .input_param_changes = null,
            .output_param_changes = null,
            .input_events = @ptrCast(&g_eventlist),
            .output_events = null,
            .process_context = null,
        };
        if (processor.vtbl.process(processor, &data) != v.kResultOk) return error.ProcessFailed;
        try rec.appendSlice(chbufs[0][0..n]);
        done += n;
    }

    try wav.writePcm16("vst3_demo.wav", rec.items, sr, 1);
    const freq = try dominantFreq(a, rec.items, sr);
    var peak: f32 = 0;
    for (rec.items) |s| peak = @max(peak, @abs(s));
    std.debug.print("instrument played MIDI 69 -> dominant {d:.1} Hz (peak {d:.3}) -> vst3_demo.wav\n", .{ freq, peak });

    if (peak < 0.1) return error.SilentOutput;
    if (@abs(freq - 440.0) > 15.0) return error.WrongPitch;
    std.debug.print("OK: VST3 instrument hosted — note event delivered, synthesized 440 Hz.\n", .{});
}

/// Hann-windowed FFT peak -> dominant frequency, from a steady-state slice.
fn dominantFreq(a: std.mem.Allocator, samples: []const f32, sr: u32) !f32 {
    const off: usize = @min(samples.len / 2, 8192);
    const avail = samples.len - off;
    var n: usize = 1;
    while (n * 2 <= avail and n < 8192) n *= 2;
    if (n < 8) return 0;
    const buf = try a.alloc(dsp.Complex, n);
    defer a.free(buf);
    for (buf, 0..) |*c, k| {
        const w = 0.5 - 0.5 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1)));
        c.* = .{ .re = samples[off + k] * w, .im = 0 };
    }
    dsp.fft(buf, false);
    var max_bin: usize = 1;
    var max_mag: f32 = 0;
    for (buf[1 .. n / 2], 1..) |c, k| {
        const m = c.mag();
        if (m > max_mag) {
            max_mag = m;
            max_bin = k;
        }
    }
    return @as(f32, @floatFromInt(max_bin)) * @as(f32, @floatFromInt(sr)) / @as(f32, @floatFromInt(n));
}
