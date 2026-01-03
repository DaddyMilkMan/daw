
import logging
import os
import signal
import socket
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, Dict, List, Optional

# Replaced structlog with standard logging
log = logging.getLogger("zenith.orchestrator")

class ServiceState(Enum):
    STOPPED = auto()
    STARTING = auto()
    RUNNING = auto()
    FAILED = auto()
    BACKOFF = auto()  # Waiting to restart

@dataclass
class ServiceDefinition:
    name: str
    command: List[str]
    env: Dict[str, str] = field(default_factory=dict)
    cwd: Optional[str] = None
    health_check: Optional[Callable[[], bool]] = None
    
    # Restarts
    max_retries: int = -1  # -1 = infinite
    backoff_base_sec: float = 1.0
    backoff_max_sec: float = 30.0

@dataclass
class RuntimeState:
    process: Optional[subprocess.Popen] = None
    state: ServiceState = ServiceState.STOPPED
    restart_count: int = 0
    next_restart_time: float = 0.0
    last_exit_code: Optional[int] = None
    last_health_status: bool = False

class ServiceSentinel:
    """
    The Sentinel responsible for orchestrating backend services.
    It starts, monitors, and auto_heals services using a supervision tree pattern.
    """

    def __init__(self, check_interval: float = 1.0):
        self.services: Dict[str, ServiceDefinition] = {}
        self.state: Dict[str, RuntimeState] = {}
        self.check_interval = check_interval
        self._stop_event = threading.Event()
        self._monitor_thread: Optional[threading.Thread] = None
        self._lock = threading.RLock()

    def add_service(self, service: ServiceDefinition) -> None:
        """Register a service to be managed."""
        with self._lock:
            self.services[service.name] = service
            self.state[service.name] = RuntimeState()
            log.info(f"Registered service: {service.name}")

    def start_all(self) -> None:
        """Start all registered services and the monitoring loop."""
        with self._lock:
            if self._monitor_thread and self._monitor_thread.is_alive():
                log.warning("Sentinel already running")
                return
                
            self._stop_event.clear()
            
            for name in self.services:
                self._spawn_service(name)

            self._monitor_thread = threading.Thread(target=self._monitor_loop, daemon=True, name="SentinelMonitor")
            self._monitor_thread.start()
            log.info("Sentinel started monitoring")

    def stop_all(self) -> None:
        """Gracefully stop all services."""
        log.info("Stopping all services...")
        self._stop_event.set()
        
        # Determine services to stop
        with self._lock:
            services_to_stop = list(self.services.keys())

        # Parallelize stops if needed, but sequential is safer for cleanup order
        for name in services_to_stop:
            self._stop_service(name)
            
        if self._monitor_thread:
            self._monitor_thread.join(timeout=2.0)
        
        log.info("All managed services stopped")

    def get_status(self) -> Dict[str, dict]:
        """Return a snapshot of system health."""
        status = {}
        with self._lock:
            for name, rt in self.state.items():
                status[name] = {
                    "state": rt.state.name,
                    "pid": rt.process.pid if rt.process else None,
                    "restarts": rt.restart_count,
                    "healthy": rt.last_health_status
                }
        return status

    def _spawn_service(self, name: str) -> None:
        """Internal: Start a service process."""
        svc = self.services[name]
        rt = self.state[name]
        
        # Merge environment
        final_env = os.environ.copy()
        final_env.update(svc.env)
        # Force unbuffered output for Python services so logs appear immediately
        final_env["PYTHONUNBUFFERED"] = "1"

        try:
            log.info(f"Spawning service: {name} cmd={svc.command}")
            rt.process = subprocess.Popen(
                svc.command,
                env=final_env,
                cwd=svc.cwd,
                # We let stdout/stderr inherit so they go to the main log for now.
                # In the future we could pipe them to specific loggers.
            )
            rt.state = ServiceState.RUNNING
            rt.last_health_status = True  # Assume healthy on start until check fails
        except Exception as e:
            log.error(f"Failed to spawn service {name}: {e}")
            rt.state = ServiceState.FAILED
            rt.last_exit_code = -1
            self._schedule_backoff(name)

    def _stop_service(self, name: str) -> None:
        """Internal: Stop a single service."""
        with self._lock:
            rt = self.state.get(name)
            if not rt or not rt.process:
                return

            if rt.process.poll() is None:
                log.info(f"Terminating service {name} (pid={rt.process.pid})")
                rt.process.terminate()
                try:
                    rt.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    log.warning(f"Service {name} did not terminate, killing")
                    rt.process.kill()
            
            rt.process = None
            rt.state = ServiceState.STOPPED

    def _schedule_backoff(self, name: str) -> None:
        """Calculate next restart time based on exponential backoff."""
        svc = self.services[name]
        rt = self.state[name]
        
        rt.restart_count += 1
        
        # jitter could be added here
        delay = min(svc.backoff_base_sec * (2 ** (rt.restart_count - 1)), svc.backoff_max_sec)
        rt.next_restart_time = time.time() + delay
        rt.state = ServiceState.BACKOFF
        
        log.warning(f"Service {name} entered backoff (count={rt.restart_count}, delay={delay})")

    def _monitor_loop(self) -> None:
        """Main supervision loop."""
        while not self._stop_event.is_set():
            with self._lock:
                now = time.time()
                for name, svc in self.services.items():
                    rt = self.state[name]

                    # 1. Check running processes
                    if rt.state == ServiceState.RUNNING:
                        if rt.process:
                            exit_code = rt.process.poll()
                            if exit_code is not None:
                                log.error(f"Service {name} crashed (exit_code={exit_code})")
                                rt.last_exit_code = exit_code
                                rt.process = None
                                self._schedule_backoff(name)
                            else:
                                # Health Check
                                if svc.health_check:
                                    try:
                                        is_healthy = svc.health_check()
                                        if is_healthy != rt.last_health_status:
                                            log.info(f"Health status changed for {name}: {is_healthy}")
                                        rt.last_health_status = is_healthy
                                    except Exception:
                                        rt.last_health_status = False

                    # 2. Check backoff restarts
                    elif rt.state == ServiceState.BACKOFF:
                        if now >= rt.next_restart_time:
                            log.info(f"Backoff expired, restarting {name}")
                            self._spawn_service(name)

            time.sleep(self.check_interval)

