"""CLI entry that wires the signaling server and UPnP port mapper for Zenith DAW.

This module provides a production-ready entry point for running Zenith backend services
with proper error handling, monitoring, and graceful shutdown capabilities.
"""

import argparse
import json
import logging
import signal
import sys
import time
import os
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from logging.handlers import RotatingFileHandler
from typing import Optional

from backend.orchestrator import ServiceSentinel, ServiceDefinition, check_tcp_port
from backend.networking.port_mapper import DEFAULT_PORT, DEFAULT_TIMEOUT


class HealthHandler(BaseHTTPRequestHandler):
    """HTTP handler for health check endpoint.
    
    Provides JSON status of all managed services via GET /health.
    Thread-safe access to sentinel status through class attribute.
    """
    
    sentinel: Optional[ServiceSentinel] = None
    
    def do_GET(self) -> None:
        """Handle GET requests to /health endpoint."""
        if self.path != "/health":
            self.send_error(404, "Not Found")
            return

        try:
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.end_headers()
            
            status = self.sentinel.get_status() if self.sentinel else {"status": "initializing"}
            self.wfile.write(json.dumps(status).encode("utf-8"))
        except Exception as e:
            # Log the error but don't try to send error response as headers may be partially sent
            logging.getLogger("zenith.backend").error("Health check failed: %s", e, exc_info=True)

    def log_message(self, format: str, *args: object) -> None:
        """Suppress default HTTP access logs."""
        pass

def start_health_server(port: int, bind_address: str = "127.0.0.1") -> ThreadingHTTPServer:
    """Start the health check HTTP server.
    
    Args:
        port: Port to bind the health endpoint to
        bind_address: Network interface to bind to (default: localhost only)
        
    Returns:
        ThreadingHTTPServer instance
        
    Raises:
        OSError: If port is already in use or binding fails
    """
    logger = logging.getLogger("zenith.backend")
    
    try:
        server = ThreadingHTTPServer((bind_address, port), HealthHandler)
        server.daemon_threads = True  # Ensure threads exit with main thread
        
        # Start server in daemon thread
        thread = threading.Thread(
            target=server.serve_forever, 
            daemon=True,
            name="HealthServer"
        )
        thread.start()
        
        logger.info("Health endpoint listening on %s:%d", bind_address, port)
        return server
    except OSError as e:
        logger.error("Failed to start health server on %s:%d: %s", bind_address, port, e)
        raise

def parse_args() -> argparse.Namespace:
    """Parse command line arguments.
    
    Returns:
        Parsed arguments namespace
    """
    parser = argparse.ArgumentParser(description="Run Zenith backend services via Sentinel")
    parser.add_argument("--disable-upnp", action="store_true", help="Skip UPnP service")
    parser.add_argument("--upnp-port", type=int, default=DEFAULT_PORT, help="TCP port to map via UPnP")
    parser.add_argument("--upnp-timeout", type=int, default=DEFAULT_TIMEOUT, help="UPnP discovery timeout")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    parser.add_argument("--log-file", help="Path to log file")
    parser.add_argument("--health-port", type=int, default=8000, help="Health check port")
    parser.add_argument("--health-bind", default="127.0.0.1", help="Health check bind address (default: 127.0.0.1)")
    return parser.parse_args()

def setup_logging(level_str: str, log_file: Optional[str]) -> None:
    """Configure logging with rotation and structured output.
    
    Args:
        level_str: Logging level (DEBUG, INFO, WARNING, ERROR)
        log_file: Optional path to log file with rotation support
    """
    level = getattr(logging, level_str.upper())
    handlers = [logging.StreamHandler(sys.stdout)]
    
    if log_file:
        try:
            handlers.append(RotatingFileHandler(
                log_file, 
                maxBytes=5_000_000, 
                backupCount=3
            ))
        except (OSError, PermissionError) as e:
            # Log to stderr before logging is fully configured
            print(f"Warning: Could not create log file {log_file}: {e}", file=sys.stderr)
    
    logging.basicConfig(
        level=level,
        format="%(asctime)s [%(name)s] %(levelname)s: %(message)s",
        handlers=handlers
    )

