//! main_clap.zig — CLAP host that plays a melody through a hosted instrument.
//! Loads a .clap, instantiates, and feeds sample-accurate note events per block.

const std = @import("std");
const abi = @import("clap_abi.zig");
const wav = @import("wav.zig");

fn hostGetExt(_: *const abi.Host, _: [*:0]const u8) callconv(.c) ?*const anyopaque {
    return null;
}
fn hostReq(_: *const abi.Host) callconv(.c) void {}

// --- host-side event list for the current block ---
const MAXEV = 64;
const BlockEvents = struct {
    notes: [MAXEV]abi.EventNote = undefined,
    count: u32 = 0,
};
var g_block: BlockEvents = .{};

fn inSize(list: *const abi.InputEvents) callconv(.c) u32 {
    const ctx: *const BlockEvents = @ptrCast(@alignCast(list.ctx.?));
    return ctx.count;
}
fn inGet(list: *const abi.InputEvents, index: u32) callconv(.c) ?*const abi.EventHeader {
    const ctx: *BlockEvents = @ptrCast(@alignCast(list.ctx.?));
    if (index >= ctx.count) return null;
    return &ctx.notes[index].header;
}
fn outTryPush(_: *const abi.OutputEvents, _: *const abi.EventHeader) callconv(.c) bool {
    return true;
}

const NoteEvent = struct { frame: u64, on: bool, key: i16 };

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);

    var args = std.process.args();
    _ = args.next();
    const path = args.next() orelse "zig-out/lib/libzenith_test_clap.so";

    var lib = std.DynLib.open(path) catch |e| {
        std.debug.print("open '{s}' failed: {any}\n", .{ path, e });
        return e;
    };
    defer lib.close();
    const entry = lib.lookup(*const abi.PluginEntry, "clap_entry") orelse return error.NoEntry;
    std.debug.print("loaded '{s}': CLAP {d}.{d}.{d}\n", .{ path, entry.clap_version.major, entry.clap_version.minor, entry.clap_version.revision });
    if (!entry.init.?(path)) return error.EntryInit;
    defer entry.deinit.?();

    const fac: *const abi.PluginFactory = @ptrCast(@alignCast(entry.get_factory.?(abi.PLUGIN_FACTORY_ID) orelse return error.NoFactory));
    const desc = fac.get_plugin_descriptor.?(fac, 0) orelse return error.NoDesc;
    std.debug.print("plugin: '{s}' by '{s}'\n", .{ desc.name.?, desc.vendor.? });

    var host = abi.Host{
        .clap_version = .{},
        .host_data = null,
        .name = "Zenith",
        .vendor = "Zenith",
        .url = "",
        .version = "0.1",
        .get_extension = hostGetExt,
        .request_restart = hostReq,
        .request_process = hostReq,
        .request_callback = hostReq,
    };
    const plugin = fac.create_plugin.?(fac, &host, desc.id.?) orelse return error.CreateFailed;
    if (!plugin.init.?(plugin)) return error.PluginInit;
    defer plugin.destroy.?(plugin);

    const block: u32 = 256;
    if (!plugin.activate.?(plugin, srf, block, block)) return error.Activate;
    defer plugin.deactivate.?(plugin);
    _ = plugin.start_processing.?(plugin);
    defer plugin.stop_processing.?(plugin);

    // a C-major scale, one note every 0.25s (each held 0.2s)
    const keys = [_]i16{ 60, 62, 64, 65, 67, 69, 71, 72 };
    var melody = std.ArrayList(NoteEvent).init(a);
    defer melody.deinit();
    const step: u64 = @intFromFloat(0.25 * srf);
    const gate: u64 = @intFromFloat(0.20 * srf);
    for (keys, 0..) |k, idx| {
        try melody.append(.{ .frame = @as(u64, idx) * step, .on = true, .key = k });
        try melody.append(.{ .frame = @as(u64, idx) * step + gate, .on = false, .key = k });
    }

    const chan = try a.alloc(f32, block);
    defer a.free(chan);
    var chan_ptrs = [_]?[*]f32{chan.ptr};
    var out_buf = abi.AudioBuffer{ .data32 = &chan_ptrs, .data64 = null, .channel_count = 1, .latency = 0, .constant_mask = 0 };
    var in_events = abi.InputEvents{ .ctx = &g_block, .size = inSize, .get = inGet };
    var out_events = abi.OutputEvents{ .ctx = null, .try_push = outTryPush };

    const total: usize = @intFromFloat(2.5 * srf);
    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    var done: usize = 0;
    var mi: usize = 0;
    while (done < total) {
        const n: u32 = @intCast(@min(@as(usize, block), total - done));

        // gather this block's note events (time = offset within block, sorted)
        g_block.count = 0;
        while (mi < melody.items.len and melody.items[mi].frame < done + n) : (mi += 1) {
            const m = melody.items[mi];
            if (m.frame < done) continue;
            g_block.notes[g_block.count] = .{
                .header = .{ .size = @sizeOf(abi.EventNote), .time = @intCast(m.frame - done), .space_id = abi.CORE_EVENT_SPACE_ID, .type = if (m.on) abi.EVENT_NOTE_ON else abi.EVENT_NOTE_OFF, .flags = 0 },
                .note_id = -1,
                .port_index = 0,
                .channel = 0,
                .key = m.key,
                .velocity = 0.9,
            };
            g_block.count += 1;
        }

        var proc = abi.Process{
            .steady_time = @intCast(done),
            .frames_count = n,
            .transport = null,
            .audio_inputs = null,
            .audio_outputs = @ptrCast(&out_buf),
            .audio_inputs_count = 0,
            .audio_outputs_count = 1,
            .in_events = &in_events,
            .out_events = &out_events,
        };
        if (plugin.process.?(plugin, &proc) == 0) break;
        try rec.appendSlice(chan[0..n]);
        done += n;
    }

    try wav.writePcm16("clap_demo.wav", rec.items, sr, 1);
    std.debug.print("played {d} notes through the hosted instrument -> clap_demo.wav\n", .{keys.len});
}
