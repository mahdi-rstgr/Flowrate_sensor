#!/bin/bash
# Raspberry Pi 5.0 Enhanced Setup Script for Integrated Flow Control System
# This script helps configure your Raspberry Pi for the specific hardware setup

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[STATUS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "Raspberry Pi 5.0 Setup for Integrated Flow Control System"
echo "=========================================================="
echo "Hardware Configuration:"
echo "- Arduino Uno: /dev/ttyACM0 (ID 2341:0043 Arduino SA Uno R3)"
echo "- ESP8266: WiFi communication (programming via /dev/ttyUSB0)"
echo ""

# Update system
print_status "Updating system packages..."
sudo apt update
if [ $? -eq 0 ]; then
    print_status "System packages updated successfully"
else
    print_error "Failed to update system packages"
    exit 1
fi

# Install required packages
print_status "Installing system packages..."
sudo apt install -y python3 python3-pip python3-serial python3-dev git ufw curl
if [ $? -eq 0 ]; then
    print_status "System packages installed successfully"
else
    print_error "Failed to install system packages"
    exit 1
fi

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
# Install Python requirements
print_status "Installing Python dependencies..."
pip3 install --user flask flask-cors pyserial requests
if [ $? -eq 0 ]; then
    print_status "Python packages installed successfully"
else
    print_error "Failed to install Python packages"
    exit 1
fi

# Configure user permissions for serial access
print_status "Configuring user permissions..."
sudo usermod -a -G dialout $USER
if [ $? -eq 0 ]; then
    print_status "User added to dialout group"
    print_warning "You'll need to logout and login for group changes to take effect"
else
    print_error "Failed to add user to dialout group"
fi

# Configure firewall
print_status "Configuring firewall..."
sudo ufw --force enable
sudo ufw allow ssh
sudo ufw allow 5000/tcp
print_status "Firewall configured (SSH + Port 5000 allowed)"

# Create USB device rules for consistent naming
print_status "Creating USB device rules..."
sudo tee /etc/udev/rules.d/99-flow-control.rules > /dev/null << 'EOF'
# Arduino Uno
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0043", SYMLINK+="arduino-pump-controller"

# CH340 USB-Serial (ESP8266)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="esp8266-programmer"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
print_status "USB device rules created"

# Test system
print_status "Testing system setup..."
echo ""
echo "System Test Results:"
echo "==================="

# Check Python version
python_version=$(python3 --version 2>&1)
echo "Python version: $python_version"

# Check installed packages
echo "Checking Python packages:"
for pkg in flask flask_cors serial requests; do
    python3 -c "import $pkg" 2>/dev/null && echo "  ? $pkg" || echo "  ? $pkg"
done

# Check serial ports
echo "Serial ports:"
ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | sed 's/^/  /' || echo "  No serial devices found"

# Check network
ip_address=$(hostname -I | awk '{print $1}')
echo "IP address: $ip_address"

echo ""
echo "=========================================="
echo "           SETUP COMPLETE!"
echo "=========================================="
echo ""
print_status "Next steps:"
echo "1. LOGOUT AND LOGIN AGAIN (required for serial permissions)"
echo "2. Connect your hardware:"
echo "   - Arduino Uno via USB (should appear as /dev/ttyACM0)"
echo "   - ESP8266 via USB for programming (/dev/ttyUSB0)"
echo "3. Program ESP8266 with flow sensor firmware and configure WiFi"
echo "4. Run diagnostics: python3 raspberry_pi_diagnostics.py"
echo "5. Start the system: python3 start_system.py"
echo ""
print_status "Access the system at: http://$ip_address:5000"
echo ""
