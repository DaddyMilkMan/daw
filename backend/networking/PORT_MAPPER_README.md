# UPnP Port Mapper - Maintainer's Guide

## Overview

The `port_mapper.py` module provides automatic port mapping configuration for NAT routers using the UPnP (Universal Plug and Play) protocol. This enables peer-to-peer connections in Zenith DAW without manual router configuration.

## Architecture

### Key Components

1. **UPnPPortMapper Class**: Main orchestrator for the port mapping workflow
2. **Custom Exception Hierarchy**: Structured error handling
3. **Retry Logic**: Exponential backoff for resilient operation
4. **Fallback Mode**: Graceful degradation when UPnP is unavailable
5. **Comprehensive Logging**: Detailed visibility into operations

## Configuration

### Environment Variables

All configuration can be customized via environment variables:

| Variable | Description | Default | Notes |
|----------|-------------|---------|-------|
| `ZENITH_UPNP_PORT` | Port to map | 54321 | TCP port |
| `ZENITH_UPNP_TIMEOUT` | Operation timeout | 3 | Seconds |
| `ZENITH_UPNP_MAX_RETRIES` | Maximum retry attempts | 3 | Per operation |
| `ZENITH_UPNP_BACKOFF_BASE` | Base backoff delay | 1.0 | Seconds |
| `ZENITH_UPNP_BACKOFF_MAX` | Maximum backoff delay | 30.0 | Seconds |
| `ZENITH_LOG_LEVEL` | Logging verbosity | INFO | DEBUG/INFO/WARNING/ERROR |

### Usage Examples

#### Basic Usage
```python
from backend.networking.port_mapper import UPnPPortMapper

mapper = UPnPPortMapper()
success = mapper.run()

if success:
    print("Port mapping successful")
else:
    print("Running in fallback mode - manual port forwarding needed")
```

#### Custom Configuration
```python
mapper = UPnPPortMapper(
    port=8080,
    timeout=5,
    max_retries=5,
    backoff_base=2.0,
    backoff_max=60.0
)
success = mapper.run()
```

#### As a Module
```bash
# Run with default settings
python -m backend.networking.port_mapper

# With custom environment
ZENITH_UPNP_PORT=8080 ZENITH_LOG_LEVEL=DEBUG python -m backend.networking.port_mapper
```

## Workflow

The port mapping process follows these steps:

1. **Local IP Detection** (`_get_local_ip`)
   - Determines the local IP address for port mapping
   - Falls back to 127.0.0.1 if detection fails

2. **Router Discovery** (`_discover_router`)
   - Sends SSDP M-SEARCH multicast request
   - Waits for UPnP-enabled router response
   - **Retries**: Yes, with exponential backoff

3. **Control URL Retrieval** (`_get_control_url`)
   - Fetches device description XML from router
   - Extracts WANIPConnection control URL
   - **Retries**: Yes, with exponential backoff

4. **Port Mapping** (`_add_port_mapping`)
   - Sends SOAP AddPortMapping request
   - Configures router to forward external port to internal IP
   - **Retries**: Yes, with exponential backoff

## Error Handling

### Exception Hierarchy

```
UPnPError (base exception)
├── RouterDiscoveryError (SSDP discovery failures)
├── ControlURLError (device description retrieval/parsing failures)
├── PortMappingError (SOAP port mapping failures)
└── NetworkError (general network operation failures)
```

### Error Recovery Strategy

1. **Transient Errors**: Automatically retried with exponential backoff
   - Network timeouts
   - Temporary connection failures
   - HTTP 5xx server errors

2. **Permanent Errors**: Trigger fallback mode immediately
   - No router found
   - Missing control URL
   - Port mapping rejected by router

3. **Fallback Mode**: Application continues without UPnP
   - Clear warnings logged
   - Manual port forwarding instructions provided
   - No application crash or exit

## Retry Logic

### Exponential Backoff

The retry mechanism uses exponential backoff to avoid overwhelming routers:

- **Attempt 1**: Immediate (0s delay)
- **Attempt 2**: After 1s delay (backoff_base * 2^0)
- **Attempt 3**: After 2s delay (backoff_base * 2^1)
- **Attempt 4**: After 4s delay (backoff_base * 2^2)
- **Attempt N**: After min(backoff_base * 2^(N-2), backoff_max)

