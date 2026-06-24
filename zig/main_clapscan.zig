//! main_clapscan.zig — verify the CLAP host can scan + inspect real plugins.
//! Reads descriptors from a .clap, instantiates it, and queries the audio-ports,
//! note-ports, and params extensions. Also scans the standard install dirs.

const std = @import("std");
const abi = @import("clap_abi.zig");
const host = @import("clap_host.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();

    var args = std.process.args();
    _ = args.next();
    const path = args.next() orelse "zig-out/lib/libzenith_test_clap.so";

    // 1) Read descriptors (metadata) from the file.
    var infos = std.ArrayList(host.PluginInfo).init(a);
    defer {
        for (infos.items) |*pi| pi.deinit(a);
        infos.deinit();
    }
    try host.scanFile(a, path, &infos);
    std.debug.print("scanned '{s}': {d} plugin(s)\n", .{ path, infos.items.len });
    for (infos.items) |pi| {
        std.debug.print("  - id='{s}' name='{s}' vendor='{s}' instrument={}\n", .{ pi.id, pi.name, pi.vendor, pi.is_instrument });
    }
    if (infos.items.len == 0) return error.NoPlugins;

    // 2) Instantiate the first plugin and inspect its ports + params.
    var lib = try std.DynLib.open(path);
    defer lib.close();
    const entry = lib.lookup(*const abi.PluginEntry, "clap_entry") orelse return error.NoEntry;
    if (entry.init) |ini| _ = ini(@ptrCast(path.ptr));
    defer if (entry.deinit) |de| de();
    const fac: *const abi.PluginFactory = @ptrCast(@alignCast(entry.get_factory.?(abi.PLUGIN_FACTORY_ID).?));

    var h = host.makeHost();
    const idz = try a.dupeZ(u8, infos.items[0].id);
    defer a.free(idz);
    const plugin = fac.create_plugin.?(fac, &h, idz) orelse return error.CreateFailed;
    if (!plugin.init.?(plugin)) return error.PluginInit;
    defer plugin.destroy.?(plugin);

    const ports = host.queryPorts(plugin);
    std.debug.print("ports: audio in={d} out={d} | note in={d} out={d}\n", .{ ports.audio_in, ports.audio_out, ports.note_in, ports.note_out });

    var pcount: u32 = 0;
    if (host.paramsExt(plugin)) |pe| {
        pcount = pe.count.?(plugin);
        std.debug.print("params: {d}\n", .{pcount});
        var i: u32 = 0;
        while (i < pcount) : (i += 1) {
            var info: abi.ParamInfo = undefined;
            if (!pe.get_info.?(plugin, i, &info)) continue;
            var val: f64 = 0;
            _ = pe.get_value.?(plugin, info.id, &val);
            std.debug.print("  [{d}] '{s}' range=[{d:.2}..{d:.2}] default={d:.2} value={d:.2}\n", .{ info.id, abi.cstr(&info.name), info.min_value, info.max_value, info.default_value, val });
        }
    }

    // 3) Scan the standard install directories for real third-party plugins.
    const found = try host.findClapFiles(a);
    defer {
        for (found) |p| a.free(p);
        a.free(found);
    }
    std.debug.print("installed .clap files in standard dirs: {d}\n", .{found.len});
    for (found) |p| std.debug.print("  {s}\n", .{p});

    // 4) State save/load round-trip through host-provided streams.
    if (plugin.get_extension.?(plugin, abi.EXT_STATE)) |raw| {
        const st: *const abi.PluginState = @ptrCast(@alignCast(raw));
        const pe = host.paramsExt(plugin).?;

        // save current state
        var saved = std.ArrayList(u8).init(a);
        defer saved.deinit();
        var ostream = abi.OStream{ .ctx = &saved, .write = ostreamWrite };
        if (!st.save.?(plugin, &ostream)) return error.SaveFailed;

        // load a crafted state (Gain=0.25, Brightness=0.75) and confirm it took
        var blob: [16]u8 = undefined;
        std.mem.writeInt(u64, blob[0..8], @bitCast(@as(f64, 0.25)), .little);
        std.mem.writeInt(u64, blob[8..16], @bitCast(@as(f64, 0.75)), .little);
        var rd = ReadCtx{ .data = &blob };
        var istream = abi.IStream{ .ctx = &rd, .read = istreamRead };
        if (!st.load.?(plugin, &istream)) return error.LoadFailed;

        var g: f64 = 0;
        var br: f64 = 0;
        _ = pe.get_value.?(plugin, 0, &g);
        _ = pe.get_value.?(plugin, 1, &br);
        std.debug.print("state: saved {d} bytes; after load -> Gain={d:.2} Brightness={d:.2}\n", .{ saved.items.len, g, br });
        std.debug.assert(@abs(g - 0.25) < 1e-9 and @abs(br - 0.75) < 1e-9);
    }

    // 5) Assert what the test plugin must expose (host verification).
    std.debug.assert(ports.audio_out == 1 and ports.audio_in == 0);
    std.debug.assert(ports.note_in == 1);
    std.debug.assert(pcount == 2);
    std.debug.print("OK: host scanned, instantiated, read ports + params, and round-tripped state.\n", .{});
}

const ReadCtx = struct { data: []const u8, pos: usize = 0 };

fn ostreamWrite(stream: *const abi.OStream, buffer: *const anyopaque, size: u64) callconv(.c) i64 {
    const list: *std.ArrayList(u8) = @ptrCast(@alignCast(stream.ctx.?));
    const bytes: [*]const u8 = @ptrCast(buffer);
    list.appendSlice(bytes[0..@intCast(size)]) catch return -1;
    return @intCast(size);
}
fn istreamRead(stream: *const abi.IStream, buffer: *anyopaque, size: u64) callconv(.c) i64 {
    const ctx: *ReadCtx = @ptrCast(@alignCast(stream.ctx.?));
    const n = @min(@as(usize, @intCast(size)), ctx.data.len - ctx.pos);
    const dst: [*]u8 = @ptrCast(buffer);
    @memcpy(dst[0..n], ctx.data[ctx.pos .. ctx.pos + n]);
    ctx.pos += n;
    return @intCast(n);
}
