# ESP8266 Flow Sensor Monitor V3 - With Real-Time Charts

This is an enhanced version of the flow sensor monitoring system that adds **real-time scatter plots** alongside the existing tabular data display. The new UI provides multiple viewing options: Tables + Charts, Tables Only, or Charts Only.

## 🆕 New Features in V3

### ✅ Real-Time Scatter Plots
- **Chart.js integration** for professional-quality charts
- **Flow vs Sample Number** scatter plots for each sensor
- **Real-time updates** every 1000ms
- **Interactive charts** with hover tooltips and zoom capabilities
- **Color-coded sensors** (Green, Blue, Orange, Purple)

### ✅ Multiple View Modes
- **Tables + Charts**: Show both data tables and scatter plots simultaneously
- **Tables Only**: Traditional tabular view (like V2)
- **Charts Only**: Focus on scatter plot visualization
- **Responsive design** that adapts to different screen sizes

### ✅ Enhanced Data Visualization
- **Rolling data buffer**: Shows last 50 points on charts, 30 rows in tables
- **Smooth animations** and professional styling
- **Better mobile support** with optimized chart sizes

## 🚀 Implementation Guide

### 1. Hardware Setup
The hardware setup is **identical** to V2:
- ESP8266 development board
- Up to 4x SLF3X flow sensors
- TCA9548A I2C multiplexer
- Same pin connections as documented in original README

### 2. Software Installation

#### Option A: Replace Existing V2 Code
1. **Backup your current V2 project**
2. Copy files from `FlowSensor_UI_ESP8266_V3_Charts/` to your existing folder
3. Apply the changes below

#### Option B: Use as New Project
1. Upload all files from `FlowSensor_UI_ESP8266_V3_Charts/FlowSensor_UI_ESP8266/` to your ESP8266
2. Configure WiFi credentials in the `.ino` file
3. Upload to ESP8266

### 3. Key Changes from V2

#### Modified Files:
- **`web.h`**: Complete overhaul with Chart.js integration
- **`.ino` and `sensors.h`**: Unchanged (100% compatible)

#### New Dependencies:
- **Chart.js**: Loaded from CDN (no manual installation required)
- **Internet connection**: Required for loading Chart.js library

### 4. Configuration

#### WiFi Setup
Edit in `FlowSensor_UI_ESP8266.ino`:
```cpp
const char* WIFI_SSID = "YourWiFiNetwork";     // Your WiFi name
const char* WIFI_PASS = "YourWiFiPassword";    // Your WiFi password
const char* MDNS_HOST = "flowsensors";         // Optional: Device name
```

#### Chart Customization
Edit in `web.h` (line ~480):
```javascript
const MAX_CHART_POINTS = 50;  // Number of points shown on charts
const MAX_ROWS = 30;          // Number of rows shown in tables
```

### 5. Accessing the Interface

1. **Find Device IP**: Check Serial Monitor for IP address
2. **Open in Browser**: Navigate to `http://[DEVICE_IP]` or `http://flowsensors.local`
3. **Select View Mode**: Use toggle buttons to switch between view modes
4. **Start Monitoring**: Click "Start" to begin data recording

## 📊 Chart Features

### Interactive Elements
- **Hover tooltips**: Show exact values when hovering over data points
- **Zooming**: Click and drag to zoom into specific time ranges
- **Pan**: Hold Shift + drag to pan across the chart
- **Reset zoom**: Double-click to reset to original view

### Real-Time Updates
- **Data refresh**: Every 1000ms (configurable)
- **Smooth animations**: 300ms transition time
- **Rolling buffer**: Automatically removes old data points
- **Color consistency**: Each sensor maintains its color across refreshes

## 🔧 Technical Details

### Chart Implementation
- **Library**: Chart.js v4 (loaded from CDN)
- **Chart type**: Scatter plot with connected lines
- **Data format**: `{x: sampleNumber, y: flowRate}`
- **Update strategy**: Add new points, remove old ones
- **Performance**: Optimized for real-time data streaming

### View Mode System
- **CSS-based**: Uses CSS classes to show/hide elements
- **No page reload**: Instant switching between modes
- **Responsive**: Charts and tables adapt to screen size
- **Memory efficient**: Charts only created once, data updated incrementally

### Browser Compatibility
- **Modern browsers**: Chrome, Firefox, Safari, Edge (latest versions)
- **Mobile support**: Responsive design for tablets and phones
- **Internet required**: For Chart.js CDN (could be made offline if needed)

## 🛠️ Customization Options

### Adding More Chart Types
In `web.h`, modify the chart configuration:
```javascript
type: 'line',        // Change from 'scatter' to 'line'
showLine: true,      // Keep connected lines
tension: 0.4         // Smooth curves
```

### Changing Colors
Modify the `chartColors` array:
```javascript
const chartColors = [
    'rgba(255, 99, 132, 1)',   // Red
    'rgba(54, 162, 235, 1)',   // Blue
    'rgba(255, 205, 86, 1)',   // Yellow
    'rgba(75, 192, 192, 1)'    // Teal
];
```

### Adjusting Chart Size
Modify CSS in `web.h`:
```css
.chart-container {
    height: 400px;  /* Increase from 300px */
}
```

## 📱 Mobile Optimization

The new design includes enhanced mobile support:
- **Responsive grid**: Automatically adjusts to screen size
- **Touch-friendly**: Larger buttons and touch targets
- **Optimized charts**: Smaller chart height on mobile
- **Readable text**: Improved font sizes and spacing

## 🔍 Troubleshooting

### Charts Not Displaying
1. **Check internet connection**: Chart.js requires CDN access
2. **Browser console**: Check for JavaScript errors
3. **Clear cache**: Hard refresh the browser page

### Performance Issues
1. **Reduce chart points**: Lower `MAX_CHART_POINTS` value
2. **Increase update interval**: Modify `POLL_INTERVAL_MS`
3. **Use "Tables Only" mode**: For better performance on slow devices

### Layout Issues
1. **Browser zoom**: Reset to 100%
2. **Clear browser cache**: Force refresh with Ctrl+F5
3. **Try different browser**: Some older browsers may have issues

## 🔄 Migration from V2

The migration is **backward compatible**:
1. All existing functionality is preserved
2. Same API endpoints and data format
3. Hardware setup unchanged
4. CSV export feature maintained
5. Added new charting capabilities

## 📈 Performance Specifications

- **Update frequency**: 1000ms
- **Data points per chart**: 50 (configurable)
- **Memory usage**: ~2KB per chart in RAM
- **ESP8266 compatibility**: Tested on NodeMCU and Wemos D1 Mini
- **Browser requirements**: Modern browser with JavaScript enabled