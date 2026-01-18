"""Agent stubs for MCP server

Provides a set of simulated agents that accept simple actions for testing
and integration with other agents.
"""

from typing import Any, Dict


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

# Register a wide range of agents (stubs) discussed during brainstorming
AGENTS = {
    'debugger': {
        'name': 'Debugger Bridge',
        'description': 'Remote debugger attachment, breakpoint and stack inspection (simulated).',
        'handler': _make_handler('Debugger Bridge')
    },
    'svg_viewer': {
        'name': 'SVG/UI Asset Viewer',
        'description': 'Renders and inspects SVG/Skia assets (simulated preview).',
        'handler': _make_handler('SVG Viewer')
    },
    'ui_inspector': {
        'name': 'UI Inspector',
        'description': 'Live component tree and property editor (simulated).',
        'handler': _make_handler('UI Inspector')
    },
    'render_profiler': {
        'name': 'Render Profiler',
        'description': 'Per-frame draw-call counts and overdraw heatmap (simulated).',
        'handler': _make_handler('Render Profiler')
    },
    'cpu_profiler': {
        'name': 'CPU Profiler',
        'description': 'Per-thread sampling and flamegraphs (simulated).',
        'handler': _make_handler('CPU Profiler')
    },
    'memory_profiler': {
        'name': 'Memory Profiler',
        'description': 'Heap snapshots, allocations timeline (simulated).',
        'handler': _make_handler('Memory Profiler')
    },
    'oscilloscope': {
        'name': 'Audio Oscilloscope',
        'description': 'Time-domain audio buffer viewer (simulated).',
        'handler': _make_handler('Oscilloscope')
    },
    'spectrogram': {
        'name': 'Spectrogram Agent',
        'description': 'Frequency-domain waterfall view (simulated).',
        'handler': _make_handler('Spectrogram')
    },
    'metering': {
        'name': 'Metering Streamer',
        'description': 'Real-time VU/PPM/LUFS graphs (simulated).',
        'handler': _make_handler('Metering')
    },
    'plugin_inspector': {
        'name': 'Plugin Inspector',
        'description': 'List loaded plugins and params (simulated).',
        'handler': _make_handler('Plugin Inspector')
    },
    'midi_monitor': {
        'name': 'MIDI Monitor',
        'description': 'Live MIDI log and injector (simulated).',
        'handler': _make_handler('MIDI Monitor')
    },
    'session_replay': {
        'name': 'Session Replay',
        'description': 'Record and deterministic replay of UI+engine events (simulated).',
        'handler': _make_handler('Session Replay')
    },
    'crash_reporter': {
        'name': 'Crash Reporter',
        'description': 'Capture crashes and symbolicate stack traces (simulated).',
        'handler': _make_handler('Crash Reporter')
    },
    'asset_hotswap': {
        'name': 'Asset Hot-swap',
        'description': 'Swap images/fonts/samples live and preview diffs (simulated).',
        'handler': _make_handler('Asset Hot-swap')
    },
    'network_sniffer': {
        'name': 'Network/MCP Sniffer',
        'description': 'Inspect MCP messages, replay and latency analytics (simulated).',
        'handler': _make_handler('Network Sniffer')
    },
    'visual_diff': {
        'name': 'Visual Diff Tool',
        'description': 'Produce before/after image diffs (simulated).',
        'handler': _make_handler('Visual Diff')
    },
    'test_runner': {
        'name': 'Test Runner/CI Agent',
        'description': 'Run tests and collect artifacts (simulated).',
        'handler': _make_handler('Test Runner')
    },
    'localization_auditor': {
        'name': 'Localization & Accessibility Auditor',
        'description': 'Find missing translations and contrast issues (simulated).',
        'handler': _make_handler('Localization Auditor')
    },
    'script_runner': {
        'name': 'Live Script Runner',
        'description': 'Hot-reloadable sandbox for UI/DSP experiments (simulated).',
        'handler': _make_handler('Script Runner')
    },
    'telemetry': {
        'name': 'Telemetry Dashboard',
        'description': 'Aggregate CPU/memory/crash trends and metrics (simulated).',
        'handler': _make_handler('Telemetry')
    },
    'security_auditor': {
        'name': 'Security Auditor',
        'description': 'Trace plugin/file/network permission flows (simulated).',
        'handler': _make_handler('Security Auditor')
    },
    'ui_inspector': {
        'name': 'UI Inspector',
        'description': 'Duplicate key mapping for convenience (ui_inspector).',
        'handler': _make_handler('UI Inspector')
    }
}


def invoke_agent(agent_id: str, action: str, params: Dict[str, Any]):
    agent = AGENTS.get(agent_id)
    if not agent:
        raise ValueError(f'Unknown agent: {agent_id}')
    return agent['handler'](action, params)
