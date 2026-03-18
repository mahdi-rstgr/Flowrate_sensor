#!/usr/bin/env python3
"""
Raspberry Pi Specific Configuration for Integrated Flow Control System
Handles device detection and setup for your specific hardware configuration

Your Hardware Setup:
- Arduino Uno: Bus 001 Device 004: ID 2341:0043 Arduino SA Uno R3 (CDC ACM)
- ESP8266: Bus 003 Device 005: ID 1a86:7523 QinHeng Electronics CH340 serial converter
"""

import subprocess
import re
import logging
import os
import sys

logger = logging.getLogger(__name__)

class RaspberryPiConfig:
    """Configuration and utilities specific to Raspberry Pi setup"""
    
    def __init__(self):
        self.arduino_vid_pid = "2341:0043"
        self.esp8266_vid_pid = "1a86:7523"  # CH340 converter
        
    def detect_usb_devices(self):
        """Detect connected USB devices and their ports"""
        try:
            # Run lsusb to get USB device information
            result = subprocess.run(['lsusb'], capture_output=True, text=True)
            usb_devices = result.stdout.strip().split('\n')
            
            # Run ls /dev/tty* to get available serial ports
            dev_result = subprocess.run(['ls', '/dev/tty*'], capture_output=True, text=True)
            available_ports = [port for port in dev_result.stdout.strip().split('\n') 
                             if 'ttyACM' in port or 'ttyUSB' in port]
            
            device_info = {
                'arduino': None,
                'esp8266_usb': None,
                'available_ports': available_ports,
                'usb_devices': usb_devices
            }
            
            # Check for Arduino
            for device in usb_devices:
                if self.arduino_vid_pid in device:
                    device_info['arduino'] = {
                        'found': True,
                        'device_string': device.strip(),
                        'likely_port': '/dev/ttyACM0'  # Most common for Arduino Uno
                    }
                    logger.info(f"Arduino Uno detected: {device.strip()}")
            
            # Check for ESP8266 (CH340)
            for device in usb_devices:
                if self.esp8266_vid_pid in device:
                    device_info['esp8266_usb'] = {
                        'found': True, 
                        'device_string': device.strip(),
                        'likely_port': '/dev/ttyUSB0'  # Most common for CH340
                    }
                    logger.info(f"CH340 (ESP8266) detected: {device.strip()}")
            
            return device_info
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Error detecting USB devices: {e}")
            return None
    
    def setup_permissions(self):
        """Setup proper permissions for serial port access"""
        try:
            # Add user to dialout group for serial port access
            username = os.getenv('USER')
            if username:
                print(f"Setting up serial port permissions for user: {username}")
                subprocess.run(['sudo', 'usermod', '-a', '-G', 'dialout', username], check=True)
                print("✓ Added user to dialout group")
                print("⚠️  You need to log out and log back in for group changes to take effect")
            
            # Set permissions for specific ports
            common_ports = ['/dev/ttyACM0', '/dev/ttyACM1', '/dev/ttyUSB0', '/dev/ttyUSB1']
            for port in common_ports:
                if os.path.exists(port):
                    subprocess.run(['sudo', 'chmod', '666', port], check=False)
                    print(f"✓ Set permissions for {port}")
                    
        except subprocess.CalledProcessError as e:
            logger.error(f"Error setting up permissions: {e}")
            print("❌ Failed to set up permissions automatically")
            print("Please run manually:")
            print(f"  sudo usermod -a -G dialout {os.getenv('USER', 'your_username')}")
            print("  sudo chmod 666 /dev/ttyACM0 /dev/ttyUSB0")

    def find_device_ports(self):
        """Find the actual serial ports for detected devices"""
        ports = {}
        
        try:
            # Use udevadm to find device paths by hardware ID
            for device_type, vid_pid in [('arduino', self.arduino_vid_pid), ('esp8266', self.esp8266_vid_pid)]:
                vid, pid = vid_pid.split(':')
                
                # Find device by vendor and product ID
                result = subprocess.run([
                    'find', '/dev/serial/by-id/', '-name', f'*{vid.upper()}*{pid.upper()}*'
                ], capture_output=True, text=True, check=False)
                
                if result.stdout.strip():
                    symbolic_link = result.stdout.strip().split('\n')[0]
                    # Resolve symbolic link to actual device
                    real_path = subprocess.run(['readlink', '-f', symbolic_link], 
                                             capture_output=True, text=True)
                    if real_path.stdout.strip():
                        ports[device_type] = real_path.stdout.strip()
                        logger.info(f"Found {device_type} at {ports[device_type]}")
                        
        except subprocess.CalledProcessError:
            logger.debug("Could not use udevadm method, falling back to manual detection")
        
        return ports
    
    def get_recommended_settings(self):
        """Get recommended settings for your Raspberry Pi setup"""
        return {
            'arduino_port': '/dev/ttyACM0',  # Most likely for Arduino Uno
            'arduino_baudrate': 9600,
            'esp8266_ip': '192.168.10.150',  # As set in main.py
            'esp8266_fallback_ports': ['/dev/ttyUSB0', '/dev/ttyUSB1'],
            'recommended_packages': [
                'python3-pip',
                'python3-serial', 
                'python3-requests',
                'python3-flask'
            ]
        }
    
    def install_system_packages(self):
        """Install required system packages for Raspberry Pi"""
        packages = self.get_recommended_settings()['recommended_packages']
        
        try:
            print("Updating package list...")
            subprocess.run(['sudo', 'apt', 'update'], check=True)
            
            print(f"Installing required packages: {' '.join(packages)}")
            subprocess.run(['sudo', 'apt', 'install', '-y'] + packages, check=True)
            print("✓ System packages installed successfully")
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Error installing packages: {e}")
            print("❌ Failed to install packages automatically")
            print("Please run manually:")
            print(f"  sudo apt update")
            print(f"  sudo apt install -y {' '.join(packages)}")

