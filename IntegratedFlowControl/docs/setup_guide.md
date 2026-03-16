# Setup Guide - Integrated Flow Control System

This guide will walk you through setting up the complete integrated flow control system from scratch.

## 📋 Prerequisites

### Software Requirements
- **Python 3.6 or later** ([Download Python](https://www.python.org/downloads/))
- **Arduino IDE** ([Download Arduino IDE](https://www.arduino.cc/en/software))
- **Git** (optional - for cloning repository)
- **Web browser** (Chrome, Firefox, Safari, or Edge)

### Hardware Requirements
- **ESP8266 development board** (NodeMCU, Wemos D1 Mini, or compatible)
- **Arduino board** (Uno, Nano, or compatible)
- **4x SLF3X flow sensors** (Sensirion)
- **TCA9548A I2C multiplexer**
- **4x stepper motors** (NEMA17 or similar)
- **4x DRV8825 stepper drivers**
- **Jumper wires and breadboards**
- **Power supplies** (5V for Arduino, 12V for steppers)

## 🔧 Hardware Setup

### Step 1: ESP8266 Flow Sensor Assembly

1. **Connect TCA9548A Multiplexer to ESP8266:**
   ```
   ESP8266    →    TCA9548A
   D1 (SCL)   →    SCL
   D2 (SDA)   →    SDA
   3.3V       →    VIN
   GND        →    GND
   ```

2. **Connect Flow Sensors to Multiplexer:**
   ```
   Sensor     →    TCA9548A Channel
   Sensor 1   →    Channel 0
   Sensor 2   →    Channel 1
   Sensor 3   →    Channel 2
   Sensor 4   →    Channel 3
   ```

3. **Power Connections:**
   - All sensors: 3.3V or 5V (check sensor specifications)
   - Common ground for all components

### Step 2: Arduino Pump Controller Assembly

1. **Connect DRV8825 Drivers to Arduino:**
   ```
   Driver  Pin    →    Arduino Pin
   Step    1      →    Digital 2    (Pump 1)
   Dir     1      →    Digital 5    (Pump 1)
   Step    2      →    Digital 3    (Pump 2)
   Dir     2      →    Digital 6    (Pump 2)
   Step    3      →    Digital 4    (Pump 3)
   Dir     3      →    Digital 7    (Pump 3)
   Step    4      →    Digital 12   (Pump 4)
   Dir     4      →    Digital 13   (Pump 4)
   Enable  All    →    Digital 8    (Shared)
   ```

2. **Connect Stepper Motors to Drivers:**
   - Follow your specific stepper motor wiring diagram
   - Ensure proper coil pairing (A1, A2, B1, B2)

3. **Power Connections:**
   - DRV8825 VDD → Arduino 5V
   - DRV8825 VMOT → 12V stepper power supply
   - Common ground for Arduino, drivers, and stepper power

## 💻 Software Installation

### Step 1: Python Backend Setup

1. **Navigate to backend folder:**
   ```bash
   cd IntegratedFlowControl/backend
   ```

2. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

   If you encounter issues, install individually:
   ```bash
   pip install flask flask-cors pyserial requests
   ```

3. **Verify installation:**
   ```bash
   python -c "import flask, serial, requests; print('All dependencies installed successfully!')"
   ```

### Step 2: Arduino Firmware Upload

1. **Open Arduino IDE**

2. **Install required libraries:**
   - Go to Sketch → Include Library → Manage Libraries
   - No additional libraries needed (uses built-in functions)

3. **Open the Arduino sketch:**
   - File → Open → `IntegratedFlowControl/arduino/PumpController.ino`

4. **Select board and port:**
   - Tools → Board → Arduino Uno (or your specific board)
   - Tools → Port → Select your Arduino's COM port

5. **Upload the firmware:**
   - Click Upload button or press Ctrl+U
   - Wait for "Done uploading" message

6. **Test connection:**
   - Open Serial Monitor (Ctrl+Shift+M)
   - Set baud rate to 9600
   - You should see "Integrated Flow Control - Pump Controller Ready"

### Step 3: ESP8266 Firmware Upload

1. **Install ESP8266 board package:**
   - File → Preferences
   - Additional Boards Manager URLs: 
     ```
     https://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "ESP8266" and install "ESP8266 by ESP8266 Community"

2. **Install required libraries:**
   - No additional libraries needed (uses ESP8266 core libraries)

3. **Configure WiFi credentials:**
   - Open `IntegratedFlowControl/esp8266/FlowSensor_ESP8266.ino`
   - **IMPORTANT:** Update these lines with your WiFi information:
     ```cpp
     const char* WIFI_SSID = "YOUR_WIFI_SSID";      // Replace with your WiFi name
     const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";  // Replace with your WiFi password
     ```

4. **Select board and upload:**
   - Tools → Board → ESP8266 → NodeMCU 1.0 (or your specific board)
   - Tools → Port → Select your ESP8266's COM port
   - Click Upload button

5. **Test WiFi connection:**
   - Open Serial Monitor (115200 baud rate)
   - You should see WiFi connection messages and an IP address

## 🚀 System Startup

### Step 1: Start the Integrated System

1. **Connect hardware:**
   - Plug in Arduino via USB
   - Power on ESP8266 (should connect to WiFi automatically)
   - Ensure all sensors and pumps are connected

2. **Navigate to backend folder:**
   ```bash
   cd IntegratedFlowControl/backend
   ```

3. **Run the integrated system:**
   ```bash
   python main.py
   ```

4. **Look for startup messages:**
   ```
   ============================================================
     INTEGRATED FLOW CONTROL SYSTEM
     Sensors + Pump Controller
   ============================================================

   [1] Connecting to Arduino (Pump Controller)...
   ✓ Arduino connected on COM3

   [2] Connecting to ESP8266 (Flow Sensors)...
   ✓ ESP8266 connected at 192.168.1.100

   [3] Checking frontend files...
   ✓ Frontend files found

   ============================================================
   Starting web server on http://localhost:5000
   Access the unified dashboard at: http://localhost:5000
   ============================================================
   ```

### Step 2: Access Web Interface

1. **Open web browser**

2. **Navigate to:** `http://localhost:5000`

3. **You should see:**
   - Unified dashboard with tabbed interface
   - System status showing connected devices
   - Real-time sensor data (if sensors connected)
   - Pump control interface

## 🔍 Verification and Testing

### Test Flow Sensors (ESP8266)

1. **Check ESP8266 directly:**
   - Find ESP8266 IP address from Serial Monitor
   - Visit `http://[ESP8266-IP]` in browser
   - You should see basic sensor interface

2. **Test sensor readings:**
   - Go to Sensors tab in unified interface
   - Verify sensor values are updating
   - Toggle individual sensors on/off

3. **Test recording:**
   - Click "Start Recording" 
   - Wait 10 seconds
   - Click "Stop Recording"
   - Download CSV to verify data

### Test Pump Control (Arduino)

1. **Check Arduino connection:**
   - System status should show "Arduino: Connected"
   - Go to Pumps tab

2. **Test individual pump:**
   - Set Pump 1 to 100 RPM, 5 seconds
   - Click "Start" - motor should rotate for 5 seconds
   - Monitor status indicator

3. **Test emergency stop:**
   - Start any pump
   - Click "EMERGENCY STOP" - all motors should stop immediately

### Test Unified Features

1. **Multi-tab navigation:**
   - Switch between Overview, Sensors, Pumps, Data tabs
   - Verify data updates across tabs

2. **Real-time updates:**
   - Watch sensor values refresh every second
   - Pump status should update when motors start/stop

3. **Data correlation:**
   - Start recording flow sensors
   - Start a pump
   - Observe flow changes in real-time chart

## 🚨 Troubleshooting

### Arduino Not Detected

**Symptoms:** "Arduino not connected" message

**Solutions:**
1. Check USB cable connection
2. Install Arduino drivers if needed
3. Try different COM port
4. Restart Arduino IDE and re-upload firmware

### ESP8266 Not Found

**Symptoms:** "ESP8266 not found" message

**Solutions:**
1. Verify WiFi credentials in code
2. Check WiFi network connectivity
3. Ensure ESP8266 and computer are on same network
4. Check ESP8266 power supply
5. Look for IP address in ESP8266 Serial Monitor

### Web Interface Won't Load

**Solutions:**
1. Ensure Python backend is running
2. Check for firewall blocking port 5000
3. Try accessing `http://127.0.0.1:5000` instead
4. Restart the Python application

### Sensors Not Reading

**Solutions:**
1. Check I2C wiring (SDA/SCL connections)
2. Verify sensor power supply (3.3V or 5V)
3. Test multiplexer addressing
4. Check sensor enable/disable status

### Pumps Not Moving

**Solutions:**
1. Verify stepper driver wiring
2. Check power supply to stepper drivers (12V)
3. Ensure Enable pin connections
4. Test with lower RPM values first
5. Check stepper motor coil continuity

### Common Error Messages

**"Port already in use":**
- Another instance is already running
- Kill existing Python processes
- Try different port in main.py

**"Module not found":**
- Missing Python dependencies
- Re-run `pip install -r requirements.txt`
- Check Python version (3.6+ required)

## 🔧 Configuration Options

### Changing Network Port

Edit `backend/main.py`:
```python
DEFAULT_PORT = 8080  # Change from 5000 to desired port
```

### Modifying Pump Limits

Edit `arduino/PumpController.ino`:
```cpp
if (rpm == 0 || abs(rpm) > 2000) {  // Change max RPM limit
    Serial.println("ERROR: RPM must be between -2000 and 2000");
    return;
}
```

### Adjusting Update Rate

Edit `frontend/script.js`:
```javascript
// Update every 2 seconds instead of 1
this.updateInterval = setInterval(() => {
    this.updateAllData();
}, 2000);
```

## 🏁 Next Steps

Once everything is working:

1. **Customize for your application:**
   - Adjust pump RPM ranges
   - Modify sensor sampling rates
   - Add custom data analysis

2. **Optimize performance:**
   - Tune update intervals
   - Add data filtering
   - Implement advanced control algorithms

3. **Add safety features:**
   - Sensor-based pump interlocks
   - Flow rate monitoring alarms
   - Automatic shutdown procedures

4. **Scale the system:**
   - Add more sensors or pumps
   - Implement networked deployment
   - Add database logging

## 📞 Getting Help

If you encounter issues:

1. **Check the documentation** in this folder
2. **Review error messages** carefully
3. **Test hardware connections** systematically
4. **Verify software versions** match requirements

The system is designed to be robust and user-friendly. Most issues are related to hardware connections or network configuration.