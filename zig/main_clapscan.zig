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

    // 4) Assert what the test plugin must expose (host verification).
    std.debug.assert(ports.audio_out == 1 and ports.audio_in == 0);
    std.debug.assert(ports.note_in == 1);
    std.debug.assert(pcount == 2);
    std.debug.print("OK: host scanned, instantiated, and read ports + params.\n", .{});
}