def main():
    """Main configuration setup for Raspberry Pi"""
    print("🍓 Raspberry Pi Configuration for Integrated Flow Control System")
    print("=" * 70)
    
    config = RaspberryPiConfig()
    
    print("\n🔍 Detecting connected devices...")
    device_info = config.detect_usb_devices()
    
    if device_info:
        print("\n📱 USB Device Detection Results:")
        if device_info.get('arduino'):
            print(f"✓ Arduino Uno found: {device_info['arduino']['device_string']}")
        else:
            print("❌ Arduino Uno not found")
            
        if device_info.get('esp8266_usb'): 
            print(f"✓ CH340 (ESP8266) found: {device_info['esp8266_usb']['device_string']}")
        else:
            print("⚠️  CH340 not detected via USB (ESP8266 may be WiFi-only)")
            
        print(f"\n📋 Available serial ports: {device_info['available_ports']}")
    
    print("\n🔧 Finding device ports...")
    ports = config.find_device_ports()
    for device, port in ports.items():
        print(f"✓ {device}: {port}")
    
    print("\n⚙️  Setting up permissions...")
    config.setup_permissions()
    
    print("\n📦 Installing system packages...")
    try:
        config.install_system_packages()
    except KeyboardInterrupt:
        print("\nSkipping package installation...")
    
    print("\n📋 Configuration Summary:")
    settings = config.get_recommended_settings()
    print(f"  Arduino Port: {settings['arduino_port']}")
    print(f"  Arduino Baudrate: {settings['arduino_baudrate']}")
    print(f"  ESP8266 IP: {settings['esp8266_ip']}")
    print(f"  ESP8266 Fallback Ports: {settings['esp8266_fallback_ports']}")
    
    print(f"\n🚀 Next Steps:")
    print(f"  1. Log out and log back in (for group permissions)")
    print(f"  2. Connect your Arduino Uno and ESP8266")
    print(f"  3. Run: python3 start_system.py")
    print(f"  4. Access web interface at: http://localhost:5000")

if __name__ == "__main__":
    main()