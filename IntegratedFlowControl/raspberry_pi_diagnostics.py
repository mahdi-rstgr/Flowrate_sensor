#!/usr/bin/env python3
"""
Raspberry Pi 5.0 Diagnostic Tool for Integrated Flow Control System
Helps troubleshoot Arduino and ESP8266 connection issues
"""

import os
import sys
import subprocess
import time
import logging
from datetime import datetime

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class RPiDiagnostics:
    def __init__(self):
        self.arduino_ports = ['/dev/ttyACM0', '/dev/ttyACM1', '/dev/ttyUSB0', '/dev/ttyUSB1']
        self.test_results = {}
        
    def print_header(self):
        """Print diagnostic header"""
        print("=" * 80)
        print("RASPBERRY PI 5.0 - INTEGRATED FLOW CONTROL DIAGNOSTICS")
        print(f"Diagnostic Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("=" * 80)
        
    def check_system_info(self):
        """Check basic system information"""
        print("\n[1] SYSTEM INFORMATION")
        print("-" * 40)
        
        try:
            # OS Information
            with open('/etc/os-release', 'r') as f:
                for line in f:
                    if line.startswith('PRETTY_NAME'):
                        print(f"OS: {line.split('=')[1].strip().strip('\"')}")
                        break
            
            # Kernel version
            result = subprocess.run(['uname', '-r'], capture_output=True, text=True)
            if result.returncode == 0:
                print(f"Kernel: {result.stdout.strip()}")
            
            # Python version
            print(f"Python: {sys.version.split()[0]}")
            
            # Hardware info
            try:
                with open('/proc/device-tree/model', 'r') as f:
                    model = f.read().strip('\x00')
                    print(f"Hardware: {model}")
            except:
                print("Hardware: Unknown")
                
            # Network info
            result = subprocess.run(['hostname', '-I'], capture_output=True, text=True)
            if result.returncode == 0:
                print(f"IP Address: {result.stdout.strip()}")
                
        except Exception as e:
            print(f"Error getting system info: {e}")
    
    def check_usb_devices(self):
        """Check USB devices and serial ports"""
        print("\n[2] USB DEVICES & SERIAL PORTS")
        print("-" * 40)
        
        # List USB devices
        try:
            result = subprocess.run(['lsusb'], capture_output=True, text=True)
            if result.returncode == 0:
                print("USB Devices:")
                for line in result.stdout.strip().split('\n'):
                    if 'Arduino' in line or '2341' in line:
                        print(f"  FOUND ARDUINO: {line}")
                    elif 'CH340' in line or '1a86' in line:
                        print(f"  FOUND CH340: {line}")
                    else:
                        print(f"  {line}")
            else:
                print("Error running lsusb")
        except Exception as e:
            print(f"Error checking USB devices: {e}")
        
        # Check serial ports
        print("\nSerial Ports:")
        for port in self.arduino_ports:
            if os.path.exists(port):
                try:
                    stat_info = os.stat(port)
                    print(f"  {port}: EXISTS")
                    print(f"    Permissions: {oct(stat_info.st_mode)[-3:]}")
                    print(f"    Readable: {os.access(port, os.R_OK)}")
                    print(f"    Writable: {os.access(port, os.W_OK)}")
                except Exception as e:
                    print(f"    Error: {e}")
            else:
                print(f"  {port}: NOT FOUND")
    
    def check_permissions(self):
        """Check user permissions for serial access"""
        print("\n[3] PERMISSIONS CHECK")
        print("-" * 40)
        
        try:
            # Check if user is in dialout group
            result = subprocess.run(['groups'], capture_output=True, text=True)
            if result.returncode == 0:
                groups = result.stdout.strip()
                print(f"User groups: {groups}")
                if 'dialout' in groups:
                    print("  ✓ User is in dialout group")
                else:
                    print("  ✗ User is NOT in dialout group")
                    print("  Fix: sudo usermod -a -G dialout $USER")
                    print("       Then logout and login again")
        except Exception as e:
            print(f"Error checking permissions: {e}")
    
    def test_arduino_connection(self):
        """Test Arduino connection on each port"""
        print("\n[4] ARDUINO CONNECTION TEST")
        print("-" * 40)
        
        try:
            import serial
            import serial.tools.list_ports
            
            # List all serial ports
            ports = list(serial.tools.list_ports.comports())
            print("Available serial ports:")
            for port in ports:
                vid_pid = ""
                if hasattr(port, 'vid') and hasattr(port, 'pid') and port.vid and port.pid:
                    vid_pid = f" (VID:PID = {port.vid:04x}:{port.pid:04x})"
                print(f"  {port.device} - {port.description}{vid_pid}")
            
            # Test each potential Arduino port
            for port_name in self.arduino_ports:
                if os.path.exists(port_name):
                    print(f"\nTesting {port_name}...")
                    success = self._test_arduino_on_port(port_name)
                    self.test_results[port_name] = success
                    if success:
                        print(f"  ✓ ARDUINO FOUND on {port_name}")
                    else:
                        print(f"  ✗ No Arduino response on {port_name}")
                        
        except ImportError:
            print("pyserial not installed. Install with: pip3 install pyserial")
        except Exception as e:
            print(f"Error during Arduino test: {e}")
    
    def _test_arduino_on_port(self, port_name):
        """Test if Arduino is on specific port"""
        try:
            import serial
            
            # Open connection
            ser = serial.Serial(port=port_name, baudrate=9600, timeout=2)
            time.sleep(2)  # Wait for Arduino to initialize
            
            # Clear buffers
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Send STATUS command
            ser.write(b"STATUS\n")
            ser.flush()
            time.sleep(0.5)
            
            # Read response
            if ser.in_waiting > 0:
                response = ser.readline().decode('utf-8').strip()
                ser.close()
                
                # Check if response looks like our pump controller JSON
                if response.startswith('{"') and '"X"' in response and '"Y"' in response:
                    print(f"    Response: {response[:60]}...")
                    return True
                else:
                    print(f"    Unexpected response: {response[:60]}")
            else:
                print(f"    No response received")
            
            ser.close()
            return False
            
        except Exception as e:
            print(f"    Error: {e}")
            return False
    
    def test_network_connectivity(self):
        """Test network connectivity and ESP8266 discovery"""
        print("\n[5] NETWORK & ESP8266 TEST")
        print("-" * 40)
        
        # Test internet connectivity
        try:
            result = subprocess.run(['ping', '-c', '1', '8.8.8.8'], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                print("✓ Internet connectivity: OK")
            else:
                print("✗ Internet connectivity: FAILED")
        except Exception as e:
            print(f"✗ Internet connectivity test failed: {e}")
        
        # Test local network
        try:
            result = subprocess.run(['hostname', '-I'], capture_output=True, text=True)
            if result.returncode == 0:
                pi_ip = result.stdout.strip().split()[0]
                print(f"Raspberry Pi IP: {pi_ip}")
                
                # Extract network base
                if '.' in pi_ip:
                    ip_parts = pi_ip.split('.')
                    if len(ip_parts) >= 3:
                        network_base = f"{ip_parts[0]}.{ip_parts[1]}.{ip_parts[2]}.1"
                        print(f"Testing gateway: {network_base}")
                        
                        result = subprocess.run(['ping', '-c', '1', network_base], 
                                              capture_output=True, text=True, timeout=3)
                        if result.returncode == 0:
                            print("✓ Local network: OK")
                        else:
                            print("✗ Local network: NO RESPONSE")
                            
                # Quick ESP8266 scan
                print("\nScanning for ESP8266 (common IPs)...")
                common_ips = [f"{ip_parts[0]}.{ip_parts[1]}.{ip_parts[2]}.{i}" for i in [100, 101, 102, 150, 200]]
                
                for ip in common_ips:
                    try:
                        result = subprocess.run(['ping', '-c', '1', '-W', '1', ip], 
                                              capture_output=True, text=True, timeout=2)
                        if result.returncode == 0:
                            print(f"  ✓ Device found at {ip}")
                            # Try HTTP test
                            try:
                                import requests
                                response = requests.get(f"http://{ip}", timeout=2)
                                if response.status_code == 200:
                                    print(f"    HTTP server responding (possible ESP8266)")
                            except:
                                pass
                        else:
                            print(f"  ✗ No device at {ip}")
                    except:
                        print(f"  ✗ Timeout testing {ip}")
                        
        except Exception as e:
            print(f"Network test error: {e}")
    
    def check_python_dependencies(self):
        """Check required Python packages"""
        print("\n[6] PYTHON DEPENDENCIES")
        print("-" * 40)
        
        required_packages = [
            'flask', 'flask_cors', 'requests', 'serial'
        ]
        
        for package in required_packages:
            try:
                if package == 'flask_cors':
                    import flask_cors
                elif package == 'serial':
                    import serial
                elif package == 'requests':
                    import requests
                elif package == 'flask':
                    import flask
                else:
                    __import__(package)
                print(f"  ✓ {package}")
            except ImportError:
                print(f"  ✗ {package} - MISSING")
                print(f"    Install: pip3 install {package}")
    
    def print_summary(self):
        """Print diagnostic summary and recommendations"""
        print("\n" + "=" * 80)
        print("DIAGNOSTIC SUMMARY & RECOMMENDATIONS")
        print("=" * 80)
        
        # Arduino summary
        arduino_found = any(self.test_results.values()) if hasattr(self, 'test_results') else False
        
        if arduino_found:
            working_port = [port for port, result in self.test_results.items() if result][0]
            print(f"✓ ARDUINO: Found on {working_port}")
            print("  - Pump control should work properly")
        else:
            print("✗ ARDUINO: Not found or not responding")
            print("  Required actions:")
            print("    1. Connect Arduino Uno via USB")
            print("    2. Upload pump controller firmware")
            print("    3. Check: sudo usermod -a -G dialout $USER")
            print("    4. Logout and login to apply group changes")
            
        print("\nFor ESP8266:")
        print("  1. Program ESP8266 with flow sensor firmware")
        print("  2. Configure ESP8266 WiFi (same network as Pi)")
        print("  3. Note ESP8266 IP address during boot")
        print("  4. Set ESP8266_IP in main.py if auto-discovery fails")
        
        print("\nTo start the system:")
        print("  cd IntegratedFlowControl")
        print("  python3 start_system.py")
        
        print("\nFor troubleshooting, run this diagnostic again:")
        print("  python3 raspberry_pi_diagnostics.py")

    def run_full_diagnostic(self):
        """Run complete diagnostic suite"""
        self.print_header()
        self.check_system_info()
        self.check_usb_devices() 
        self.check_permissions()
        self.test_arduino_connection()
        self.test_network_connectivity()
        self.check_python_dependencies()
        self.print_summary()

def main():
    """Main function"""
    try:
        diagnostics = RPiDiagnostics()
        diagnostics.run_full_diagnostic()
    except KeyboardInterrupt:
        print("\n\nDiagnostic interrupted by user")
    except Exception as e:
        print(f"\nError during diagnostics: {e}")
        logger.exception("Diagnostic error")

if __name__ == '__main__':
    main()