def main() -> None:
    """Main entry point for Zenith backend services.
    
    Initializes and manages the following services:
    - Signaling server for peer-to-peer connections
    - UPnP port mapper (optional)
    - Health check HTTP endpoint
    
    The function:
    1. Parses command line arguments
    2. Sets up logging infrastructure
    3. Initializes the ServiceSentinel orchestrator
    4. Registers and starts services
    5. Handles graceful shutdown on SIGINT/SIGTERM
    
    Exits with code 1 on fatal errors, 0 on graceful shutdown.
    """
    args = parse_args()
    
    # Setup logging first so all subsequent operations are logged
    try:
        setup_logging(args.log_level, args.log_file)
    except Exception as e:
        print(f"Fatal: Failed to setup logging: {e}", file=sys.stderr)
        sys.exit(1)
    
    logger = logging.getLogger("zenith.backend")
    logger.info("Starting Zenith Backend Services...")
    
    # Initialize sentinel
    sentinel: Optional[ServiceSentinel] = None
    health_server: Optional[ThreadingHTTPServer] = None
    
    try:
        logger.info("Initializing Service Sentinel...")
        sentinel = ServiceSentinel(check_interval=1.0)
        
        # Make sentinel available to health handler via class attribute (thread-safe)
        HealthHandler.sentinel = sentinel

        # 1. Define Signaling Service
        signaling_env = {
            "ZENITH_SIGNALING_PORT": str(54320),
            "ZENITH_SIGNALING_UDP_PORT": str(54321),
            "ZENITH_SIGNALING_HOST": "0.0.0.0"
        }
        
        def check_signaling_health() -> bool:
            """Check if signaling TCP port is accepting connections."""
            return check_tcp_port("127.0.0.1", 54320)

        sentinel.add_service(
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
            logger.info("UPnP service enabled")
            upnp_env = {
                "ZENITH_UPNP_PORT": str(args.upnp_port),
                "ZENITH_UPNP_TIMEOUT": str(args.upnp_timeout)
            }
            
            sentinel.add_service(
                ServiceDefinition(
                    name="upnp",
                    command=[sys.executable, "-m", "backend.networking.port_mapper"],
                    env=upnp_env,
                    backoff_base_sec=60.0,  # Retry every minute if it exits
                    backoff_max_sec=300.0,
                    cwd=os.getcwd(),
                )
            )
        else:
            logger.info("UPnP service disabled")

        # 3. Start Health Server
        if args.health_port > 0:
            try:
                health_server = start_health_server(args.health_port, args.health_bind)
            except OSError as e:
                logger.error("Failed to start health server: %s", e)
                raise

        # 4. Install Signal Handlers for graceful shutdown
        shutdown_initiated = threading.Event()
        
        def handle_shutdown(signum: int, frame) -> None:
            """Handle shutdown signals gracefully.
            
            Args:
                signum: Signal number received
                frame: Current stack frame (unused)
            """
            if shutdown_initiated.is_set():
                logger.warning("Shutdown already in progress, signal %d ignored", signum)
                return
                
            shutdown_initiated.set()
            logger.info("Received signal %d, initiating graceful shutdown...", signum)
            
            try:
                if sentinel:
                    sentinel.stop_all()
                if health_server:
                    health_server.shutdown()
                logger.info("Shutdown complete")
            except Exception as e:
                logger.error("Error during shutdown: %s", e, exc_info=True)
            finally:
                sys.exit(0)

        signal.signal(signal.SIGINT, handle_shutdown)
        signal.signal(signal.SIGTERM, handle_shutdown)
        
        logger.info("Signal handlers installed")

        # 5. Start Services
        logger.info("Starting all managed services...")
        sentinel.start_all()
        logger.info("All services started, entering monitoring loop")

        # 6. Block main thread - monitor loop runs in background
        try:
            while not shutdown_initiated.is_set():
                shutdown_initiated.wait(timeout=1.0)
        except KeyboardInterrupt:
            # Handle Ctrl+C directly if signal handler doesn't catch it
            handle_shutdown(signal.SIGINT, None)
            
    except Exception as e:
        logger.error("Fatal error in main: %s", e, exc_info=True)
        
        # Attempt cleanup on error
        try:
            if sentinel:
                logger.info("Attempting emergency shutdown...")
                sentinel.stop_all()
            if health_server:
                health_server.shutdown()
        except Exception as cleanup_error:
            logger.error("Error during emergency cleanup: %s", cleanup_error, exc_info=True)
        
        sys.exit(1)

if __name__ == "__main__":
    main()
