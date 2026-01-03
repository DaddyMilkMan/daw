"""
CLI entry point for Zenith DAW backend services.

This module wires the signaling server and UPnP port mapper together using
the ServiceSentinel orchestration system. It provides:
- Health check HTTP endpoint for monitoring
- Command-line configuration options
- Signal handling for graceful shutdown
- Structured logging

Usage:
    python -m backend.run_backend [options]
    
See --help for available options.
"""

import argparse
import json
import logging
import signal
import sys
import time
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from logging.handlers import RotatingFileHandler
from typing import Optional

from backend.orchestrator import ServiceSentinel, ServiceDefinition, check_tcp_port
from backend.networking.port_mapper import DEFAULT_PORT, DEFAULT_TIMEOUT

# Global sentinel instance for health checks
SENTINEL: Optional[ServiceSentinel] = None

class HealthHandler(BaseHTTPRequestHandler):
    """
    HTTP handler for health check endpoint.
    
    Responds to GET /health with JSON status of all managed services.
    """
    
    def do_GET(self) -> None:
        """Handle GET requests to /health endpoint."""
        if self.path != "/health":
            self.send_error(404)
            return

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        
        status = SENTINEL.get_status() if SENTINEL else {"status": "initializing"}
        self.wfile.write(json.dumps(status).encode("utf-8"))

    def log_message(self, format: str, *args: object) -> None:
        """Suppress HTTP access logs."""
        return

def start_health_server(port: int) -> ThreadingHTTPServer:
    """
    Start the health check HTTP server.
    
    Args:
        port: Port number for the health endpoint.
        
    Returns:
        ThreadingHTTPServer: The running server instance.
    """
    server = ThreadingHTTPServer(("0.0.0.0", port), HealthHandler)
    # We run the server in a daemon thread so it doesn't block exit
    import threading
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()
    logging.getLogger("zenith.backend").info("Health endpoint listening on %d", port)
    return server

def parse_args() -> argparse.Namespace:
    """
    Parse command-line arguments.
    
    Returns:
        argparse.Namespace: Parsed arguments with default values.
    """
    parser = argparse.ArgumentParser(description="Run Zenith backend services via Sentinel")
    parser.add_argument("--disable-upnp", action="store_true", help="Skip UPnP service")
    parser.add_argument("--upnp-port", type=int, default=DEFAULT_PORT, help="TCP port to map via UPnP")
    parser.add_argument("--upnp-timeout", type=int, default=DEFAULT_TIMEOUT, help="UPnP discovery timeout")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    parser.add_argument("--log-file", help="Path to log file")
    parser.add_argument("--health-port", type=int, default=8000, help="Health check port")
    return parser.parse_args()

def setup_logging(level_str: str, log_file: Optional[str]) -> None:
    """
    Configure logging for the backend.
    
    Args:
        level_str: Log level as string (DEBUG, INFO, WARNING, ERROR).
        log_file: Optional path to log file for rotating file logging.
    """
    level = getattr(logging, level_str.upper())
    handlers = [logging.StreamHandler(sys.stdout)]
    if log_file:
        handlers.append(RotatingFileHandler(log_file, maxBytes=5_000_000, backupCount=3))
    
    logging.basicConfig(
        level=level,
        format="%(asctime)s [%(name)s] %(levelname)s: %(message)s",
        handlers=handlers
    )

def main() -> None:
    """
    Main entry point for the backend services.
    
    Initializes the ServiceSentinel, configures signaling and optional UPnP
    services, starts the health endpoint, and blocks until interrupted.
    """
    global SENTINEL
    args = parse_args()
    setup_logging(args.log_level, args.log_file)
    logger = logging.getLogger("zenith.backend")

    logger.info("Initializing Service Sentinel...")
    SENTINEL = ServiceSentinel(check_interval=1.0)

    # 1. Define Signaling Service
    signaling_env = {
        "ZENITH_SIGNALING_PORT": str(54320),
        "ZENITH_SIGNALING_UDP_PORT": str(54321),
        "ZENITH_SIGNALING_HOST": "0.0.0.0"
    }
    
    def check_signaling_health() -> bool:
        """Health check for signaling service via TCP port."""
        return check_tcp_port("127.0.0.1", 54320)

    SENTINEL.add_service(
        ServiceDefinition(
            name="signaling",
            command=[sys.executable, "-m", "backend.signaling.signaling_server"],
            env=signaling_env,
            health_check=check_signaling_health,
            cwd=os.getcwd(),
        )
    )

    # 2. Define UPnP Service (if enabled)
    if not args.disable_upnp:
        upnp_env = {
            "ZENITH_UPNP_PORT": str(args.upnp_port),
            "ZENITH_UPNP_TIMEOUT": str(args.upnp_timeout)
        }
        
        SENTINEL.add_service(
            ServiceDefinition(
                name="upnp",
                command=[sys.executable, "-m", "backend.networking.port_mapper"],
                env=upnp_env,
                # UPnP runs once and exits, so we use longer backoff for periodic re-mapping
                backoff_base_sec=60.0,
                backoff_max_sec=300.0,
                cwd=os.getcwd(),
            )
        )

    # Start Health Server
    if args.health_port > 0:
        start_health_server(args.health_port)

    # Install Signal Handlers for graceful shutdown
    def handle_stop(signum: int, frame: object) -> None:
        """Handle shutdown signals gracefully."""
        logger.info("Received signal %d, stopping...", signum)
        SENTINEL.stop_all()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_stop)
    signal.signal(signal.SIGTERM, handle_stop)

    # Start Services
    SENTINEL.start_all()

    # Block main thread
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        handle_stop(signal.SIGINT, None)

if __name__ == "__main__":
    main()
