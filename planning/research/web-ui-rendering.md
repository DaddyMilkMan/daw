# How web UI got that good — and how to match it in Zig at native speed

Deep-research synthesis (2026-06-13). 21 sources, 25 claims adversarially verified (2/3-vote), 22 confirmed. Primary sources: Zed GPUI engineering blog + actual Metal shader source, Raph Levien (piet-gpu/Vello) blogs + repo, the 2025 ETH Vello thesis, Valve/Chris Green SIGGRAPH 2007, Inigo Quilez SDF reference, Evan Wallace (Figma) shadow shader.

## The one big idea

Web UI looks good because the browser is, under the hood, a **GPU compositor that draws each kind of primitive with a purpose-built shader** and composites in a color-correct pipeline. You do **not** reproduce that by getting a CPU rasterizer "good enough" — you reproduce it by copying the architecture the fastest native UI engine (Zed's **GPUI**) uses:

> Drop the general-purpose 2D library. Render each UI primitive type — rounded rects, shadows, glyphs, icons, images — with its **own dedicated GPU shader**, each **batched into one instanced draw call per type**, in **linear (gamma-correct) space**. Shapes come from **signed distance fields (SDF)**; shadows come from a **closed-form analytic blur** (no offscreen blur pass). *(verified 3-0, zed.dev/blog/videogame + shaders.metal)*

That single pivot is what separates "looks like a toy" from "looks like Linear/Figma," and it runs faster than what we do now (one big CPU framebuffer re-rasterized every frame).

---

## Layer 1 — the rendering pipeline (what makes it crisp)

### Rounded rectangles → SDF, not coverage
The canonical rounded-box SDF (Inigo Quilez, used verbatim by GPUI):
```glsl
// p = pixel relative to rect center, b = half-size, r = corner radius
vec2 q = abs(p) - b + r;
float d = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
// AA edge: derivative-sized smooth step
float aa = fwidth(d);
float alpha = 1.0 - smoothstep(-aa, aa, d);
```
One distance field gives you the fill, a 1px border (`abs(d) < t`), an inner highlight, and a glow — all from the same `d`. *(verified 3-0)*

### Drop shadows → analytic erf, not box blur
A Gaussian convolved with a step is the **error function (erf)**. Evan Wallace's result (adopted by GPUI, independently confirmed by Levien): a blurred **axis-aligned** rectangle factors into **two perpendicular 1-D erf-blurred boxes multiplied** — a constant-time per-pixel expression, *no offscreen render + separable blur*:
```
F(x) = 0.5 * (1 + erf(x / (sigma * sqrt(2))))     // 1-D blurred-box edge
shadow = (F(x2)-F(x1)) * (F(y2)-F(y1))            // axis-aligned rect
```
Evaluate `erf` with the Abramowitz-Stegun 7.1.27 polynomial (coeffs 0.278393 / 0.230389 / 0.078108, max err 2.5e-5). For a **rounded**-rect shadow there's no full closed form: use erf along one axis and **4 fixed samples** sliding the Gaussian along the other — still O(1) per pixel regardless of blur radius. *(verified 3-0)* This replaces our `boxBlur` + layered `dropShadow` entirely and is both cheaper and correct.

### Glyphs & vector icons → SDF atlas
Valve/Green SIGGRAPH 2007: generate an SDF from a high-res binary glyph, store distance in **one 8-bit channel** of a small texture. Because distance is locally linear, GPU bilinear filtering reconstructs **sharp edges at any scale** (coverage textures produce nonlinear "wiggles"); a 0.5 threshold needs no shader, and `smoothstep`+`fwidth` gives scale-invariant AA. Same field yields outlines, glows, offset-lookup shadows — shipped in Team Fortress 2 "with no significant performance degradation." Caveat: single-channel SDF rounds sharp corners → use **MSDF** (Chlumsky) for detailed icons. *(verified 3-0)* `dist/length(gradient)` is a more accurate AA than literal `smoothstep`.

### Gamma-correct / premultiplied alpha (the cheap, invisible-until-you-see-it win)
Browsers blend in **linear** space, not sRGB. Blending in sRGB (what our CPU `pset` does today) makes edges and gradients look muddy/dark — the classic "why does my AA look dirty." Fix: decode sRGB→linear, blend with **premultiplied alpha**, encode back. On GPU it's nearly free: `glEnable(GL_FRAMEBUFFER_SRGB)` + a linear-space texture, or decode/encode in the shader. *(flagged by research as an unaddressed gap; standard graphics knowledge — Blinn compositing, premult)*

---

## Layer 2 — the design system (what makes it look "designed")

The research flagged this layer as **not yet verified** (the searches went deep on the engine layer), but the conventions are well established and are the **lowest-effort, highest-visual-impact** changes because they're just constants:

- **OKLCH/OKLab color** — perceptually uniform. Build each track/accent color as a *ramp* by varying only L (lightness) at fixed C/H, so hovers/press/disabled states are perceptually even instead of the muddy RGB lerps we do now. (evilmartians OKLCH)
- **8pt spacing grid + modular type scale** — every gap/pad/size a multiple of 4/8; font sizes on a ratio (e.g. 1.25). Kills the "random spacing" toy tell.
- **Layered elevation** — real surfaces use **2-3 stacked shadows** (tight dark contact + soft wide ambient), not one blur. Pair with a slightly *lighter* surface as it rises. (Material elevation, designsystems.surf)
- **1px hairlines + inner top highlight** — exactly the rim we just added; the convention is `border: rgba(255,255,255,~0.08)` + a brighter 1px top inset. Linear/Geist/shadcn all do this.

---

## Layer 3 — the longer horizon: arbitrary vector content (waveforms, automation curves)

SDF/analytic shaders cover rects/text/shadows/icons — i.e. ~95% of a DAW chrome. For **arbitrary curves** (audio waveforms, automation lanes, EQ curves) the reference is **piet-gpu / Vello** compute rasterization (sort-middle): GPU curve flattening → tile allocation → coarse raster via atomics → **prefix-sum** backdrop propagation → fine raster, with **exact analytic area-coverage AA via winding-number summation** (not MSAA). Runs ~7.6ms for dense vector text even on Intel HD 630. *(verified 3-0)*

Most portable to Zig: **Vello CPU "sparse strips"** — a run-length-encoded intermediate that stores only the anti-aliased boundary pixels explicitly and fills/blanks implicitly. SIMD- and thread-friendly, **no GPU required**, and the pipeline (4×4 tile gen → strip gen with analytic area coverage → 256×4 wide-tile coarse → fine raster) cleanly supports a CPU/GPU hybrid. This is the cleanest thing to port for waveform rendering. *(verified 3-0, 2025 ETH thesis advised by Levien)*

---

## Prioritized plan for Zenith (mapped to our code)

**Phase 0 — color & design tokens (hours, zero architecture change, big visible win).** In the *current* CPU renderer: switch `pset`/`fillRect` blending to linear space (sRGB decode→blend→encode); build the palette as OKLCH L-ramps; put spacing/type on an 8pt/modular scale; upgrade `dropShadow` to a 2-layer elevation set. Verifiable immediately via our BMP/GPU capture.

**Phase 1 — the GPUI pivot (the real upgrade).** Replace "rasterize whole frame on CPU → upload one texture → present quad" with **per-primitive instanced GPU shaders** drawn into the GLX framebuffer:
1. Rounded-rect SDF shader (fill + border + highlight from one `d`).
2. Analytic erf shadow shader (kills `boxBlur`).
3. Glyph/icon SDF atlas shader.
We already proved we can load modern GL entrypoints via `glXGetProcAddressARB` (window_gl.zig runs a GLSL blur), so the context is ready. Keep the CPU path as a fallback. Batch one instanced draw call per primitive type.

**Phase 2 — Vello-style sparse strips** for waveforms/automation curves, when we build the clip editor / piano roll.

**Still to research (the gaps the verifier honestly flagged):** exact GL sRGB/premult recipe for our GLX present, the backdrop-blur "glassmorphism" pipeline (downsample + dual-Kawase on a copy of the framebuffer behind translucent panels), and the design-token specifics. These deserve their own short pass before Phase 1.

## Sources (highest quality)
- Zed GPUI: zed.dev/blog/videogame, /blog/120fps, github.com/zed-industries/zed (shaders.metal)
- Shadows: madebyevan.com/shaders/fast-rounded-rectangle-shadows, raphlinus.github.io/graphics/2020/04/21/blurred-rounded-rects.html
- SDF text: dl.acm.org/doi/10.1145/1281500.1281665 (Green 2007), redblobgames.com/x/2403-distance-field-fonts
- SDF shapes: iquilezles.org/articles/distfunctions
- Compute 2D: raphlinus.github.io/.../fast-2d-rendering.html, github.com/linebender/vello, 2025 ETH Vello thesis
- Color/design: evilmartians.com/chronicles/oklch-in-css-why-quit-rgb-hsl, designsystems.surf/articles/depth-with-purpose
