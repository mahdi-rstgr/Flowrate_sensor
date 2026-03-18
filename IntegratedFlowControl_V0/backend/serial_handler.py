#!/usr/bin/env python3
"""
Serial Handler module manages serial communication with Arduino for pump control.
Enhanced for integrated flow control system with Raspberry Pi support.
"""
import serial
import serial.tools.list_ports
import time
import threading
import logging
import glob
import os

logger = logging.getLogger(__name__)

class SerialHandler:
    def __init__(self, baudrate=9600, timeout=2):
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_connection = None
        self.port = None
        self.lock = threading.Lock()  
        
    def find_arduino_port(self):
        """Find Arduino port automatically with specific USB device ID support for Raspberry Pi"""
        logger.info("Scanning for Arduino on Raspberry Pi...")
        ports = serial.tools.list_ports.comports()
        
        # Specific hardware identifiers for your Raspberry Pi setup
        arduino_hardware_ids = [
            ('2341', '0043'),  # Arduino SA Uno R3 (CDC ACM) - your specific device
            ('1a86', '7523'),  # QinHeng Electronics CH340 - backup option
        ]
        
        # First, try to find Arduino by exact hardware ID
        for port in ports:
            if hasattr(port, 'vid') and hasattr(port, 'pid'):
                vid_hex = f"{port.vid:04x}" if port.vid else ""
                pid_hex = f"{port.pid:04x}" if port.pid else ""
                
                for target_vid, target_pid in arduino_hardware_ids:
                    if vid_hex == target_vid and pid_hex == target_pid:
                        logger.info(f"Arduino found by hardware ID {target_vid}:{target_pid} on port: {port.device}")
                        return port.device
        
        # Alternative method: check by serial number or manufacturer for your specific devices
        for port in ports:
            port_info = f"{port.device} - {port.description}"
            if hasattr(port, 'manufacturer'):
                port_info += f" - {port.manufacturer}"
            if hasattr(port, 'serial_number'):
                port_info += f" - SN:{port.serial_number}"
            logger.debug(f"Found port: {port_info}")

            # Look for Arduino Uno R3 specifically
            if 'Arduino' in str(port.description) and ('Uno' in str(port.description) or 'CDC ACM' in str(port.description)):
                logger.info(f"Arduino Uno R3 found on port: {port.device}")
                return port.device
                
            # Look for CH340 serial converter (could be ESP8266 or Arduino)
            if 'CH340' in str(port.description) or '1a86:7523' in str(port.description):
                logger.info(f"CH340 device found on port: {port.device}")
                # We'll prefer the Arduino Uno, but this could be a fallback
                return port.device
        
        # Raspberry Pi specific port patterns
        rpi_arduino_ports = [
            '/dev/ttyACM0',    # Most common for Arduino Uno on Raspberry Pi
            '/dev/ttyACM1',
            '/dev/ttyUSB0',    # Common for CH340 devices
            '/dev/ttyUSB1',
            '/dev/serial/by-id/usb-Arduino__www.arduino.cc__0043_*',  # Arduino by ID
            '/dev/serial/by-id/usb-1a86_USB2.0-Serial-*',            # CH340 by ID
        ]
        
        # Try Raspberry Pi specific ports
        import glob
        for port_pattern in rpi_arduino_ports:
            if '*' in port_pattern:
                matching_ports = glob.glob(port_pattern)
                for port_name in matching_ports:
                    if any(port.device == port_name for port in ports):
                        logger.info(f"Arduino found via pattern match: {port_name}")
                        return port_name
            else:
                if any(port.device == port_pattern for port in ports):
                    logger.info(f"Arduino found on standard Raspberry Pi port: {port_pattern}")
                    return port_pattern
        
        logger.warning("No Arduino port found automatically")
        logger.info("Available ports:")
        for port in ports:
            vid_pid = ""
            if hasattr(port, 'vid') and hasattr(port, 'pid') and port.vid and port.pid:
                vid_pid = f" (VID:PID = {port.vid:04x}:{port.pid:04x})"
            logger.info(f"  {port.device} - {port.description}{vid_pid}")
        
        return None
    
    def connect_by_device_id(self, vid_pid="2341:0043"):
        """Connect to Arduino by specific USB device ID (VID:PID)"""
        logger.info(f"Looking for device with VID:PID = {vid_pid}")
        
        ports = serial.tools.list_ports.comports()
        target_vid, target_pid = vid_pid.split(':')
        
        for port in ports:
            if hasattr(port, 'vid') and hasattr(port, 'pid') and port.vid and port.pid:
                vid_hex = f"{port.vid:04x}"
                pid_hex = f"{port.pid:04x}"
                
                if vid_hex == target_vid and pid_hex == target_pid:
                    logger.info(f"Found device {vid_pid} on {port.device}")
                    return self.connect(port.device)
        
        logger.warning(f"Device with VID:PID {vid_pid} not found")
        return False
    
    def get_rpi_device_status(self):
        """Get detailed device status for Raspberry Pi troubleshooting"""
        status = {
            'ports_found': [],
            'arduino_candidates': [],
            'permissions': {},
            'system_info': {}
        }
        
        try:
            # List all serial ports
            ports = serial.tools.list_ports.comports()
            for port in ports:
                port_info = {
                    'device': port.device,
                    'description': port.description,
                    'manufacturer': getattr(port, 'manufacturer', 'Unknown'),
                    'vid_pid': None
                }
                
                if hasattr(port, 'vid') and hasattr(port, 'pid') and port.vid and port.pid:
                    port_info['vid_pid'] = f"{port.vid:04x}:{port.pid:04x}"
                    
                    # Check if this looks like Arduino
                    if port_info['vid_pid'] in ['2341:0043', '1a86:7523']:
                        status['arduino_candidates'].append(port_info)
                
                status['ports_found'].append(port_info)
            
            # Check port permissions
            import os
            import stat
            common_ports = ['/dev/ttyACM0', '/dev/ttyUSB0']
            for port_path in common_ports:
                if os.path.exists(port_path):
                    file_stat = os.stat(port_path)
                    status['permissions'][port_path] = {
                        'exists': True,
                        'readable': os.access(port_path, os.R_OK),
                        'writable': os.access(port_path, os.W_OK),
                        'mode': oct(stat.S_IMODE(file_stat.st_mode))
                    }
                else:
                    status['permissions'][port_path] = {'exists': False}
                    
        except Exception as e:
            logger.error(f"Error getting device status: {e}")
            status['error'] = str(e)
        
        return status
    
    def connect(self, port=None):
        """Connect to Arduino with enhanced Raspberry Pi support"""
        with self.lock:
            try:
                if self.serial_connection and self.serial_connection.is_open:
                    self.disconnect()
                
                if port is None:
                    port = self.find_arduino_port()
                    if port is None:
                        logger.error("Could not find Arduino port")
                        logger.info("Try specifying the port manually:")
                        logger.info("  For Arduino Uno: /dev/ttyACM0")
                        logger.info("  For CH340 devices: /dev/ttyUSB0")
                        logger.info("  Run 'python3 raspberry_pi_config.py' for device detection")
                        return False
                
                # Attempt to connect
                logger.info(f"Attempting to connect to Arduino on {port}")
                self.serial_connection = serial.Serial(
                    port=port,
                    baudrate=self.baudrate,
                    timeout=self.timeout,
                    write_timeout=self.timeout
                )
                
                # Wait for Arduino to initialize
                time.sleep(2)
                
                # Test connection by sending a status request
                if self.test_connection():
                    self.port = port
                    logger.info(f"Successfully connected to Arduino on {port}")
                    return True
                else:
                    logger.error("Arduino not responding to test command")
                    self.disconnect()
                    return False
                    
            except serial.SerialException as e:
                logger.error(f"Serial connection error: {e}")
                return False
            except Exception as e:
                logger.error(f"Unexpected error connecting to Arduino: {e}")
                return False
    
    def disconnect(self):
        """Disconnect from Arduino"""
        with self.lock:
            if self.serial_connection and self.serial_connection.is_open:
                try:
                    self.serial_connection.close()
                    logger.info("Disconnected from Arduino")
                except Exception as e:
                    logger.error(f"Error disconnecting: {e}")
            
            self.serial_connection = None
            self.port = None
    
    def is_connected(self):
        """Check if Arduino is connected and responding"""
        with self.lock:
            if not self.serial_connection or not self.serial_connection.is_open:
                return False
            
            return self.test_connection()
    
    def test_connection(self):
        """Test connection by sending STATUS command"""
        try:
            # Clear any pending data
            if self.serial_connection.in_waiting > 0:
                self.serial_connection.read_all()
            
            # Send test command
            self.serial_connection.write(b"STATUS\n")
            self.serial_connection.flush()
            
            # Wait for response
            start_time = time.time()
            while time.time() - start_time < self.timeout:
                if self.serial_connection.in_waiting > 0:
                    response = self.serial_connection.readline().decode('utf-8').strip()
                    if response.startswith('{') and response.endswith('}'):
                        return True
                time.sleep(0.1)
            
            return False
            
        except Exception as e:
            logger.error(f"Error testing connection: {e}")
            return False
    
    def send_command(self, command):
        """Send command to Arduino and get response"""
        with self.lock:
            if not self.serial_connection or not self.serial_connection.is_open:
                logger.error("Arduino not connected")
                return "ERROR: Arduino not connected"
            
            try:
                # Clear any pending input
                if self.serial_connection.in_waiting > 0:
                    self.serial_connection.read_all()
                
                # Send command with proper formatting
                command_bytes = (command + '\n').encode('utf-8')
                self.serial_connection.write(command_bytes)
                self.serial_connection.flush()
                
                logger.debug(f"Sent command: {command}")
                
                # Wait for response - read multiple lines if needed
                response_lines = []
                start_time = time.time()
                
                while time.time() - start_time < self.timeout:
                    if self.serial_connection.in_waiting > 0:
                        line = self.serial_connection.readline().decode('utf-8').strip()
                        if line:  # Skip empty lines
                            logger.debug(f"Received line: {line}")
                            response_lines.append(line)
                            
                            # For STATUS command, look for JSON response
                            if command == "STATUS" and line.startswith('{') and line.endswith('}'):
                                return line
                            
                            # For other commands, look for OK or ERROR
                            if line == "OK" or line.startswith("ERROR"):
                                return line
                    
                    time.sleep(0.05)  # Small delay to prevent busy waiting
                
                # If we got some response but not the expected format, return the last line
                if response_lines:
                    return response_lines[-1]
                
                logger.error(f"Timeout waiting for response to command: {command}")
                return "ERROR: Timeout"
                
            except serial.SerialException as e:
                logger.error(f"Serial error sending command '{command}': {e}")
                return f"ERROR: Serial error - {e}"
            except Exception as e:
                logger.error(f"Unexpected error sending command '{command}': {e}")
                return f"ERROR: {e}"
    
    def get_port_info(self):
        """Get current port information"""
        if self.port:
            return self.port
        return "Not connected"
    
    def list_available_ports(self):
        """List all available serial ports"""
        ports = serial.tools.list_ports.comports()
        port_list = []
        
        for port in ports:
            port_info = {
                'device': port.device,
                'description': port.description,
                'manufacturer': port.manufacturer or 'Unknown'
            }
            port_list.append(port_info)
        
        return port_list