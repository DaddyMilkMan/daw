"""
UPnP port mapper for automatic NAT punch-through assistance.

This module implements Universal Plug and Play (UPnP) port mapping functionality
to automatically configure port forwarding on routers that support UPnP. This
enables peer-to-peer connections without manual router configuration.

Security Note: UPnP has known security implications. This implementation:
- Only maps specific ports as configured
- Uses standard UPnP protocols (SSDP discovery, SOAP control)
- Does not expose additional services beyond the specified port
- Requires router UPnP support to be enabled

For production deployments, consider manual port forwarding or more secure
alternatives like STUN/TURN servers.
"""

import logging
import os
import re
import socket
import urllib.error
import urllib.parse
import urllib.request
from typing import Optional, Tuple

SSDP_ADDR = "239.255.255.250"
SSDP_PORT = 1900
SSDP_MX = 2
SSDP_ST = "urn:schemas-upnp-org:service:WANIPConnection:1"
DEFAULT_PORT = int(os.environ.get("ZENITH_UPNP_PORT", "54321"))
DEFAULT_TIMEOUT = int(os.environ.get("ZENITH_UPNP_TIMEOUT", "3"))

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger("zenith.port_mapper")


class UPnPPortMapper:
    """
    UPnP-based automatic port mapper for NAT traversal.
    
    Discovers UPnP-enabled routers on the local network and creates port
    mappings to enable external connections to the specified port.
    
    Attributes:
        port: The TCP port to map.
        timeout: Timeout in seconds for network operations.
        ssdp_request: The SSDP discovery request message.
    """

    def __init__(self, port: int = DEFAULT_PORT, timeout: int = DEFAULT_TIMEOUT) -> None:
        """
        Initialize the UPnP port mapper.
        
        Args:
            port: TCP port number to map (default: from ZENITH_UPNP_PORT env or 54321).
            timeout: Network operation timeout in seconds (default: from ZENITH_UPNP_TIMEOUT env or 3).
        """
        self.port = port
        self.timeout = timeout
        self.ssdp_request = """M-SEARCH * HTTP/1.1\r\n""" + \
            f"HOST: {SSDP_ADDR}:{SSDP_PORT}\r\n" + \
            "MAN: \"ssdp:discover\"\r\n" + \
            f"MX: {SSDP_MX}\r\n" + \
            f"ST: {SSDP_ST}\r\n\r\n"

    def _get_local_ip(self) -> str:
        """
        Get the local IP address of this machine.
        
        Uses a dummy connection to determine the local network interface IP.
        
        Returns:
            str: Local IP address, or "127.0.0.1" if detection fails.
        """
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            try:
                sock.connect(("10.255.255.255", 1))
                return sock.getsockname()[0]
            except Exception:
                return "127.0.0.1"

    def _discover_router(self) -> Tuple[Optional[str], Optional[str]]:
        """
        Discover UPnP-enabled router via SSDP multicast.
        
        Sends an SSDP M-SEARCH request and waits for router response.
        
        Returns:
            Tuple[Optional[str], Optional[str]]: SSDP response data and router IP,
                                                 or (None, None) if no router found.
        """
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(self.timeout)
            sock.sendto(self.ssdp_request.encode("utf-8"), (SSDP_ADDR, SSDP_PORT))
            try:
                data, addr = sock.recvfrom(2048)
                return data.decode("utf-8", errors="ignore"), addr[0]
            except socket.timeout:
                return None, None
            except Exception as exc:
                logger.warning("Router discovery error: %s", exc)
                return None, None

    def _extract_location(self, data: str) -> Optional[str]:
        """
        Extract the LOCATION URL from SSDP response.
        
        Args:
            data: Raw SSDP response string.
            
        Returns:
            Optional[str]: The location URL if found, None otherwise.
        """
        match = re.search(r"LOCATION:\s*(.*)", data, re.IGNORECASE)
        return match.group(1).strip() if match else None

    def _get_control_url(self, location: str) -> Optional[str]:
        """
        Fetch device description and extract control URL.
        
        Retrieves the UPnP device description XML from the location URL and
        parses it to find the WANIPConnection control URL.
        
        Args:
            location: URL to the device description XML.
            
        Returns:
            Optional[str]: Full control URL if found, None otherwise.
        """
        try:
            with urllib.request.urlopen(location, timeout=self.timeout) as response:
                xml = response.read().decode("utf-8", errors="ignore")
        except urllib.error.URLError as exc:
            logger.warning("Failed to fetch description XML: %s", exc)
            return None

        match = re.search(
            r"urn:schemas-upnp-org:service:WANIPConnection:1.*?<controlURL>(.*?)</controlURL>",
            xml,
            re.DOTALL,
        )
        if not match:
            return None

        path = match.group(1).strip()
        parsed = urllib.parse.urlparse(location)
        base = f"{parsed.scheme}://{parsed.netloc}"
        if not path.startswith("/"):
            path = "/" + path
        return urllib.parse.urljoin(base, path)

    def _add_port_mapping(self, control_url: str, local_ip: str) -> bool:
        """
        Add port mapping via UPnP SOAP request.
        
        Sends a SOAP AddPortMapping request to the router's control URL.
        
        Args:
            control_url: Full URL to the UPnP control endpoint.
            local_ip: Local IP address to map the port to.
            
        Returns:
            bool: True if mapping succeeded, False otherwise.
        """
        soap_body = f"""<?xml version=\"1.0\"?>
<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">
<s:Body>
 <u:AddPortMapping xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">
  <NewRemoteHost></NewRemoteHost>
  <NewExternalPort>{self.port}</NewExternalPort>
  <NewProtocol>TCP</NewProtocol>
  <NewInternalPort>{self.port}</NewInternalPort>
  <NewInternalClient>{local_ip}</NewInternalClient>
  <NewEnabled>1</NewEnabled>
  <NewPortMappingDescription>ZenithDAW</NewPortMappingDescription>
  <NewLeaseDuration>0</NewLeaseDuration>
 </u:AddPortMapping>
</s:Body>
</s:Envelope>"""

        headers = {
            "Content-Type": "text/xml",
            "SOAPAction": '"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping"',
        }

        request = urllib.request.Request(
            control_url, data=soap_body.encode("utf-8"), headers=headers, method="POST"
        )
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as resp:
                if resp.status == 200:
                    logger.info("Successfully mapped TCP port %d", self.port)
                    return True
        except urllib.error.URLError as exc:
            logger.warning("Port mapping request failed: %s", exc)
        return False

    def run(self) -> bool:
        """
        Execute the complete UPnP port mapping process.
        
        Performs the full sequence: discover router, fetch control URL, and
        request port mapping.
        
        Returns:
            bool: True if port mapping succeeded, False otherwise.
        """
        logger.info("Starting UPnP port mapper for TCP port %d", self.port)
        local_ip = self._get_local_ip()
        logger.info("Local IP: %s", local_ip)

        data, router_ip = self._discover_router()
        if not data:
            logger.warning("No router found via SSDP discovery")
            return False

        logger.info("Found router at %s", router_ip)
        location = self._extract_location(data)
        if not location:
            logger.warning("SSDP response missing LOCATION header")
            return False

        logger.info("Description URL: %s", location)
        control_url = self._get_control_url(location)
        if not control_url:
            logger.warning("Could not find WANIPConnection control URL")
            return False

        logger.info("Control URL: %s", control_url)
        return self._add_port_mapping(control_url, local_ip)


if __name__ == "__main__":
    UPnPPortMapper().run()
