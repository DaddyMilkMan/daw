
import logging
import os
import signal
import socket
import subprocess
import threading
import time
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, Dict, List, Optional

# Replaced structlog with standard logging
log = logging.getLogger("zenith.orchestrator")

# Constants for process management
GRACEFUL_SHUTDOWN_TIMEOUT_SEC = 5
FORCE_KILL_TIMEOUT_SEC = 2

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

            try:
                if rt.process.poll() is None:
                    log.info(f"Terminating service {name} (pid={rt.process.pid})")
                    rt.process.terminate()
                    try:
                        rt.process.wait(timeout=GRACEFUL_SHUTDOWN_TIMEOUT_SEC)
                        log.info(f"Service {name} terminated gracefully")
                    except subprocess.TimeoutExpired:
                        log.warning(f"Service {name} did not terminate, killing")
                        rt.process.kill()
                        rt.process.wait(timeout=FORCE_KILL_TIMEOUT_SEC)  # Wait for kill to complete
            except Exception as e:
                log.error(f"Error stopping service {name}: {e}", exc_info=True)
            finally:
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
        log.info("Monitor loop started")
        try:
            while not self._stop_event.is_set():
                try:
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
                                            except Exception as e:
                                                log.warning(f"Health check failed for {name}: {e}")
                                                rt.last_health_status = False

                            # 2. Check backoff restarts
                            elif rt.state == ServiceState.BACKOFF:
                                if now >= rt.next_restart_time:
                                    log.info(f"Backoff expired, restarting {name}")
                                    self._spawn_service(name)
                except Exception as e:
                    log.error(f"Error in monitor loop iteration: {e}", exc_info=True)

                time.sleep(self.check_interval)
        except Exception as e:
            log.error(f"Fatal error in monitor loop: {e}", exc_info=True)
        finally:
            log.info("Monitor loop exited")

# --- Helper Health Checks ---

def check_tcp_port(host: str, port: int, timeout: float = 1.0) -> bool:
    """True if efficient TCP connect succeeds."""
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except (OSError, ConnectionRefusedError):
        return False
