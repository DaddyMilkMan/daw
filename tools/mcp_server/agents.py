"""Agent stubs for MCP server

Provides a set of simulated agents that accept simple actions for testing
and integration with other agents.
"""

from typing import Any, Dict


import subprocess
import os
import json


def _make_handler(name: str):
    def handler(action: str, params: Dict[str, Any]):
        # Simple, deterministic stub behavior for common actions
        if action == 'status':
            return {'status': 'ok', 'agent': name}
        if action == 'simulate':
            return {'simulated': True, 'agent': name, 'params': params}
        if action == 'press_button':
            return {'pressed': params.get('id')}
        if action == 'inspect':
            return {'inspected': params}
        return {'error': f'unknown action: {action}'}
    return handler


def _run_tests(repo_root: str):
    """Run unit tests in the repository and return a summary."""
    # Use pytest if available, otherwise fall back to unittest discovery
    env = os.environ.copy()
    try:
        # Prefer pytest for nicer output when installed
        res = subprocess.run(["pytest", "-q", repo_root], capture_output=True, text=True, timeout=120)
        return {'tool': 'pytest', 'returncode': res.returncode, 'stdout': res.stdout, 'stderr': res.stderr}
    except FileNotFoundError:
        # Fallback: run unittest discovery
        proc = subprocess.run(["python3", "-m", "unittest", "discover", "-v"], capture_output=True, text=True, timeout=120)
        return {'tool': 'unittest', 'returncode': proc.returncode, 'stdout': proc.stdout, 'stderr': proc.stderr}


def _render_svg_preview(svg_text: str, max_width: int = 800):
    """Return the raw SVG text as a preview. If cairosvg is available, return a PNG base64 raster.
    Otherwise return the SVG text for client-side rendering.
    """
    if not svg_text or '<svg' not in svg_text:
        return {'error': 'not_valid_svg'}
    # Try to rasterize using cairosvg if present
    try:
        import base64
        import io
        import cairosvg
        png_bytes = cairosvg.svg2png(bytestring=svg_text.encode('utf-8'))
        b64 = base64.b64encode(png_bytes).decode('ascii')
        return {'png_base64': b64}
    except Exception:
        # Fallback: return SVG text
        return {'svg': svg_text[:max_width*10]}


def _visual_diff(before: str, after: str):
    """Compute a simple textual diff of two small SVG strings. This is a placeholder for
    a real visual diff that would rasterize and compute pixel diffs.
    """
    # Use simple line-based diff
    import difflib
    diff = difflib.unified_diff(before.splitlines(True), after.splitlines(True), lineterm='')
    return {'diff': ''.join(diff)}


def _pixel_diff_svgs(before_svg: str, after_svg: str):
    """Attempt pixel-level diff by rasterizing SVGs to PNGs (requires cairosvg and Pillow).
    Returns simple stats and a small base64-encoded diff PNG if possible.
    """
    try:
        import cairosvg
        from PIL import Image, ImageChops
        import io, base64

        before_png = cairosvg.svg2png(bytestring=before_svg.encode('utf-8'))
        after_png = cairosvg.svg2png(bytestring=after_svg.encode('utf-8'))

        ib = Image.open(io.BytesIO(before_png)).convert('RGBA')
        ia = Image.open(io.BytesIO(after_png)).convert('RGBA')

        # Resize to smallest common size
        w = min(ib.width, ia.width)
        h = min(ib.height, ia.height)
        ib = ib.resize((w, h))
        ia = ia.resize((w, h))

        diff = ImageChops.difference(ib, ia)
        # Compute non-zero pixel count
        bbox = diff.getbbox()
        if not bbox:
            return {'pixels_changed': 0}
        # Return diff PNG
        buf = io.BytesIO()
        diff.save(buf, format='PNG')
        b64 = base64.b64encode(buf.getvalue()).decode('ascii')
        return {'pixels_changed': 1, 'diff_png_base64': b64}
    except Exception as e:
        return {'error': 'pixel_diff_not_available', 'detail': str(e)}


def _read_asset(path: str):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        return None

# In-memory meter store for Metering agent
METERS = {'last': None}


def _meter_handler(action: str, params: Dict[str, Any]):
    if action == 'push':
        METERS['last'] = params
        return {'status': 'ok'}
    if action == 'get':
        return {'last': METERS['last']}
    return {'error': f'unknown action: {action}'}


def _svg_viewer_handler(action: str, params: Dict[str, Any]):
    svg = params.get('svg', '') or params.get('path') and _read_asset(params.get('path'))
    if action in ('render', 'preview', 'status'):
        return _render_svg_preview(svg or params.get('svg', ''))
    if action in ('rasterize', 'raster'):
        if not svg:
            return {'error': 'no_svg_provided'}
        try:
        return _rasterize_svg_to_png(svg)
    except Exception as e:
        return {'error': 'rasterize_failed', 'detail': str(e)}
    return _make_handler('SVG Viewer')(action, params)


def _visual_diff_handler(action: str, params: Dict[str, Any]):
    if action == 'diff':
        return _visual_diff(params.get('before', ''), params.get('after', ''))
    if action == 'pixel_diff':
        try:
            return _pixel_diff_svgs(params.get('before', ''), params.get('after', ''))
        except Exception as e:
            return {'error': 'pixel_diff_failed', 'detail': str(e)}
    return _make_handler('Visual Diff')(action, params)


