"""
UPnP Port Mapper for Automatic NAT Punch-Through Assistance
============================================================

This module provides UPnP (Universal Plug and Play) port mapping functionality
to automatically configure NAT routers for peer-to-peer connections in Zenith DAW.

Key Features:
    - Automatic router discovery via SSDP (Simple Service Discovery Protocol)
    - Port mapping with UPnP IGD (Internet Gateway Device) protocol
    - Retry mechanism with exponential backoff for resilient operation
    - Comprehensive error handling and logging
    - Secure fallback mode when UPnP is unavailable

Usage:
    Basic usage:
        >>> mapper = UPnPPortMapper()
        >>> success = mapper.run()
        >>> if success:
        ...     print("Port mapped successfully")
        ... else:
        ...     print("Port mapping failed, fallback mode active")
    
    Custom configuration:
        >>> mapper = UPnPPortMapper(
        ...     port=8080,
        ...     timeout=5,
        ...     max_retries=5,
        ...     backoff_base=2.0
        ... )
        >>> success = mapper.run()
    
    Environment Variables:
        ZENITH_UPNP_PORT: Port to map (default: 54321)
        ZENITH_UPNP_TIMEOUT: Operation timeout in seconds (default: 3)
        ZENITH_UPNP_MAX_RETRIES: Maximum retry attempts (default: 3)
        ZENITH_UPNP_BACKOFF_BASE: Base backoff delay in seconds (default: 1.0)
        ZENITH_UPNP_BACKOFF_MAX: Maximum backoff delay in seconds (default: 30.0)

Error Handling:
    The module uses custom exception classes to distinguish between different
    failure modes and implements retry logic for transient errors.

Security:
    - All network operations have configurable timeouts
    - Fallback mode ensures application continues when UPnP fails
    - No sensitive data is exposed in logs
"""

import logging
import os
import re
import socket
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Optional, Tuple

# ==============================================================================
# Constants - Centralized Configuration
# ==============================================================================

# SSDP (Simple Service Discovery Protocol) Configuration
SSDP_MULTICAST_ADDR = "239.255.255.250"
SSDP_PORT = 1900
SSDP_MX = 2  # Maximum wait time for SSDP responses (seconds)
SSDP_ST = "urn:schemas-upnp-org:service:WANIPConnection:1"  # Service Type

# Default Configuration (overridable via environment variables)
DEFAULT_PORT = int(os.environ.get("ZENITH_UPNP_PORT", "54321"))
DEFAULT_TIMEOUT = int(os.environ.get("ZENITH_UPNP_TIMEOUT", "3"))
DEFAULT_MAX_RETRIES = int(os.environ.get("ZENITH_UPNP_MAX_RETRIES", "3"))
DEFAULT_BACKOFF_BASE = float(os.environ.get("ZENITH_UPNP_BACKOFF_BASE", "1.0"))
DEFAULT_BACKOFF_MAX = float(os.environ.get("ZENITH_UPNP_BACKOFF_MAX", "30.0"))

# UPnP Protocol Constants
UPNP_PROTOCOL = "TCP"
UPNP_LEASE_DURATION = 0  # 0 = permanent until router reboot
UPNP_DESCRIPTION = "ZenithDAW"

# Network Constants
LOCAL_IP_PROBE_ADDRESS = "10.255.255.255"  # Non-routable address for local IP detection
LOCAL_IP_PROBE_PORT = 1
LOCALHOST_FALLBACK = "127.0.0.1"
MAX_SSDP_RESPONSE_SIZE = 2048

# XML/SOAP Constants
SOAP_ENVELOPE_XMLNS = "http://schemas.xmlsoap.org/soap/envelope/"
SOAP_ENCODING_STYLE = "http://schemas.xmlsoap.org/soap/encoding/"
UPNP_SERVICE_TYPE = "urn:schemas-upnp-org:service:WANIPConnection:1"
UPNP_CONTROL_URL_PATTERN = r"urn:schemas-upnp-org:service:WANIPConnection:1.*?<controlURL>(.*?)</controlURL>"
LOCATION_HEADER_PATTERN = r"LOCATION:\s*(.*)"

# ==============================================================================
# Custom Exceptions
# ==============================================================================


class UPnPError(Exception):
    """Base exception for all UPnP-related errors."""
    pass


class RouterDiscoveryError(UPnPError):
    """Raised when router discovery via SSDP fails."""
    pass


