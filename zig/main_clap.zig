//! main_clap.zig — a CLAP plugin host. Loads a .clap shared library, gets the
//! factory, instantiates a plugin, activates it, and processes audio through it.
//! Verified against our test sine plugin; the same path loads real .clap files.

const std = @import("std");
const abi = @import("clap_abi.zig");
const wav = @import("wav.zig");

fn hostGetExt(_: *const abi.Host, _: [*:0]const u8) callconv(.c) ?*const anyopaque {
    return null;
}
fn hostReqRestart(_: *const abi.Host) callconv(.c) void {}
fn hostReqProcess(_: *const abi.Host) callconv(.c) void {}
fn hostReqCallback(_: *const abi.Host) callconv(.c) void {}

fn inSize(_: *const abi.InputEvents) callconv(.c) u32 {
    return 0;
}
fn inGet(_: *const abi.InputEvents, _: u32) callconv(.c) ?*const abi.EventHeader {
    return null;
}
fn outTryPush(_: *const abi.OutputEvents, _: *const abi.EventHeader) callconv(.c) bool {
    return true;
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;

    var args = std.process.args();
    _ = args.next();
    const path = args.next() orelse "zig-out/lib/libzenith_test_clap.so";

    var lib = std.DynLib.open(path) catch |e| {
        std.debug.print("failed to open '{s}': {any}\n", .{ path, e });
        return e;
    };
    defer lib.close();

    const entry = lib.lookup(*const abi.PluginEntry, "clap_entry") orelse {
        std.debug.print("no clap_entry symbol in {s}\n", .{path});
        return error.NoEntry;
    };
    std.debug.print("loaded '{s}': CLAP {d}.{d}.{d}\n", .{ path, entry.clap_version.major, entry.clap_version.minor, entry.clap_version.revision });

    if (!entry.init.?(path)) return error.EntryInit;
    defer entry.deinit.?();

    const fac_any = entry.get_factory.?(abi.PLUGIN_FACTORY_ID) orelse return error.NoFactory;
    const factory: *const abi.PluginFactory = @ptrCast(@alignCast(fac_any));
    const count = factory.get_plugin_count.?(factory);
    std.debug.print("factory: {d} plugin(s)\n", .{count});
    const desc = factory.get_plugin_descriptor.?(factory, 0) orelse return error.NoDesc;
    std.debug.print("  -> id='{s}'  name='{s}'  vendor='{s}'\n", .{ desc.id.?, desc.name.?, desc.vendor.? });

    var host = abi.Host{
        .clap_version = .{},
        .host_data = null,
        .name = "Zenith",
        .vendor = "Zenith",
        .url = "",
        .version = "0.1",
        .get_extension = hostGetExt,
        .request_restart = hostReqRestart,
        .request_process = hostReqProcess,
        .request_callback = hostReqCallback,
    };

    const plugin = factory.create_plugin.?(factory, &host, desc.id.?) orelse return error.CreateFailed;
    if (!plugin.init.?(plugin)) return error.PluginInit;
    defer plugin.destroy.?(plugin);

    const block: u32 = 256;
    if (!plugin.activate.?(plugin, @floatFromInt(sr), block, block)) return error.Activate;
    defer plugin.deactivate.?(plugin);
    _ = plugin.start_processing.?(plugin);
    defer plugin.stop_processing.?(plugin);

    // one mono output buffer
    const chan = try a.alloc(f32, block);
    defer a.free(chan);
    var chan_ptrs = [_]?[*]f32{chan.ptr};
    var out_buf = abi.AudioBuffer{
        .data32 = &chan_ptrs,
        .data64 = null,
        .channel_count = 1,
        .latency = 0,
        .constant_mask = 0,
    };
    var in_events = abi.InputEvents{ .ctx = null, .size = inSize, .get = inGet };
    var out_events = abi.OutputEvents{ .ctx = null, .try_push = outTryPush };

    const total: usize = @intFromFloat(2.0 * @as(f64, @floatFromInt(sr)));
    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    var done: usize = 0;
    var steady: i64 = 0;
    while (done < total) {
        const n: u32 = @intCast(@min(@as(usize, block), total - done));
        var proc = abi.Process{
            .steady_time = steady,
            .frames_count = n,
            .transport = null,
            .audio_inputs = null,
            .audio_outputs = @ptrCast(&out_buf),
            .audio_inputs_count = 0,
            .audio_outputs_count = 1,
            .in_events = &in_events,
            .out_events = &out_events,
        };
        const status = plugin.process.?(plugin, &proc);
        if (status == 0) {
            std.debug.print("process returned ERROR\n", .{});
            break;
        }
        try rec.appendSlice(chan[0..n]);
        done += n;
        steady += @intCast(n);
    }

    try wav.writePcm16("clap_demo.wav", rec.items, sr, 1);
    std.debug.print("hosted plugin produced {d} frames -> clap_demo.wav\n", .{done});
}
