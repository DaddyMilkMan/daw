# Zenith Backend Services

This directory contains the signaling server and UPnP helpers that keep the peer-to-peer backend working.

## Running

Use the CLI entry point to start the signaling server together with the optional UPnP port mapper:

```bash
python -m backend.run_backend
```

The CLI now exposes log file controls and a health endpoint for monitoring:

- `--log-file`: path for a rotating logfile (default keeps console-only).
- `--log-max-bytes` / `--log-backups`: configure rotation limits.
- `--health-port`: serves the `/health` JSON endpoint (default `8000`).

To skip the UPnP step, pass `--disable-upnp`. Other flags remain:

- `--upnp-port`: TCP port to map via UPnP (default `54321`).
- `--upnp-timeout`: Discovery/SOAP timeout in seconds (default `3`).
- `--log-level`: Logging verbosity (`DEBUG`, `INFO`, etc.).

The signaling component respects these environment variables as well:

| Variable | Description | Default |
|----------|-------------|---------|
| `ZENITH_SIGNALING_HOST` | Interface to bind TCP/UDP sockets | `0.0.0.0` |
| `ZENITH_SIGNALING_PORT` | TLS TCP port for signaling | `54320` |
| `ZENITH_SIGNALING_UDP_PORT` | UDP port for hole punching | `54321` |
| `ZENITH_SIGNALING_SESSION_TTL` | Seconds before sessions expire | `300` |
| `ZENITH_SIGNALING_CLEAN_FREQ` | Cleanup interval (seconds) | `60` |

TLS certificates must live in this directory as `cert.pem`/`key.pem` before starting the service.

### Health endpoint

The service exposes `/health` on the supplied port (default `8000`). Querying it returns JSON with the signaling and UPnP status, e.g.:

```json
{"signaling": "running", "upnp": "mapped"}
```

This makes it easy to hook into containers or monitoring stacks. For Prometheus users, add a `blackbox_exporter` job:

```yaml
scrape_configs:
  - job_name: zenith-backend
    metrics_path: /probe
    params:
      module: [http_2xx]
    static_configs:
      - targets:
        - http://localhost:8000/health
    relabel_configs:
      - source_labels: [__address__]
        target_label: __param_target
      - source_labels: [__param_target]
        target_label: instance
      - target_label: __address__
        replacement: blackbox-exporter:9115
```

Then raise alerts like:

```yaml
alert: ZenithBackendDown
expr: probe_success{job="zenith-backend"} == 0
for: 5m
labels:
  severity: critical
annotations:
  summary: "Zenith backend unreachable"
```

You can also use `scripts/check_backend_health.sh` from cron/CI to trigger notifications when the endpoint is unhealthy.

### Systemd service


Copy `services/ai/runtime/zenith-backend.service` to `/etc/systemd/system/`, adjust the paths/log file, and enable the unit:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now zenith-backend.service
```
