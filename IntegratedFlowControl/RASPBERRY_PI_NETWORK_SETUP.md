# Raspberry Pi 5.0 Network Setup Guide

## WiFi Network Access Configuration

The Integrated Flow Control System is configured to be accessible from any device on the same WiFi network.

### System Configuration

- **Arduino Connection**: `/dev/ttyACM0` (Pump Controller)
- **ESP8266 Programming**: `/dev/ttyUSB0` (Flow Sensors)  
- **ESP8266 Communication**: WiFi/HTTP
- **Web Server**: Port 5000, accessible on all network interfaces

### How to Access the System

1. **Start the system** on Raspberry Pi:
   ```bash
   python3 start_system.py
   ```

2. **Find your Raspberry Pi's IP address**:
   ```bash
   hostname -I
   ```
   Or visit: http://localhost:5000/api/system/network-info

3. **Access from other devices**:
   - On the same WiFi network, open: `http://[PI_IP]:5000`
   - Replace `[PI_IP]` with your Raspberry Pi's IP address

### Network Troubleshooting

If you can't access from other devices:

1. **Check firewall** on Raspberry Pi:
   ```bash
   sudo ufw status
   # If active, allow port 5000:
   sudo ufw allow 5000
   ```

2. **Verify network binding**:
   - The system binds to `0.0.0.0:5000` (all interfaces)
   - Check with: `netstat -tlnp | grep 5000`

3. **Test connectivity**:
   - From another device: `ping [PI_IP]`
   - Test port: `telnet [PI_IP] 5000`

### WiFi Configuration

If your Raspberry Pi needs WiFi setup:

1. **Edit wpa_supplicant**:
   ```bash
   sudo nano /etc/wpa_supplicant/wpa_supplicant.conf
   ```

2. **Add network**:
   ```
   network={
       ssid="YourWiFiName"
       psk="YourWiFiPassword"
   }
   ```

3. **Restart WiFi**:
   ```bash
   sudo systemctl restart wpa_supplicant
   ```

### Port Configuration Summary

- **Flask Web Server**: Port 5000 (accessible on WiFi network)
- **Arduino Uno**: `/dev/ttyACM0` (serial communication)
- **ESP8266**: WiFi HTTP communication (auto-detect IP or configure manually)

### Debug Information

Access debug information at:
- System status: http://[PI_IP]:5000/api/system/status
- Network info: http://[PI_IP]:5000/api/system/network-info  
- ESP8266 debug: http://[PI_IP]:5000/api/system/esp8266-debug

The system will automatically display network access URLs when starting up.