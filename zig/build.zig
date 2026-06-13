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
}
