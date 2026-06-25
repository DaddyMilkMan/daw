//! mcp_bridge.zig — MCP JSON-RPC stdio bridge to Zenith's embedded HTTP MCP server.
//! Replaces tools/mcp_http_stdio_bridge.py.
//!
//! Reads MCP JSON-RPC from stdin, bridges resources/read calls to the DAW's
//! HTTP /mcp/* endpoints, writes responses to stdout.
//!
//! Usage:
//!   zenith_mcp_bridge --url http://127.0.0.1:8008
//!   zenith_mcp_bridge --port 8008 [--host 127.0.0.1] [--token <tok>]
//!   MCP_HTTP_URL=http://127.0.0.1:8008 zenith_mcp_bridge

const std = @import("std");

const PROTOCOL_VERSION = "2024-11-05";
const SERVER_NAME = "zenith-http-bridge";
const SERVER_VERSION = "0.1.0";

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    var base_url: []const u8 = std.posix.getenv("MCP_HTTP_URL") orelse "";
    var port: u16 = 0;
    var host: []const u8 = std.posix.getenv("MCP_HTTP_HOST") orelse "127.0.0.1";
    var token: []const u8 = std.posix.getenv("MCP_HTTP_TOKEN") orelse
        (std.posix.getenv("MCP_SERVER_TOKEN") orelse "");

    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        if (std.mem.eql(u8, args[i], "--url") and i + 1 < args.len) {
            i += 1;
            base_url = args[i];
        } else if (std.mem.eql(u8, args[i], "--port") and i + 1 < args.len) {
            i += 1;
            port = std.fmt.parseInt(u16, args[i], 10) catch 0;
        } else if (std.mem.eql(u8, args[i], "--host") and i + 1 < args.len) {
            i += 1;
            host = args[i];
        } else if (std.mem.eql(u8, args[i], "--token") and i + 1 < args.len) {
            i += 1;
            token = args[i];
        }
    }

    if (base_url.len == 0) {
        if (port == 0) {
            const port_env = std.posix.getenv("MCP_HTTP_PORT") orelse "0";
            port = std.fmt.parseInt(u16, port_env, 10) catch 0;
        }
        if (port == 0) {
            try std.io.getStdErr().writer().writeAll(
                "Missing HTTP target: set --url or --port / MCP_HTTP_URL / MCP_HTTP_PORT\n",
            );
            std.process.exit(2);
        }
        base_url = try std.fmt.allocPrint(alloc, "http://{s}:{d}", .{ host, port });
    }

    try std.io.getStdErr().writer().print("[bridge] MCP HTTP base: {s}\n", .{base_url});

    var http_client = std.http.Client{ .allocator = alloc };
    defer http_client.deinit();

    const stdin_file = std.io.getStdIn();
    var buf_reader = std.io.bufferedReader(stdin_file.reader());
    const reader = buf_reader.reader();
    const stdout = std.io.getStdOut().writer();

    var line_buf = std.ArrayList(u8).init(alloc);
    defer line_buf.deinit();

    var initialized = false;

    while (true) {
        line_buf.clearRetainingCapacity();
        reader.readUntilDelimiterArrayList(&line_buf, '\n', 4 * 1024 * 1024) catch |e| switch (e) {
            error.EndOfStream => {
                if (line_buf.items.len == 0) break;
            },
            else => return e,
        };
        const line = std.mem.trim(u8, line_buf.items, &std.ascii.whitespace);
        if (line.len == 0) continue;

        const doc = std.json.parseFromSlice(std.json.Value, alloc, line, .{}) catch {
            try sendError(stdout, alloc, null, -32700, "Parse error");
            continue;
        };
        defer doc.deinit();

        const obj = switch (doc.value) {
            .object => |o| o,
            else => {
                try sendError(stdout, alloc, null, -32600, "Not an object");
                continue;
            },
        };

        const id = obj.get("id");
        const method = getString(obj, "method") orelse {
            try sendError(stdout, alloc, id, -32600, "Missing method");
            continue;
        };

        if (std.mem.eql(u8, method, "initialize")) {
            try sendResult(stdout, alloc, id,
                \\{"protocolVersion":"2024-11-05","capabilities":{"tools":{},"resources":{}},"serverInfo":{"name":"zenith-http-bridge","version":"0.1.0"}}
            );
            continue;
        }
        if (std.mem.eql(u8, method, "notifications/initialized")) {
            initialized = true;
            continue;
        }
        if (std.mem.eql(u8, method, "ping")) {
            try sendResult(stdout, alloc, id, "{}");
            continue;
        }
        if (!initialized) {
            try sendError(stdout, alloc, id, -32002, "Not initialized");
            continue;
        }
        if (std.mem.eql(u8, method, "tools/list")) {
            try sendResult(stdout, alloc, id, "{\"tools\":[]}");
            continue;
        }
        if (std.mem.eql(u8, method, "tools/call")) {
            try sendError(stdout, alloc, id, -32002, "Tool not available via HTTP bridge");
            continue;
        }
        if (std.mem.eql(u8, method, "resources/list")) {
            try sendResult(stdout, alloc, id,
                \\{"resources":[{"uri":"zenith://metrics","name":"Metrics Snapshot","mimeType":"application/json"},{"uri":"zenith://plugins","name":"Plugin List","mimeType":"application/json"},{"uri":"zenith://snapshot","name":"Engine Snapshot","mimeType":"application/json"}]}
            );
            continue;
        }
        if (std.mem.eql(u8, method, "resources/read")) {
            const params = getObject(obj, "params");
            const uri = (if (params) |p| getString(p, "uri") else null) orelse {
                try sendError(stdout, alloc, id, -32600, "Missing uri");
                continue;
            };

            const http_path: []const u8 = if (std.mem.eql(u8, uri, "zenith://metrics"))
                "/mcp/metrics"
            else if (std.mem.eql(u8, uri, "zenith://plugins"))
                "/mcp/plugins"
            else if (std.mem.eql(u8, uri, "zenith://snapshot"))
                "/mcp/snapshot"
            else {
                try sendError(stdout, alloc, id, -32001, "Resource not found");
                continue;
            };

            const url = try std.fmt.allocPrint(alloc, "{s}{s}", .{ base_url, http_path });
            defer alloc.free(url);

            var response_body = std.ArrayList(u8).init(alloc);
            defer response_body.deinit();

            const extra_headers: []const std.http.Header = if (token.len > 0)
                &.{.{ .name = "X-MCP-Token", .value = token }}
            else
                &.{};

            const res = http_client.fetch(.{
                .location = .{ .url = url },
                .response_storage = .{ .dynamic = &response_body },
                .extra_headers = extra_headers,
            }) catch |e| {
                const msg = try std.fmt.allocPrint(alloc, "HTTP failure: {}", .{e});
                defer alloc.free(msg);
                try sendError(stdout, alloc, id, -32603, msg);
                continue;
            };

            if (res.status != .ok) {
                const msg = try std.fmt.allocPrint(alloc, "HTTP error {d}", .{@intFromEnum(res.status)});
                defer alloc.free(msg);
                try sendError(stdout, alloc, id, -32001, msg);
                continue;
            }

            var body_json = std.ArrayList(u8).init(alloc);
            defer body_json.deinit();
            try std.json.stringify(response_body.items, .{}, body_json.writer());

            var uri_json = std.ArrayList(u8).init(alloc);
            defer uri_json.deinit();
            try std.json.stringify(uri, .{}, uri_json.writer());

            const result = try std.fmt.allocPrint(
                alloc,
                "{{\"contents\":[{{\"uri\":{s},\"mimeType\":\"application/json\",\"text\":{s}}}]}}",
                .{ uri_json.items, body_json.items },
            );
            defer alloc.free(result);
            try sendResult(stdout, alloc, id, result);
            continue;
        }

        const msg = try std.fmt.allocPrint(alloc, "Method not found: {s}", .{method});
        defer alloc.free(msg);
        try sendError(stdout, alloc, id, -32601, msg);
    }
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

