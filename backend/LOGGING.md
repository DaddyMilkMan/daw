# Logging Guide for Zenith Backend

This document describes the standardized logging approach for the Zenith DAW backend services.

## Overview

The Zenith backend uses **structlog** for application logging, providing:

- Structured logging with key-value pairs
- Consistent formatting across all services
- JSON output support for production environments
- Integration with Python's standard logging for third-party libraries

## Quick Start

### Basic Usage

In any backend module:

```python
from backend.logger import get_logger

log = get_logger(__name__)

log.info("Operation completed", items_processed=42)
log.warning("Resource running low", available_memory="10MB")
log.error("Failed to connect", host="example.com", port=8080)
```

### Initialization

At the application entry point (e.g., `run_backend.py`, `cli.py`):

```python
from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger

config = ZenithConfig()
configure_logging(config)

log = get_logger(__name__)
log.info("Application started")
```

## Logging Levels

Use appropriate logging levels:

| Level | When to Use | Example |
|-------|-------------|---------|
| `DEBUG` | Detailed diagnostic information | `log.debug("Connection established", socket_id=123)` |
| `INFO` | Normal operational messages | `log.info("Service started", port=8080)` |
| `WARNING` | Potentially problematic situations | `log.warning("Retry attempt", attempt=3, max_attempts=5)` |
| `ERROR` | Error events that might still allow the application to continue | `log.error("Database query failed", query="SELECT *", error=str(e))` |

## Structured Logging Best Practices

### ✅ DO: Use key-value pairs for context

```python
log.info("User authenticated", user_id="12345", session_id="abc-def")
log.error("File not found", path="/data/config.json", operation="read")
```

### ✅ DO: Use meaningful log messages

```python
log.info("TCP connection established", remote_addr="192.168.1.100", port=54320)
```

### ❌ DON'T: Use printf-style formatting

```python
# Bad - don't do this
log.info("User %s logged in at %s", user_id, timestamp)

# Good - do this instead
log.info("User logged in", user_id=user_id, timestamp=timestamp)
```

### ❌ DON'T: Use f-strings for structured data

```python
# Bad - loses structure
log.info(f"Processing item {item_id} of {total}")

# Good - preserves structure
log.info("Processing item", item_id=item_id, total=total, progress=f"{item_id}/{total}")
```

## Exception Logging

For exceptions, you can pass exception information:

```python
try:
    risky_operation()
except Exception as e:
    log.error("Operation failed", error=str(e), exc_info=True)
```

Or use `log.exception()` which automatically includes traceback:

```python
try:
    risky_operation()
except Exception:
    log.exception("Operation failed", additional_context="value")
```

## Configuration

Logging is configured via `ZenithConfig` (environment variables or `.env` file):

| Environment Variable | Default | Description |
|---------------------|---------|-------------|
| `ZENITH_LOG_LEVEL` | `INFO` | Logging level: DEBUG, INFO, WARNING, ERROR |
| `ZENITH_LOG_JSON` | `False` | If true, output JSON logs; otherwise use console format |

### Development vs Production

**Development (console output):**
```bash
export ZENITH_LOG_LEVEL=DEBUG
export ZENITH_LOG_JSON=false
python -m backend.run_backend
```

**Production (JSON output for log aggregation):**
```bash
export ZENITH_LOG_LEVEL=INFO
export ZENITH_LOG_JSON=true
python -m backend.run_backend
```

## Output Formats

### Console Format (Development)
```
2026-01-03T21:00:00.123Z [info    ] Service started                port=8080 service=signaling
2026-01-03T21:00:05.456Z [warning ] Retry attempt                  attempt=2 max_attempts=5
```

### JSON Format (Production)
```json
{"event": "Service started", "port": 8080, "service": "signaling", "level": "info", "timestamp": "2026-01-03T21:00:00.123Z"}
{"event": "Retry attempt", "attempt": 2, "max_attempts": 5, "level": "warning", "timestamp": "2026-01-03T21:00:05.456Z"}
```

## Common Patterns

### Service Initialization
```python
log.info("Initializing service", service_name="signaling", version="1.0.0")
```

### Network Operations
```python
log.info("Connection accepted", remote_addr=str(addr), protocol="TLS")
log.debug("Sending packet", size_bytes=len(data), destination=host)
```

### Health Checks
```python
log.debug("Health check passed", service="upnp", response_time_ms=15)
log.warning("Health check failed", service="signaling", error="Connection refused")
```

### Background Tasks
```python
log.info("Cleanup task started", frequency_seconds=60)
log.info("Cleanup completed", items_removed=5, duration_ms=123)
```

## Migration from Standard Logging

If you encounter code using standard `logging` module:

```python
# Old approach - DON'T USE
import logging
logger = logging.getLogger("zenith.mymodule")
logger.info("User %s connected", user_id)

# New approach - USE THIS
from backend.logger import get_logger
log = get_logger(__name__)
log.info("User connected", user_id=user_id)
```

## Testing

When writing tests, you can configure logging for test output:

```python
from backend.config import ZenithConfig
from backend.logger import configure_logging

def setup_module():
    config = ZenithConfig()
    config.log_level = "DEBUG"
    configure_logging(config)
```

## Troubleshooting

### Logs not appearing
- Ensure `configure_logging()` is called before any logging
- Check that `ZENITH_LOG_LEVEL` is set appropriately
- Verify the logger is obtained via `get_logger(__name__)`

### Third-party library logs too verbose
Standard logging is configured via `configure_logging()`. To suppress specific libraries:

```python
import logging
logging.getLogger("urllib3").setLevel(logging.WARNING)
```

### JSON logs are hard to read locally
For development, always set `ZENITH_LOG_JSON=false` or use log parsing tools like `jq`:

```bash
python -m backend.run_backend | jq '.'
```

## Additional Resources

- [structlog documentation](https://www.structlog.org/)
- [Python logging HOWTO](https://docs.python.org/3/howto/logging.html)
- Backend configuration: `backend/config.py`
- Logger implementation: `backend/logger.py`
