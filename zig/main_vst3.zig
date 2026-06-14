//! main_vst3.zig — host a VST3 plugin end-to-end and pull audio through it.
//! Loads the module, instantiates IComponent + IAudioProcessor, sets up
//! processing, activates the output bus, and runs process(); writes the result
//! to a WAV and asserts the plugin produced non-silent audio.

const std = @import("std");
const v = @import("vst3_abi.zig");
const host = @import("vst3_host.zig");
const wav = @import("wav.zig");

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
    var ci_idx: i32 = 0;
    while (ci_idx < mod.classCount()) : (ci_idx += 1) {
        if (mod.classInfo(ci_idx)) |ci| {
            std.debug.print("  class[{d}] '{s}' category='{s}'\n", .{ ci_idx, host.sliceZ(&ci.name), host.sliceZ(&ci.category) });
        }
    }

    const component = try mod.createComponent();
    if (component.vtbl.initialize(component, null) != v.kResultOk) return error.InitFailed;
    defer _ = component.vtbl.terminate(component);

    const processor = host.getProcessor(component) orelse return error.NoProcessor;
    std.debug.print("got IAudioProcessor via queryInterface\n", .{});

    var setup = v.ProcessSetup{ .process_mode = v.kRealtime, .symbolic_sample_size = v.kSample32, .max_samples_per_block = @intCast(block), .sample_rate = @floatFromInt(sr) };
    if (processor.vtbl.setupProcessing(processor, &setup) != v.kResultOk) return error.SetupFailed;
    if (processor.vtbl.canProcessSampleSize(processor, v.kSample32) != v.kResultOk) return error.No32Bit;

    const nout = component.vtbl.getBusCount(component, v.kAudio, v.kOutput);
    var binfo: v.BusInfo = std.mem.zeroes(v.BusInfo);
    _ = component.vtbl.getBusInfo(component, v.kAudio, v.kOutput, 0, &binfo);
    std.debug.print("audio outputs: {d} bus(es), bus0 channels={d}\n", .{ nout, binfo.channel_count });
    const nch: usize = @intCast(binfo.channel_count);

    _ = component.vtbl.activateBus(component, v.kAudio, v.kOutput, 0, 1);
    if (component.vtbl.setActive(component, 1) != v.kResultOk) return error.SetActiveFailed;
    defer _ = component.vtbl.setActive(component, 0);
    _ = processor.vtbl.setProcessing(processor, 1);
    defer _ = processor.vtbl.setProcessing(processor, 0);

    // channel buffers
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

    const total: usize = @intFromFloat(0.5 * @as(f64, @floatFromInt(sr))); // 0.5s
    var done: usize = 0;
    while (done < total) {
        const n: u32 = @intCast(@min(@as(usize, block), total - done));
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
            .input_events = null,
            .output_events = null,
            .process_context = null,
        };
        if (processor.vtbl.process(processor, &data) != v.kResultOk) return error.ProcessFailed;
        try rec.appendSlice(chbufs[0][0..n]); // left channel
        done += n;
    }

    var peak: f32 = 0;
    for (rec.items) |s| peak = @max(peak, @abs(s));
    try wav.writePcm16("vst3_demo.wav", rec.items, sr, 1);
    std.debug.print("processed {d} samples through the VST3 plugin -> vst3_demo.wav (peak {d:.3})\n", .{ rec.items.len, peak });

    if (peak < 0.1) return error.SilentOutput;
    std.debug.print("OK: VST3 host loaded, instantiated, set up, and pulled non-silent audio.\n", .{});
}
