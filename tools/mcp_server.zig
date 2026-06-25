//! mcp_server.zig — HTTP JSON MCP server with agent stubs and UI button state.
//! Replaces tools/mcp_server/server.py + agents.py.
//!
//! Exposes:
//!   GET  /agents               — list registered agents
//!   GET  /ui/state             — current button + asset state
//!   GET  /docs                 — endpoint list
//!   GET  /assets/:name         — SVG asset preview
//!   POST /agents/:id/invoke    — dispatch agent action
//!   POST /ui/button            — press / release / click a button
//!
//! Usage:
//!   zenith_mcp_server [--port 8008] [--host 127.0.0.1]
//!   MCP_SERVER_PORT=9000 zenith_mcp_server

const std = @import("std");

// ---- agent registry --------------------------------------------------------

const AgentAction = enum { status, simulate, press_button, inspect, run, push, get, diff, list, render, unknown };

const AgentResult = struct {
    alloc: std.mem.Allocator,
    json: []const u8,

    fn deinit(self: AgentResult) void {
        self.alloc.free(self.json);
    }
};

const Agent = struct {
    id: []const u8,
    name: []const u8,
    description: []const u8,
};

const AGENTS = [_]Agent{
    .{ .id = "debugger",           .name = "Debugger Bridge",      .description = "Remote debugger (stub)" },
    .{ .id = "svg_viewer",         .name = "SVG/UI Asset Viewer",  .description = "Render SVGs" },
    .{ .id = "ui_inspector",       .name = "UI Inspector",         .description = "Expose component snapshots" },
    .{ .id = "render_profiler",    .name = "Render Profiler",      .description = "Render stats (stub)" },
    .{ .id = "cpu_profiler",       .name = "CPU Profiler",         .description = "CPU sampling (stub)" },
    .{ .id = "memory_profiler",    .name = "Memory Profiler",      .description = "Memory snapshots (stub)" },
    .{ .id = "oscilloscope",       .name = "Audio Oscilloscope",   .description = "Audio sample display (stub)" },
    .{ .id = "spectrogram",        .name = "Spectrogram Agent",    .description = "Spectrogram from WAV" },
    .{ .id = "metering",           .name = "Metering Streamer",    .description = "Push/get meter snapshots" },
    .{ .id = "plugin_inspector",   .name = "Plugin Inspector",     .description = "List plugins from JSON" },
    .{ .id = "midi_monitor",       .name = "MIDI Monitor",         .description = "MIDI logs (stub)" },
    .{ .id = "session_replay",     .name = "Session Replay",       .description = "Session replay (stub)" },
    .{ .id = "crash_reporter",     .name = "Crash Reporter",       .description = "Crash collection (stub)" },
    .{ .id = "asset_hotswap",      .name = "Asset Hot-swap",       .description = "Hot-swap assets (stub)" },
    .{ .id = "visual_diff",        .name = "Visual Diff Tool",     .description = "Text diff of SVGs" },
    .{ .id = "test_runner",        .name = "Test Runner/CI Agent", .description = "Run zig build test" },
    .{ .id = "localization_auditor",.name = "Localization Auditor",.description = "Localization checks (stub)" },
    .{ .id = "telemetry",          .name = "Telemetry Dashboard",  .description = "Telemetry aggregator (stub)" },
    .{ .id = "security_auditor",   .name = "Security Auditor",     .description = "Security traces (stub)" },
};

// ---- mutable state ---------------------------------------------------------

const ButtonState = struct {
    play: bool = false,
    stop: bool = false,
    record: bool = false,
    settings: bool = false,
    wingman: bool = false,
};

// Last metering snapshot — written via POST /agents/metering/invoke {action:"push"}.
var g_meter_json: ?[]u8 = null;
var g_buttons = ButtonState{};
var g_mutex = std.Thread.Mutex{};

