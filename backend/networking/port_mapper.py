"""UPnP port mapper for automatic NAT punch-through assistance."""

import os
import re
import socket
import urllib.error
import urllib.parse
import urllib.request
from typing import Optional, Tuple

from backend.logger import get_logger

SSDP_ADDR = "239.255.255.250"
SSDP_PORT = 1900
SSDP_MX = 2
SSDP_ST = "urn:schemas-upnp-org:service:WANIPConnection:1"
DEFAULT_PORT = int(os.environ.get("ZENITH_UPNP_PORT", "54321"))
DEFAULT_TIMEOUT = int(os.environ.get("ZENITH_UPNP_TIMEOUT", "3"))

logger = get_logger("zenith.port_mapper")


class UPnPPortMapper:
    def __init__(self, port: int = DEFAULT_PORT, timeout: int = DEFAULT_TIMEOUT) -> None:
        self.port = port
        self.timeout = timeout
        self.ssdp_request = """M-SEARCH * HTTP/1.1\r\n""" + \
            f"HOST: {SSDP_ADDR}:{SSDP_PORT}\r\n" + \
            "MAN: \"ssdp:discover\"\r\n" + \
            f"MX: {SSDP_MX}\r\n" + \
            f"ST: {SSDP_ST}\r\n\r\n"

    def _get_local_ip(self) -> str:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            try:
                sock.connect(("10.255.255.255", 1))
                return sock.getsockname()[0]
            except Exception:
                return "127.0.0.1"

    def _discover_router(self) -> Tuple[Optional[str], Optional[str]]:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(self.timeout)
            sock.sendto(self.ssdp_request.encode("utf-8"), (SSDP_ADDR, SSDP_PORT))
            try:
                data, addr = sock.recvfrom(2048)
                return data.decode("utf-8", errors="ignore"), addr[0]
            except socket.timeout:
                return None, None
            except Exception as exc:
                logger.warning("Router discovery error", error=str(exc))
                return None, None

    def _extract_location(self, data: str) -> str | None:
        match = re.search(r"LOCATION:\s*(.*)", data, re.IGNORECASE)
        return match.group(1).strip() if match else None

    def _get_control_url(self, location: str) -> str | None:
        try:
            with urllib.request.urlopen(location, timeout=self.timeout) as response:
                xml = response.read().decode("utf-8", errors="ignore")
        except urllib.error.URLError as exc:
            logger.warning("Failed to fetch description XML", error=str(exc))
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
                    logger.info("Successfully mapped TCP port", port=self.port)
                    return True
        except urllib.error.URLError as exc:
            logger.warning("Port mapping request failed", error=str(exc))
        return False

    def run(self) -> bool:
        logger.info("Starting UPnP port mapper for TCP port", port=self.port)
        local_ip = self._get_local_ip()
        logger.info("Local IP detected", ip=local_ip)

        data, router_ip = self._discover_router()
        if not data:
            logger.warning("No router found via SSDP discovery")
            return False

        logger.info("Found router", router_ip=router_ip)
        location = self._extract_location(data)
        if not location:
            logger.warning("SSDP response missing LOCATION header")
            return False

        logger.info("Description URL found", url=location)
        control_url = self._get_control_url(location)
        if not control_url:
            logger.warning("Could not find WANIPConnection control URL")
            return False

        logger.info("Control URL found", url=control_url)
        return self._add_port_mapping(control_url, local_ip)


if __name__ == "__main__":
    # When run as a standalone script, configure basic logging
    # In production, logging is configured by the parent orchestrator
    from backend.config import ZenithConfig
    from backend.logger import configure_logging
    
    config = ZenithConfig()
    configure_logging(config)
    UPnPPortMapper().run()
