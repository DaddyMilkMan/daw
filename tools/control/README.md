# Scripted control — "Playwright for Zenith"

Drive the **real** app (showcase, daw, any GPU window) with synthetic input and
capture screenshots — no app-UI changes required. The control channel lives in
`zig/window_glx.zig`: when `ZENITH_SCRIPT=<file>` is set, the window replays the
script's input into `poll()` instead of the OS, and `glReadPixels` → PNG on `shot`.

## Run

```sh
ZENITH_SCRIPT=tools/control/interact.txt ZENITH_WINDOW_SECONDS=12 zig build showcase
# also works for: zig build daw, zig build window, ...
```

## Script grammar (one action per line, `#` = comment)

```
move <x> <y>     # move the cursor (window pixels)
down             # press the left mouse button at the current position
up               # release
key <keycode>    # send a key (X11 keycode; e.g. 9 = Escape, 65 = Space)
wait <frames>    # idle N frames (~16ms each) — put `wait 1` between drag steps
shot <file.png>  # screenshot the current frame (real GPU framebuffer) to file.png
quit             # close the window
```

Actions run within a frame until a `wait`/`shot`/`quit`. For a drag, alternate
`move`/`wait 1` so the widget processes each incremental position. Mouse button and
position are sticky between frames.

Keyboard shortcuts work too via `key <keycode>` (e.g. the daw's transport keys) —
the event goes straight into the app's normal key handling.

## Examples
- `baseline.txt` — settle, screenshot, quit.
- `interact.txt` — showcase: drag a fader low→high, turn a knob, sweep a slider, click a
  toggle (verified: each widget responded).
- `daw_interact.txt` — the live **DAW**: Space toggles transport (Playing↔Stopped), then
  drag the Drums mixer fader 85→0→100. Verified by reading the on-screen value/status.

Screenshots (`*.png`) are git-ignored; scripts are kept as fixtures.

## Zoom tool

Thumbnails are too small to read values; `zig/main_crop.zig` crops + upscales a region:

```sh
zig run zig/main_crop.zig -- <in.png> <x> <y> <w> <h> <scale> <out.png>
```

The act → screenshot → **zoom to read the value/state** → adjust loop is exactly how a
browser agent locates and verifies elements.

## Notes
- The live DAW binds **Space → transport toggle** and **Esc → quit** (`main_daw.zig`); the
  transport play button is at ~(170, 29) and mixer faders are in the bottom strips.