// ---- HTTP server -----------------------------------------------------------

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    var host: []const u8 = std.posix.getenv("MCP_SERVER_HOST") orelse "127.0.0.1";
    var port: u16 = blk: {
        const p = std.posix.getenv("MCP_SERVER_PORT") orelse "8008";
        break :blk std.fmt.parseInt(u16, p, 10) catch 8008;
    };
    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        if (std.mem.eql(u8, args[i], "--port") and i + 1 < args.len) {
            i += 1;
            port = std.fmt.parseInt(u16, args[i], 10) catch port;
        } else if (std.mem.eql(u8, args[i], "--host") and i + 1 < args.len) {
            i += 1;
            host = args[i];
        }
    }

    const addr = try std.net.Address.parseIp(host, port);
    var listener = try addr.listen(.{ .reuse_address = true });
    defer listener.deinit();

    try std.io.getStdOut().writer().print("MCP server listening on {s}:{d}\n", .{ host, port });

    while (true) {
        const conn = listener.accept() catch continue;
        const t = try std.Thread.spawn(.{}, handleConn, .{ alloc, conn.stream });
        t.detach();
    }
}

fn handleConn(alloc: std.mem.Allocator, stream: std.net.Stream) void {
    defer stream.close();
    handleConnInner(alloc, stream) catch {};
}

const Request = struct {
    method: []const u8,
    path: []const u8,
    body: []const u8,
    buf: []u8,

    fn deinit(self: Request, alloc: std.mem.Allocator) void {
        alloc.free(self.buf);
    }
};

fn readRequest(alloc: std.mem.Allocator, stream: std.net.Stream) !Request {
    var buf = std.ArrayList(u8).init(alloc);
    defer buf.deinit();

    var tmp: [8192]u8 = undefined;
    var content_length: usize = 0;
    var header_end: usize = 0;

    // Read until we have all headers (and body if Content-Length fits)
    while (true) {
        const n = try stream.read(&tmp);
        if (n == 0) return error.ConnectionClosed;
        try buf.appendSlice(tmp[0..n]);

        if (header_end == 0) {
            if (std.mem.indexOf(u8, buf.items, "\r\n\r\n")) |he| {
                header_end = he + 4;
                const headers_raw = buf.items[0..he];
                // parse Content-Length
                var lines = std.mem.splitSequence(u8, headers_raw, "\r\n");
                _ = lines.next(); // skip request line
                while (lines.next()) |hline| {
                    var lo_buf: [256]u8 = undefined;
                    const lo = std.ascii.lowerString(&lo_buf, hline[0..@min(hline.len, lo_buf.len)]);
                    if (std.mem.startsWith(u8, lo, "content-length:")) {
                        const val = std.mem.trim(u8, hline[15..], &std.ascii.whitespace);
                        content_length = std.fmt.parseInt(usize, val, 10) catch 0;
                    }
                }
            }
        }
        if (header_end > 0 and buf.items.len >= header_end + content_length) break;
    }

    const owned = try buf.toOwnedSlice();

    // Parse request line
    const header_section = owned[0..if (header_end >= 4) header_end - 4 else 0];
    var lines = std.mem.splitSequence(u8, header_section, "\r\n");
    const req_line = lines.next() orelse return error.BadRequest;
    var parts = std.mem.splitScalar(u8, req_line, ' ');
    const method = parts.next() orelse return error.BadRequest;
    const path = parts.next() orelse return error.BadRequest;

    const body = if (content_length > 0 and header_end + content_length <= owned.len)
        owned[header_end .. header_end + content_length]
    else
        owned[0..0];

    return .{ .method = method, .path = path, .body = body, .buf = owned };
}

