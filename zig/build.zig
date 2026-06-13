//! Zenith Zig build.
//!   zig build render   -> writes zenith_hello.wav
//!   zig build play     -> plays the demo phrase via ALSA (links libasound)

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const render = b.addExecutable(.{
        .name = "zenith_render",
        .root_source_file = b.path("main_render.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(render);

    const play = b.addExecutable(.{
        .name = "zenith_play",
        .root_source_file = b.path("main_play.zig"),
        .target = target,
        .optimize = optimize,
    });
    play.linkSystemLibrary("asound");
    play.linkLibC();
    b.installArtifact(play);

    const run_render = b.addRunArtifact(render);
    const render_step = b.step("render", "Render the demo phrase to zenith_hello.wav");
    render_step.dependOn(&run_render.step);

    const run_play = b.addRunArtifact(play);
    const play_step = b.step("play", "Play the demo phrase via ALSA");
    play_step.dependOn(&run_play.step);

    // Real-time, MIDI-driven engine.
    const live = b.addExecutable(.{
        .name = "zenith_live",
        .root_source_file = b.path("main_live.zig"),
        .target = target,
        .optimize = optimize,
    });
    live.linkSystemLibrary("asound");
    live.linkLibC();
    b.installArtifact(live);

    const run_live = b.addRunArtifact(live);
    const live_step = b.step("live", "Run the real-time MIDI engine (connect a keyboard via aconnect)");
    live_step.dependOn(&run_live.step);

    // Unit tests (MIDI event struct layout, etc.)
    const tests = b.addTest(.{ .root_source_file = b.path("midi_alsa.zig"), .target = target, .optimize = optimize });
    tests.linkSystemLibrary("asound");
    tests.linkLibC();
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&b.addRunArtifact(tests).step);
}
