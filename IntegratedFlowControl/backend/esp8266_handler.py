#!/usr/bin/env python3
"""
ESP8266 Handler module - manages HTTP communication with ESP8266 flow sensors
Handles automatic discovery, sensor data retrieval, and recording control
"""

import requests
import socket
import logging
import time
from typing import Dict, Any, Optional

logger = logging.getLogger(__name__)

class ESP8266Handler:
    """Handles communication with ESP8266 flow sensor system"""
    
    def __init__(self, timeout=3):
        self.timeout = timeout
        self.ip_address = None
        self.port = 80
        self.base_url = None
        
    def discover_and_connect(self, ip_range_base="192.168.1.") -> bool:
        """
        Automatically discover ESP8266 on the network
        Enhanced discovery for Raspberry Pi with multiple methods
        """
        logger.info("Scanning for ESP8266 flow sensors...")
        
        # Method 1: Try mDNS names first
        mdns_names = [
            "flowsensors.local",
            "flowsensor.local", 
            "esp8266.local",
            "arduino.local"
        ]
        
        for mdns_name in mdns_names:
            try:
                import socket
                ip = socket.gethostbyname(mdns_name)
                logger.info(f"Trying mDNS name {mdns_name} -> {ip}")
                if self._test_esp8266_connection(ip):
                    self.ip_address = ip
                    self.base_url = f"http://{ip}:{self.port}"
                    logger.info(f"ESP8266 found via mDNS at {ip}")
                    return True
            except Exception as e:
                logger.debug(f"mDNS lookup failed for {mdns_name}: {e}")
        
        # Method 2: Try to get current network range from Raspberry Pi
        try:
            import subprocess
            result = subprocess.run(['hostname', '-I'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                pi_ip = result.stdout.strip().split()[0]
                if '.' in pi_ip:
                    # Extract network base (e.g. 192.168.1. from 192.168.1.100)
                    ip_parts = pi_ip.split('.')
                    if len(ip_parts) >= 3:
                        current_network = f"{ip_parts[0]}.{ip_parts[1]}.{ip_parts[2]}."
                        logger.info(f"Detected Raspberry Pi network: {current_network}xxx")
                        # Try current network range first
                        if self._scan_network_range(current_network):
                            return True
        except Exception as e:
            logger.debug(f"Failed to detect current network range: {e}")
        
        # Method 3: Common IP addresses for different network configurations
        common_ips = [
            "192.168.10.100", "192.168.10.101", "192.168.10.102", "192.168.10.150",
            "192.168.1.100", "192.168.1.101", "192.168.1.102", "192.168.1.150",
            "192.168.0.100", "192.168.0.101", "192.168.0.102", "192.168.0.150", 
            "192.168.4.1",   # ESP8266 AP mode default
            "10.0.0.100", "10.0.0.101", "10.0.0.102",
            "192.168.1.200", "192.168.1.201", "192.168.1.123"
        ]
        
        logger.info(f"Testing {len(common_ips)} common IP addresses...")
        for ip in common_ips:
            logger.debug(f"Testing {ip}...")
            if self._test_esp8266_connection(ip):
                self.ip_address = ip
                self.base_url = f"http://{ip}:{self.port}"
                logger.info(f"ESP8266 found at {ip}")
                return True
        
        # Method 4: Network range scanning
        logger.info("Performing network range scan...")
        common_ranges = ["192.168.10.", "192.168.1.", "192.168.0.", "10.0.0."]
        for base in common_ranges:
            if self._scan_network_range(base):
                return True
        
        logger.warning("ESP8266 not found on network")
        logger.info("ESP8266 troubleshooting tips:")
        logger.info("  1. Make sure ESP8266 is powered and connected to WiFi")
        logger.info("  2. Check ESP8266 serial output for IP address during boot")
        logger.info("  3. Look for ESP8266 in your router's device list")
        logger.info("  4. Try accessing: http://[ESP8266_IP] directly in a browser")
        logger.info("  5. If you know the IP, set ESP8266_IP = \"x.x.x.x\" in main.py")
        return False
    
    def _scan_network_range(self, base_ip: str, start: int = 100, end: int = 200) -> bool:
        """Scan a range of IPs in a network"""
        logger.info(f"Scanning {base_ip}{start}-{end}...")
        import threading
        import queue
        
        found_ip = None
        result_queue = queue.Queue()
        
        def test_ip(ip):
            if self._test_esp8266_connection(ip):
                result_queue.put(ip)
        
        # Use threading for faster scanning
        threads = []
        for i in range(start, min(end + 1, 255)):
            ip = f"{base_ip}{i}"
            thread = threading.Thread(target=test_ip, args=(ip,))
            threads.append(thread)
            thread.start()
            
            # Limit concurrent threads
            if len(threads) >= 20:
                for t in threads:
                    t.join(timeout=1)
                threads = []
        
        # Wait for remaining threads
        for thread in threads:
            thread.join(timeout=1)
        
        # Check results
        try:
            found_ip = result_queue.get_nowait()
            self.ip_address = found_ip
            self.base_url = f"http://{found_ip}:{self.port}"
            logger.info(f"ESP8266 found at {found_ip}")
            return True
        except queue.Empty:
            return False
    
    def _test_esp8266_connection(self, ip: str) -> bool:
        """Test if ESP8266 is responding at given IP with comprehensive checks"""
        try:
            # Quick socket test first
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((ip, self.port))
            sock.close()
            
            if result != 0:
                return False
            
            # Test multiple HTTP endpoints that might exist on ESP8266
            test_endpoints = [
                '/api',
                '/',
                '/status',
                '/sensor',
                '/data'
            ]
            
            for endpoint in test_endpoints:
                try:
                    response = requests.get(f"http://{ip}{endpoint}", timeout=2)
                    if response.status_code == 200:
                        data = response.text
                        # Check if it looks like our sensor system
                        if 's1' in data or 's2' in data or 'run' in data or 'sensor' in data.lower() or 'flow' in data.lower():
                            logger.debug(f"ESP8266 confirmed at {ip}{endpoint}")
                            return True
                except Exception as e:
                    logger.debug(f"Failed to test {ip}{endpoint}: {e}")
                    continue
                    
            # Even if endpoints don't match exactly, if we got HTTP responses, it might be our device
            try:
                response = requests.get(f"http://{ip}", timeout=2)
                if response.status_code == 200 and len(response.text) > 10:
                    logger.info(f"Found HTTP server at {ip}, assuming it's our ESP8266")
                    return True
            except:
                pass
            
            return False
        except Exception as e:
            logger.debug(f"Connection test failed for {ip}: {e}")
            return False
    
    def connect(self, ip_address: str) -> bool:
        """Connect to ESP8266 at specific IP address"""
        if self._test_esp8266_connection(ip_address):
            self.ip_address = ip_address
            self.base_url = f"http://{ip_address}:{self.port}"
            logger.info(f"Connected to ESP8266 at {ip_address}")
            return True
        else:
            logger.error(f"Cannot connect to ESP8266 at {ip_address}")
            return False
    
    def is_connected(self) -> bool:
        """Check if ESP8266 is currently reachable"""
        if not self.base_url:
            return False
        
        try:
            response = requests.get(f"{self.base_url}/api", timeout=2)
            return response.status_code == 200
        except:
            return False
    
    def get_ip_address(self) -> str:
        """Get current ESP8266 IP address"""
        return self.ip_address or "Not connected"
    
    def get_sensor_data(self) -> Dict[str, Any]:
        """Get real-time sensor data from ESP8266"""
        if not self.base_url:
            raise Exception("ESP8266 not connected")
        
        try:
            response = requests.get(f"{self.base_url}/api", timeout=self.timeout)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            raise Exception(f"Failed to get sensor data: {str(e)}")
    
    def start_recording(self) -> bool:
        """Start recording on ESP8266"""
        if not self.base_url:
            logger.error("ESP8266 not connected - cannot start recording")
            return False
        
        try:
            # Try multiple endpoints that might work on different ESP8266 firmware versions
            endpoints_to_try = [
                f"{self.base_url}/start",
                f"{self.base_url}/api/start-recording", 
                f"{self.base_url}/record/start"
            ]
            
            for endpoint in endpoints_to_try:
                try:
                    logger.info(f"Trying to start recording at: {endpoint}")
                    response = requests.post(endpoint, timeout=self.timeout)
                    if response.status_code == 200:
                        logger.info(f"Recording started successfully via {endpoint}")
                        return True
                except requests.RequestException as e:
                    logger.debug(f"Failed to start recording at {endpoint}: {e}")
                    continue
            
            logger.error("Failed to start recording on all attempted endpoints")
            return False
            
        except Exception as e:
            logger.error(f"Unexpected error starting recording: {str(e)}")
            return False
    
    def stop_recording(self) -> bool:
        """Stop recording on ESP8266"""
        if not self.base_url:
            logger.error("ESP8266 not connected - cannot stop recording")
            return False
        
        try:
            # Try multiple endpoints that might work on different ESP8266 firmware versions
            endpoints_to_try = [
                f"{self.base_url}/stop",
                f"{self.base_url}/api/stop-recording",
                f"{self.base_url}/record/stop"
            ]
            
            for endpoint in endpoints_to_try:
                try:
                    logger.info(f"Trying to stop recording at: {endpoint}")
                    response = requests.post(endpoint, timeout=self.timeout)
                    if response.status_code == 200:
                        logger.info(f"Recording stopped successfully via {endpoint}")
                        return True
                except requests.RequestException as e:
                    logger.debug(f"Failed to stop recording at {endpoint}: {e}")
                    continue
            
            logger.error("Failed to stop recording on all attempted endpoints")
            return False
            
        except Exception as e:
            logger.error(f"Unexpected error stopping recording: {str(e)}")
            return False
    
    def download_csv(self) -> Optional[str]:
        """Download CSV data from ESP8266"""
        if not self.base_url:
            logger.error("ESP8266 not connected - cannot download CSV")
            return None
        
        try:
            # Try multiple endpoints for CSV download
            endpoints_to_try = [
                f"{self.base_url}/log.csv",
                f"{self.base_url}/data.csv", 
                f"{self.base_url}/download/csv",
                f"{self.base_url}/api/download-csv"
            ]
            
            for endpoint in endpoints_to_try:
                try:
                    logger.info(f"Trying to download CSV from: {endpoint}")
                    response = requests.get(endpoint, timeout=10)
                    if response.status_code == 200 and len(response.text) > 0:
                        logger.info(f"CSV downloaded successfully from {endpoint} ({len(response.text)} bytes)")
                        return response.text
                except requests.RequestException as e:
                    logger.debug(f"Failed to download CSV from {endpoint}: {e}")
                    continue
            
            logger.error("Failed to download CSV from all attempted endpoints")
            return None
            
        except Exception as e:
            logger.error(f"Unexpected error downloading CSV: {str(e)}")
            return None
    
    def toggle_sensor(self, sensor_id: int, enabled: bool) -> bool:
        """Enable/disable a sensor on ESP8266"""
        if not self.base_url:
            logger.error("ESP8266 not connected")
            return False
        
        if sensor_id not in [1, 2, 3, 4]:
            logger.error(f"Invalid sensor ID: {sensor_id}")
            return False
        
        try:
            action = "on" if enabled else "off"
            response = requests.post(f"{self.base_url}/sensor/{sensor_id}/{action}", timeout=self.timeout)
            return response.status_code == 200
        except requests.RequestException as e:
            logger.error(f"Failed to toggle sensor {sensor_id}: {str(e)}")
            return False
    
    def get_system_info(self) -> Dict[str, str]:
        """Get ESP8266 system information from root page"""
        if not self.base_url:
            return {"status": "Not connected"}
        
        try:
            # Try to get some basic info
            response = requests.get(f"{self.base_url}/", timeout=self.timeout)
            if response.status_code == 200:
                return {
                    "status": "Connected",
                    "ip": self.ip_address,
                    "response_size": f"{len(response.content)} bytes"
                }
            else:
                return {"status": f"HTTP {response.status_code}"}
        except requests.RequestException as e:
            return {"status": f"Error: {str(e)}"}