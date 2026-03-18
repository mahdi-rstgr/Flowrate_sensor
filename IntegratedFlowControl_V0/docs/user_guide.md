# User Guide - Integrated Flow Control System

This guide explains how to use the unified flow control system once it's set up and running.

## 🎯 Getting Started

### Starting the System

**Windows Users:**
1. Double-click `start_system.bat`
2. Follow on-screen instructions

**All Users:**
1. Run `python start_system.py` from the main folder
2. Wait for startup messages
3. Open browser to `http://localhost:5000`

### First-Time Setup Checklist

Before using the system, verify:
- ✅ Arduino shows "Connected" status
- ✅ ESP8266 shows "Connected" status  
- ✅ Sensor readings are updating
- ✅ All pumps show "Stopped" status

## 🖥️ Web Interface Overview

The system provides a tabbed interface with four main sections:

### 📊 Overview Tab
Your main dashboard showing:
- **System Status Bar:** Connection status for Arduino and ESP8266
- **Sensor Grid:** Real-time flow readings for all 4 sensors
- **Pump Grid:** Quick status and controls for all 4 pumps
- **Live Chart:** Real-time flow monitoring graph
- **Emergency Stop:** Big red button to stop all pumps immediately

### 🔍 Sensors Tab
Detailed sensor management:
- **Recording Controls:** Start/stop data recording
- **Sensor Toggles:** Enable/disable individual sensors
- **Flow Charts:** Detailed visualization of sensor data
- **Data Table:** Scrolling table of recent sensor readings
- **CSV Download:** Export recorded data

### ⚡ Pumps Tab
Comprehensive pump control:
- **Multi-Pump Selection:** Control multiple pumps simultaneously
- **Individual Controls:** Precise RPM and timing for each pump
- **Batch Operations:** Set same parameters for multiple pumps
- **Status Monitoring:** Real-time pump operation status

### 📁 Data Tab
Data management and analysis:
- **Export Functions:** Download sensor data and system reports
- **Statistics:** View recording duration and sample counts
- **Analysis Tools:** Basic data analysis capabilities

## 🔄 Common Operations

### Recording Flow Data

1. **Start Recording:**
   - Go to Overview or Sensors tab
   - Click "Start Recording" button
   - Status changes to "Recording" (red, blinking)

2. **Monitor Recording:**
   - Watch sensor values update in real-time
   - Check storage status on ESP8266
   - Recording stops automatically if storage full

3. **Stop and Download:**
   - Click "Stop Recording" when finished
   - Go to Sensors or Data tab
   - Click "Download CSV" to save data
   - File saves as `flow_data_[timestamp].csv`

### Operating Pumps

#### Single Pump Operation

1. **Go to Pumps tab**
2. **Select pump (1-4)**
3. **Set parameters:**
   - **RPM:** 1-2000 (speed of rotation)
   - **Time:** Duration in seconds
   - **Continuous:** Check for indefinite operation
4. **Click "Start"**
5. **Monitor status indicator**
6. **Click "Stop" to halt manually**

#### Multi-Pump Operation

1. **Go to top of Pumps tab**
2. **Set batch parameters:**
   - Batch RPM, Time, and Continuous mode
3. **Select pumps to control:**
   - Check individual pump boxes
   - Or use "Select All" for all pumps
4. **Click "Start Selected"**
5. **All selected pumps start with same parameters**

#### Emergency Procedures

**Emergency Stop:**
- Click large "EMERGENCY STOP" button (Overview or Pumps tab)
- ALL pumps stop immediately regardless of settings
- Use when safety issue detected

**Individual Stop:**
- Click "Stop" button on specific pump
- Only that pump stops, others continue

## 📈 Understanding the Data

### Sensor Readings

**Flow Rate:** Measured in mL/min (milliliters per minute)
- Positive values: Fluid flowing in normal direction
- Negative values: Reverse flow detected
- Zero: No flow or sensor disabled

**Temperature:** Measured in °C (Celsius)
- Ambient temperature of the flowing fluid
- Used for flow compensation calculations

**Status Indicators:**
- **Green "OK":** Sensor reading successfully
- **Red "Error":** Sensor communication issue
- **Gray "Disabled":** Sensor manually turned off

### Pump Status

**Status Colors:**
- **Green "Running":** Pump actively rotating
- **Gray "Stopped":** Pump idle, not rotating

