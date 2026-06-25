//! mcp_client.zig — MCP server discovery and HTTP client helper.
//! Replaces tools/mcp_server/ai_agent_client.py.
//!
//! Probes local ports for running MCP servers (agents server and embedded DAW
//! HTTP server), then provides get_agents / invoke_agent / get_ui_state /
//! press_button / get_metrics operations.
//!
//! Usage (CLI):
//!   zenith_mcp_client --discover
//!   zenith_mcp_client --agents
//!   zenith_mcp_client --metrics
//!   zenith_mcp_client --invoke <agent_id> <action> [params_json]
//!   zenith_mcp_client --button <id> [click|press|release]

const std = @import("std");

const ENV_FILE = "/tmp/zenith_mcp_env";

// Ports to probe: ephemeral range + well-known
const PROBE_PORTS = [_]u16{ 49152, 49153, 49154, 49155, 49156, 49157, 49158, 49159, 49160, 49161, 49162, 8090, 8008 };

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    if (args.len < 2) {
        usage();
        return;
    }

    const cmd = args[1];

    if (std.mem.eql(u8, cmd, "--discover") or std.mem.eql(u8, cmd, "--discover-all")) {
        const result = try discoverAll(alloc);
        defer freeDiscovery(alloc, result);
        if (result.agents_url) |u| {
            try std.io.getStdOut().writer().print("agents: {s}\n", .{u});
        }
        if (result.embedded_url) |u| {
            try std.io.getStdOut().writer().print("embedded: {s}\n", .{u});
        }
        if (result.agents_url == null and result.embedded_url == null) {
            try std.io.getStdOut().writer().writeAll("No MCP server found\n");
        }
        return;
    }

    if (std.mem.eql(u8, cmd, "--agents")) {
        const result = try discoverAll(alloc);
        defer freeDiscovery(alloc, result);
        const url = result.agents_url orelse result.embedded_url orelse {
            try std.io.getStdErr().writer().writeAll("No MCP server found\n");
            std.process.exit(1);
        };
        const body = try httpGet(alloc, url, "/agents", result.token);
        defer alloc.free(body);
        try std.io.getStdOut().writer().print("{s}\n", .{body});
        return;
    }

    if (std.mem.eql(u8, cmd, "--metrics")) {
        const result = try discoverAll(alloc);
        defer freeDiscovery(alloc, result);
        const url = result.embedded_url orelse result.agents_url orelse {
            try std.io.getStdErr().writer().writeAll("No MCP server found\n");
            std.process.exit(1);
        };
        const body = try httpGet(alloc, url, "/mcp/metrics", result.token);
        defer alloc.free(body);
        try std.io.getStdOut().writer().print("{s}\n", .{body});
        return;
    }

    if (std.mem.eql(u8, cmd, "--invoke") and args.len >= 4) {
        const agent_id = args[2];
        const action = args[3];
        const params = if (args.len >= 5) args[4] else "{}";
        const payload = try std.fmt.allocPrint(alloc,
            "{{\"action\":\"{s}\",\"params\":{s}}}",
            .{ action, params },
        );
        defer alloc.free(payload);

        const result = try discoverAll(alloc);
        defer freeDiscovery(alloc, result);
        const url = result.agents_url orelse result.embedded_url orelse {
            try std.io.getStdErr().writer().writeAll("No MCP server found\n");
            std.process.exit(1);
        };
        const path = try std.fmt.allocPrint(alloc, "/agents/{s}/invoke", .{agent_id});
        defer alloc.free(path);
        const body = try httpPost(alloc, url, path, payload, result.token);
        defer alloc.free(body);
        try std.io.getStdOut().writer().print("{s}\n", .{body});
        return;
    }

    if (std.mem.eql(u8, cmd, "--button") and args.len >= 3) {
        const button_id = args[2];
        const action = if (args.len >= 4) args[3] else "click";
        const payload = try std.fmt.allocPrint(alloc,
            "{{\"id\":\"{s}\",\"action\":\"{s}\"}}",
            .{ button_id, action },
        );
        defer alloc.free(payload);

        const result = try discoverAll(alloc);
        defer freeDiscovery(alloc, result);
        const url = result.agents_url orelse result.embedded_url orelse {
            try std.io.getStdErr().writer().writeAll("No MCP server found\n");
            std.process.exit(1);
        };
        const body = try httpPost(alloc, url, "/ui/button", payload, result.token);
        defer alloc.free(body);
        try std.io.getStdOut().writer().print("{s}\n", .{body});
        return;
    }

    usage();
}

fn usage() void {
    std.io.getStdErr().writer().writeAll(
        \\Usage:
        \\  zenith_mcp_client --discover
        \\  zenith_mcp_client --agents
        \\  zenith_mcp_client --metrics
        \\  zenith_mcp_client --invoke <agent_id> <action> [params_json]
        \\  zenith_mcp_client --button <id> [click|press|release]
        \\
    ) catch {};
}

// ---- discovery -------------------------------------------------------------

const Discovery = struct {
    agents_url: ?[]const u8,
    embedded_url: ?[]const u8,
    token: ?[]const u8,
    alloc: std.mem.Allocator,
};

