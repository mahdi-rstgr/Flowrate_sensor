#!/usr/bin/env python3
# Flask web server that serves the frontend and provides API endpoints for communicating with Arduino via serial connection

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import os
import json
import logging
from serial_handler import SerialHandler

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Initialize serial handler
serial_handler = SerialHandler()

# Configuration
FRONTEND_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'frontend')
DEFAULT_PORT = 5000

@app.route('/')
def index():
    # Serve the main HTML page
    try:
        return send_from_directory(FRONTEND_DIR, 'index.html')
    except FileNotFoundError:
        return "Frontend files not found. Please ensure index.html is in the frontend directory.", 404

@app.route('/<path:filename>')
def serve_static(filename):
    # Serve static files (CSS, JS, images)
    try:
        return send_from_directory(FRONTEND_DIR, filename)
    except FileNotFoundError:
        return f"File {filename} not found.", 404

@app.route('/api/start_pump', methods=['POST'])
def start_pump():
    # Start a specific pump with given parameters
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
        
        pump = data['pump'].upper()
        rpm = float(data['rpm'])
        time = int(data['time'])
        continuous = int(data['continuous'])
        
        # Validate pump identifier
        if pump not in ['X', 'Y', 'Z', 'A']:
            return jsonify({
                'success': False,
                'message': 'Invalid pump identifier.'
            }), 400
        
        # Validate RPM
        if abs(rpm) > 2000:
            return jsonify({
                'success': False,
                'message': 'RPM must be between -2000 and 2000.'
            }), 400
        
        # Validate time for non-continuous operation
        if continuous == 0 and time <= 0:
            return jsonify({
                'success': False,
                'message': 'Time must be positive for non-continuous operation.'
            }), 400
        
        # Send command to Arduino
        command = f"START,{pump},{rpm},{time},{continuous}"
        response = serial_handler.send_command(command)
        
        if response == "OK":
            logger.info(f"Started pump {pump} at {rpm} RPM")
            return jsonify({
                'success': True,
                'message': f'Pump {pump} started successfully'
            })
        else:
            logger.error(f"Arduino error starting pump {pump}: {response}")
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

@app.route('/api/stop_pump', methods=['POST'])
def stop_pump():
    # Stop a specific pump
    try:
        data = request.get_json()
        if 'pump' not in data:
            return jsonify({
                'success': False,
                'message': 'Missing required field: pump'
            }), 400
        pump = data['pump'].upper()
        
        # Validate pump identifier
        if pump not in ['X', 'Y', 'Z', 'A']:
            return jsonify({
                'success': False,
                'message': 'Invalid pump identifier.'
            }), 400
        
        # Send command to Arduino
        command = f"STOP,{pump}"
        response = serial_handler.send_command(command)
        
        if response == "OK":
            logger.info(f"Stopped pump {pump}")
            return jsonify({
                'success': True,
                'message': f'Pump {pump} stopped successfully'
            })
        else:
            logger.error(f"Arduino error stopping pump {pump}: {response}")
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

@app.route('/api/emergency_stop', methods=['POST'])
def emergency_stop():
    # Emergency stop - stop all pumps immediately
    try:
        # Send emergency command to Arduino
        response = serial_handler.send_command("EMERGENCY")
        
        if response == "OK":
            logger.warning("Emergency stop activated")
            return jsonify({
                'success': True,
                'message': 'Emergency stop activated, All pumps stopped'
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

@app.route('/api/status', methods=['GET'])
def get_status():
    # Get current status of all pumps
    try:
        # Request status from Arduino
        response = serial_handler.send_command("STATUS")
        
        if response.startswith('{') and response.endswith('}'):
            # Parse JSON response from Arduino
            try:
                status_data = json.loads(response)
                return jsonify({
                    'success': True,
                    'status': status_data,
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
        logger.error(f"Error getting status: {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Internal error: {str(e)}'
        }), 500

@app.route('/api/connection_status', methods=['GET'])
def connection_status():
    # Check if Arduino is connected and responding
    try:
        is_connected = serial_handler.is_connected()
        return jsonify({
            'success': True,
            'connected': is_connected,
            'port': serial_handler.get_port_info(),
            'message': 'Connected' if is_connected else 'Disconnected'
        })
    except Exception as e:
        logger.error(f"Error checking connection status: {str(e)}")
        return jsonify({
            'success': False,
            'connected': False,
            'message': f'Error: {str(e)}'
        }), 500

@app.errorhandler(404)
def not_found(error):
    # Handle 404 errors 
    return jsonify({
        'success': False,
        'message': 'Endpoint not found'
    }), 404

@app.errorhandler(500)
def internal_error(error):
    # Handle 500 errors 
    return jsonify({
        'success': False,
        'message': 'Internal server error'
    }), 500

# Main function to start the Flask application 
def main():
    print("Pump Controller Backend Starting")
    # Initialize serial connection
    if serial_handler.connect():
        print(f"Arduino connected on {serial_handler.get_port_info()}")
    else:
        print(" Warning: Arduino not connected, Check connection.")
        print(" The web interface will still start, but pump control will not work.")
    
    # Check if frontend files exist
    index_path = os.path.join(FRONTEND_DIR, 'index.html')
    if not os.path.exists(index_path):
        print(f"Warning: Frontend files not found at {FRONTEND_DIR}")
        print(" Ensure the frontend files are in the correct directory.")
    else:
        print(f"Frontend files found at {FRONTEND_DIR}")
    
    print(f"\nStarting web server on http://localhost:{DEFAULT_PORT}")
    print("   Press Ctrl+C to stop the server")
    
    try:
        # Start Flask application
        app.run(
            host='0.0.0.0',  # Allow external connections
            port=DEFAULT_PORT,
            debug=False,  # Set to True for development
            threaded=True
        )
    except KeyboardInterrupt:
        print("\n\n Server stopped by user")
    except Exception as e:
        print(f"\n Error starting server: {e}")
    finally:
        # Clean up serial connection
        serial_handler.disconnect()
        print(" Serial connection closed")

if __name__ == '__main__':
    main()