**RPM Display:** Shows actual commanded speed
**Duration:** Remaining time for timed operations

### Charts and Graphs

**Overview Chart:**
- Shows last 20 data points for all sensors
- Updates every second
- Time axis shows actual time stamps

**Detailed Charts:**
- Shows last 50 data points
- Sample-based X-axis for analysis
- Higher resolution than overview

**Data Table:**
- Scrolling list of recent readings
- Shows timestamp, flow, and temperature for all sensors
- Automatically removes old entries (keeps last 30)

## 🧪 Experimental Procedures

### Basic Flow Measurement

1. **Prepare system:**
   - Ensure all sensors enabled and reading
   - Connect fluid lines to sensors
   - Check for leaks or air bubbles

2. **Start baseline recording:**
   - Start recording with no pump activity
   - Record for 30 seconds to establish baseline
   - Note ambient flow rates

3. **Activate pumps:**
   - Start pumps with known flow rates
   - Record for desired duration
   - Monitor real-time changes in sensor readings

4. **Analysis:**
   - Stop recording and download CSV
   - Analyze data in Excel or analysis software
   - Compare pump RPM vs. actual flow measurements

### System Calibration

1. **RPM Accuracy Test:**
   - Set pump to specific RPM (e.g., 100)
   - Run for exactly 60 seconds
   - Count actual motor rotations manually
   - Verify: Expected rotations = RPM × (motor steps/200) × (1/16)

2. **Flow Sensor Calibration:**
   - Use known flow rate source
   - Compare sensor readings to reference measurements
   - Adjust sensor scaling factors if needed

3. **Response Time Testing:**
   - Start/stop pumps quickly
   - Monitor sensor response delay
   - Typical response: 1-2 seconds for flow changes

## ⚠️ Safety and Best Practices

### Operational Safety

**Before Each Use:**
- Check all connections for security
- Verify emergency stop functionality
- Ensure adequate ventilation if using solvents
- Check pump power supply voltage (12V for steppers)

**During Operation:**
- Monitor system status indicators continuously
- Don't exceed recommended pump RPM limits
- Stop immediately if unusual noises or vibrations
- Keep emergency stop easily accessible

**After Use:**
- Stop all pumps and recording
- Flush fluid lines if needed
- Power down in sequence: Pumps → Arduino → ESP8266

### Data Management

**File Organization:**
- Download CSV files regularly
- Use descriptive filenames with dates
- Back up important experimental data

**Storage Monitoring:**
- ESP8266 has limited storage (~1MB)
- Download data before starting long experiments
- Clear old files if storage warning appears

### Troubleshooting Tips

**If sensors stop reading:**
- Check WiFi connection (ESP8266 status)
- Toggle individual sensors off/on
- Restart ESP8266 if needed

**If pumps become unresponsive:**
- Check Arduino connection (USB)
- Try emergency stop to reset
- Restart Python backend if needed

**If web interface freezes:**
- Refresh browser page
- Check Python backend console for errors
- Restart system if persistent

## 🔧 Advanced Features

### Custom Recording Intervals

The system records at 1-second intervals by default. For different rates:
1. Modify ESP8266 code: `RECORD_MS = 500` (for 0.5s intervals)
2. Re-upload ESP8266 firmware
3. Note: Higher frequency = less recording time

### Pump Profiles

For complex pump sequences:
1. Use individual pump controls with precise timing
2. Start pumps in sequence manually
3. Record timing in notebook for repeatability

### Data Analysis

The CSV files contain columns:
- `time_s`: Elapsed time since recording start
- `s1_flow_ml_min`, `s1_temp_c`: Sensor 1 data
- `s2_flow_ml_min`, `s2_temp_c`: Sensor 2 data
- (continues for sensors 3 and 4)

**Excel Analysis Tips:**
- Create scatter plots of flow vs. time
- Calculate average flow rates for steady periods
- Use filtering to analyze specific time ranges

## 📞 Getting Help

**System not starting:**
- Check setup guide for missing dependencies
- Verify hardware connections
- Review error messages in console

**Unexpected behavior:**
- Check all connections first
- Restart system components in order
- Consult troubleshooting section in setup guide

**Data quality issues:**
- Verify sensor calibration
- Check for air bubbles in fluid lines
- Ensure stable pump mounting

The integrated system is designed for reliable, user-friendly operation. Most issues can be resolved by checking connections and restarting components systematically.