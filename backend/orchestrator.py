"""
Service Orchestrator for Zenith DAW Backend.

This module provides a robust service supervision system for managing backend services
with automatic restart, health checking, and exponential backoff. It uses a supervisor
pattern to monitor and heal services that crash or become unhealthy.

Key Features:
    - Automatic service restart with exponential backoff
    - Health check monitoring
    - Thread-safe service state management
    - Graceful shutdown handling
    - Configurable retry limits and backoff strategies
"""

import logging
import os
import random
import socket
import subprocess
import threading
import time
from contextlib import contextmanager
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, Dict, List, Optional

# Use standard logging for consistent log handling
log = logging.getLogger("zenith.orchestrator")

class ServiceState(Enum):
    """Represents the current state of a managed service."""
    STOPPED = auto()      # Service is not running
    STARTING = auto()     # Service is being started
    RUNNING = auto()      # Service is running normally
    FAILED = auto()       # Service has failed permanently (max retries exceeded)
    BACKOFF = auto()      # Waiting to restart after a failure

@dataclass
class ServiceDefinition:
    """
    Configuration for a managed service.
    
    Attributes:
        name: Unique identifier for the service
        command: Command and arguments to execute
        env: Additional environment variables for the service
        cwd: Working directory for the service (None = inherit)
        health_check: Optional callable that returns True if service is healthy
        max_retries: Maximum restart attempts (-1 = infinite)
        backoff_base_sec: Base delay for exponential backoff (default 1.0s)
        backoff_max_sec: Maximum backoff delay (default 30.0s)
        backoff_jitter: Add random jitter to backoff (0.0-1.0, default 0.1)
    """
    name: str
    command: List[str]
    env: Dict[str, str] = field(default_factory=dict)
    cwd: Optional[str] = None
    health_check: Optional[Callable[[], bool]] = None
    
    # Restart configuration
    max_retries: int = -1  # -1 = infinite
    backoff_base_sec: float = 1.0
    backoff_max_sec: float = 30.0
    backoff_jitter: float = 0.1  # 10% jitter by default

@dataclass
class RuntimeState:
    """
    Runtime state tracking for a managed service.
    
    This class tracks the current execution state, process handle,
    restart attempts, and health status of a service.
    
    Attributes:
        process: The subprocess.Popen instance (None if not running)
        state: Current ServiceState
        restart_count: Number of restart attempts
        next_restart_time: Unix timestamp when next restart should occur
        last_exit_code: Exit code from last process termination
        last_health_status: Result of most recent health check
    """
    process: Optional[subprocess.Popen] = None
    state: ServiceState = ServiceState.STOPPED
    restart_count: int = 0
    next_restart_time: float = 0.0
    last_exit_code: Optional[int] = None
    last_health_status: bool = False