fn freeDiscovery(alloc: std.mem.Allocator, d: Discovery) void {
    if (d.agents_url) |u| alloc.free(u);
    if (d.embedded_url) |u| alloc.free(u);
    if (d.token) |t| alloc.free(t);
}

fn discoverAll(alloc: std.mem.Allocator) !Discovery {
    var agents_url: ?[]const u8 = null;
    var embedded_url: ?[]const u8 = null;
    var found_token: ?[]const u8 = null;

    // Check env file for port hint
    var env_port: ?u16 = null;
    var env_token: ?[]const u8 = null;
    if (std.fs.cwd().openFile(ENV_FILE, .{})) |f| {
        defer f.close();
        const content = f.readToEndAlloc(alloc, 4096) catch null;
        if (content) |c| {
            defer alloc.free(c);
            var lines = std.mem.splitScalar(u8, c, '\n');
            while (lines.next()) |line| {
                if (std.mem.startsWith(u8, line, "MCP_HTTP_PORT=")) {
                    env_port = std.fmt.parseInt(u16, line["MCP_HTTP_PORT=".len..], 10) catch null;
                } else if (std.mem.startsWith(u8, line, "MCP_HTTP_TOKEN=")) {
                    env_token = try alloc.dupe(u8, line["MCP_HTTP_TOKEN=".len..]);
                }
            }
        }
    } else |_| {}

    // Build probe list: env port first, then standard ports
    var ports = std.ArrayList(u16).init(alloc);
    defer ports.deinit();
    if (env_port) |p| try ports.append(p);
    for (PROBE_PORTS) |p| try ports.append(p);

    var seen = std.AutoHashMap(u16, void).init(alloc);
    defer seen.deinit();

    for (ports.items) |p| {
        if (seen.contains(p)) continue;
        try seen.put(p, {});

        if (!probePort(p)) continue;
        const base = try std.fmt.allocPrint(alloc, "http://127.0.0.1:{d}", .{p});
        defer alloc.free(base);
        const tok = env_token;

        // Try /agents (agents server)
        if (agents_url == null) {
            const body = httpGet(alloc, base, "/agents", tok) catch null;
            if (body) |b| {
                alloc.free(b);
                agents_url = try alloc.dupe(u8, base);
                found_token = if (tok) |t| try alloc.dupe(u8, t) else null;
            }
        }
        // Try /mcp/metrics (embedded DAW)
        if (embedded_url == null) {
            const body = httpGet(alloc, base, "/mcp/metrics", tok) catch null;
            if (body) |b| {
                alloc.free(b);
                embedded_url = try alloc.dupe(u8, base);
                if (found_token == null) {
                    found_token = if (tok) |t| try alloc.dupe(u8, t) else null;
                }
            }
        }
        if (agents_url != null and embedded_url != null) break;
    }

    if (env_token) |t| alloc.free(t);
    return .{
        .agents_url = agents_url,
        .embedded_url = embedded_url,
        .token = found_token,
        .alloc = alloc,
    };
}

fn probePort(port: u16) bool {
    const addr = std.net.Address.initIp4(.{ 127, 0, 0, 1 }, port);
    const stream = std.net.tcpConnectToAddress(addr) catch return false;
    stream.close();
    return true;
}

// ---- HTTP helpers ----------------------------------------------------------

fn httpGet(alloc: std.mem.Allocator, base_url: []const u8, path: []const u8, token: ?[]const u8) ![]const u8 {
    const url = try std.fmt.allocPrint(alloc, "{s}{s}", .{ base_url, path });
    defer alloc.free(url);

    var client = std.http.Client{ .allocator = alloc };
    defer client.deinit();

    var body = std.ArrayList(u8).init(alloc);
    defer body.deinit();

    const headers: []const std.http.Header = if (token) |t|
        &.{.{ .name = "X-MCP-Token", .value = t }}
    else
        &.{};

    const result = try client.fetch(.{
        .location = .{ .url = url },
        .response_storage = .{ .dynamic = &body },
        .extra_headers = headers,
    });
    if (result.status != .ok) return error.HttpError;
    return body.toOwnedSlice();
}

fn httpPost(alloc: std.mem.Allocator, base_url: []const u8, path: []const u8, payload: []const u8, token: ?[]const u8) ![]const u8 {
    const url = try std.fmt.allocPrint(alloc, "{s}{s}", .{ base_url, path });
    defer alloc.free(url);

    var client = std.http.Client{ .allocator = alloc };
    defer client.deinit();

    var body = std.ArrayList(u8).init(alloc);
    defer body.deinit();

    var extra = std.ArrayList(std.http.Header).init(alloc);
    defer extra.deinit();
    try extra.append(.{ .name = "Content-Type", .value = "application/json" });
    if (token) |t| try extra.append(.{ .name = "X-MCP-Token", .value = t });

    const result = try client.fetch(.{
        .method = .POST,
        .location = .{ .url = url },
        .payload = payload,
        .response_storage = .{ .dynamic = &body },
        .extra_headers = extra.items,
    });
    if (result.status != .ok) return error.HttpError;
    return body.toOwnedSlice();
}
