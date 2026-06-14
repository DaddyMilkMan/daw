# realvst3 — a real VST3 plugin (built from the official Steinberg SDK)

`plugin.cpp` is a minimal but genuine VST3 plugin compiled against the vendored
Steinberg VST3 SDK interface headers. Unlike `zig/vst3_test_plugin.zig` (which
hand-builds the COM vtables in Zig), this produces **compiler-generated C++ vtables**
from the real SDK interface classes — the authentic third-party ABI. It's a 440 Hz
stereo tone generator that also persists a gain via `IBStream`.

Used to prove Zenith's VST3 host (`zig/vst3_host.zig`) works against real plugins,
through the full **install → scan/bundle-resolve → host → uninstall** lifecycle.

## Build + install + host + remove

```sh
SDK="build/_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK"
zig c++ -std=c++17 -fPIC -shared -fvisibility=hidden -O2 -I"$SDK" \
    tools/realvst3/plugin.cpp -o /tmp/ZenithReal.so

# install as a standard Linux .vst3 bundle
B="$HOME/.vst3/ZenithReal.vst3/Contents/x86_64-linux"
mkdir -p "$B" && cp /tmp/ZenithReal.so "$B/ZenithReal.so"

# host it in zenith (resolves the bundle, processes, FFT-verifies 440 Hz)
zig build vst3 -- "$HOME/.vst3/ZenithReal.vst3"

# remove it
rm -rf "$HOME/.vst3/ZenithReal.vst3"
```

Verified 2026-06-14: `module 'ZenithReal.vst3' vendor='ZenithReal' … dominant 439.5 Hz …
state round-trips`. The built `.so` is a binary artifact (git-ignored); only the
source is committed.