### Configurable Parameters

```python
# Conservative (slower, more patient)
mapper = UPnPPortMapper(
    max_retries=5,
    backoff_base=2.0,
    backoff_max=120.0
)

# Aggressive (faster, less patient)
mapper = UPnPPortMapper(
    max_retries=2,
    backoff_base=0.5,
    backoff_max=10.0
)
```

## Logging

### Log Levels

- **DEBUG**: Detailed operation traces, suitable for troubleshooting
  - SSDP request/response details
  - XML parsing steps
  - Backoff calculations

- **INFO**: Normal operational information
  - Workflow progress (Step 1/4, 2/4, etc.)
  - Successful operations
  - Configuration summary

- **WARNING**: Recoverable issues
  - Retry attempts
  - Fallback mode activation
  - Missing optional features

- **ERROR**: Critical failures
  - Exhausted retries
  - Unexpected exceptions
  - Network errors

### Example Log Output

```
2026-01-03 22:26:22,886 INFO ======================================================================
2026-01-03 22:26:22,886 INFO Starting UPnP port mapper for TCP port 54321
2026-01-03 22:26:22,886 INFO Configuration: timeout=3s, max_retries=3
2026-01-03 22:26:22,886 INFO ======================================================================
2026-01-03 22:26:22,886 INFO Step 1/4: Detecting local IP address...
2026-01-03 22:26:22,886 INFO Local IP address: 192.168.1.100
2026-01-03 22:26:22,886 INFO Step 2/4: Discovering UPnP-enabled router...
2026-01-03 22:26:25,887 INFO Found router at 192.168.1.1
2026-01-03 22:26:25,888 INFO Step 3/4: Retrieving router control URL...
2026-01-03 22:26:26,100 INFO Control URL: http://192.168.1.1:5000/ctl/IPConn
2026-01-03 22:26:26,100 INFO Step 4/4: Adding port mapping...
2026-01-03 22:26:26,250 INFO ======================================================================
2026-01-03 22:26:26,250 INFO SUCCESS: Port 54321 is now mapped via UPnP
2026-01-03 22:26:26,250 INFO External port 54321 -> 192.168.1.100:54321
2026-01-03 22:26:26,250 INFO ======================================================================
```

## Security Considerations

### Implemented Security Measures

1. **Timeouts**: All network operations have configurable timeouts
   - Prevents indefinite blocking
   - Mitigates DoS attempts

2. **Input Validation**: Router responses are parsed safely
   - Regex-based extraction with bounds checking
   - Error handling for malformed responses

3. **Fallback Mode**: Application continues even if UPnP fails
   - No crashes or exits on network failures
   - Graceful degradation

4. **No Sensitive Data in Logs**: 
   - Only operational information logged
   - No credentials or secrets exposed

5. **Limited Retry Attempts**: Prevents infinite loops
   - Configurable maximum retries
   - Exponential backoff prevents rapid retries

### Known Limitations

1. **UPnP Security**: UPnP protocol itself has known security issues
   - Use only on trusted networks
   - Consider firewall rules as primary security

2. **No Authentication**: UPnP operations are unauthenticated
   - Relies on LAN-only multicast
   - Assumes router is trusted

3. **Permanent Mappings**: Default lease duration is 0 (permanent)
   - Mapping persists until router reboot
   - Consider periodic re-mapping for production

## Testing

### Running Tests

```bash
# Run all tests
cd /home/runner/work/daw/daw
python3 -m unittest discover -s backend/networking -p "test_*.py" -v

# Run specific test class
python3 -m unittest backend.networking.test_port_mapper.TestRetryLogic -v

# Run with pytest (if available)
pytest backend/networking/test_port_mapper.py -v
```

### Test Coverage

The test suite covers:
- Configuration and initialization (2 tests)
- Exponential backoff calculation (2 tests)
- Local IP detection (2 tests)
- SSDP response parsing (2 tests)
- Fallback mode activation (2 tests)
- Retry logic (2 tests)
- Custom exception hierarchy (1 test)

**Total: 13 tests, all passing**

### Manual Testing

To test the module manually in a real network environment:

