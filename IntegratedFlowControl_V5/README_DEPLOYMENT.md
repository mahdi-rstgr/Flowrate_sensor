# Integrated Flow Control System - Raspberry Pi Deployment

## 📦 System Overview
Complete flow monitoring and pump control system combining ESP8266 flow sensors with Arduino Uno pump controller, managed through a unified web interface on Raspberry Pi.

## 🏗️ System Architecture
- **Raspberry Pi**: Main controller running Flask web server
- **ESP8266**: Flow sensors (4x) via WiFi HTTP communication  
- **Arduino Uno**: Pump controller (4x pumps) via USB serial
- **Web Interface**: Real-time monitoring and control dashboard

## 📋 Hardware Requirements
- Raspberry Pi (3B+ or newer recommended)
- ESP8266 with flow sensor firmware
- Arduino Uno with pump controller firmware
- USB cables for Arduino connection
- WiFi network for ESP8266

## ⚡ Quick Deploy
```bash
# 1. Clone or copy this directory to Raspberry Pi
# 2. Run automated setup script
chmod +x setup_raspberry_pi.sh
./setup_raspberry_pi.sh

# 3. Start the system
python3 start_system.py
```

## 🌐 Access Points
- **Web Interface**: http://localhost:5000 or http://[PI_IP]:5000
- **ESP8266 Direct**: http://[ESP8266_IP] (auto-discovered)

## 📁 Directory Structure
```
IntegratedFlowControl/
├── backend/              # Flask backend server
│   ├── main.py           # Main server application
│   ├── esp8266_handler.py # ESP8266 communication
│   ├── serial_handler.py  # Arduino communication  
│   └── requirements.txt   # Python dependencies
├── frontend/             # Web interface
│   ├── index.html        # Main dashboard
│   ├── script.js         # Client-side logic
│   └── style.css         # Styling
├── arduino/              # Arduino Uno firmware
│   └── PumpController/
│       └── PumpController.ino
├── esp8266/              # ESP8266 firmware
│   └── FlowSensor_ESP8266/
│       ├── FlowSensor_ESP8266.ino
│       ├── sensors.h
│       └── web.h
├── docs/                 # Documentation
├── start_system.py       # System launcher
├── setup_raspberry_pi.sh # Automated setup
└── README_DEPLOYMENT.md  # This file
```

## 🔧 Manual Setup (if needed)
```bash
# Install dependencies
sudo apt update && sudo apt upgrade -y
sudo apt install python3-pip python3-venv git -y

# Create Python environment
python3 -m venv venv
source venv/bin/activate
pip install -r backend/requirements.txt

# Set permissions for serial access
sudo usermod -a -G dialout $USER
```

## 🚀 Operation
1. **Power on ESP8266** - ensure it connects to WiFi
2. **Connect Arduino Uno** via USB to Raspberry Pi  
3. **Run start_system.py** - launches web server and connects devices
4. **Access web interface** at http://[PI_IP]:5000

## 🔍 Troubleshooting
- **ESP8266 not found**: Check WiFi connection, verify IP in router admin
- **Arduino not detected**: Verify USB connection, check /dev/ttyACM0 permissions  
- **Port 5000 in use**: Stop existing Flask processes or change port in main.py
- **Permission denied**: Ensure user is in dialout group, reboot after adding

## 📞 Support Files
- `raspberry_pi_config.py` - Raspberry Pi specific configuration
- `RASPBERRY_PI_FIXES_SUMMARY.md` - Common issue solutions
- `docs/setup_guide.md` - Detailed setup instructions