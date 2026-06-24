//! main_ai.zig — CLI driver for the Zenith AI wedge.
//!
//! Runs the produce-with-you agent against a fresh project. Uses the live xAI
//! Grok provider when `XAI_API_KEY` is set, otherwise a scripted mock so the
//! whole spine (provider → agent loop → project-mutating tools) demonstrates
//! end-to-end with no network or key.
//!
//!   zig build ai                               # offline mock demo
//!   zig build ai -- "make a 90 bpm lofi beat"  # prompt via args (mock or live)
//!   XAI_API_KEY=xai-... zig build ai -- "..."   # live Grok
//!   ZENITH_AI_SAVE=out.znpr zig build ai        # persist the result

const std = @import("std");
const project = @import("project.zig");
const prov = @import("ai_provider.zig");
const tools = @import("ai_tools.zig");
const Agent = @import("ai_agent.zig").Agent;

const DEFAULT_PROMPT = "Make a 124 BPM house sketch: a drums track and a bass track, then generate a 4-bar house drum pattern and a matching bassline.";

/// The offline demo plan — what a model *would* do for the default prompt,
/// scripted so the mock provider drives the real tools (including the
/// algorithmic generator) deterministically.
fn demoScript() []const prov.MockProvider.Canned {
    const S = struct {
        const script = [_]prov.MockProvider.Canned{
            .{ .tool_calls = &.{.{ .id = "t1", .name = "set_tempo", .arguments = "{\"bpm\":124}" }} },
            .{ .tool_calls = &.{
                .{ .id = "t2", .name = "create_track", .arguments = "{\"name\":\"Drums\",\"instrument\":\"sampler\"}" },
                .{ .id = "t3", .name = "create_track", .arguments = "{\"name\":\"Bass\",\"instrument\":\"synth\"}" },
            } },
            .{ .tool_calls = &.{
                .{ .id = "t4", .name = "add_clip", .arguments = "{\"track\":0,\"name\":\"beat\",\"start\":0}" },
                .{ .id = "t5", .name = "add_clip", .arguments = "{\"track\":1,\"name\":\"bassline\",\"start\":0}" },
            } },
            .{ .tool_calls = &.{
                .{ .id = "g1", .name = "generate_pattern", .arguments = "{\"track\":0,\"clip\":0,\"kind\":\"drums\",\"style\":\"house\",\"bars\":4,\"complexity\":0.7,\"seed\":2024}" },
                .{ .id = "g2", .name = "generate_pattern", .arguments = "{\"track\":1,\"clip\":0,\"kind\":\"bass\",\"style\":\"house\",\"key\":\"C\",\"scale\":\"minor\",\"bars\":4,\"seed\":2024}" },
            } },
            .{ .content = "Set 124 BPM, added a sampler Drums track and a synth Bass track, and generated a 4-bar house drum pattern plus a C-minor bassline." },
        };
    };
    return &S.script;
}

fn resolvePrompt(a: std.mem.Allocator) ![]u8 {
    const args = try std.process.argsAlloc(a);
    defer std.process.argsFree(a, args);
    if (args.len > 1) {
        var parts = std.ArrayList(u8).init(a);
        defer parts.deinit();
        for (args[1..], 0..) |arg, i| {
            if (i != 0) try parts.append(' ');
            try parts.appendSlice(arg);
        }
        return parts.toOwnedSlice();
    }
    if (std.process.getEnvVarOwned(a, "ZENITH_AI_PROMPT")) |p| {
        return p;
    } else |_| {}
    return a.dupe(u8, DEFAULT_PROMPT);
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();

    var proj = project.Project.init(a);
    defer proj.deinit();
    var hist = project.History.init(a);
    defer hist.deinit();
    var ctx = tools.Context{ .project = &proj, .history = &hist, .alloc = a };

    const prompt = try resolvePrompt(a);
    defer a.free(prompt);

    const stdout = std.io.getStdOut().writer();

    var grok: prov.GrokProvider = undefined;
    var mock: prov.MockProvider = undefined;
    var provider: prov.Provider = undefined;
    var using_grok = false;
    if (std.posix.getenv("XAI_API_KEY")) |key| {
        grok = .{ .api_key = key };
        if (std.posix.getenv("ZENITH_AI_MODEL")) |m| grok.model = m;
        provider = grok.provider();
        using_grok = true;
    } else {
        mock = .{ .script = demoScript() };
        provider = mock.provider();
    }

    try stdout.print("Zenith AI wedge — provider: {s}\n", .{
        if (using_grok) "xAI Grok (live)" else "MOCK (offline demo; set XAI_API_KEY for live)",
    });
    try stdout.print("User: {s}\n\nTool calls:\n", .{prompt});

    var agent = try Agent.init(a, provider, &ctx);
    defer agent.deinit();
    agent.trace = true;

    const reply = agent.run(prompt) catch |e| {
        try stdout.print("\n[agent error: {s}]\n", .{@errorName(e)});
        if (!using_grok) return e;
        try stdout.print("(live call failed — check XAI_API_KEY / network)\n", .{});
        return;
    };
    defer a.free(reply);

    try stdout.print("\nAssistant: {s}\n\n", .{reply});
    try stdout.print("Project now: tempo={d:.0} bpm, sr={d}, {d} track(s)\n", .{ proj.tempo, proj.sample_rate, proj.tracks.items.len });
    for (proj.tracks.items, 0..) |t, i| {
        var note_count: usize = 0;
        for (t.clips.items) |c| note_count += c.notes.items.len;
        try stdout.print("  [{d}] {s:<8} {s:<8} vol={d:.2} pan={d:.2} clips={d} notes={d}\n", .{
            i, t.name.items, @tagName(t.instrument), t.gain, t.pan, t.clips.items.len, note_count,
        });
    }

    if (std.process.getEnvVarOwned(a, "ZENITH_AI_SAVE")) |path| {
        defer a.free(path);
        try proj.save(path);
        try stdout.print("\nSaved project to {s}\n", .{path});
    } else |_| {}
}