class ControlURLError(UPnPError):
    """Raised when control URL cannot be determined."""
    pass


class PortMappingError(UPnPError):
    """Raised when port mapping request fails."""
    pass


class NetworkError(UPnPError):
    """Raised when network operations fail."""
    pass


# ==============================================================================
# Logging Configuration
# ==============================================================================

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("zenith.port_mapper")


# ==============================================================================
# Main UPnP Port Mapper Class
# ==============================================================================


class UPnPPortMapper:
    """
    UPnP Port Mapper with retry logic and comprehensive error handling.
    
    This class handles automatic port mapping configuration on UPnP-enabled
    routers, with built-in retry logic, exponential backoff, and fallback modes.
    
    Attributes:
        port (int): The port number to map.
        timeout (int): Network operation timeout in seconds.
        max_retries (int): Maximum number of retry attempts for operations.
        backoff_base (float): Base delay for exponential backoff (seconds).
        backoff_max (float): Maximum delay for exponential backoff (seconds).
        fallback_mode (bool): Whether the mapper is in fallback mode.
        ssdp_request (str): The SSDP discovery request message.
    
    Example:
        >>> mapper = UPnPPortMapper(port=8080, timeout=5)
        >>> if mapper.run():
        ...     print("Port mapping successful")
        ... else:
        ...     print("Running in fallback mode")
    """
    
    def __init__(
        self,
        port: int = DEFAULT_PORT,
        timeout: int = DEFAULT_TIMEOUT,
        max_retries: int = DEFAULT_MAX_RETRIES,
        backoff_base: float = DEFAULT_BACKOFF_BASE,
        backoff_max: float = DEFAULT_BACKOFF_MAX,
    ) -> None:
        """
        Initialize the UPnP port mapper.
        
        Args:
            port: The port number to map (default: from ZENITH_UPNP_PORT or 54321).
            timeout: Network operation timeout in seconds (default: 3).
            max_retries: Maximum retry attempts for operations (default: 3).
            backoff_base: Base delay for exponential backoff in seconds (default: 1.0).
            backoff_max: Maximum backoff delay in seconds (default: 30.0).
        """
        self.port = port
        self.timeout = timeout
        self.max_retries = max_retries
        self.backoff_base = backoff_base
        self.backoff_max = backoff_max
        self.fallback_mode = False
        
        # Construct SSDP discovery request
        self.ssdp_request = (
            "M-SEARCH * HTTP/1.1\r\n"
            f"HOST: {SSDP_MULTICAST_ADDR}:{SSDP_PORT}\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            f"MX: {SSDP_MX}\r\n"
            f"ST: {SSDP_ST}\r\n\r\n"
        )
        
        logger.debug(
            "UPnPPortMapper initialized: port=%d, timeout=%d, max_retries=%d",
            self.port, self.timeout, self.max_retries
        )

    def _calculate_backoff(self, attempt: int) -> float:
        """
        Calculate exponential backoff delay for retry attempts.
        
        Args:
            attempt: The current retry attempt number (0-indexed).
        
        Returns:
            The delay in seconds before the next retry.
        """
        delay = min(self.backoff_base * (2 ** attempt), self.backoff_max)
        logger.debug("Calculated backoff delay: %.2f seconds (attempt %d)", delay, attempt)
        return delay

    def _get_local_ip(self) -> str:
        """
        Determine the local IP address for port mapping.
        
        This method attempts to connect to a non-routable address to determine
        the local IP that would be used for external connections.
        
        Returns:
            The local IP address as a string. Falls back to '127.0.0.1' if
            detection fails.
        
        Raises:
            NetworkError: If local IP detection encounters a critical error.
        """
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                # Connect to a non-routable address to determine local IP
                sock.connect((LOCAL_IP_PROBE_ADDRESS, LOCAL_IP_PROBE_PORT))
                local_ip = sock.getsockname()[0]
                logger.debug("Detected local IP: %s", local_ip)
                return local_ip
        except OSError as exc:
            logger.warning("Could not detect local IP: %s. Using fallback.", exc)
            return LOCALHOST_FALLBACK
        except Exception as exc:
            logger.error("Unexpected error detecting local IP: %s", exc)
            raise NetworkError(f"Failed to determine local IP: {exc}") from exc

    def _discover_router(self) -> Tuple[Optional[str], Optional[str]]:
        """
        Discover UPnP-enabled router via SSDP multicast.
        
        Sends an SSDP M-SEARCH request and waits for a router response.
        
        Returns:
            A tuple of (response_data, router_ip). Returns (None, None) if
            no router is discovered.
        
        Raises:
            RouterDiscoveryError: If discovery encounters a critical error.
        """
        logger.debug("Starting SSDP router discovery on %s:%d", SSDP_MULTICAST_ADDR, SSDP_PORT)
        
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                sock.settimeout(self.timeout)
                
                # Send SSDP discovery request
                logger.debug("Sending SSDP M-SEARCH request")
                sock.sendto(
                    self.ssdp_request.encode("utf-8"),
                    (SSDP_MULTICAST_ADDR, SSDP_PORT)
                )
                
                # Wait for response
                try:
                    data, addr = sock.recvfrom(MAX_SSDP_RESPONSE_SIZE)
                    response_data = data.decode("utf-8", errors="ignore")
                    router_ip = addr[0]
                    logger.info("Received SSDP response from router at %s", router_ip)
                    logger.debug("SSDP response data: %s", response_data[:200])
                    return response_data, router_ip
                except socket.timeout:
                    logger.warning("SSDP discovery timeout after %d seconds", self.timeout)
                    return None, None
        except OSError as exc:
            logger.error("Network error during router discovery: %s", exc)
            raise RouterDiscoveryError(f"SSDP discovery failed: {exc}") from exc
        except Exception as exc:
            logger.error("Unexpected error during router discovery: %s", exc)
            raise RouterDiscoveryError(f"Unexpected discovery error: {exc}") from exc

    def _extract_location(self, data: str) -> Optional[str]:
        """
        Extract the LOCATION header from SSDP response.
        
        Args:
            data: The SSDP response data.
        
        Returns:
            The location URL if found, None otherwise.
        """
        logger.debug("Extracting LOCATION header from SSDP response")
        match = re.search(LOCATION_HEADER_PATTERN, data, re.IGNORECASE)
        
        if match:
            location = match.group(1).strip()
            logger.debug("Extracted LOCATION: %s", location)
            return location
        else:
            logger.warning("LOCATION header not found in SSDP response")
            return None

    def _get_control_url(self, location: str) -> Optional[str]:
        """
        Retrieve and parse the UPnP control URL from device description.
        
        Fetches the device description XML from the location URL and extracts
        the WANIPConnection control URL.
        
        Args:
            location: The device description URL from SSDP response.
        
        Returns:
            The full control URL if found, None otherwise.
        
        Raises:
            ControlURLError: If fetching or parsing the description fails critically.
        """
        logger.debug("Fetching device description from: %s", location)
        
        try:
            # Fetch device description XML
            with urllib.request.urlopen(location, timeout=self.timeout) as response:
                xml = response.read().decode("utf-8", errors="ignore")
                logger.debug("Successfully fetched device description (size: %d bytes)", len(xml))
        except urllib.error.HTTPError as exc:
            logger.error("HTTP error fetching description: %s %s", exc.code, exc.reason)
            raise ControlURLError(f"HTTP {exc.code}: {exc.reason}") from exc
        except urllib.error.URLError as exc:
            logger.warning("Failed to fetch description XML: %s", exc)
            return None
        except Exception as exc:
            logger.error("Unexpected error fetching description: %s", exc)
            raise ControlURLError(f"Failed to fetch description: {exc}") from exc

        # Parse control URL from XML
        logger.debug("Parsing control URL from device description")
        match = re.search(UPNP_CONTROL_URL_PATTERN, xml, re.DOTALL)
        
        if not match:
            logger.warning("WANIPConnection control URL not found in device description")
            return None

        # Construct full control URL
        path = match.group(1).strip()
        parsed = urllib.parse.urlparse(location)
        base = f"{parsed.scheme}://{parsed.netloc}"
        
        if not path.startswith("/"):
            path = "/" + path
        
        control_url = urllib.parse.urljoin(base, path)
        logger.info("Resolved control URL: %s", control_url)
        return control_url

    def _add_port_mapping(self, control_url: str, local_ip: str) -> bool:
        """
        Add port mapping via UPnP SOAP request.
        
        Sends a SOAP AddPortMapping request to the router's control URL.
        
        Args:
            control_url: The UPnP control URL.
            local_ip: The local IP address to map to.
        
        Returns:
            True if mapping was successful, False otherwise.
        
        Raises:
            PortMappingError: If the mapping request fails critically.
        """
        logger.info(
            "Attempting to map port %d (TCP) to %s via UPnP",
            self.port, local_ip
        )
        
        # Construct SOAP request body
        soap_body = f"""<?xml version="1.0"?>
<s:Envelope xmlns:s="{SOAP_ENVELOPE_XMLNS}" s:encodingStyle="{SOAP_ENCODING_STYLE}">
<s:Body>
 <u:AddPortMapping xmlns:u="{UPNP_SERVICE_TYPE}">
  <NewRemoteHost></NewRemoteHost>
  <NewExternalPort>{self.port}</NewExternalPort>
  <NewProtocol>{UPNP_PROTOCOL}</NewProtocol>
  <NewInternalPort>{self.port}</NewInternalPort>
  <NewInternalClient>{local_ip}</NewInternalClient>
  <NewEnabled>1</NewEnabled>
  <NewPortMappingDescription>{UPNP_DESCRIPTION}</NewPortMappingDescription>
  <NewLeaseDuration>{UPNP_LEASE_DURATION}</NewLeaseDuration>
 </u:AddPortMapping>
</s:Body>
</s:Envelope>"""

        headers = {
            "Content-Type": "text/xml",
            "SOAPAction": f'"{UPNP_SERVICE_TYPE}#AddPortMapping"',
        }

        logger.debug("Sending SOAP AddPortMapping request")
        request = urllib.request.Request(
            control_url,
            data=soap_body.encode("utf-8"),
            headers=headers,
            method="POST"
        )
        
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as resp:
                status = resp.status
                logger.debug("SOAP response status: %d", status)
                
                if status == 200:
                    logger.info(
                        "Successfully mapped TCP port %d to %s",
                        self.port, local_ip
                    )
                    return True
                else:
                    logger.warning("Unexpected SOAP response status: %d", status)
                    return False
        except urllib.error.HTTPError as exc:
            logger.error("HTTP error during port mapping: %s %s", exc.code, exc.reason)
            if exc.code >= 500:
                # Server errors might be transient
                raise PortMappingError(f"Server error: {exc.code}") from exc
            return False
        except urllib.error.URLError as exc:
            logger.warning("Port mapping request failed: %s", exc)
            return False
        except Exception as exc:
            logger.error("Unexpected error during port mapping: %s", exc)
            raise PortMappingError(f"Port mapping failed: {exc}") from exc

    def _retry_operation(self, operation_name: str, operation_func, *args, **kwargs):
        """
        Execute an operation with retry logic and exponential backoff.
        
        Args:
            operation_name: Human-readable name for logging.
            operation_func: The function to execute.
            *args: Positional arguments for the operation.
            **kwargs: Keyword arguments for the operation.
        
        Returns:
            The result of the operation if successful.
        
        Raises:
            The last exception encountered if all retries fail.
        """
        last_exception = None
        
        for attempt in range(self.max_retries + 1):
            try:
                logger.debug(
                    "Executing %s (attempt %d/%d)",
                    operation_name, attempt + 1, self.max_retries + 1
                )
                result = operation_func(*args, **kwargs)
                
                if attempt > 0:
                    logger.info(
                        "%s succeeded on attempt %d",
                        operation_name, attempt + 1
                    )
                
                return result
            except Exception as exc:
                last_exception = exc
                logger.warning(
                    "%s failed (attempt %d/%d): %s",
                    operation_name, attempt + 1, self.max_retries + 1, exc
                )
                
                if attempt < self.max_retries:
                    delay = self._calculate_backoff(attempt)
                    logger.info("Retrying %s after %.2f seconds...", operation_name, delay)
                    time.sleep(delay)
        
        # All retries exhausted
        logger.error("%s failed after %d attempts", operation_name, self.max_retries + 1)
        raise last_exception

    def _enable_fallback_mode(self, reason: str) -> None:
        """
        Enable fallback mode when UPnP operations fail.
        
        In fallback mode, the application continues without UPnP port mapping,
        relying on manual port forwarding or other connectivity methods.
        
        Args:
            reason: The reason for entering fallback mode.
        """
        self.fallback_mode = True
        logger.warning("=" * 70)
        logger.warning("ENTERING FALLBACK MODE: %s", reason)
        logger.warning("UPnP port mapping is not available.")
        logger.warning("The application will continue, but peer-to-peer connections")
        logger.warning("may require manual port forwarding on your router.")
        logger.warning("Port to forward: %d (TCP)", self.port)
        logger.warning("=" * 70)

    def run(self) -> bool:
        """
        Execute the complete UPnP port mapping workflow.
        
        This method orchestrates the entire port mapping process:
        1. Detect local IP address
        2. Discover router via SSDP (with retries)
        3. Retrieve control URL (with retries)
        4. Add port mapping (with retries)
        
        Returns:
            True if port mapping was successful, False if fallback mode is active.
        
        Note:
            Even if this method returns False, the application can continue
            in fallback mode. Check the fallback_mode attribute to determine
            the current state.
        """
        logger.info("=" * 70)
        logger.info("Starting UPnP port mapper for TCP port %d", self.port)
        logger.info("Configuration: timeout=%ds, max_retries=%d",
                   self.timeout, self.max_retries)
        logger.info("=" * 70)
        
        try:
            # Step 1: Get local IP
            logger.info("Step 1/4: Detecting local IP address...")
            local_ip = self._get_local_ip()
            logger.info("Local IP address: %s", local_ip)
            
            # Step 2: Discover router
            logger.info("Step 2/4: Discovering UPnP-enabled router...")
            try:
                data, router_ip = self._retry_operation(
                    "Router discovery",
                    self._discover_router
                )
            except Exception as exc:
                self._enable_fallback_mode(f"Router discovery failed: {exc}")
                return False
            
            if not data:
                self._enable_fallback_mode("No UPnP-enabled router found")
                return False
            
            logger.info("Found router at %s", router_ip)
            
            # Step 3: Extract and retrieve control URL
            logger.info("Step 3/4: Retrieving router control URL...")
            location = self._extract_location(data)
            
            if not location:
                self._enable_fallback_mode("SSDP response missing LOCATION header")
                return False
            
            logger.debug("Device description URL: %s", location)
            
            try:
                control_url = self._retry_operation(
                    "Control URL retrieval",
                    self._get_control_url,
                    location
                )
            except Exception as exc:
                self._enable_fallback_mode(f"Control URL retrieval failed: {exc}")
                return False
            
            if not control_url:
                self._enable_fallback_mode(
                    "WANIPConnection control URL not found in device description"
                )
                return False
            
            logger.info("Control URL: %s", control_url)
            
            # Step 4: Add port mapping
            logger.info("Step 4/4: Adding port mapping...")
            try:
                success = self._retry_operation(
                    "Port mapping",
                    self._add_port_mapping,
                    control_url,
                    local_ip
                )
            except Exception as exc:
                self._enable_fallback_mode(f"Port mapping failed: {exc}")
                return False
            
            if not success:
                self._enable_fallback_mode("Port mapping request was rejected by router")
                return False
            
            # Success!
            logger.info("=" * 70)
            logger.info("SUCCESS: Port %d is now mapped via UPnP", self.port)
            logger.info("External port %d -> %s:%d", self.port, local_ip, self.port)
            logger.info("=" * 70)
            return True
            
        except Exception as exc:
            # Catch any unexpected errors
            logger.exception("Unexpected error in UPnP port mapper: %s", exc)
            self._enable_fallback_mode(f"Unexpected error: {exc}")
            return False


# ==============================================================================
# Module Entry Point
# ==============================================================================


def main() -> None:
    """
    Main entry point when module is run directly.
    
    Configures logging and runs the UPnP port mapper with default settings.
    Exit codes:
        0: Port mapping successful
        1: Port mapping failed (fallback mode active)
    """
    # Configure logging level from environment
    log_level_str = os.environ.get("ZENITH_LOG_LEVEL", "INFO").upper()
    log_level = getattr(logging, log_level_str, logging.INFO)
    logging.getLogger().setLevel(log_level)
    
    logger.info("Zenith DAW UPnP Port Mapper")
    logger.info("Version: 2.0")
    logger.info("")
    
    # Create and run mapper
    mapper = UPnPPortMapper()
    success = mapper.run()
    
    # Exit with appropriate code
    if success:
        logger.info("Port mapper completed successfully")
        return
    else:
        logger.warning("Port mapper completed in fallback mode")
        # Note: We don't exit with error code because fallback mode is a valid state


if __name__ == "__main__":
    main()
