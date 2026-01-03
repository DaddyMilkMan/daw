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
| `ZENITH_PERSISTENCE_BACKEND` | Session persistence backend: `memory`, `disk`, or `redis` | `memory` |
| `ZENITH_PERSISTENCE_DISK_PATH` | Path for disk-based persistence | `/tmp/zenith_sessions.json` |
| `ZENITH_PERSISTENCE_REDIS_URL` | Redis connection URL (requires redis-py) | `redis://localhost:6379/0` |
| `ZENITH_RATE_LIMIT_ENABLED` | Enable per-IP rate limiting | `true` |
| `ZENITH_RATE_LIMIT_WINDOW` | Rate limit window in seconds | `60` |
| `ZENITH_RATE_LIMIT_MAX` | Max session creations per window per IP | `10` |

### Session Persistence

By default, sessions are stored in memory only and will be lost on restart. For production deployments, enable persistence:

**Disk-based persistence** (single instance):
```bash
export ZENITH_PERSISTENCE_BACKEND=disk
export ZENITH_PERSISTENCE_DISK_PATH=/var/lib/zenith/sessions.json
```

**Redis-based persistence** (multi-instance):
```bash
pip install redis
export ZENITH_PERSISTENCE_BACKEND=redis
export ZENITH_PERSISTENCE_REDIS_URL=redis://localhost:6379/0
```

Sessions will automatically be restored after restart. See `signaling/SESSION_STORE_REFACTORING.md` for detailed documentation.

### Security Features

The signaling server includes security hardening:

- **Cryptographically secure session codes**: Uses Python's `secrets` module for unpredictable 6-digit codes (900K possibilities)
- **Rate limiting**: Prevents brute-force attacks by limiting session creation to 10 requests per minute per IP (configurable)
- **Thread-safe operations**: All session operations are protected with reentrant locks

To disable rate limiting for internal trusted networks:
```bash
export ZENITH_RATE_LIMIT_ENABLED=false
```

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


Copy `backend/zenith-backend.service` to `/etc/systemd/system/`, adjust the paths/log file, and enable the unit:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now zenith-backend.service
```
