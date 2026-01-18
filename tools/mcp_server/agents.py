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
    """Return the raw SVG text as a preview. A real renderer could rasterize to PNG with cairosvg,
    but we keep this dependency-free for now and return the SVG content for clients to render.
    """
    # Minimal validation: ensure it looks like SVG
    if '<svg' not in svg_text:
        return {'error': 'not_valid_svg'}
    return {'svg': svg_text[:max_width*10]}


def _visual_diff(before: str, after: str):
    """Compute a simple textual diff of two small SVG strings. This is a placeholder for
    a real visual diff that would rasterize and compute pixel diffs.
    """
    # Use simple line-based diff
    import difflib
    diff = difflib.unified_diff(before.splitlines(True), after.splitlines(True), lineterm='')
    return {'diff': ''.join(diff)}


def _read_asset(path: str):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        return None

# Register a wide range of agents (realistic backends for Phase 1)
AGENTS = {
    'debugger': {
        'name': 'Debugger Bridge',
        'description': 'Remote debugger attachment, breakpoint and stack inspection (NOT IMPLEMENTED - placeholder).',
        'handler': _make_handler('Debugger Bridge')
    },
    'svg_viewer': {
        'name': 'SVG/UI Asset Viewer',
        'description': 'Renders and inspects SVG/Skia assets. Returns raw SVG or simple preview.',
        'handler': lambda action, params: _render_svg_preview(params.get('svg', '')) if action in ('render', 'preview', 'status') else _make_handler('SVG Viewer')(action, params)
    },
    'ui_inspector': {
        'name': 'UI Inspector',
        'description': 'Expose component tree snapshots from files under docs/ui_snapshots (if present).',
        'handler': lambda action, params: {'tree': _read_asset(params.get('path', 'docs/ui_snapshots/tree.json'))} if action == 'inspect' else _make_handler('UI Inspector')(action, params)
    },
    'render_profiler': {
        'name': 'Render Profiler',
        'description': 'Collects render statistics from recorded traces (placeholder).',
        'handler': _make_handler('Render Profiler')
    },
    'cpu_profiler': {
        'name': 'CPU Profiler',
        'description': 'Run a light-weight sampling profiler on test commands (uses /usr/bin/time where available).',
        'handler': _make_handler('CPU Profiler')
    },
    'memory_profiler': {
        'name': 'Memory Profiler',
        'description': 'Capture memory usage via external tools (placeholder).',
        'handler': _make_handler('Memory Profiler')
    },
    'oscilloscope': {
        'name': 'Audio Oscilloscope',
        'description': 'Stream audio buffers from saved files under tools/mcp_server/audio_samples (if present).',
        'handler': _make_handler('Oscilloscope')
    },
    'spectrogram': {
        'name': 'Spectrogram Agent',
        'description': 'Generate spectrogram images from audio files (placeholder).',
        'handler': _make_handler('Spectrogram')
    },
    'metering': {
        'name': 'Metering Streamer',
        'description': 'Expose MeteringSystem snapshots via saved JSON (placeholder).',
        'handler': _make_handler('Metering')
    },
    'plugin_inspector': {
        'name': 'Plugin Inspector',
        'description': 'List loaded plugins by reading a plugins.json file (if present).',
        'handler': lambda action, params: json.loads(_read_asset(params.get('path', 'docs/plugins.json')) or '{}') if action == 'list' else _make_handler('Plugin Inspector')(action, params)
    },
    'midi_monitor': {
        'name': 'MIDI Monitor',
        'description': 'Return recorded MIDI events from tools/mcp_server/midi_log.json if present.',
        'handler': _make_handler('MIDI Monitor')
    },
    'session_replay': {
        'name': 'Session Replay',
        'description': 'Replay recorded session events (placeholder).',
        'handler': _make_handler('Session Replay')
    },
    'crash_reporter': {
        'name': 'Crash Reporter',
        'description': 'Collect crash dumps and symbolicate (placeholder).',
        'handler': _make_handler('Crash Reporter')
    },
    'asset_hotswap': {
        'name': 'Asset Hot-swap',
        'description': 'Swap assets on disk (safe paths only under tools/mcp_server/assets).',
        'handler': _make_handler('Asset Hot-swap')
    },
    'network_sniffer': {
        'name': 'Network/MCP Sniffer',
        'description': 'Return recent MCP traffic logs from memory (server keeps a short log).',
        'handler': _make_handler('Network Sniffer')
    },
    'visual_diff': {
        'name': 'Visual Diff Tool',
        'description': 'Produce simple textual diffs for SVGs or return placeholder pixel diffs.',
        'handler': lambda action, params: _visual_diff(params.get('before',''), params.get('after','')) if action == 'diff' else _make_handler('Visual Diff')(action, params)
    },
    'test_runner': {
        'name': 'Test Runner/CI Agent',
        'description': 'Run repository tests and return the output.',
        'handler': lambda action, params: _run_tests(params.get('path', '.')) if action == 'run' else _make_handler('Test Runner')(action, params)
    },
    'localization_auditor': {
        'name': 'Localization & Accessibility Auditor',
        'description': 'Scan resources for missing localization keys (placeholder).',
        'handler': _make_handler('Localization Auditor')
    },
    'script_runner': {
        'name': 'Live Script Runner',
        'description': 'Execute sandboxed scripts with timeouts (placeholder).',
        'handler': _make_handler('Script Runner')
    },
    'telemetry': {
        'name': 'Telemetry Dashboard',
        'description': 'Aggregate CPU/memory/crash trends and metrics (placeholder).',
        'handler': _make_handler('Telemetry')
    },
    'security_auditor': {
        'name': 'Security Auditor',
        'description': 'Trace plugin/file/network permission flows (placeholder).',
        'handler': _make_handler('Security Auditor')
    }
}


def invoke_agent(agent_id: str, action: str, params: Dict[str, Any]):
    agent = AGENTS.get(agent_id)
    if not agent:
        raise ValueError(f'Unknown agent: {agent_id}')
    return agent['handler'](action, params)
