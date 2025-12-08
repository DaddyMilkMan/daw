import socket
import urllib.request
import urllib.parse
import re
import socket

# This script attempts to open TCP Port 54321 using UPnP (IGD) without external dependencies.
# It sends a multicast discovery packet, finds the router, and sends a SOAP AddPortMapping request.

SSDP_ADDR = "239.255.255.250"
SSDP_PORT = 1900
SSDP_MX = 2
SSDP_ST = "urn:schemas-upnp-org:service:WANIPConnection:1"

ssdpRequest = "M-SEARCH * HTTP/1.1\r\n" + \
              "HOST: %s:%d\r\n" % (SSDP_ADDR, SSDP_PORT) + \
              "MAN: \"ssdp:discover\"\r\n" + \
              "MX: %d\r\n" % (SSDP_MX, ) + \
              "ST: %s\r\n" % (SSDP_ST, ) + "\r\n"

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Doesn't need to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def discover_router():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(3)
    sock.sendto(ssdpRequest.encode(), (SSDP_ADDR, SSDP_PORT))
    
    try:
        data, addr = sock.recvfrom(1024)
        return data.decode(), addr[0]
    except socket.timeout:
        return None, None

def parse_location(data):
    # Extract LOCATION: url
    match = re.search(r'LOCATION: (.*)', data, re.IGNORECASE)
    if match:
        return match.group(1).strip()
    return None

def get_control_url(location):
    # Fetch description XML and find ControlURL for WANIPConnection
    try:
        with urllib.request.urlopen(location) as response:
            xml = response.read().decode()
            
        # Very simple regex parsing to avoid ElementTree namespace issues complexity
        # Look for serviceType WANIPConnection and then controlURL
        # Note: This is fragile but works on many standard routers
        wan_service = re.search(r'urn:schemas-upnp-org:service:WANIPConnection:1.*?<controlURL>(.*?)</controlURL>', xml, re.DOTALL)
        if wan_service:
            path = wan_service.group(1)
            # Combine with base URL
            parsed = urllib.parse.urlparse(location)
            base = f"{parsed.scheme}://{parsed.netloc}"
            if not path.startswith('/'): path = '/' + path
            return base + path
            
    except Exception as e:
        print(f"Error parsing XML: {e}")
    return None

def add_port_mapping(control_url, local_ip, port=54321):
    soap_body = f"""<?xml version="1.0"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<s:Body>
 <u:AddPortMapping xmlns:u="urn:schemas-upnp-org:service:WANIPConnection:1">
  <NewRemoteHost></NewRemoteHost>
  <NewExternalPort>{port}</NewExternalPort>
  <NewProtocol>TCP</NewProtocol>
  <NewInternalPort>{port}</NewInternalPort>
  <NewInternalClient>{local_ip}</NewInternalClient>
  <NewEnabled>1</NewEnabled>
  <NewPortMappingDescription>ZenithDAW</NewPortMappingDescription>
  <NewLeaseDuration>0</NewLeaseDuration>
 </u:AddPortMapping>
</s:Body>
</s:Envelope>"""

    headers = {
        'Content-Type': 'text/xml',
        'SOAPAction': '"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping"'
    }

    try:
        req = urllib.request.Request(control_url, data=soap_body.encode(), headers=headers, method='POST')
        with urllib.request.urlopen(req) as resp:
            if resp.status == 200:
                print("SUCCESS: Port mapped.")
                return True
    except Exception as e:
        print(f"SOAP Error: {e}")
    return False

if __name__ == "__main__":
    print("Starting UPnP Port Mapper...")
    local_ip = get_local_ip()
    print(f"Local IP: {local_ip}")
    
    data, router_ip = discover_router()
    if data:
        print(f"Found Router at {router_ip}")
        location = parse_location(data)
        if location:
            print(f"Description URL: {location}")
            control_url = get_control_url(location)
            if control_url:
                print(f"Control URL: {control_url}")
                add_port_mapping(control_url, local_ip, 54321)
            else:
                print("Could not find WANIPConnection Control URL.")
        else:
            print("No Location in SSDP.")
    else:
        print("No router found via SSDP.")