# --- Helper Health Checks ---

def check_tcp_port(host: str, port: int, timeout: float = 1.0) -> bool:
    """True if efficient TCP connect succeeds."""
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except (OSError, ConnectionRefusedError):
        return False


class ServiceManager:
    """
    High-level service manager for Zenith DAW backend services.
    
    This class provides a simplified interface to manage backend services
    including signaling server and UPnP port mapping. It wraps the ServiceSentinel
    for process management and health monitoring.
    
    Thread-safe for concurrent operations through internal locking.
    """
    
    def __init__(self, config):
        """
        Initialize the ServiceManager with the given configuration.
        
        Args:
            config: ZenithConfig instance containing service configuration
        """
        from backend.config import ZenithConfig
        if not isinstance(config, ZenithConfig):
            raise TypeError(f"Expected ZenithConfig, got {type(config)}")
            
        self.config = config
        self.sentinel = ServiceSentinel(check_interval=1.0)
        self._setup_services()
        
    def _setup_services(self) -> None:
        """Configure services based on the configuration."""
        # Setup signaling service
        signaling_env = {
            "ZENITH_SIGNALING_HOST": self.config.signaling_host,
            "ZENITH_SIGNALING_PORT": str(self.config.signaling_port),
            "ZENITH_LOG_LEVEL": self.config.log_level,
        }
        
        def check_signaling_health() -> bool:
            return check_tcp_port("127.0.0.1", self.config.signaling_port)
        
        self.sentinel.add_service(
            ServiceDefinition(
                name="signaling",
                command=[sys.executable, "-m", "backend.signaling.signaling_server"],
                env=signaling_env,
                health_check=check_signaling_health,
                cwd=os.getcwd(),
            )
        )
        
        # Setup UPnP service if enabled
        if self.config.upnp_enabled:
            upnp_env = {
                "ZENITH_UPNP_PORT": str(self.config.upnp_port),
                "ZENITH_UPNP_TIMEOUT": str(self.config.upnp_timeout),
            }
            
            self.sentinel.add_service(
                ServiceDefinition(
                    name="upnp",
                    command=[sys.executable, "-m", "backend.networking.port_mapper"],
                    env=upnp_env,
                    backoff_base_sec=60.0,
                    backoff_max_sec=300.0,
                    cwd=os.getcwd(),
                )
            )
    
    def start_services(self, dry_run: bool = False) -> None:
        """
        Start all configured services.
        
        Args:
            dry_run: If True, validate configuration but don't actually start services
            
        Raises:
            RuntimeError: If services fail to start
        """
        if dry_run:
            log.info("Dry run: would start services", 
                    services=list(self.sentinel.services.keys()))
            return
            
        try:
            self.sentinel.start_all()
            log.info("All services started successfully")
        except Exception as e:
            log.error("Failed to start services", error=str(e))
            raise RuntimeError(f"Service startup failed: {e}") from e
    
    def stop_services(self) -> None:
        """Stop all running services gracefully."""
        try:
            self.sentinel.stop_all()
            log.info("All services stopped")
        except Exception as e:
            log.error("Error stopping services", error=str(e))
            raise RuntimeError(f"Service shutdown failed: {e}") from e
    
    def get_status(self) -> Dict[str, dict]:
        """
        Get the current status of all services.
        
        Returns:
            Dictionary mapping service names to their status information
        """
        return self.sentinel.get_status()
    
    def wait_forever(self) -> None:
        """Block until interrupted, keeping services running."""
        import signal as sig
        
        def handle_shutdown(signum, frame):
            log.info("Shutdown signal received", signal=signum)
            self.stop_services()
            sys.exit(0)
        
        sig.signal(sig.SIGINT, handle_shutdown)
        sig.signal(sig.SIGTERM, handle_shutdown)
        
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            handle_shutdown(sig.SIGINT, None)
