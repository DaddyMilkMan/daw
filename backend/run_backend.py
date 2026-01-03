"""CLI entry that wires the signaling server and UPnP port mapper for Zenith DAW."""

import argparse
import json
import signal
import sys
import time
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Optional

from backend.logger import configure_logging, get_logger
from backend.config import ZenithConfig
from backend.orchestrator import ServiceSentinel, ServiceDefinition, check_tcp_port
from backend.networking.port_mapper import DEFAULT_PORT, DEFAULT_TIMEOUT

# Global sentinel instance for health checks
SENTINEL: Optional[ServiceSentinel] = None

class HealthHandler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.path != "/health":
            self.send_error(404)
            return

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        
        status = SENTINEL.get_status() if SENTINEL else {"status": "initializing"}
        self.wfile.write(json.dumps(status).encode("utf-8"))

    def log_message(self, format: str, *args: object) -> None:
        return  # Silence access logs

def start_health_server(port: int) -> ThreadingHTTPServer:
    server = ThreadingHTTPServer(("0.0.0.0", port), HealthHandler)
    # We run the server in a daemon thread so it doesn't block exit
    import threading
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()
    logger = get_logger("zenith.backend")
    logger.info("Health endpoint listening on port", port=port)
    return server

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run Zenith backend services via Sentinel")
    parser.add_argument("--disable-upnp", action="store_true", help="Skip UPnP service")
    parser.add_argument("--upnp-port", type=int, default=DEFAULT_PORT, help="TCP port to map via UPnP")
    parser.add_argument("--upnp-timeout", type=int, default=DEFAULT_TIMEOUT, help="UPnP discovery timeout")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    parser.add_argument("--log-file", help="Path to log file (Note: file logging not yet implemented, logs go to stdout)")
    parser.add_argument("--health-port", type=int, default=8000, help="Health check port")
    parser.add_argument("--log-json", action="store_true", help="Output logs as JSON")
    return parser.parse_args()

def main() -> None:
    global SENTINEL
    args = parse_args()
    
    # Create config and configure logging
    config = ZenithConfig(
        log_level=args.log_level,
        log_json=args.log_json
    )
    configure_logging(config)
    logger = get_logger("zenith.backend")

    logger.info("Initializing Service Sentinel")
    SENTINEL = ServiceSentinel(check_interval=1.0)

    # 1. Define Signaling Service
    # We run it as a module. We need to pass the environment variables it expects.
    signaling_env = {
        "ZENITH_SIGNALING_PORT": str(54320),
        "ZENITH_SIGNALING_UDP_PORT": str(54321),
        "ZENITH_SIGNALING_HOST": "0.0.0.0"
    }
    
    def check_signaling_health() -> bool:
        # Check if the TCP port is accepting connections
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
        # We need a small wrapper or just run the module if it has __main__
        # backend/networking/port_mapper.py has: if __name__ == "__main__": UPnPPortMapper().run()
        # We need to pass ENV vars for it to pick up args, since it reads os.environ in global scope
        # Wait, port_mapper.py reads env vars at module level: DEFAULT_PORT = int(os.environ.get(...))
        upnp_env = {
            "ZENITH_UPNP_PORT": str(args.upnp_port),
            "ZENITH_UPNP_TIMEOUT": str(args.upnp_timeout)
        }
        
        SENTINEL.add_service(
            ServiceDefinition(
                name="upnp",
                command=[sys.executable, "-m", "backend.networking.port_mapper"],
                env=upnp_env,
                # UPnP is a "one-shot" task that might exit? 
                # Actually port_mapper.py's run() does the work and returns. 
                # If it's a one-shot script, Sentinel might restart it forever if it exits cleanly.
                # However, port_mapper.py as currently written just runs and exits.
                # We should probably WRAP it to stay alive or change Sentinel to support one-shots.
                # For now, let's assume we want a persistent service that keeps the mapping alive (periodic re-map).
                # But the current code just runs once.
                # Use a wrapper command that sleeps? 
                # Better: Let's explicitly mark it as "don't restart if exit 0" or similar?
                # The Sentinel implementation assumes services should be RUNNING.
                # Backoff logic restarts it.
                # Let's adjust UPnP to be a periodic check service or just a one-off.
                # The user request implies "keep synchronized".
                # For this iteration, let's run it. If it exits, it will be restarted. 
                # That's actually verify good for UPnP to re-assert mapping periodically!
                # We'll set a higher backoff for it maybe?
                backoff_base_sec=60.0, # Retry every minute if it exits
                backoff_max_sec=300.0,
                cwd=os.getcwd(),
            )
        )

    # Start Health Server
    if args.health_port > 0:
        start_health_server(args.health_port)

    # Install Signal Handlers
    def handle_stop(signum, frame):
        logger.info("Received signal, stopping", signal_number=signum)
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
            # We could print status periodically to log?
            pass
    except KeyboardInterrupt:
        handle_stop(signal.SIGINT, None)

if __name__ == "__main__":
    main()
