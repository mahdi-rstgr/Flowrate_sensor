#!/usr/bin/env python3
# Serial Handler module manages serial communication with Arduino for pump control.
import serial
import serial.tools.list_ports
import time
import threading
import logging

logger = logging.getLogger(__name__)

class SerialHandler:
    def __init__(self, baudrate=9600, timeout=2):
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_connection = None
        self.port = None
        self.lock = threading.Lock()  
        
    def find_arduino_port(self):
        logger.info("Scanning for Arduino...")
        ports = serial.tools.list_ports.comports()
        # Common Arduino identifiers
        arduino_identifiers = [
            'Arduino',
            'CH340',  # Common USB-to-serial chip
            'CP210',  # Silicon Labs USB-to-serial
            'FT232',  # FTDI USB-to-serial
            'USB Serial',
            'ttyACM',  # Linux Arduino identifier
            'ttyUSB',  # Linux USB serial identifier
        ]
        
        for port in ports:
            port_info = f"{port.device} - {port.description} - {port.manufacturer}"
            logger.debug(f"Found port: {port_info}")

            for identifier in arduino_identifiers:
                if identifier.lower() in port.description.lower() or \
                   (port.manufacturer and identifier.lower() in port.manufacturer.lower()):
                    logger.info(f"Arduino found on port: {port.device}")
                    return port.device
        
        # If no Arduino-specific port found, try common port names
        common_ports = [
            '/dev/ttyACM0',  # Linux
            '/dev/ttyACM1',
            '/dev/ttyUSB0',
            '/dev/ttyUSB1',
            'COM3',  # Windows
            'COM4',
            'COM5',
            'COM6',
            '/dev/cu.usbmodem*',  # macOS
            '/dev/cu.usbserial*'
        ]
        
        for port_name in common_ports:
            if any(port.device == port_name for port in ports):
                logger.info(f"Trying common Arduino port: {port_name}")
                return port_name
        
        logger.warning("No Arduino port found automatically")
        return None
    
    def connect(self, port=None):
        with self.lock:
            try:
                if self.serial_connection and self.serial_connection.is_open:
                    self.disconnect()
                
                if port is None:
                    port = self.find_arduino_port()
                    if port is None:
                        logger.error("Could not find Arduino port")
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
        with self.lock:
            if not self.serial_connection or not self.serial_connection.is_open:
                return False
            
            return self.test_connection()
    
    def test_connection(self):
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
        if self.port:
            return self.port
        return "Not connected"
    
    def list_available_ports(self):
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
    
    def reconnect(self):
        logger.info("Attempting to reconnect to Arduino...")
        self.disconnect()
        time.sleep(1)  # Brief delay before reconnecting
        return self.connect(self.port)

# Example usage and testing
if __name__ == '__main__':
    # Configure logging for testing
    logging.basicConfig(level=logging.DEBUG)
    
    # Create serial handler
    handler = SerialHandler()
    
    print("Available serial ports:")
    for port in handler.list_available_ports():
        print(f"  {port['device']} - {port['description']} ({port['manufacturer']})")
    
    # Test connection
    if handler.connect():
        print("✓ Connected to Arduino")
        
        # Test commands
        test_commands = [
            "STATUS",
            "START,X,50,10,0",
            "STATUS",
            "STOP,X",
            "STATUS"
        ]
        
        for cmd in test_commands:
            print(f"\nSending: {cmd}")
            response = handler.send_command(cmd)
            print(f"Response: {response}")
            time.sleep(1)
        
        handler.disconnect()
    else:
        print("Failed to connect to Arduino")