def _spectrogram_from_wav(path: str):
    """Generate a simple spectrogram summary from a WAV file using numpy and scipy if available.
    Returns a small JSON summary (not an image) to avoid heavy dependencies.
    """
    try:
        import wave
        import numpy as np
        with wave.open(path, 'rb') as wf:
            nchan = wf.getnchannels()
            fr = wf.getframerate()
            nframes = wf.getnframes()
            raw = wf.readframes(min(nframes, 44100))
        # Convert to numpy
        import struct
        fmt = '<' + 'h' * (len(raw)//2)
        data = np.array(struct.unpack(fmt, raw)).astype(np.float32)
        if nchan > 1:
            data = data.reshape(-1, nchan)[:,0]
        # Simple FFT
        fft = np.abs(np.fft.rfft(data))
        freqs = np.fft.rfftfreq(len(data), 1.0/fr)
        # Return peak frequency
        peak_idx = int(np.argmax(fft))
        return {'peak_freq': float(freqs[peak_idx]), 'sample_rate': fr}
    except Exception as e:
        return {'error': 'spectrogram_failed', 'detail': str(e)}


def _spectrogram_handler(action: str, params: Dict[str, Any]):
    if action in ('render', 'spectrogram', 'spec'):
        path = params.get('path')
        if not path:
            return {'error': 'no_path_provided'}
        return _spectrogram_from_wav(path)
    return _make_handler('Spectrogram')(action, params)


def _plugin_inspector_handler(action: str, params: Dict[str, Any]):
    if action == 'list':
        path = params.get('path', 'docs/plugins.json')
        text = _read_asset(path)
        try:
            return json.loads(text) if text else {}
        except Exception:
            return {'error': 'invalid_json', 'content': text}
    return _make_handler('Plugin Inspector')(action, params)


# Register agents with richer backends where possible
AGENTS = {
    'debugger': {'name': 'Debugger Bridge', 'description': 'Remote debugger (placeholder)', 'handler': _make_handler('Debugger Bridge')},
    'svg_viewer': {'name': 'SVG/UI Asset Viewer', 'description': 'Render or rasterize SVGs (optional cairosvg)', 'handler': _svg_viewer_handler},
    'ui_inspector': {'name': 'UI Inspector', 'description': 'Expose component snapshots from disk', 'handler': lambda action, params: {'tree': _read_asset(params.get('path', 'docs/ui_snapshots/tree.json'))} if action == 'inspect' else _make_handler('UI Inspector')(action, params)},
    'render_profiler': {'name': 'Render Profiler', 'description': 'Render stats (placeholder)', 'handler': _make_handler('Render Profiler')},
    'cpu_profiler': {'name': 'CPU Profiler', 'description': 'CPU sampling (placeholder)', 'handler': _make_handler('CPU Profiler')},
    'memory_profiler': {'name': 'Memory Profiler', 'description': 'Memory snapshots (placeholder)', 'handler': _make_handler('Memory Profiler')},
    'oscilloscope': {'name': 'Audio Oscilloscope', 'description': 'Return saved audio samples (placeholder)', 'handler': _make_handler('Oscilloscope')},
    'spectrogram': {'name': 'Spectrogram Agent', 'description': 'Generate spectrograms from WAV (optional numpy/scipy/matplotlib)', 'handler': _spectrogram_handler},
    'metering': {'name': 'Metering Streamer', 'description': 'Push/get meter snapshots', 'handler': _meter_handler},
    'plugin_inspector': {'name': 'Plugin Inspector', 'description': 'List plugins from JSON file', 'handler': _plugin_inspector_handler},
    'midi_monitor': {'name': 'MIDI Monitor', 'description': 'MIDI logs (placeholder)', 'handler': _make_handler('MIDI Monitor')},
    'session_replay': {'name': 'Session Replay', 'description': 'Session replay (placeholder)', 'handler': _make_handler('Session Replay')},
    'crash_reporter': {'name': 'Crash Reporter', 'description': 'Crash collection (placeholder)', 'handler': _make_handler('Crash Reporter')},
    'asset_hotswap': {'name': 'Asset Hot-swap', 'description': 'Hot-swap assets on disk (safe paths)', 'handler': _make_handler('Asset Hot-swap')},
    'network_sniffer': {'name': 'Network/MCP Sniffer', 'description': 'Return MCP logs (placeholder)', 'handler': _make_handler('Network Sniffer')},
    'visual_diff': {'name': 'Visual Diff Tool', 'description': 'Textual or pixel diffs (pixel requires cairosvg+pillow)', 'handler': _visual_diff_handler},
    'test_runner': {'name': 'Test Runner/CI Agent', 'description': 'Run repo tests', 'handler': lambda action, params: _run_tests(params.get('path', '.')) if action == 'run' else _make_handler('Test Runner')(action, params)},
    'localization_auditor': {'name': 'Localization Auditor', 'description': 'Localization checks (placeholder)', 'handler': _make_handler('Localization Auditor')},
    'script_runner': {'name': 'Live Script Runner', 'description': 'Run small scripts sandboxed (placeholder)', 'handler': _make_handler('Script Runner')},
    'telemetry': {'name': 'Telemetry Dashboard', 'description': 'Telemetry aggregator (placeholder)', 'handler': _make_handler('Telemetry')},
    'security_auditor': {'name': 'Security Auditor', 'description': 'Security traces (placeholder)', 'handler': _make_handler('Security Auditor')}
}


def invoke_agent(agent_id: str, action: str, params: Dict[str, Any]):
    agent = AGENTS.get(agent_id)
    if not agent:
        raise ValueError(f'Unknown agent: {agent_id}')
    return agent['handler'](action, params)
