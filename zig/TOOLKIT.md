# Zenith UI toolkit — reusable components

These modules are written from scratch in Zig for the Zenith DAW, but they are
**self-contained and reusable on their own**. Each is a single `.zig` file (plus
the small dependency list below), so you can lift just the part you want into
another project. Everything is original Zig — only published *techniques* are
borrowed (SDF shapes, the erf shadow, dual-Kawase blur, flow layout; sources
cited in `../planning/research/web-ui-rendering.md`). No JUCE / Skia / browser /
egui code is copied.

License: **AGPL-3.0** (see `../LICENSE`).

## Components (lowest layer first)

| File | What it is | Depends on |
|------|------------|-----------|
| `color.zig` | 8-bit RGBA `Color` type + `rgb()/rgba()` | — (nothing) |
| `font.zig` | Antialiased font descriptor (glyph atlas metrics + coverage). `font_body.zig` / `font_ui.zig` / `font_display.zig` are baked data sets. | — |
| `gpu2d.zig` | **GPU 2D renderer.** Per-primitive instanced shaders, batched one draw call per type, composited in linear light via sRGB framebuffer + premultiplied alpha, 4× MSAA. Primitives: SDF rounded rects (+ hairline border, gradient, material depth), analytic gaussian drop shadows (closed-form erf, no blur pass), triangles/lines, atlas text (`GpuFont`), and **dual-Kawase backdrop blur / frosted glass**. | `color`, `font` + OpenGL 3.3 + a `glGetProcAddress`-style loader |
| `flex.zig` | **Declarative layout engine** (our own; flow/box model). Sizing `px/grow/percent/fit`, `row`/`col`, `gap`, `pad`, `justify`, `align`; per-frame tree, two-pass measure→arrange, rendered by walk. Built-in hover/press animation + click-by-id; `rectOf(id)` for overlays. | `gpu2d` (→ `color`, `font`) |
| `widgets.zig` | **GPU immediate-mode widgets** — faders, sliders, rotary knobs, icon slots, with per-id hover/press animation. No layout/DAW coupling; drive with `flex` rects or raw coordinates. | `gpu2d` (→ `color`) |
| `window_glx.zig` | Borderless, resizable, custom-chrome **GLX window** (full input, EWMH move/resize/min/max, MSAA visual, `glGetProcAddress`). Hand-declared C ABIs, no `@cImport`. | X11 + GL + libc (Linux/X11) |
| `render2d.zig` | Alternative **CPU software 2D renderer** (AA rounded rects, subpixel/grayscale text, gamma-correct linear blending, separable blur, layered shadow). The original render path; `gpu2d` supersedes it for the live UI but this remains a dependency-light, GPU-free option. | `color`, `font` |

## To extract a piece

- **Just the color type:** copy `color.zig`.
- **The GPU renderer:** copy `color.zig`, `font.zig` (+ a `font_*.zig` data set), `gpu2d.zig`. Provide an OpenGL 3.3 context and a proc loader (one function: `fn(name: [*:0]const u8) ?*const anyopaque`).
- **The layout engine + widgets:** the above + `flex.zig` + `widgets.zig`.
- **A windowed Linux app:** add `window_glx.zig` (links `X11`, `GL`, `libc`).
- **A CPU-only renderer (no GPU):** copy `color.zig`, `font.zig`, `render2d.zig`.

The full stack assembled together is the live DAW: `daw.zig` (view) +
`main_daw.zig` (`zig build daw`, binary `zenith`).

DAW-specific code (not part of the reusable toolkit): `daw.zig`, `project.zig`,
`arrangement.zig`, the audio/DSP modules, and the `main_*.zig` entry points. The
older CPU UI (`ui.zig`, `ui_gpu.zig`, `uikit.zig`) is superseded by `daw.zig`.
