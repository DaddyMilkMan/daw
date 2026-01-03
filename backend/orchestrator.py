"""
Service orchestration and supervision for Zenith DAW backend.

This module provides a supervision tree pattern for managing backend services
with automatic restart, health checking, and exponential backoff. The 
ServiceSentinel class monitors services and ensures they remain running,
automatically restarting failed processes with configurable retry logic.

Thread Safety: All operations are protected by RLock for concurrent access.
"""

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

class ServiceState(Enum):
    """
    Represents the current state of a managed service.
    
    States:
        STOPPED: Service is not running.
        STARTING: Service is being launched.
        RUNNING: Service is actively running.
        FAILED: Service has failed and is not restarting.
        BACKOFF: Service is waiting to restart after a failure.
    """
    STOPPED = auto()
    STARTING = auto()
    RUNNING = auto()
    FAILED = auto()
    BACKOFF = auto()  # Waiting to restart

@dataclass
class ServiceDefinition:
    """
    Configuration for a managed service.
    
    Attributes:
        name: Human-readable service name.
        command: Command and arguments to execute.
        env: Additional environment variables for the service.
        cwd: Working directory for the service process.
        health_check: Optional callable to check service health.
        max_retries: Maximum restart attempts (-1 for infinite).
        backoff_base_sec: Base delay for exponential backoff.
        backoff_max_sec: Maximum backoff delay in seconds.
    """
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
    """
    Runtime state for a managed service.
    
    Attributes:
        process: The subprocess.Popen object if running.
        state: Current ServiceState.
        restart_count: Number of restarts performed.
        next_restart_time: Unix timestamp for next restart attempt.
        last_exit_code: Exit code from last process termination.
        last_health_status: Result of last health check.
    """
    process: Optional[subprocess.Popen] = None
    state: ServiceState = ServiceState.STOPPED
    restart_count: int = 0
    next_restart_time: float = 0.0
    last_exit_code: Optional[int] = None
    last_health_status: bool = False

class ServiceSentinel:
    """
    Service supervisor implementing a supervision tree pattern.
    
    The Sentinel orchestrates backend services by starting, monitoring, and
    automatically healing them using exponential backoff for retries. It
    provides health checking and graceful shutdown capabilities.
    
    Thread Safety: All public methods are thread-safe via RLock protection.
    
    Attributes:
        services: Registered service definitions.
        state: Runtime state for each service.
        check_interval: Monitoring loop interval in seconds.
    """

    def __init__(self, check_interval: float = 1.0):
        """
        Initialize the ServiceSentinel.
        
        Args:
            check_interval: How often to check service health in seconds (default: 1.0).
        """
        self.services: Dict[str, ServiceDefinition] = {}
        self.state: Dict[str, RuntimeState] = {}
        self.check_interval = check_interval
        self._stop_event = threading.Event()
        self._monitor_thread: Optional[threading.Thread] = None
        self._lock = threading.RLock()

    def add_service(self, service: ServiceDefinition) -> None:
        """
        Register a service to be managed.
        
        Args:
            service: ServiceDefinition describing the service to manage.
        """
        with self._lock:
            self.services[service.name] = service
            self.state[service.name] = RuntimeState()
            log.info(f"Registered service: {service.name}")

    def start_all(self) -> None:
        """
        Start all registered services and begin monitoring.
        
        Spawns each service process and starts the monitoring thread that
        handles health checks and automatic restarts.
        """
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
        """
        Gracefully stop all managed services.
        
        Terminates all running services with SIGTERM, falling back to SIGKILL
        if they don't respond within the timeout period.
        """
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
        """
        Get current status snapshot of all services.
        
        Returns:
            Dict[str, dict]: Dictionary mapping service names to status info
                           including state, PID, restart count, and health.
        """
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
        """
        Internal: Start a service process.
        
        Args:
            name: Name of the service to spawn.
        """
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
        """
        Internal: Stop a single service gracefully.
        
        Args:
            name: Name of the service to stop.
        """
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
        """
        Calculate next restart time using exponential backoff.
        
        Args:
            name: Name of the service to schedule for restart.
        """
        svc = self.services[name]
        rt = self.state[name]
        
        rt.restart_count += 1
        
        # jitter could be added here
        delay = min(svc.backoff_base_sec * (2 ** (rt.restart_count - 1)), svc.backoff_max_sec)
        rt.next_restart_time = time.time() + delay
        rt.state = ServiceState.BACKOFF
        
        log.warning(f"Service {name} entered backoff (count={rt.restart_count}, delay={delay})")

    def _monitor_loop(self) -> None:
        """
        Main supervision loop.
        
        Continuously monitors service health and handles restarts.
        """
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
    """
    Check if a TCP port is accepting connections.
    
    Args:
        host: Hostname or IP address to check.
        port: TCP port number to test.
        timeout: Connection timeout in seconds (default: 1.0).
        
    Returns:
        bool: True if connection succeeds, False otherwise.
    """
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except (OSError, ConnectionRefusedError):
        return False