```bash
# Test with verbose logging
ZENITH_LOG_LEVEL=DEBUG python -m backend.networking.port_mapper

# Test with custom port
ZENITH_UPNP_PORT=8080 python -m backend.networking.port_mapper

# Test retry behavior (short timeout to trigger retries)
ZENITH_UPNP_TIMEOUT=1 ZENITH_UPNP_MAX_RETRIES=5 python -m backend.networking.port_mapper
```

## Integration with Orchestrator

The port mapper is managed by the `ServiceSentinel` orchestrator in `backend/orchestrator.py`. It's configured as a service that:

1. Runs as a subprocess
2. Automatically restarts if it exits (with backoff)
3. Reports health status to the health endpoint
4. Can be disabled via `--disable-upnp` flag

### Configuration in run_backend.py

```python
SENTINEL.add_service(
    ServiceDefinition(
        name="upnp",
        command=[sys.executable, "-m", "backend.networking.port_mapper"],
        env={
            "ZENITH_UPNP_PORT": str(args.upnp_port),
            "ZENITH_UPNP_TIMEOUT": str(args.upnp_timeout)
        },
        backoff_base_sec=60.0,  # Retry every minute if it exits
        backoff_max_sec=300.0,
        cwd=os.getcwd(),
    )
)
```

## Troubleshooting

### Common Issues

#### 1. "No router found via SSDP discovery"

**Causes:**
- No UPnP-enabled router on network
- UPnP disabled on router
- Multicast traffic blocked
- Timeout too short

**Solutions:**
- Enable UPnP/IGD on router (check router settings)
- Increase timeout: `ZENITH_UPNP_TIMEOUT=10`
- Check network connectivity
- Verify multicast is not blocked by firewall

#### 2. "Could not find WANIPConnection control URL"

**Causes:**
- Router uses different UPnP service type
- Device description XML parsing failure

**Solutions:**
- Check router's device description manually
- Some routers use WANPPPConnection instead
- May need protocol adaptation for specific routers

#### 3. "Port mapping request failed"

**Causes:**
- Port already mapped
- Router policy blocks the port
- Router's port mapping table is full

**Solutions:**
- Clear existing mappings on router
- Try a different port
- Reboot router to clear mapping table

#### 4. "[Errno 1] Operation not permitted"

**Causes:**
- Insufficient permissions for multicast
- Running in restricted environment (container without network capabilities)

**Solutions:**
- Run with appropriate permissions
- For containers, add `--cap-add=NET_ADMIN` or `--network=host`
- Use fallback mode in restricted environments

### Debug Mode

Enable debug logging to see detailed operation traces:

```bash
ZENITH_LOG_LEVEL=DEBUG python -m backend.networking.port_mapper
```

Debug output includes:
- SSDP request content
- Router response details
- XML parsing steps
- SOAP request/response
- Retry calculations

## Future Enhancements

Potential improvements for future maintainers:

1. **IPv6 Support**: Add support for IPv6 UPnP
2. **Port Refresh**: Periodic re-mapping to maintain active mappings
3. **Multiple Protocols**: Support both TCP and UDP simultaneously
4. **NAT-PMP/PCP**: Fallback to NAT-PMP or PCP if UPnP fails
5. **Mapping Verification**: Verify mapping is active after creation
6. **Router-Specific Adapters**: Support for non-standard router implementations
7. **Metrics**: Export Prometheus metrics for monitoring
8. **Async/Await**: Refactor to use asyncio for better concurrency

## References

- [UPnP IGD Specification](http://upnp.org/specs/gw/UPnP-gw-InternetGatewayDevice-v2-Device.pdf)
- [SSDP Protocol](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol)
- [NAT Port Mapping Protocol](https://tools.ietf.org/html/rfc6886)

## Changelog

### Version 2.0 (2026-01-03)
- Complete refactoring with improved error handling
- Added retry mechanism with exponential backoff
- Implemented fallback mode for graceful degradation
- Comprehensive logging at all levels
- Custom exception hierarchy
- Full documentation and test coverage

### Version 1.0 (Original)
- Basic UPnP port mapping functionality
- Simple error handling
- Basic logging

## Contact

For questions or issues with the port mapper:
1. Check this documentation first
2. Review test cases for usage examples
3. Enable debug logging to diagnose issues
4. Check router compatibility with UPnP IGD protocol