fn idJson(alloc: std.mem.Allocator, id: ?std.json.Value) ![]const u8 {
    if (id) |v| {
        var buf = std.ArrayList(u8).init(alloc);
        try std.json.stringify(v, .{}, buf.writer());
        return buf.toOwnedSlice();
    }
    return alloc.dupe(u8, "null");
}

fn sendResult(writer: anytype, alloc: std.mem.Allocator, id: ?std.json.Value, result_json: []const u8) !void {
    const id_str = try idJson(alloc, id);
    defer alloc.free(id_str);
    try writer.print("{{\"jsonrpc\":\"2.0\",\"id\":{s},\"result\":{s}}}\n", .{ id_str, result_json });
}

fn sendError(writer: anytype, alloc: std.mem.Allocator, id: ?std.json.Value, code: i32, message: []const u8) !void {
    const id_str = try idJson(alloc, id);
    defer alloc.free(id_str);
    var msg_json = std.ArrayList(u8).init(alloc);
    defer msg_json.deinit();
    try std.json.stringify(message, .{}, msg_json.writer());
    try writer.print(
        "{{\"jsonrpc\":\"2.0\",\"id\":{s},\"error\":{{\"code\":{d},\"message\":{s}}}}}\n",
        .{ id_str, code, msg_json.items },
    );
}
