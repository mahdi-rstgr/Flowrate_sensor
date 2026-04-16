# Raspberry Pi Setup Guide for Integrated Flow Control System

## Your Hardware Configuration
- **Arduino Uno**: `Bus 001 Device 004: ID 2341:0043 Arduino SA Uno R3 (CDC ACM)`
- **ESP8266**: `Bus 003 Device 005: ID 1a86:7523 QinHeng Electronics CH340 serial converter`

## Quick Setup

### Method 1: Automatic Setup (Recommended)
```bash
# Make setup script executable
chmod +x setup_raspberry_pi.sh

# Run setup script (will ask for sudo password)
./setup_raspberry_pi.sh

# Log out and log back in for permissions to take effect
logout
```

### Method 2: Manual Setup
```bash
# 1. Install system packages
sudo apt update
sudo apt install -y python3 python3-pip python3-serial python3-dev

# 2. Install Python dependencies
pip3 install --user flask flask-cors pyserial requests

# 3. Add user to dialout group for serial access
sudo usermod -a -G dialout $USER

# 4. Set port permissions
sudo chmod 666 /dev/ttyACM0 /dev/ttyUSB0

# 5. Log out and log back in
logout
```

## Running the System

1. **Connect your devices**:
   - Arduino Uno should appear as `/dev/ttyACM0`
   - ESP8266 (if using USB) should appear as `/dev/ttyUSB0`

2. **Test device detection**:
   ```bash
   python3 raspberry_pi_config.py
   ```

3. **Start the system**:
   ```bash
   python3 start_system.py
   ```

4. **Access the web interface**:
   Open browser to: `http://localhost:5000`

## Troubleshooting

### Device Not Found
```bash
# Check connected USB devices
lsusb

# Should show:
# Bus XXX Device XXX: ID 2341:0043 Arduino SA Uno R3
# Bus XXX Device XXX: ID 1a86:7523 QinHeng Electronics CH340

# Check serial ports
ls -la /dev/tty{ACM,USB}*

# Should show:
# /dev/ttyACM0 (Arduino)
# /dev/ttyUSB0 (ESP8266 if connected via USB)
```

### Permission Issues
```bash
# Check if you're in dialout group
groups $USER

# Should include 'dialout'

# If not, add user to group and logout/login
sudo usermod -a -G dialout $USER
logout
```

### Port Detection Issues
```bash
# Check device details
python3 -c "
from backend.serial_handler import SerialHandler
sh = SerialHandler()
print(sh.get_rpi_device_status())
"
```

### Connection Issues
```bash
# Test manual connection
python3 -c "
from backend.serial_handler import SerialHandler
sh = SerialHandler()
print('Arduino by ID:', sh.connect_by_device_id('2341:0043'))
"
```

## File Structure
```
IntegratedFlowControl/
├── setup_raspberry_pi.sh       # Automatic setup script
├── raspberry_pi_config.py      # Device detection and configuration
├── start_system.py             # Main startup script
├── backend/
│   ├── main.py                 # Flask web server
│   ├── serial_handler.py       # Arduino communication (enhanced for RPi)
│   └── esp8266_handler.py      # ESP8266 communication
└── frontend/
    ├── index.html              # Web interface
    ├── script.js               # Frontend JavaScript
    └── style.css               # Styling
```

## Notes

- **ESP8266 Connection**: The ESP8266 typically connects via WiFi, not USB. The CH340 converter may be used for programming/debugging only.
- **IP Configuration**: ESP8266 IP is set to `192.168.10.150` in `backend/main.py` - change if needed.
- **Serial Ports**: Arduino Uno typically uses `/dev/ttyACM0`, while CH340 devices use `/dev/ttyUSB0`.
- **Permissions**: Make sure to log out and log back in after adding user to dialout group.

## Support

If you encounter issues:
1. Run `python3 raspberry_pi_config.py` for detailed diagnosis
2. Check the system logs when starting up
3. Verify your devices are properly connected and recognized by `lsusb`