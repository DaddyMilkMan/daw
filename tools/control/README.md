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

**By id (DEFAULT — deterministic, no eyeballing):**
```
dumpids <file>   # write the interactive widget map (id -> rect) — the "DOM"
moveid <id>      # move the cursor to the CENTER of widget <id>
clickid <id>     # click widget <id> (press + auto-release next frame)
rmove <dx> <dy>  # move relative to the current cursor (for drags after moveid)
```

**By pixel (fallback — when a target has no id, or for empty-space drags):**
```
move <x> <y>     # move the cursor to absolute window pixels
down / up        # press / release the left mouse button
```

**Shared:**
```
key <keycode>    # send a key (X11 keycode; 9 = Escape, 65 = Space)
wait <frames>    # idle N frames (~16ms each) — put `wait 1` between drag steps
shot <file.png>  # screenshot the current frame (real GPU framebuffer)
quit             # close the window
```

Actions run within a frame until a `wait`/`shot`/`clickid`/`quit`. For a drag, alternate
moves with `wait 1` so the widget processes each step. Cursor + button state are sticky.

### Why by-id is the default
`flex.zig`/`widgets.zig` publish every interactive widget's rect to `uireg.zig` each
frame; `moveid`/`clickid` resolve the exact center, and `dumpids` lists every widget.
Tested both ways on the DAW: **by-id hit the play button + Drums fader first try**, while
by-pixel needed a wrong guess → zoom → corrected coordinate. So: `dumpids` once to learn
the ids, then drive by id. Pixel actions remain for un-id'd targets.

Keyboard shortcuts work too via `key <keycode>` (e.g. the daw's transport keys) —
the event goes straight into the app's normal key handling.

## Examples
- `baseline.txt` — settle, screenshot, quit.
- `interact.txt` — showcase: drag a fader low→high, turn a knob, sweep a slider, click a
  toggle (verified: each widget responded).
- `daw_interact.txt` — the live **DAW**, BY ID: `clickid 1` toggles transport, `moveid 100`
  + `rmove` drags the Drums fader to 0 then 100. Verified by reading the on-screen value/status.
- `daw_ids.txt` — a captured `dumpids` of the DAW: 71 interactive widgets. Key ids:
  `1` play, `2` stop, `3` record; `100..105` mixer faders (Drums..Master), `300+`/`320+`
  track mute/solo, `400+` pan, `600+`/`700+` send knobs, `1000+` browser items.

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