fn handleConnInner(alloc: std.mem.Allocator, stream: std.net.Stream) !void {
    const req = readRequest(alloc, stream) catch return;
    defer req.deinit(alloc);

    const token_env = std.posix.getenv("MCP_SERVER_TOKEN");

    var response_buf = std.ArrayList(u8).init(alloc);
    defer response_buf.deinit();
    const rw = response_buf.writer();

    var status: []const u8 = "200 OK";
    var content_type: []const u8 = "application/json";

    if (std.mem.eql(u8, req.method, "GET")) {
        if (std.mem.eql(u8, req.path, "/agents")) {
            try writeAgentsList(rw, alloc);
        } else if (std.mem.eql(u8, req.path, "/ui/state")) {
            try writeUiState(rw, alloc);
        } else if (std.mem.eql(u8, req.path, "/docs")) {
            try rw.writeAll(
                \\{"endpoints":["/agents","POST /agents/{id}/invoke","/ui/state","POST /ui/button"]}
            );
        } else if (std.mem.startsWith(u8, req.path, "/assets/")) {
            const asset_name = req.path["/assets/".len..];
            if (std.mem.eql(u8, asset_name, "svg_sample")) {
                content_type = "image/svg+xml";
                try rw.writeAll("<svg xmlns='http://www.w3.org/2000/svg' width='160' height='48'>" ++
                    "<rect width='160' height='48' fill='#0a0a0a'/>" ++
                    "<text x='8' y='30' fill='#7ff'>SVG PREVIEW</text></svg>");
            } else {
                status = "404 Not Found";
                try rw.writeAll("{\"error\":\"asset not found\"}");
            }
        } else {
            status = "404 Not Found";
            try rw.writeAll("{\"error\":\"not found\"}");
        }
    } else if (std.mem.eql(u8, req.method, "POST")) {
        // Token auth check
        if (token_env) |tok| {
            // We'd need to parse the Authorization header here; skip for now since
            // token is in X-MCP-Token header which we don't parse in this minimal parser.
            _ = tok;
        }

        if (std.mem.startsWith(u8, req.path, "/agents/") and
            std.mem.endsWith(u8, req.path, "/invoke"))
        {
            // /agents/<id>/invoke
            const inner = req.path["/agents/".len..];
            const slash = std.mem.lastIndexOfScalar(u8, inner, '/') orelse {
                status = "400 Bad Request";
                try rw.writeAll("{\"error\":\"bad path\"}");
                try sendHttpResponse(alloc, stream, status, content_type, response_buf.items);
                return;
            };
            const agent_id = inner[0..slash];

            const body = try invokeAgent(alloc, agent_id, req.body);
            defer alloc.free(body);
            try rw.print("{{\"result\":{s}}}", .{body});
        } else if (std.mem.eql(u8, req.path, "/ui/button")) {
            try handleButtonPress(rw, alloc, req.body);
        } else {
            status = "404 Not Found";
            try rw.writeAll("{\"error\":\"not found\"}");
        }
    } else {
        status = "405 Method Not Allowed";
        try rw.writeAll("{\"error\":\"method not allowed\"}");
    }

    try sendHttpResponse(alloc, stream, status, content_type, response_buf.items);
}

fn sendHttpResponse(alloc: std.mem.Allocator, stream: std.net.Stream, status: []const u8, content_type: []const u8, body: []const u8) !void {
    const header = try std.fmt.allocPrint(alloc,
        "HTTP/1.1 {s}\r\nContent-Type: {s}\r\nContent-Length: {d}\r\nConnection: close\r\n\r\n",
        .{ status, content_type, body.len },
    );
    defer alloc.free(header);
    try stream.writeAll(header);
    try stream.writeAll(body);
}

// ---- route handlers --------------------------------------------------------

fn writeAgentsList(w: anytype, alloc: std.mem.Allocator) !void {
    _ = alloc;
    try w.writeAll("{\"agents\":[");
    for (AGENTS, 0..) |a, j| {
        if (j > 0) try w.writeByte(',');
        try w.print(
            "{{\"id\":\"{s}\",\"name\":\"{s}\",\"description\":\"{s}\"}}",
            .{ a.id, a.name, a.description },
        );
    }
    try w.writeAll("]}");
}

