#!/usr/bin/env python3
"""
ESP8266 Network Diagnostic Tool for In-line Viscometry System
Run this to troubleshoot ESP8266 static IP connection issues

Usage: python debug_esp8266.py
"""

import socket
import requests
import subprocess
import platform
import time

ESP8266_STATIC_IP = "192.168.10.200"
ESP8266_PORT = 80
EXPECTED_HOSTNAME = "viscometry.local"

def banner():
    print("=" * 60)
    print("ESP8266 NETWORK DIAGNOSTICS - In-line Viscometry System")  
    print("=" * 60)
    print(f"Target IP: {ESP8266_STATIC_IP}")
    print(f"Expected Hostname: {EXPECTED_HOSTNAME}")
    print(f"Port: {ESP8266_PORT}")
    print()

def test_ping():
    """Test basic network connectivity with ping"""
    print("[1] Testing Network Connectivity...")
    
    try:
        if platform.system().lower() == "windows":
            result = subprocess.run(['ping', '-n', '3', ESP8266_STATIC_IP], 
                                  capture_output=True, text=True, timeout=10)
        else:
            result = subprocess.run(['ping', '-c', '3', ESP8266_STATIC_IP], 
                                  capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ PING: ESP8266 responds to ping")
            # Extract ping times if available
            output_lines = result.stdout.split('\n')
            for line in output_lines:
                if 'time=' in line or 'TTL=' in line:
                    print(f"   {line.strip()}")
        else:
            print("❌ PING: ESP8266 not responding to ping")
            print("   This indicates the ESP8266 is not on the network")
            print("   Possible causes:")
            print("   - ESP8266 not powered on")
            print("   - WiFi connection failed") 
            print("   - Wrong static IP configuration")
            print("   - Network/router issues")
            
    except subprocess.TimeoutExpired:
        print("❌ PING: Timeout - network unreachable")
    except Exception as e:
        print(f"❌ PING: Error - {e}")
    print()

def test_port():
    """Test if port 80 is open"""
    print("[2] Testing Port Connectivity...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        result = sock.connect_ex((ESP8266_STATIC_IP, ESP8266_PORT))
        sock.close()
        
        if result == 0:
            print(f"✅ PORT: {ESP8266_PORT} is open and accepting connections")
        else:
            print(f"❌ PORT: {ESP8266_PORT} is not accessible (error code: {result})")
            print("   Possible causes:")
            print("   - ESP8266 web server not running")
            print("   - Wrong port configuration") 
            print("   - Firewall blocking connection")
            
    except Exception as e:
        print(f"❌ PORT: Error testing port - {e}")
    print()

def test_http_endpoints():
    """Test HTTP endpoints"""
    print("[3] Testing HTTP Endpoints...")
    
    endpoints = [
        '/',
        '/api',
        '/status',
        '/sensor',
        '/data'
    ]
    
    success_count = 0
    
    for endpoint in endpoints:
        try:
            url = f"http://{ESP8266_STATIC_IP}{endpoint}"
            response = requests.get(url, timeout=5)
            
            if response.status_code == 200:
                print(f"✅ HTTP: {endpoint} - OK (Status: {response.status_code})")
                
                # Check for expected content
                content = response.text.lower()
                if 'sensor' in content or 'flow' in content or 'viscometry' in content:
                    print(f"   Content indicates this is a flow sensor system")
                    
                success_count += 1
            else:
                print(f"⚠️  HTTP: {endpoint} - Response {response.status_code}")
                
        except requests.exceptions.ConnectTimeout:
            print(f"❌ HTTP: {endpoint} - Connection timeout")
        except requests.exceptions.ConnectionError:
            print(f"❌ HTTP: {endpoint} - Connection refused")
        except Exception as e:
            print(f"❌ HTTP: {endpoint} - Error: {e}")
    
    if success_count > 0:
        print(f"\n✅ {success_count} endpoints responding - ESP8266 web server is running")
    else:
        print("\n❌ No HTTP endpoints responding - ESP8266 web server not accessible")
    print()

def test_hostname():
    """Test mDNS hostname resolution"""
    print("[4] Testing Hostname Resolution...")
    
    try:
        resolved_ip = socket.gethostbyname(EXPECTED_HOSTNAME)
        if resolved_ip == ESP8266_STATIC_IP:
            print(f"✅ mDNS: {EXPECTED_HOSTNAME} resolves to {resolved_ip}")
        else:
            print(f"⚠️  mDNS: {EXPECTED_HOSTNAME} resolves to {resolved_ip}")
            print(f"   Expected: {ESP8266_STATIC_IP}")
            print(f"   This might be another ESP8266 system")
            
    except socket.gaierror:
        print(f"❌ mDNS: {EXPECTED_HOSTNAME} does not resolve")
        print("   Possible causes:")
        print("   - ESP8266 mDNS not configured")
        print("   - Wrong hostname in firmware")
        print("   - mDNS service not running")
    except Exception as e:
        print(f"❌ mDNS: Error resolving hostname - {e}")
    print()

def scan_network():
    """Scan for other ESP8266 devices on network"""
    print("[5] Scanning for Other ESP8266 Devices...")
    print("   This helps identify IP conflicts...")
    
    # Get network range
    network_base = ".".join(ESP8266_STATIC_IP.split(".")[:-1]) + "."
    common_ips = [
        "164", "165", "166", "167", "168", "169", "170", 
        "100", "101", "102", "103", "104", "150", "151", "152"
    ]
    
    found_devices = []
    
    for ip_suffix in common_ips:
        test_ip = network_base + ip_suffix
        if test_ip == ESP8266_STATIC_IP:
            continue  # Skip our target IP
            
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((test_ip, 80))
            sock.close()
            
            if result == 0:
                # Test if it's an ESP8266
                try:
                    response = requests.get(f"http://{test_ip}/", timeout=2)
                    if response.status_code == 200:
                        content = response.text.lower()
                        if 'esp8266' in content or 'sensor' in content or 'flow' in content:
                            found_devices.append(test_ip)
                            print(f"   Found ESP8266-like device at: {test_ip}")
                except:
                    pass
                    
        except:
            pass
    
    if found_devices:
        print(f"\n⚠️  Found {len(found_devices)} other ESP8266-like devices:")
        for device_ip in found_devices:
            print(f"   - {device_ip}")
        print("   This confirms other systems are working on the network")
    else:
        print("\n✅ No other ESP8266 devices found in common IP range")
    print()

def main():
    banner()
    
    print("Running comprehensive ESP8266 diagnostics...")
    print("This will take about 30 seconds...")
    print()
    
    test_ping()
    test_port() 
    test_http_endpoints()
    test_hostname()
    scan_network()
    
    print("=" * 60)
    print("DIAGNOSTIC SUMMARY")
    print("=" * 60)
    print()
    print("If ALL tests pass:")
    print("✅ ESP8266 is working correctly - check backend code")
    print()
    print("If PING fails:")
    print("❌ ESP8266 not on network - check power, WiFi, static IP config")
    print()
    print("If PING works but PORT/HTTP fails:")
    print("❌ ESP8266 connected but web server not running")
    print("   Re-upload firmware or check ESP8266 serial output")
    print()
    print("Next steps:")
    print("1. Upload ESP8266 firmware with correct static IP configuration")
    print("2. Check ESP8266 serial monitor for connection status") 
    print("3. Verify router allows static IP 192.168.10.200")
    print("4. Run: python start_system.py")

if __name__ == "__main__":
    main()