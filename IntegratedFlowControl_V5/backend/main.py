#!/usr/bin/env python3
"""
Integrated Flow Control System - Main Flask Backend
Combines flow sensor monitoring (ESP8266) with pump control (Arduino)
Provides unified web interface for both systems

QUICK SETUP:
1. If ESP8266 connection fails, edit line 26: ESP8266_IP = "192.168.1.XXX"
2. Replace XXX with your ESP8266's actual IP address
3. Save and restart the system
"""

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import os
import json
import logging
import requests
import time
import threading
from datetime import datetime
from serial_handler import SerialHandler
from esp8266_handler import ESP8266Handler

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')
logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Initialize hardware handlers
serial_handler = SerialHandler()
esp8266_handler = ESP8266Handler()

# ============================================================================
# CONFIGURATION - Modify these settings as needed
# ============================================================================

# ESP8266 Configuration for Raspberry Pi 5.0
# ESP8266 communicates via WiFi/HTTP (connect to /dev/ttyUSB0 for programming only)
# Arduino Uno communicates via serial on /dev/ttyACM0
# Set ESP8266_IP to your ESP8266's IP address if auto-discovery fails
ESP8266_IP = "192.168.10.164"  # Your current ESP8266 IP

# Automatic IP monitoring and recovery (when ESP8266_IP is set)
# If connection fails, system will try to discover new IP automatically
ESP8266_AUTO_RECOVERY = False    # Enable automatic IP discovery on connection failure
ESP8266_MONITOR_INTERVAL = 30   # Check connection every 30 seconds

# IMPORTANT: ESP8266 Setup Instructions
# 1. Program ESP8266 via USB (/dev/ttyUSB0) with flow sensor firmware
# 2. Configure ESP8266 WiFi to connect to same network as Raspberry Pi
# 3. ESP8266 will get IP from router - check router admin or ESP8266 serial output
# 4. For STATIC IP: Set USE_STATIC_IP = true in ESP8266 firmware (recommended)
# 5. For DHCP: Enable auto-recovery in this file (ESP8266_AUTO_RECOVERY = True)
# 4. This system will auto-discover ESP8266 IP if ESP8266_IP is None

# General Configuration
FRONTEND_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'frontend')
DEFAULT_PORT = 5000

# Pump name mapping: 1,2,3,4 -> X,Y,Z,A for Arduino compatibility
PUMP_MAPPING = {'1': 'X', '2': 'Y', '3': 'Z', '4': 'A'}
PUMP_REVERSE_MAPPING = {v: k for k, v in PUMP_MAPPING.items()}

@app.route('/')
def index():
    """Serve the unified dashboard"""
    try:
        return send_from_directory(FRONTEND_DIR, 'index.html')
    except FileNotFoundError:
        return "Frontend files not found. Please ensure index.html is in the frontend directory.", 404

@app.route('/<path:filename>')
def serve_static(filename):
    """Serve static files (CSS, JS, images)"""
    try:
        return send_from_directory(FRONTEND_DIR, filename)
    except FileNotFoundError:
        return f"File {filename} not found.", 404

# =============================================================================
# FLOW SENSOR ENDPOINTS (ESP8266 Integration)
# =============================================================================

@app.route('/api/esp8266/status', methods=['GET'])
def get_esp8266_status():
    """Get ESP8266 connection status and IP information"""
    try:
        if esp8266_handler.ip_address:
            # Test connection
            try:
                sensor_data = esp8266_handler.get_sensor_data()
                return jsonify({
                    'connected': True,
                    'ip_address': esp8266_handler.ip_address,
                    'base_url': esp8266_handler.base_url,
                    'mDNS': 'flowsensors.local',
                    'last_test': datetime.now().isoformat(),
                    'status': 'Connected and responsive'
                })
            except Exception as e:
                return jsonify({
                    'connected': False,
                    'ip_address': esp8266_handler.ip_address,
                    'base_url': esp8266_handler.base_url,
                    'error': str(e),
                    'last_test': datetime.now().isoformat(),
                    'status': 'IP configured but not responding'
                }), 500
        else:
            return jsonify({
                'connected': False,
                'ip_address': None,
                'base_url': None,
                'status': 'Not connected',
                'message': 'ESP8266 not found. Check network connection and IP configuration.'
            }), 500
    except Exception as e:
        return jsonify({
            'connected': False,
            'error': str(e),
            'status': 'Connection check failed'
        }), 500

