#!/usr/bin/env python3
"""
Quick Start Script for Integrated Flow Control System
Run this file to start the complete system with one command
"""

import os
import sys
import subprocess
import platform

#HERE WE GO



def check_python_version():
    """Check if Python version is compatible"""
    version = sys.version_info
    if version.major < 3 or (version.major == 3 and version.minor < 6):
        print("ERROR: Python 3.6+ required")
        print(f"Current version: {version.major}.{version.minor}.{version.micro}")
        print("Please install a newer version of Python")
        return False
    print(f"OK: Python {version.major}.{version.minor}.{version.micro} - OK")
    return True

def check_dependencies():
    """Check if required packages are installed"""
    required_packages = ['flask', 'flask_cors', 'serial', 'requests']
    missing_packages = []
    
    for package in required_packages:
        try:
            if package == 'flask_cors':
                import flask_cors
            elif package == 'serial':
                import serial
            else:
                __import__(package)
            print(f"OK: {package} - OK")
        except ImportError:
            missing_packages.append(package)
            print(f"MISSING: {package} - Missing")
    
    if missing_packages:
        print(f"\nInstalling missing packages...")
        try:
            # Map package names to pip install names
            pip_names = {
                'flask_cors': 'flask-cors',
                'serial': 'pyserial'
            }
            
            for package in missing_packages:
                pip_name = pip_names.get(package, package)
                subprocess.check_call([sys.executable, '-m', 'pip', 'install', pip_name])
                print(f"INSTALLED: {package}")
            return True
        except subprocess.CalledProcessError:
            print("ERROR: Failed to install dependencies automatically")
            print("Please run manually: pip install flask flask-cors pyserial requests")
            return False
    return True

def get_script_directory():
    """Get the directory where this script is located"""
    return os.path.dirname(os.path.abspath(__file__))

def check_file_structure():
    """Verify the expected file structure exists"""
    script_dir = get_script_directory()
    
    required_files = [
        'backend/main.py',
        'backend/serial_handler.py',
        'backend/esp8266_handler.py',
        'frontend/index.html',
        'frontend/style.css',
        'frontend/script.js'
    ]
    
    missing_files = []
    for file_path in required_files:
        full_path = os.path.join(script_dir, file_path)
        if os.path.exists(full_path):
            print(f"OK: {file_path}")
        else:
            missing_files.append(file_path)
            print(f"MISSING: {file_path}")
    
    if missing_files:
        print(f"\nERROR: Missing {len(missing_files)} required files:")
        for file_path in missing_files:
            print(f"   {file_path}")
        print("\nPlease ensure all system files are in the correct locations.")
        return False
    
    return True

def start_system():
    """Start the integrated flow control system"""
    script_dir = get_script_directory()
    backend_dir = os.path.join(script_dir, 'backend')
    main_script = os.path.join(backend_dir, 'main.py')
    
    print("\nStarting Integrated Flow Control System...")
    print("=" * 60)
    
    try:
        # Change to backend directory
        os.chdir(backend_dir)
        
        # Run the main application
        subprocess.run([sys.executable, 'main.py'])
        
    except KeyboardInterrupt:
        print("\n\nSYSTEM STOPPED: System stopped by user")
    except FileNotFoundError:
        print(f"ERROR: Could not find main.py at {main_script}")
    except Exception as e:
        print(f"ERROR: Error starting system: {e}")

def main():
    """Main startup function"""
    print("Integrated Flow Control System - Quick Start")
    print("=" * 60)
    
    # System checks
    print("\n📋 Running system checks...")
    
    if not check_python_version():
        input("Press Enter to exit...")
        return
    
    if not check_dependencies():
        input("Press Enter to exit...")
        return
    
    print("\n📁 Checking file structure...")
    if not check_file_structure():
        input("Press Enter to exit...")
        return
    
    print("\n✅ All checks passed!")
    
    # Show system information
    print(f"\n💻 System Information:")
    print(f"   Platform: {platform.system()} {platform.release()}")
    print(f"   Python: {sys.executable}")
    print(f"   Working Directory: {get_script_directory()}")
    
    print(f"\n🌐 Web Interface Will Be Available At:")
    print(f"   http://localhost:5000")
    print(f"   http://192.168.10.200 (ESP8266 direct)")
    print(f"   Press Ctrl+C to stop the system")
    
    print(f"\n🔧 Troubleshooting:")
    print(f"   ESP8266 connection issues: python debug_esp8266.py")
    print(f"   USB device aliases setup: sudo bash setup_usb_aliases.sh")
    print(f"   Arduino: /dev/arduino_pump | ESP8266: /dev/esp8266_sensors")
    
    input("\nPress Enter to start the In-line Viscometry system...")
    
    start_system()

if __name__ == '__main__':
    main()