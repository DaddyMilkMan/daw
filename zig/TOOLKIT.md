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
| `color.zig` | 8-bit RGBA `Color` + a color **utility kit**: `lerp`, `shade`/`lighten`/`darken`, `tint`, `alpha`, and `oklch()` (perceptual; comptime-friendly for muted palettes) | — (nothing) |
| `font.zig` / `ttf.zig` | Antialiased font descriptor + a **runtime TrueType rasterizer** (`ttf.rasterizeAscii` loads any installed `.ttf`). `font_*.zig` are baked data sets. | — |
| `image.zig` / `svg.zig` | **PNG decoder** + **SVG vector rasterizer**, both → straight-alpha `image.Image` (→ `gpu2d.GpuImage`). | `std` only |
| `gpu2d.zig` | **GPU 2D renderer.** Instanced shaders, one draw call per type, linear-light + premultiplied, 4× MSAA. Primitives **(à la carte)**: SDF rounded rects (`rect`/`card`/`rectGrad`/`stroke`), analytic gaussian shadows (`shadow`) + **elevation presets** (`elevate`, `Elevation.e1..e4`), **`glow`**, triangles/lines, atlas text (`GpuFont`), textured images (`image`), **dual-Kawase blur + `glass`** (Liquid Glass), and **`grain`** (film-grain finish). | `color`, `font` + OpenGL 3.3 + a `glGetProcAddress` loader |
| `trellis.zig` | **Trellis** — a declarative layout engine (flow/box). Sizing `px/grow/percent/fit`, `row`/`col`, `gap`/`pad`/`justify`/`align`; two-pass measure→arrange; hover/press anim + click-by-id; `rectOf(id)` overlays. Plus **design tokens** `sp` (4px grid) + `radius`, and **motion** helpers `easeOutCubic`/`easeInOut`/`approach`. | `gpu2d` (→ `color`, `font`) |
| `widgets.zig` | **GPU immediate-mode widgets** — faders, sliders (+bipolar), rotary knobs, toggles, checkboxes, segmented tabs, progress, **data tables**, **VU `meter`** (attack/release ballistics), **`waveform`** (peak-analysis audio display), icon slots; per-id hover/press animation. Drive with `trellis` rects or raw coords. | `gpu2d` (→ `color`) |
| `midi2.zig` / `midi2_alsa.zig` | **MIDI 2.0 / UMP** packets + protocol, and a native UMP ALSA in/out client (auto-connect + hot-plug). | `std` (+ `asound` for the ALSA client) |
| `audio_engine.zig` / `audio_alsa.zig` | Real-time audio engine (synth + loop, lock-free note queue) over an ALSA PCM device. | `std` + `asound` |
| `window_glx.zig` | Borderless, resizable, custom-chrome **GLX window** (full input, EWMH move/resize/min/max, MSAA visual, `glGetProcAddress`). Hand-declared C ABIs, no `@cImport`. | X11 + GL + libc (Linux/X11) |
| `render2d.zig` | Alternative **CPU software 2D renderer** (AA rounded rects, subpixel/grayscale text, gamma-correct linear blending, separable blur, layered shadow). The original render path; `gpu2d` supersedes it for the live UI but this remains a dependency-light, GPU-free option. | `color`, `font` |

## Reusable primitives — things you can reach for (not templates)

À-la-carte building blocks. Compose them; there's no "use the whole widget or
nothing" coupling.

- **Color** (`color.zig`): `lerp(a,b,t)`, `c.lighten(amt)` / `c.darken(amt)` /
  `shade(c, ±amt)`, `c.tint(toward, t)` (the muted-surface trick), `c.alpha(a)`,
  `oklch(L,C,H)` for perceptual palettes (keep chroma ~0.07 for modern muted tints).
- **Shadows / depth** (`gpu2d.zig`): `g.shadow(x,y,w,h,r,sigma,color)` (raw), or the
  presets `g.elevate(x,y,w,h,r, .e1.. .e4)` for consistent two-layer elevation.
- **Effects** (`gpu2d.zig`): `g.glow(cx,cy,r,color)` (focus/active bloom);
  `g.captureBlur(w,h)` + `g.glass(x,y,w,h,r,tint,border)` (frosted Liquid Glass);
  `g.grain(w,h,amount)` (film-grain finish — call last, ~0.014).
- **Surfaces / shapes** (`gpu2d.zig`): `g.rect`, `g.rectGrad`, `g.card` (material
  depth + dither), `g.stroke` (hairline ring), `g.tri`/`g.line`, `g.image`.
- **Motion** (`trellis.zig`): `approach(cur,target,dt,speed)` (smooth, frame-rate-
  independent), `easeOutCubic`, `easeInOut`.
- **Spacing / radii tokens** (`trellis.zig`): `sp."1".. "10"` (4px grid), `radius.xs..xl/pill`.
- **Text** (`gpu2d.GpuFont`): `text`, `textTracked` (letter-spacing), `textNum`
  (tabular figures), `textWidth`; load any `.ttf` at runtime via `ttf.rasterizeAscii`.

## To extract a piece

- **Just the color type:** copy `color.zig`.
- **The GPU renderer:** copy `color.zig`, `font.zig` (+ a `font_*.zig` data set), `gpu2d.zig`. Provide an OpenGL 3.3 context and a proc loader (one function: `fn(name: [*:0]const u8) ?*const anyopaque`).
- **The layout engine + widgets:** the above + `trellis.zig` + `widgets.zig`.
- **A windowed Linux app:** add `window_glx.zig` (links `X11`, `GL`, `libc`).
- **A CPU-only renderer (no GPU):** copy `color.zig`, `font.zig`, `render2d.zig`.

The full stack assembled together is the live DAW: `daw.zig` (view) +
`main_daw.zig` (`zig build daw`, binary `zenith`).

DAW-specific code (not part of the reusable toolkit): `daw.zig`, `project.zig`,
`arrangement.zig`, the audio/DSP modules, and the `main_*.zig` entry points. The
older CPU UI (`ui.zig`, `ui_gpu.zig`, `uikit.zig`) is superseded by `daw.zig`.