@app.route('/api/esp8266/rediscover', methods=['POST'])  
def rediscover_esp8266():
    """Force rediscovery of ESP8266"""
    try:
        logger.info("Manual ESP8266 rediscovery requested")
        if esp8266_handler.discover_and_connect():
            new_ip = esp8266_handler.get_ip_address()
            logger.info(f"ESP8266 rediscovered at: {new_ip}")
            
            # Update global IP configuration
            global ESP8266_IP
            old_ip = ESP8266_IP
            ESP8266_IP = new_ip
            
            return jsonify({
                'success': True,
                'ip_address': new_ip,
                'old_ip': old_ip,
                'message': f'ESP8266 rediscovered at {new_ip}' + 
                          (f' (changed from {old_ip})' if old_ip != new_ip else '')
            })
        else:
            return jsonify({
                'success': False,
                'message': 'ESP8266 not found on network'
            }), 404
    except Exception as e:
        logger.error(f"Error in manual ESP8266 rediscovery: {e}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/esp8266/storage', methods=['GET'])
def get_esp8266_storage_status():
    """Get ESP8266 storage status information"""
    try:
        if not esp8266_handler.ip_address:
            return jsonify({
                'error': 'ESP8266 not connected'
            }), 503
        
        # Get sensor data which includes storage info
        sensor_data = esp8266_handler.get_sensor_data()
        
        if 'storage' in sensor_data:
            storage_info = sensor_data['storage']
            return jsonify({
                'used_bytes': storage_info.get('used_bytes', 0),
                'total_bytes': storage_info.get('total_bytes', 1),
                'used_kb': storage_info.get('used_kb', 0),
                'total_kb': storage_info.get('total_kb', 0),
                'percent_used': storage_info.get('percent_used', 0)
            })
        else:
            return jsonify({
                'error': 'Storage information not available from ESP8266'
            }), 404
            
    except Exception as e:
        logger.error(f"Error getting ESP8266 storage status: {e}")
        return jsonify({
            'error': str(e)
        }), 500

@app.route('/api/esp8266/storage/clean', methods=['POST'])
def clean_esp8266_storage():
    """Clean ESP8266 storage by removing all data files"""
    try:
        if not esp8266_handler.ip_address:
            return jsonify({
                'error': 'ESP8266 not connected'
            }), 503
        
        # Call ESP8266 cleanup endpoint
        cleanup_url = f"http://{esp8266_handler.ip_address}/cleanup"
        response = requests.post(cleanup_url, timeout=10)
        
        if response.status_code == 200:
            cleanup_result = response.text
            logger.info(f"ESP8266 storage cleaned successfully: {cleanup_result}")
            return cleanup_result
        else:
            error_msg = f"ESP8266 cleanup failed: HTTP {response.status_code}"
            logger.error(error_msg)
            return error_msg, response.status_code
            
    except requests.RequestException as e:
        error_msg = f"Failed to communicate with ESP8266: {str(e)}"
        logger.error(error_msg)
        return error_msg, 500
    except Exception as e:
        error_msg = f"Error cleaning ESP8266 storage: {str(e)}"
        logger.error(error_msg)
        return error_msg, 500

@app.route('/api/flow-sensors', methods=['GET'])
def get_flow_sensors():
    """Get real-time flow sensor data from ESP8266"""
    try:
        sensor_data = esp8266_handler.get_sensor_data()
        return jsonify({
            'success': True,
            'data': sensor_data,
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        logger.error(f"Error getting flow sensor data: {str(e)}")
        return jsonify({
            'success': False,
            'error': f'ESP8266 communication error: {str(e)}',
            'timestamp': datetime.now().isoformat()
        }), 500

@app.route('/api/sensors/start-recording', methods=['POST'])
def start_sensor_recording():
    """Start recording on ESP8266"""
    try:
        result = esp8266_handler.start_recording()
        return jsonify({
            'success': result,
            'message': 'Recording started' if result else 'Failed to start recording'
        })
    except Exception as e:
        logger.error(f"Error starting recording: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

@app.route('/api/sensors/stop-recording', methods=['POST'])
def stop_sensor_recording():
    """Stop recording on ESP8266"""
    try:
        result = esp8266_handler.stop_recording()
        return jsonify({
            'success': result,
            'message': 'Recording stopped' if result else 'Failed to stop recording'
        })
    except Exception as e:
        logger.error(f"Error stopping recording: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

@app.route('/api/sensors/download-csv', methods=['GET'])
def download_sensor_csv():
    """Download CSV data from ESP8266"""
    try:
        csv_data = esp8266_handler.download_csv()
        if csv_data:
            return csv_data, 200, {
                'Content-Type': 'text/csv',
                'Content-Disposition': f'attachment; filename=flow_data_{int(time.time())}.csv'
            }
        else:
            return jsonify({
                'success': False,
                'message': 'No CSV data available'
            }), 404
    except Exception as e:
        logger.error(f"Error downloading CSV: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

@app.route('/api/sensors/toggle/<int:sensor_id>', methods=['POST'])
def toggle_sensor(sensor_id):
    """Enable/disable a sensor on ESP8266"""
    try:
        data = request.get_json()
        enabled = data.get('enabled', True)
        result = esp8266_handler.toggle_sensor(sensor_id, enabled)
        return jsonify({
            'success': result,
            'message': f'Sensor {sensor_id} {"enabled" if enabled else "disabled"}'
        })
    except Exception as e:
        logger.error(f"Error toggling sensor: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

# =============================================================================
# PUMP CONTROL ENDPOINTS (Arduino Integration)
# =============================================================================

@app.route('/api/pumps/start', methods=['POST'])
def start_pump():
    """Start a specific pump with given parameters"""
    try:
        data = request.get_json()
        # Validate required fields
        required_fields = ['pump', 'rpm', 'time', 'continuous']
        for field in required_fields:
            if field not in data:
                return jsonify({
                    'success': False,
                    'message': f'Missing required field: {field}'
                }), 400
        
        pump_id = str(data['pump'])
        rpm = float(data['rpm'])
        time_duration = int(data['time'])
        continuous = int(data['continuous'])
        
        # Validate pump identifier (1,2,3,4)
        if pump_id not in ['1', '2', '3', '4']:
            return jsonify({
                'success': False,
                'message': 'Invalid pump identifier. Must be 1, 2, 3, or 4.'
            }), 400
        
        # Validate RPM
        if abs(rpm) > 2000:
            return jsonify({
                'success': False,
                'message': 'RPM must be between -2000 and 2000.'
            }), 400
        
        # Validate minimum RPM (absolute value must be at least 0.25)
        if rpm != 0 and abs(rpm) < 0.25:
            return jsonify({
                'success': False,
                'message': 'RPM must be 0 or have absolute value of at least 0.25 (absolute value).'
            }), 400
        
        # Validate time for non-continuous operation
        if continuous == 0 and time_duration <= 0:
            return jsonify({
                'success': False,
                'message': 'Time must be positive for non-continuous operation.'
            }), 400
        
        # Convert pump number to Arduino format (1->X, 2->Y, 3->Z, 4->A)
        arduino_pump = PUMP_MAPPING[pump_id]
        
        # Send command to Arduino
        command = f"START,{arduino_pump},{rpm},{time_duration},{continuous}"
        response = serial_handler.send_command(command)
        
        if response == "OK":
            logger.info(f"Started pump {pump_id} at {rpm} RPM")
            return jsonify({
                'success': True,
                'message': f'Pump {pump_id} started successfully'
            })
        else:
            logger.error(f"Arduino error starting pump {pump_id}: {response}")
            return jsonify({
                'success': False,
                'message': f'Arduino error: {response}'
            }), 500
            
    except ValueError as e:
        return jsonify({
            'success': False,
            'message': f'Invalid parameter value: {str(e)}'
        }), 400
    except Exception as e:
        logger.error(f"Error starting pump: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Internal error: {str(e)}'
        }), 500

@app.route('/api/pumps/stop', methods=['POST'])
def stop_pump():
    """Stop a specific pump"""
    try:
        data = request.get_json()
        if 'pump' not in data:
            return jsonify({
                'success': False,
                'message': 'Missing required field: pump'
            }), 400
        
        pump_id = str(data['pump'])
        
        # Validate pump identifier (1,2,3,4)
        if pump_id not in ['1', '2', '3', '4']:
            return jsonify({
                'success': False,
                'message': 'Invalid pump identifier. Must be 1, 2, 3, or 4.'
            }), 400
        
        # Convert pump number to Arduino format
        arduino_pump = PUMP_MAPPING[pump_id]
        
        # Send command to Arduino
        command = f"STOP,{arduino_pump}"
        response = serial_handler.send_command(command)
        
        if response == "OK":
            logger.info(f"Stopped pump {pump_id}")
            return jsonify({
                'success': True,
                'message': f'Pump {pump_id} stopped successfully'
            })
        else:
            logger.error(f"Arduino error stopping pump {pump_id}: {response}")
            return jsonify({
                'success': False,
                'message': f'Arduino error: {response}'
            }), 500
            
    except Exception as e:
        logger.error(f"Error stopping pump: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Internal error: {str(e)}'
        }), 500

@app.route('/api/pumps/emergency-stop', methods=['POST'])
def emergency_stop():
    """Emergency stop - stop all pumps immediately"""
    try:
        # Send emergency command to Arduino
        response = serial_handler.send_command("EMERGENCY")
        
        if response == "OK":
            logger.warning("Emergency stop activated")
            return jsonify({
                'success': True,
                'message': 'Emergency stop activated - All pumps stopped'
            })
        else:
            logger.error(f"Arduino error during emergency stop: {response}")
            return jsonify({
                'success': False,
                'message': f'Arduino error: {response}'
            }), 500
            
    except Exception as e:
        logger.error(f"Error during emergency stop: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Internal error: {str(e)}'
        }), 500

@app.route('/api/pumps/status', methods=['GET'])
def get_pump_status():
    """Get current status of all pumps"""
    try:
        # Request status from Arduino
        response = serial_handler.send_command("STATUS")
        
        if response.startswith('{') and response.endswith('}'):
            # Parse JSON response from Arduino
            try:
                arduino_status = json.loads(response)
                # Convert Arduino pump names (X,Y,Z,A) to numbers (1,2,3,4)
                pump_status = {}
                for arduino_pump, status in arduino_status.items():
                    if arduino_pump in PUMP_REVERSE_MAPPING:
                        pump_number = PUMP_REVERSE_MAPPING[arduino_pump]
                        pump_status[pump_number] = status
                
                return jsonify({
                    'success': True,
                    'status': pump_status,
                    'message': 'Status retrieved successfully'
                })
            except json.JSONDecodeError:
                logger.error(f"Invalid JSON response from Arduino: {response}")
                return jsonify({
                    'success': False,
                    'message': 'Invalid status response from Arduino'
                }), 500
        else:
            logger.error(f"Unexpected status response from Arduino: {response}")
            return jsonify({
                'success': False,
                'message': f'Arduino error: {response}'
            }), 500
            
    except Exception as e:
        logger.error(f"Error getting pump status: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Internal error: {str(e)}'
        }), 500

# =============================================================================
# SYSTEM STATUS ENDPOINTS
# =============================================================================

@app.route('/api/system/network-info', methods=['GET'])
def get_network_info():
    """Get network information for accessing the system from other devices"""
    try:
        import socket
        
        network_info = {
            'hostname': socket.gethostname(),
            'local_urls': [],
            'port': DEFAULT_PORT
        }
        
        # Get all network interfaces
        try:
            # Get local IP address
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            local_ip = s.getsockname()[0]
            s.close()
            network_info['local_urls'].append(f"http://{local_ip}:{DEFAULT_PORT}")
        except Exception:
            pass
        
        # Alternative method to get IP
        try:
            local_ip = socket.gethostbyname(socket.gethostname())
            if local_ip not in [url.split('//')[-1].split(':')[0] for url in network_info['local_urls']]:
                network_info['local_urls'].append(f"http://{local_ip}:{DEFAULT_PORT}")
        except Exception:
            pass
        
        # Add localhost
        network_info['local_urls'].append(f"http://localhost:{DEFAULT_PORT}")
        
        return jsonify({
            'success': True,
            'network_info': network_info,
            'instructions': 'Share any of the network URLs with other devices on the same WiFi network'
        })
    except Exception as e:
        logger.error(f"Error getting network info: {str(e)}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/system/esp8266-debug', methods=['GET'])
def debug_esp8266():
    """Get detailed ESP8266 connection debug information"""
    try:
        debug_info = {
            'configured_ip': ESP8266_IP,
            'current_ip': esp8266_handler.get_ip_address(),
            'base_url': esp8266_handler.base_url,
            'is_connected': esp8266_handler.is_connected(),
            'endpoints_tested': [],
            'sensor_data_test': None,
            'recording_endpoints_test': {}
        }
        
        # Test basic connectivity
        if esp8266_handler.base_url:
            # Test different endpoints
            test_endpoints = ['/api', '/status', '/', '/info']
            for endpoint in test_endpoints:
                try:
                    import requests
                    response = requests.get(f"{esp8266_handler.base_url}{endpoint}", timeout=2)
                    debug_info['endpoints_tested'].append({
                        'endpoint': endpoint,
                        'status_code': response.status_code,
                        'response_length': len(response.text)
                    })
                except Exception as e:
                    debug_info['endpoints_tested'].append({
                        'endpoint': endpoint,
                        'error': str(e)
                    })
            
            # Test sensor data
            try:
                sensor_data = esp8266_handler.get_sensor_data()
                debug_info['sensor_data_test'] = 'Success'
            except Exception as e:
                debug_info['sensor_data_test'] = f'Error: {str(e)}'
            
            # Test recording endpoints
            recording_test_endpoints = ['/start', '/stop', '/api/start-recording', '/api/stop-recording', '/record/start', '/record/stop']
            for endpoint in recording_test_endpoints:
                try:
                    response = requests.get(f"{esp8266_handler.base_url}{endpoint}", timeout=2)
                    debug_info['recording_endpoints_test'][endpoint] = {
                        'status_code': response.status_code,
                        'accessible': True
                    }
                except Exception as e:
                    debug_info['recording_endpoints_test'][endpoint] = {
                        'error': str(e),
                        'accessible': False
                    }
        
        return jsonify({
            'success': True,
            'debug_info': debug_info
        })
    except Exception as e:
        logger.error(f"Error in ESP8266 debug: {str(e)}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/system/status', methods=['GET'])
def get_system_status():
    """Get overall system status (Arduino + ESP8266 connectivity)"""
    try:
        # Check Arduino connection
        arduino_connected = serial_handler.is_connected()
        arduino_port = serial_handler.get_port_info()
        
        # Check ESP8266 connection
        esp8266_connected = esp8266_handler.is_connected()
        esp8266_ip = esp8266_handler.get_ip_address()
        
        return jsonify({
            'success': True,
            'arduino': {
                'connected': arduino_connected,
                'port': arduino_port
            },
            'esp8266': {
                'connected': esp8266_connected,
                'ip': esp8266_ip
            },
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        logger.error(f"Error checking system status: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

@app.route('/api/unified-data', methods=['GET'])
def get_unified_data():
    """Get combined sensor and pump data for unified dashboard"""
    try:
        # Get sensor data
        sensor_data = esp8266_handler.get_sensor_data()
        
        # Get pump status
        pump_response = serial_handler.send_command("STATUS")
        pump_status = {}
        if pump_response.startswith('{') and pump_response.endswith('}'):
            try:
                arduino_status = json.loads(pump_response)
                for arduino_pump, status in arduino_status.items():
                    if arduino_pump in PUMP_REVERSE_MAPPING:
                        pump_number = PUMP_REVERSE_MAPPING[arduino_pump]
                        pump_status[pump_number] = status
            except json.JSONDecodeError:
                logger.warning("Failed to parse pump status")
        
        return jsonify({
            'success': True,
            'sensors': sensor_data,
            'pumps': pump_status,
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        logger.error(f"Error getting unified data: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Error: {str(e)}'
        }), 500

# =============================================================================
# ERROR HANDLERS
# =============================================================================

@app.errorhandler(404)
def not_found(error):
    """Handle 404 errors"""
    return jsonify({
        'success': False,
        'message': 'Endpoint not found'
    }), 404

@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors"""
    return jsonify({
        'success': False,
        'message': 'Internal server error'
    }), 500

# =============================================================================
# MAIN APPLICATION
# =============================================================================

# ============================================================================
# ESP8266 IP MONITORING FUNCTIONALITY
# ============================================================================

def esp8266_ip_monitor():
    """Monitor ESP8266 connection and automatically recover if IP changes"""
    if not ESP8266_AUTO_RECOVERY:
        return
    
    logger.info(f"Starting ESP8266 IP monitor (checking every {ESP8266_MONITOR_INTERVAL}s)")
    
    while True:
        try:
            time.sleep(ESP8266_MONITOR_INTERVAL)
            
            # Test current connection
            if esp8266_handler.ip_address:
                try:
                    sensor_data = esp8266_handler.get_sensor_data()
                    # Connection is working fine
                    continue
                except Exception as e:
                    logger.warning(f"ESP8266 connection lost: {e}")
                    logger.info("Attempting to rediscover ESP8266...")
                    
                    # Try to rediscover ESP8266
                    if esp8266_handler.discover_and_connect():
                        new_ip = esp8266_handler.get_ip_address()
                        logger.info(f"ESP8266 rediscovered at new IP: {new_ip}")
                        
                        # Update the global IP configuration if it changed
                        global ESP8266_IP
                        if ESP8266_IP != new_ip:
                            old_ip = ESP8266_IP
                            ESP8266_IP = new_ip
                            logger.info(f"IP address changed from {old_ip} to {new_ip}")
                            print(f"\n*** ESP8266 IP CHANGED: {old_ip} → {new_ip} ***")
                            print(f"*** System automatically reconnected! ***\n")
                    else:
                        logger.error("Failed to rediscover ESP8266")
                        
        except Exception as e:
            logger.error(f"Error in ESP8266 IP monitor: {e}")
            time.sleep(5)  # Short delay before retrying

def main():
    """Main function to start the integrated flow control system"""
    print("=" * 70)
    print("  INTEGRATED FLOW CONTROL SYSTEM - RASPBERRY PI 5.0")
    print("  Flow Sensors (ESP8266) + Pump Controller (Arduino Uno)")
    print("=" * 70)
    
    # Log system information
    logger.info("Starting Integrated Flow Control System on Raspberry Pi 5.0")
    logger.info(f"Expected connections:")
    logger.info(f"  - Arduino Uno (Pump Controller): /dev/ttyACM0")
    logger.info(f"  - ESP8266 (Flow Sensors): WiFi HTTP communication")
    
    # Initialize Arduino connection
    print("\n[1] Connecting to Arduino Uno (Pump Controller)...")
    print("    Expected port: /dev/ttyACM0")
    if serial_handler.connect():
        print(f"OK: Arduino connected on {serial_handler.get_port_info()}")
        logger.info(f"Arduino Uno successfully connected on {serial_handler.get_port_info()}")
    else:
        print("WARNING: Arduino not connected")
        print("  The web interface will still start, but pump control will not work.")
        print("  Check:")
        print("    - Arduino Uno is connected via USB to Raspberry Pi")
        print("    - Arduino is running pump controller firmware")
        print("    - USB permissions: sudo usermod -a -G dialout $USER")
        logger.warning("Arduino Uno not connected - pump control disabled")
    
    # Initialize ESP8266 connection
    print("\n[2] Connecting to ESP8266 (Flow Sensors via WiFi)...")
    
    connection_success = False
    
    if ESP8266_IP:
        # Use manual IP address
        print(f"    Using configured IP address: {ESP8266_IP}")
        if esp8266_handler.connect(ESP8266_IP):
            print(f"OK: ESP8266 connected at {esp8266_handler.get_ip_address()}")
            logger.info(f"ESP8266 connected via manual IP: {ESP8266_IP}")
            connection_success = True
        else:
            print(f"ERROR: Failed to connect to ESP8266 at {ESP8266_IP}")
            print("  Troubleshooting:")
            print("    - Verify ESP8266 is powered and connected to WiFi")
            print("    - Check IP address is correct")
            print("    - Try: ping {ESP8266_IP}")
            print("    - Check ESP8266 web interface: http://{ESP8266_IP}")
            logger.error(f"Failed to connect to ESP8266 at configured IP {ESP8266_IP}")
    
    if not connection_success:
        # Use automatic discovery
        print("    Auto-discovering ESP8266 on network...")
        print("    This may take 30-60 seconds...")
        if esp8266_handler.discover_and_connect():
            print(f"OK: ESP8266 connected at {esp8266_handler.get_ip_address()}")
            logger.info(f"ESP8266 auto-discovered at: {esp8266_handler.get_ip_address()}")
            connection_success = True
        else:
            print("WARNING: ESP8266 not found")
            print("  The web interface will still start, but sensor monitoring will not work.")
            print("  ESP8266 Setup Required:")
            print("    1. Program ESP8266 with flow sensor firmware")
            print("    2. Configure ESP8266 WiFi (same network as Raspberry Pi)")
            print("    3. Find ESP8266 IP address and set ESP8266_IP in main.py")
            print("    4. Restart this system")
            logger.warning("ESP8266 not found - sensor monitoring disabled")
    
    if connection_success:
        print(f"ESP8266 Network Info:")
        print(f"  - IP Address: {esp8266_handler.get_ip_address()}")
        print(f"  - Base URL: {esp8266_handler.base_url}")
        print(f"  - Test URL: {esp8266_handler.base_url}/api")
        
        # Start IP monitoring thread if auto-recovery is enabled
        if ESP8266_AUTO_RECOVERY:
            print(f"  - IP Monitoring: ENABLED (check every {ESP8266_MONITOR_INTERVAL}s)")
            monitor_thread = threading.Thread(target=esp8266_ip_monitor, daemon=True)
            monitor_thread.start()
            logger.info("ESP8266 IP monitoring thread started")
        else:
            print(f"  - IP Monitoring: DISABLED")
    
    # Check frontend files
    print("\n[3] Checking frontend files...")
    index_path = os.path.join(FRONTEND_DIR, 'index.html')
    if not os.path.exists(index_path):
        print(f"WARNING: Frontend files not found at {FRONTEND_DIR}")
        print("  Ensure the frontend files are in the correct directory.")
    else:
        print(f"OK: Frontend files found at {FRONTEND_DIR}")
    
    print("\n" + "=" * 60)
    print(f"Starting web server on all network interfaces")
    print(f"Local access: http://localhost:{DEFAULT_PORT}")
    
    # Get local IP addresses for network access
    try:
        import socket
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        print(f"Network access: http://{local_ip}:{DEFAULT_PORT}")
        print(f"From other devices on WiFi: http://{local_ip}:{DEFAULT_PORT}")
    except Exception:
        print("Network access: http://[RASPBERRY_PI_IP]:5000")
        print("  (Replace [RASPBERRY_PI_IP] with your Pi's IP address)")
    
    print("Access from any device on the same WiFi network!")
    print("Press Ctrl+C to stop the server")
    print("=" * 60)
    
    try:
        # Start Flask application
        app.run(
            host='0.0.0.0',  # Allow external connections
            port=DEFAULT_PORT,
            debug=False,  # Set to True for development
            threaded=True
        )
    except KeyboardInterrupt:
        print("\n\nSYSTEM STOPPED: Server stopped by user")
    except Exception as e:
        print(f"\nERROR: Error starting server: {e}")
    finally:
        # Clean up connections
        print("\n[Cleanup] Disconnecting from hardware...")
        serial_handler.disconnect()
        print("✓ Serial connection closed")
        print("✓ Integrated Flow Control System shutdown complete")

if __name__ == '__main__':
    main()