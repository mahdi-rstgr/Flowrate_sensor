# USB Alias Configuration for In-line Viscometry System

## Overview
USB aliases provide persistent device names that don't change when devices are reconnected or when the system reboots. Instead of dealing with changing `/dev/ttyACM0`, `/dev/ttyUSB0` names, you get consistent `/dev/arduino_pump` and `/dev/esp8266_sensors` aliases.

## Quick Setup 

### Automatic Setup (Recommended)
```bash
sudo bash setup_usb_aliases.sh
```

This script automatically:
- Detects connected Arduino and ESP8266 devices
- Creates udev rules for persistent naming
- Sets up proper permissions
- Shows current device status

### Manual Setup
If you prefer to set up manually:

1. **Create udev rules file:**
```bash
sudo nano /etc/udev/rules.d/99-viscometry.rules
```

2. **Add these rules:**
```
# Arduino Uno (Pump Controller) 
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0043", SYMLINK+="arduino_pump"

# ESP8266 with CH340 (Flow Sensors)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="esp8266_sensors"

# Set permissions
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0043", MODE="0666", GROUP="dialout"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", MODE="0666", GROUP="dialout"
```

3. **Apply rules:**
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Device Mapping

| Device | USB VID:PID | Physical Port | Alias | Purpose |
|--------|-------------|---------------|--------|---------|
| Arduino Uno | 2341:0043 | /dev/ttyACM0 | /dev/arduino_pump | Pump control |
| ESP8266 (CH340) | 1a86:7523 | /dev/ttyUSB0 | /dev/esp8266_sensors | Programming ESP8266 |

## Benefits

✅ **Consistent device names**: Never worry about changing port numbers  
✅ **Production ready**: Reliable for long-term deployments  
✅ **Easy troubleshooting**: Clear device identification  
✅ **No conflicts**: Each system uses unique aliases  

## Verification

Check if aliases are working:
```bash
ls -la /dev/arduino_pump /dev/esp8266_sensors
```

Expected output:
```
lrwxrwxrwx 1 root root 7 Apr 10 10:30 /dev/arduino_pump -> ttyACM0
lrwxrwxrwx 1 root root 7 Apr 10 10:30 /dev/esp8266_sensors -> ttyUSB0
```

## System Integration

The viscometry system automatically prioritizes USB aliases:

1. **First**: Check for `/dev/arduino_pump` and `/dev/esp8266_sensors`
2. **Fallback**: Standard hardware ID detection
3. **Last resort**: Common port patterns

## Programming ESP8266

With USB aliases, ESP8266 programming is consistent:
```bash
# Always use the same device name
arduino-cli compile --fqbn esp8266:esp8266:nodemcu -p /dev/esp8266_sensors --upload FlowSensor_ESP8266.ino
```

## Troubleshooting

### Alias not found
```bash
# Check if device is connected
lsusb | grep -E "(2341|1a86)"

# Check udev rules
cat /etc/udev/rules.d/99-viscometry.rules

# Reload rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### Wrong device mapping
```bash
# Check actual hardware IDs
for port in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$port" ]; then
        echo "$port: $(udevadm info --query=property --name="$port" | grep "ID_VENDOR_ID\|ID_MODEL_ID")"
    fi
done
```

### Permissions issues
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in for changes to take effect
```

## Compatible with System 1

Since you mentioned you already have USB aliases set up for "system 1" with `esp8266_sensors`, this viscometry system setup is compatible and follows the same naming convention! 🎯