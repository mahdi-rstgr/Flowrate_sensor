#!/usr/bin/env python3
"""
ESP8266 Handler module - manages HTTP communication with ESP8266 flow sensors
Handles automatic discovery, sensor data retrieval, and recording control
"""

import requests
import socket
import logging
import time
from typing import Dict, Any, Optional, Tuple

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
        Scans common IP ranges for ESP8266 web server
        """
        logger.info("Scanning for ESP8266 flow sensors...")
        
        # Common IP addresses to try
        common_ips = [
            "192.168.1.100", "192.168.1.101", "192.168.1.102",
            "192.168.1.150", "192.168.1.200", "192.168.1.201",
            "192.168.0.100", "192.168.0.101", "192.168.0.102", 
            "192.168.4.1",   # ESP8266 AP mode default
            "10.0.0.100", "10.0.0.101", "10.0.0.102"
        ]
        
        # Try mDNS name first
        try:
            import socket
            ip = socket.gethostbyname("flowssensors.local")
            if self._test_esp8266_connection(ip):
                self.ip_address = ip
                self.base_url = f"http://{ip}:{self.port}"
                logger.info(f"ESP8266 found via mDNS at {ip}")
                return True
        except:
            pass
        
        # Try common IP addresses
        for ip in common_ips:
            if self._test_esp8266_connection(ip):
                self.ip_address = ip
                self.base_url = f"http://{ip}:{self.port}"
                logger.info(f"ESP8266 found at {ip}")
                return True
        
        # Try scanning common subnet ranges
        for base in ["192.168.1.", "192.168.0.", "10.0.0."]:
            for i in range(100, 255):
                ip = f"{base}{i}"
                if self._test_esp8266_connection(ip):
                    self.ip_address = ip
                    self.base_url = f"http://{ip}:{self.port}"
                    logger.info(f"ESP8266 found at {ip}")
                    return True
        
        logger.warning("ESP8266 not found on network")
        return False
    
    def _test_esp8266_connection(self, ip: str) -> bool:
        """Test if ESP8266 is responding at given IP"""
        try:
            # Quick socket test first
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((ip, self.port))
            sock.close()
            
            if result != 0:
                return False
            
            # Test HTTP endpoint
            response = requests.get(f"http://{ip}/api", timeout=2)
            if response.status_code == 200:
                data = response.json()
                # Check if it looks like our sensor system
                if 's1' in data or 'run' in data:
                    return True
            return False
        except:
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
    
    def start_recording(self) -> Tuple[bool, str]:
        """Start recording on ESP8266

        Returns:
            Tuple[bool, str]: (success, message)
        """
        if not self.base_url:
            msg = "ESP8266 not connected"
            logger.error(msg)
            return False, msg
        
        try:
            response = requests.get(f"{self.base_url}/start", timeout=self.timeout)
            response.raise_for_status()
            try:
                result = response.json()
                success = result.get('success', False)
                message = result.get('message', '') or ''
                return success, message
            except ValueError:
                # Some firmware versions might return plain text
                msg = f"Unexpected response: {response.text.strip()}"
                logger.error(msg)
                return False, msg
        except requests.RequestException as e:
            msg = f"Failed to start recording: {str(e)}"
            logger.error(msg)
            return False, msg
    
    def stop_recording(self) -> Tuple[bool, str]:
        """Stop recording on ESP8266

        Returns:
            Tuple[bool, str]: (success, message)
        """
        if not self.base_url:
            msg = "ESP8266 not connected"
            logger.error(msg)
            return False, msg
        
        try:
            response = requests.get(f"{self.base_url}/stop", timeout=self.timeout)
            response.raise_for_status()
            try:
                result = response.json()
                success = result.get('success', False)
                message = result.get('message', '') or ''
                return success, message
            except ValueError:
                msg = f"Unexpected response: {response.text.strip()}"
                logger.error(msg)
                return False, msg
        except requests.RequestException as e:
            msg = f"Failed to stop recording: {str(e)}"
            logger.error(msg)
            return False, msg
    
    def download_csv(self) -> Optional[str]:
        """Download CSV data from ESP8266"""
        if not self.base_url:
            logger.error("ESP8266 not connected")
            return None
        
        try:
            response = requests.get(f"{self.base_url}/log.csv", timeout=10)
            response.raise_for_status()
            return response.text
        except requests.RequestException as e:
            logger.error(f"Failed to download CSV: {str(e)}")
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