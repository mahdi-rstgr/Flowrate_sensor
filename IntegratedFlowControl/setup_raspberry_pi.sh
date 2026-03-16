#!/bin/bash
# Raspberry Pi Setup Script for Integrated Flow Control System
# This script helps configure your Raspberry Pi for the specific hardware setup

echo "🍓 Raspberry Pi Setup for Integrated Flow Control System"
echo "========================================================"
echo "Your Hardware:"
echo "- Arduino Uno: ID 2341:0043 Arduino SA Uno R3 (CDC ACM)"
echo "- ESP8266: ID 1a86:7523 QinHeng Electronics CH340 serial converter"
echo ""

# Update system
echo "📦 Updating system packages..."
sudo apt update
sudo apt install -y python3 python3-pip python3-serial python3-dev git

# Install Python requirements
echo "🐍 Installing Python dependencies..."
pip3 install --user flask flask-cors pyserial requests

# Setup user permissions for serial access
echo "🔐 Setting up serial port permissions..."
sudo usermod -a -G dialout $USER

# Create udev rules for consistent device naming
echo "📋 Creating udev rules for device recognition..."
sudo tee /etc/udev/rules.d/99-flowcontrol.rules > /dev/null << EOF
# Arduino Uno R3
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0043", SYMLINK+="arduino_uno", MODE="0666"

# CH340 Serial Converter (ESP8266)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="esp8266_ch340", MODE="0666"
EOF

# Reload udev rules
sudo udevadm control --reload-rules

# Set permissions on common ports
echo "🔧 Setting port permissions..."
for port in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyUSB0 /dev/ttyUSB1; do
    if [ -e "$port" ]; then
        sudo chmod 666 "$port"
        echo "✓ Set permissions for $port"
    fi
done

# Check device detection
echo "🔍 Checking for connected devices..."
echo "USB Devices:"
lsusb | grep -E "(2341:0043|1a86:7523)"

echo "Serial Ports:"
ls -la /dev/tty{ACM,USB}* 2>/dev/null || echo "No serial ports found"

echo ""
echo "✅ Setup complete!"
echo ""
echo "📋 Next Steps:"
echo "1. Log out and log back in (for group permissions to take effect)"
echo "2. Connect your Arduino Uno and ESP8266 devices"  
echo "3. Run: python3 raspberry_pi_config.py (to test device detection)"
echo "4. Run: python3 start_system.py (to start the system)"
echo "5. Open web browser to: http://localhost:5000"
echo ""
echo "🔧 Troubleshooting:"
echo "- If Arduino not detected: Check /dev/ttyACM0"
echo "- If ESP8266 CH340 not detected: Check /dev/ttyUSB0"  
echo "- Run 'python3 raspberry_pi_config.py' for detailed device info"
echo "- Check logs in the terminal when starting the system"
echo ""
echo "⚠️  IMPORTANT: You must log out and log back in for serial permissions!"