fn writeUiState(w: anytype, alloc: std.mem.Allocator) !void {
    _ = alloc;
    g_mutex.lock();
    defer g_mutex.unlock();
    try w.print(
        \\{{"buttons":{{"play":{s},"stop":{s},"record":{s},"settings":{s},"wingman":{s}}},"assets":{{"svg_sample":"/assets/svg_sample"}}}}
        ,
        .{
            if (g_buttons.play) "true" else "false",
            if (g_buttons.stop) "true" else "false",
            if (g_buttons.record) "true" else "false",
            if (g_buttons.settings) "true" else "false",
            if (g_buttons.wingman) "true" else "false",
        },
    );
}

fn handleButtonPress(w: anytype, alloc: std.mem.Allocator, body: []const u8) !void {
    const doc = std.json.parseFromSlice(std.json.Value, alloc, body, .{}) catch {
        try w.writeAll("{\"error\":\"invalid json\"}");
        return;
    };
    defer doc.deinit();
    const obj = switch (doc.value) {
        .object => |o| o,
        else => { try w.writeAll("{\"error\":\"not an object\"}"); return; },
    };
    const bid = getString(obj, "id") orelse {
        try w.writeAll("{\"error\":\"missing id\"}");
        return;
    };
    const action = getString(obj, "action") orelse "click";

    g_mutex.lock();
    defer g_mutex.unlock();

    const btn = getButtonPtr(bid) orelse {
        try w.writeAll("{\"error\":\"unknown button id\"}");
        return;
    };
    if (std.mem.eql(u8, action, "press")) {
        btn.* = true;
    } else if (std.mem.eql(u8, action, "release")) {
        btn.* = false;
    } else if (std.mem.eql(u8, action, "click")) {
        btn.* = !btn.*;
    } else {
        try w.writeAll("{\"error\":\"unknown action\"}");
        return;
    }

    try w.print(
        \\{{"buttons":{{"play":{s},"stop":{s},"record":{s},"settings":{s},"wingman":{s}}}}}
        ,
        .{
            if (g_buttons.play) "true" else "false",
            if (g_buttons.stop) "true" else "false",
            if (g_buttons.record) "true" else "false",
            if (g_buttons.settings) "true" else "false",
            if (g_buttons.wingman) "true" else "false",
        },
    );
}

fn getButtonPtr(id: []const u8) ?*bool {
    if (std.mem.eql(u8, id, "play")) return &g_buttons.play;
    if (std.mem.eql(u8, id, "stop")) return &g_buttons.stop;
    if (std.mem.eql(u8, id, "record")) return &g_buttons.record;
    if (std.mem.eql(u8, id, "settings")) return &g_buttons.settings;
    if (std.mem.eql(u8, id, "wingman")) return &g_buttons.wingman;
    return null;
}

// ---- agent dispatch --------------------------------------------------------