class ServiceSentinel:
    """
    Robust service supervisor implementing the supervision tree pattern.
    
    The Sentinel orchestrates backend services by starting them, monitoring
    their health, and automatically restarting them on failure. It provides
    thread-safe operations and configurable restart strategies with exponential
    backoff.
    
    Key Responsibilities:
        - Start and stop services
        - Monitor service health via health checks
        - Automatically restart crashed services with backoff
        - Enforce max retry limits
        - Provide service status reporting
    
    Thread Safety:
        All public methods are thread-safe. Internal state is protected by
        a reentrant lock to prevent race conditions.
    
    Example:
        >>> sentinel = ServiceSentinel(check_interval=1.0)
        >>> service = ServiceDefinition(
        ...     name="my_service",
        ...     command=["python", "server.py"],
        ...     health_check=lambda: check_tcp_port("localhost", 8000)
        ... )
        >>> sentinel.add_service(service)
        >>> sentinel.start_all()
        >>> # ... later ...
        >>> sentinel.stop_all()
    """

    def __init__(self, check_interval: float = 1.0):
        """
        Initialize the ServiceSentinel.
        
        Args:
            check_interval: Seconds between monitor loop iterations (default 1.0)
        """
        self.services: Dict[str, ServiceDefinition] = {}
        self.state: Dict[str, RuntimeState] = {}
        self.check_interval = check_interval
        self._stop_event = threading.Event()
        self._monitor_thread: Optional[threading.Thread] = None
        self._lock = threading.RLock()

    @contextmanager
    def _acquire_lock(self):
        """Context manager for acquiring the state lock."""
        self._lock.acquire()
        try:
            yield
        finally:
            self._lock.release()

    def add_service(self, service: ServiceDefinition) -> None:
        """
        Register a service to be managed by the Sentinel.
        
        Services must be added before calling start_all(). Adding a service
        after starting will not automatically start it.
        
        Args:
            service: ServiceDefinition to register
            
        Raises:
            ValueError: If a service with the same name already exists
        """
        with self._acquire_lock():
            if service.name in self.services:
                raise ValueError(f"Service '{service.name}' is already registered")
            
            self.services[service.name] = service
            self.state[service.name] = RuntimeState()
            log.info(f"Registered service: {service.name}")

    def start_all(self) -> None:
        """
        Start all registered services and begin monitoring.
        
        This starts the monitor thread which continuously checks service health
        and restarts failed services. Services are started sequentially to avoid
        overwhelming system resources.
        
        This method is idempotent - calling it multiple times has no effect if
        already running.
        
        Raises:
            RuntimeError: If no services are registered
        """
        with self._acquire_lock():
            if not self.services:
                raise RuntimeError("No services registered")
            
            if self._monitor_thread and self._monitor_thread.is_alive():
                log.warning("Sentinel already running")
                return
                
            self._stop_event.clear()
            
            # Start all services
            for name in self.services:
                self._spawn_service(name)

            # Start monitor thread
            self._monitor_thread = threading.Thread(
                target=self._monitor_loop, 
                daemon=True, 
                name="SentinelMonitor"
            )
            self._monitor_thread.start()
            log.info("Sentinel started monitoring")

    def stop_all(self, timeout: float = 10.0) -> None:
        """
        Gracefully stop all services and the monitor thread.
        
        Services are stopped sequentially to ensure proper cleanup order.
        The monitor thread is signaled to stop and joined with a timeout.
        
        Args:
            timeout: Maximum time in seconds to wait for monitor thread to stop
            
        Note:
            Individual services have their own termination timeout (5 seconds),
            after which they are forcefully killed.
        """
        log.info("Stopping all services...")
        self._stop_event.set()
        
        # Get list of services to stop (avoid holding lock during stop operations)
        with self._acquire_lock():
            services_to_stop = list(self.services.keys())

        # Stop each service sequentially
        for name in services_to_stop:
            self._stop_service(name)
        
        # Wait for monitor thread to finish
        if self._monitor_thread and self._monitor_thread.is_alive():
            self._monitor_thread.join(timeout=timeout)
            if self._monitor_thread.is_alive():
                log.warning("Monitor thread did not stop within timeout")
        
        log.info("All managed services stopped")

    def get_status(self) -> Dict[str, dict]:
        """
        Return a snapshot of all service states.
        
        Returns:
            Dictionary mapping service names to status dictionaries containing:
                - state: Current ServiceState as string
                - pid: Process ID (None if not running)
                - restarts: Number of restart attempts
                - healthy: Most recent health check result
                - exit_code: Last exit code (None if never exited)
        
        Thread Safety:
            This method acquires the lock to ensure a consistent snapshot.
        """
        status = {}
        with self._acquire_lock():
            for name, rt in self.state.items():
                status[name] = {
                    "state": rt.state.name,
                    "pid": rt.process.pid if rt.process else None,
                    "restarts": rt.restart_count,
                    "healthy": rt.last_health_status,
                    "exit_code": rt.last_exit_code
                }
        return status

    def _spawn_service(self, name: str) -> None:
        """
        Internal: Start a service process.
        
        Creates a subprocess for the service with the configured environment
        and working directory. Sets the service state to RUNNING on success,
        or schedules a backoff retry on failure.
        
        Args:
            name: Name of the service to spawn
            
        Note:
            Caller must hold the lock when calling this method.
        """
        svc = self.services[name]
        rt = self.state[name]
        
        # Check if we've exceeded max retries
        if svc.max_retries >= 0 and rt.restart_count >= svc.max_retries:
            log.error(
                f"Service {name} has exceeded max retries "
                f"({rt.restart_count}/{svc.max_retries}), marking as FAILED"
            )
            rt.state = ServiceState.FAILED
            return
        
        # Merge environment variables
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
                # Let stdout/stderr inherit to appear in main log
                # Future: Could pipe to service-specific loggers
            )
            rt.state = ServiceState.RUNNING
            rt.last_health_status = True  # Assume healthy on start
            log.info(f"Service {name} started with PID {rt.process.pid}")
        except FileNotFoundError as e:
            log.error(f"Service {name} command not found: {e}")
            rt.state = ServiceState.FAILED
            rt.last_exit_code = -1
            self._schedule_backoff(name)
        except PermissionError as e:
            log.error(f"Service {name} permission denied: {e}")
            rt.state = ServiceState.FAILED
            rt.last_exit_code = -1
            self._schedule_backoff(name)
        except Exception as e:
            log.error(f"Failed to spawn service {name}: {e}", exc_info=True)
            rt.state = ServiceState.FAILED
            rt.last_exit_code = -1
            self._schedule_backoff(name)

    def _stop_service(self, name: str) -> None:
        """
        Internal: Stop a single service gracefully.
        
        Attempts to terminate the process gracefully, waiting up to 5 seconds.
        If the process doesn't terminate, it is forcefully killed.
        
        Args:
            name: Name of the service to stop
            
        Thread Safety:
            This method acquires the lock internally.
        """
        with self._acquire_lock():
            rt = self.state.get(name)
            if not rt or not rt.process:
                return

            # Check if process is still running
            if rt.process.poll() is None:
                log.info(f"Terminating service {name} (pid={rt.process.pid})")
                try:
                    rt.process.terminate()
                    rt.process.wait(timeout=5)
                    log.info(f"Service {name} terminated gracefully")
                except subprocess.TimeoutExpired:
                    log.warning(f"Service {name} did not terminate, killing")
                    rt.process.kill()
                    # Wait for kill to complete
                    try:
                        rt.process.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        log.error(f"Service {name} could not be killed")
                except Exception as e:
                    log.error(f"Error stopping service {name}: {e}", exc_info=True)
            
            rt.process = None
            rt.state = ServiceState.STOPPED

    def _schedule_backoff(self, name: str) -> None:
        """
        Calculate and schedule the next restart time using exponential backoff.
        
        Uses exponential backoff with optional jitter to avoid thundering herd
        problems when multiple services restart simultaneously.
        
        Formula: delay = min(base * 2^(retries-1), max) * (1 + jitter * random)
        
        Args:
            name: Name of the service to schedule for restart
            
        Note:
            Caller must hold the lock when calling this method.
        """
        svc = self.services[name]
        rt = self.state[name]
        
        rt.restart_count += 1
        
        # Calculate exponential backoff
        delay = min(
            svc.backoff_base_sec * (2 ** (rt.restart_count - 1)), 
            svc.backoff_max_sec
        )
        
        # Add jitter to avoid thundering herd
        if svc.backoff_jitter > 0:
            jitter = random.uniform(0, svc.backoff_jitter * delay)
            delay += jitter
        
        rt.next_restart_time = time.time() + delay
        rt.state = ServiceState.BACKOFF
        
        log.warning(
            f"Service {name} scheduled for restart in {delay:.1f}s "
            f"(attempt {rt.restart_count}, max={svc.max_retries})"
        )

    def _monitor_loop(self) -> None:
        """
        Main supervision loop that monitors and heals services.
        
        This runs in a separate thread and continuously:
        1. Checks if running processes have crashed
        2. Performs health checks on running services
        3. Restarts services in BACKOFF state when their backoff expires
        
        The loop acquires the lock for each service check but releases it
        between iterations to avoid blocking other operations.
        
        Thread Safety:
            Uses fine-grained locking to check each service individually,
            releasing the lock between services to prevent blocking.
        """
        log.info("Monitor loop started")
        
        while not self._stop_event.is_set():
            try:
                now = time.time()
                
                # Get snapshot of service names to avoid holding lock
                with self._acquire_lock():
                    service_names = list(self.services.keys())
                
                # Check each service with fine-grained locking
                for name in service_names:
                    if self._stop_event.is_set():
                        break
                    
                    with self._acquire_lock():
                        svc = self.services.get(name)
                        rt = self.state.get(name)
                        
                        if not svc or not rt:
                            continue

                        # Check running processes for crashes
                        if rt.state == ServiceState.RUNNING:
                            if rt.process:
                                exit_code = rt.process.poll()
                                if exit_code is not None:
                                    log.error(
                                        f"Service {name} crashed with exit_code={exit_code}"
                                    )
                                    rt.last_exit_code = exit_code
                                    rt.process = None
                                    self._schedule_backoff(name)
                                else:
                                    # Perform health check
                                    if svc.health_check:
                                        try:
                                            is_healthy = svc.health_check()
                                            if is_healthy != rt.last_health_status:
                                                log.info(
                                                    f"Service {name} health changed: "
                                                    f"{rt.last_health_status} -> {is_healthy}"
                                                )
                                            rt.last_health_status = is_healthy
                                        except Exception as e:
                                            log.warning(
                                                f"Health check failed for {name}: {e}"
                                            )
                                            rt.last_health_status = False

                        # Check if backoff has expired and restart
                        elif rt.state == ServiceState.BACKOFF:
                            if now >= rt.next_restart_time:
                                log.info(f"Backoff expired, restarting {name}")
                                self._spawn_service(name)
                
                # Sleep until next check
                time.sleep(self.check_interval)
                
            except Exception as e:
                log.error(f"Error in monitor loop: {e}", exc_info=True)
                time.sleep(self.check_interval)
        
        log.info("Monitor loop stopped")

# --- Helper Health Checks ---

def check_tcp_port(host: str, port: int, timeout: float = 1.0) -> bool:
    """
    Check if a TCP port is accepting connections.
    
    Performs a simple TCP connection attempt to verify that a service
    is listening on the specified host and port. Useful as a basic
    health check for network services.
    
    Args:
        host: Hostname or IP address to check
        port: TCP port number
        timeout: Connection timeout in seconds (default 1.0)
    
    Returns:
        True if connection succeeds, False otherwise
        
    Example:
        >>> # Check if web server is running
        >>> is_running = check_tcp_port("localhost", 8080)
        >>> if is_running:
        ...     print("Server is up")
    """
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except (OSError, ConnectionRefusedError, socket.timeout):
        return False
    except Exception as e:
        log.debug(f"Unexpected error checking {host}:{port}: {e}")
        return False
