# Integrated Flow Control System

A unified system that combines flow sensor monitoring and pump control into a single web interface. This system integrates ESP8266-based flow sensors with Arduino-controlled stepper pumps, providing real-time monitoring and control capabilities through a modern web dashboard.

## 🌟 Features

### Flow Monitoring (ESP8266)
- **4x SLF3X Flow Sensors** with I2C multiplexing
- **Real-time data collection** at 20Hz sampling rate
- **Binary data storage** for efficient recording
- **Web-based charts** with Chart.js visualization
- **CSV export** functionality
- **Individual sensor enable/disable**

### Pump Control (Arduino)
- **4x Stepper Pumps** with DRV8825 drivers
- **Precise RPM control** (1-3000 RPM range)
- **Duration-based** or **continuous** operation modes
- **Emergency stop** functionality
- **Multi-pump coordination**
- **Real-time status monitoring**

### Unified Interface
- **Single web dashboard** for all operations
- **Tabbed interface** (Overview, Sensors, Pumps, Data)
- **Real-time updates** with automatic data refresh
- **Mobile-responsive** design
- **No manual file execution** required

## 📁 System Architecture

```
IntegratedFlowControl/
├── backend/                    # Python Flask Backend
│   ├── main.py                # Main Flask application
│   ├── serial_handler.py      # Arduino communication
│   ├── esp8266_handler.py     # ESP8266 communication
│   └── requirements.txt       # Python dependencies
├── frontend/                   # Web Interface
│   ├── index.html             # Unified dashboard
│   ├── style.css              # Styling
│   └── script.js              # JavaScript logic
├── arduino/                    # Arduino Code
│   └── PumpController.ino     # Pump control firmware
├── esp8266/                    # ESP8266 Code
│   ├── FlowSensor_ESP8266.ino # Main sensor code
│   ├── sensors.h              # Sensor functions
│   └── web.h                  # Web server functions
└── docs/                       # Documentation
    ├── setup_guide.md         # Setup instructions
    └── user_guide.md          # User manual
```

## 🚀 Quick Start

### Prerequisites
- **Python 3.6+** with pip
- **Arduino IDE** for firmware upload
- **ESP8266 development board** (NodeMCU/Wemos D1)
- **Arduino board** (Uno/Nano)
- **Hardware components** (sensors, pumps, drivers)

### Installation Steps

1. **Clone/Download** the integrated system
2. **Install Python dependencies:**
   ```bash
   cd backend
   pip install -r requirements.txt
   ```
3. **Upload Arduino firmware:** Open `arduino/PumpController.ino` in Arduino IDE and upload
4. **Upload ESP8266 firmware:** Open `esp8266/FlowSensor_ESP8266.ino` and upload
5. **Configure WiFi:** Edit ESP8266 code with your WiFi credentials
6. **Run the system:**
   ```bash
   python main.py
   ```
7. **Open web browser:** Navigate to `http://localhost:5000`

## 🔧 Hardware Configuration

### Flow Sensors (ESP8266)
- **ESP8266:** NodeMCU or Wemos D1 Mini
- **Sensors:** 4x SLF3X flow sensors (Sensirion)
- **Multiplexer:** TCA9548A I2C multiplexer
- **Connections:**
  - SDA → D2 (ESP8266)
  - SCL → D1 (ESP8266)
  - Power → 3.3V/5V

### Pump Controller (Arduino)
- **Arduino:** Uno, Nano, or compatible
- **Drivers:** 4x DRV8825 stepper drivers
- **Motors:** 4x stepper motors (200 steps/rev)
- **Pin Mapping:**
  ```
  Pump 1: Step=2,  Dir=5,  Enable=8
  Pump 2: Step=3,  Dir=6,  Enable=8  
  Pump 3: Step=4,  Dir=7,  Enable=8
  Pump 4: Step=12, Dir=13, Enable=8
  ```

## 💻 Web Interface Guide

### Overview Tab
- **System status** indicators (Arduino/ESP8266 connectivity)
- **Real-time sensor readings** with status indicators
- **Pump quick controls** and status
- **Live flow chart** with all 4 sensors
- **Emergency stop** button

### Sensors Tab
- **Recording controls** (Start/Stop/Download)
- **Individual sensor toggles**
- **Detailed flow charts**
- **Live data table** with rolling history

### Pumps Tab
- **Multi-pump selection** and batch control
- **Individual pump controls** with RPM/duration settings
- **Continuous mode** options
- **Real-time pump status** indicators

### Data Tab
- **CSV export** functionality
- **System report** generation
- **Data analysis** statistics
- **Recording duration** tracking

## 🔌 API Endpoints

### Flow Sensors
- `GET /api/flow-sensors` - Real-time sensor data
- `POST /api/sensors/start-recording` - Start recording
- `POST /api/sensors/stop-recording` - Stop recording
- `GET /api/sensors/download-csv` - Download CSV data
- `POST /api/sensors/toggle/{id}` - Enable/disable sensor

### Pump Control
- `POST /api/pumps/start` - Start pump
- `POST /api/pumps/stop` - Stop pump
- `POST /api/pumps/emergency-stop` - Emergency stop all
- `GET /api/pumps/status` - Get pump status

### System Status
- `GET /api/system/status` - Hardware connectivity
- `GET /api/unified-data` - Combined sensor + pump data

## 🔒 Safety Features

- **Emergency stop** functionality for all pumps
- **Storage monitoring** prevents ESP8266 overflow
- **Error handling** for communication failures
- **Parameter validation** for pump commands
- **Automatic reconnection** for lost connections

## 🛠 Troubleshooting

### Connection Issues
- **Arduino not found:** Check USB cable and COM port
- **ESP8266 not found:** Verify WiFi credentials and network
- **Sensors not reading:** Check I2C connections and multiplexer

### Performance Issues
- **Slow web interface:** Restart Python backend
- **Missing data:** Check storage space on ESP8266
- **Pump not responding:** Verify wiring and power supply

## 📋 Pump Naming Convention

The system uses unified naming throughout:
- **User Interface:** Pump 1, Pump 2, Pump 3, Pump 4
- **Internal Arduino:** X, Y, Z, A (for legacy compatibility)
- **API Calls:** 1, 2, 3, 4 (converted automatically)

## 🔄 Data Flow

1. **Sensors:** ESP8266 samples at 20Hz → Averages to 1Hz → Stores binary
2. **Pumps:** Arduino receives serial commands → Controls steppers → Reports status
3. **Backend:** Python Flask proxies between systems → Provides unified API
4. **Frontend:** JavaScript polls backend → Updates charts/tables → Sends commands

## 📊 Technical Specifications

### Performance
- **Sensor sampling:** 20Hz real-time, 1Hz recording
- **Pump precision:** ±1 RPM accuracy
- **Update rate:** 1000ms web interface refresh
- **Storage:** ~2 hours recording at 1Hz (ESP8266)

### Compatibility
- **Python:** 3.6+ (Windows/Linux/MacOS)
- **Browsers:** Chrome, Firefox, Safari, Edge
- **Arduino:** Uno, Nano, Mega (5V boards)
- **ESP8266:** NodeMCU, Wemos D1, generic modules

Developed for unified flow control applications with safety and reliability in mind.