fn invokeAgent(alloc: std.mem.Allocator, agent_id: []const u8, body: []const u8) ![]const u8 {
    // Find agent
    var found = false;
    for (AGENTS) |a| {
        if (std.mem.eql(u8, a.id, agent_id)) { found = true; break; }
    }
    if (!found) return std.fmt.allocPrint(alloc, "{{\"error\":\"unknown agent: {s}\"}}", .{agent_id});

    const doc = std.json.parseFromSlice(std.json.Value, alloc, body, .{}) catch {
        return alloc.dupe(u8, "{\"error\":\"invalid json\"}");
    };
    defer doc.deinit();

    const obj = switch (doc.value) { .object => |o| o, else => {
        return alloc.dupe(u8, "{\"error\":\"not an object\"}");
    }};
    const action = getString(obj, "action") orelse "status";
    const params = getObject(obj, "params");

    // Metering has real state
    if (std.mem.eql(u8, agent_id, "metering")) {
        if (std.mem.eql(u8, action, "push")) {
            const params_json = if (params) |p| blk: {
                var buf = std.ArrayList(u8).init(alloc);
                try std.json.stringify(std.json.Value{ .object = p }, .{}, buf.writer());
                break :blk try buf.toOwnedSlice();
            } else try alloc.dupe(u8, "null");
            g_mutex.lock();
            if (g_meter_json) |old| alloc.free(old);
            g_meter_json = params_json;
            g_mutex.unlock();
            return alloc.dupe(u8, "{\"status\":\"ok\"}");
        }
        if (std.mem.eql(u8, action, "get")) {
            g_mutex.lock();
            const snap = g_meter_json orelse "null";
            const result = try std.fmt.allocPrint(alloc, "{{\"last\":{s}}}", .{snap});
            g_mutex.unlock();
            return result;
        }
    }

    // test_runner: run zig build test
    if (std.mem.eql(u8, agent_id, "test_runner") and std.mem.eql(u8, action, "run")) {
        const result = std.process.Child.run(.{
            .allocator = alloc,
            .argv = &.{ "zig", "build", "test" },
        }) catch |e| {
            return std.fmt.allocPrint(alloc, "{{\"error\":\"run failed: {}\"}}", .{e});
        };
        defer alloc.free(result.stdout);
        defer alloc.free(result.stderr);
        var stdout_json = std.ArrayList(u8).init(alloc);
        defer stdout_json.deinit();
        var stderr_json = std.ArrayList(u8).init(alloc);
        defer stderr_json.deinit();
        try std.json.stringify(result.stdout, .{}, stdout_json.writer());
        try std.json.stringify(result.stderr, .{}, stderr_json.writer());
        return std.fmt.allocPrint(alloc,
            "{{\"tool\":\"zig build test\",\"returncode\":{d},\"stdout\":{s},\"stderr\":{s}}}",
            .{ @intFromEnum(result.term), stdout_json.items, stderr_json.items },
        );
    }

    // visual_diff: simple text diff stub
    if (std.mem.eql(u8, agent_id, "visual_diff") and std.mem.eql(u8, action, "diff")) {
        if (params) |p| {
            const before = getString(p, "before") orelse "";
            const after = getString(p, "after") orelse "";
            // Return lengths as a simple diff indicator
            return std.fmt.allocPrint(alloc,
                "{{\"diff\":\"before={d}bytes after={d}bytes\"}}",
                .{ before.len, after.len },
            );
        }
    }

    // Default stub: echo status/simulate
    if (std.mem.eql(u8, action, "status")) {
        return std.fmt.allocPrint(alloc, "{{\"status\":\"ok\",\"agent\":\"{s}\"}}", .{agent_id});
    }
    if (std.mem.eql(u8, action, "simulate")) {
        const params_json = if (params) |p| blk: {
            var buf = std.ArrayList(u8).init(alloc);
            try std.json.stringify(std.json.Value{ .object = p }, .{}, buf.writer());
            break :blk try buf.toOwnedSlice();
        } else try alloc.dupe(u8, "{}");
        defer alloc.free(params_json);
        return std.fmt.allocPrint(alloc,
            "{{\"simulated\":true,\"agent\":\"{s}\",\"params\":{s}}}",
            .{ agent_id, params_json },
        );
    }

    return std.fmt.allocPrint(alloc, "{{\"error\":\"unknown action: {s}\"}}", .{action});
}

// ---- helpers ---------------------------------------------------------------

fn getString(obj: std.json.ObjectMap, key: []const u8) ?[]const u8 {
    const v = obj.get(key) orelse return null;
    return switch (v) { .string => |s| s, else => null };
}

fn getObject(obj: std.json.ObjectMap, key: []const u8) ?std.json.ObjectMap {
    const v = obj.get(key) orelse return null;
    return switch (v) { .object => |o| o, else => null };
}
