#pragma once
#include <ESP8266WebServer.h>
#include <uri/UriRegex.h>
#include "sensors.h"   // bring in NUM_SENSORS + get/set_sensor_enabled + extern sensor_enabled[]

#define POLL_INTERVAL_MS 1000
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static ESP8266WebServer _server(80);

// Provided by the .ino file
extern void get_ui_snapshot(
  float s_flow_1s[NUM_SENSORS], float s_temp_1s[NUM_SENSORS], 
  float s_mean10[NUM_SENSORS], float s_rms10[NUM_SENSORS], float s_cv10[NUM_SENSORS], 
  bool s_ok[NUM_SENSORS],
  bool& is_recording, bool& is_csv_ready
);
extern void start_run();
extern void stop_run();
extern bool stream_csv_to_client(ESP8266WebServer& server);

// HTML Dashboard with 4 sensors, brown glassmorphism theme
static const char _PAGE_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Flow Sensors Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #3e2723 0%, #4e342e 50%, #5d4037 100%);
            min-height: 100vh;
            padding: 20px;
            color: #fff;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
        }

        h1 {
            text-align: center;
            font-size: 2.5rem;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
            letter-spacing: 1px;
        }

        .sensors-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }

        .sensor-card {
            background: rgba(121, 85, 72, 0.25);
            backdrop-filter: blur(12px);
            border: 1px solid rgba(255, 255, 255, 0.18);
            border-radius: 16px;
            padding: 20px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }

        .sensor-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 12px 40px rgba(0, 0, 0, 0.4);
        }

        .sensor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 15px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }

        .sensor-title {
            font-size: 1.2rem;
            font-weight: 600;
            color: #ffccbc;
        }

        .sensor-info {
            display: flex;
            gap: 15px;
            font-size: 0.9rem;
            margin-bottom: 15px;
            flex-wrap: wrap;
        }

        .info-badge {
            background: rgba(255, 204, 188, 0.2);
            padding: 5px 12px;
            border-radius: 20px;
            border: 1px solid rgba(255, 204, 188, 0.3);
        }

        .status-ok {
            color: #81c784;
        }

        .status-error {
            color: #ef5350;
        }

        .metrics {
            margin-bottom: 20px;
        }

        .metric-row {
            font-size: 1.2rem;
            font-weight: bold;
            margin: 10px 0;
            color: #ffccbc;
        }

        .metric-label {
            font-size: 0.9rem;
            color: #bcaaa4;
            font-weight: normal;
        }

        .data-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 0.85rem;
            margin-top: 15px;
        }

        .data-table th {
            background: rgba(255, 204, 188, 0.15);
            padding: 8px;
            text-align: left;
            font-weight: 600;
            color: #ffccbc;
            border-bottom: 2px solid rgba(255, 255, 255, 0.2);
        }

        .data-table td {
            padding: 6px 8px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }

        .data-table tr:hover {
            background: rgba(255, 255, 255, 0.05);
        }

        .controls {
            background: rgba(121, 85, 72, 0.25);
            backdrop-filter: blur(12px);
            border: 1px solid rgba(255, 255, 255, 0.18);
            border-radius: 16px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }

        .controls-header {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 20px;
            color: #ffccbc;
            text-align: center;
        }

        .control-buttons {
            display: flex;
            gap: 15px;
            justify-content: center;
            flex-wrap: wrap;
            margin-bottom: 20px;
        }

        .btn {
            padding: 12px 28px;
            border: none;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            text-decoration: none;
            display: inline-block;
        }

        .btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .btn-start {
            background: linear-gradient(135deg, #66bb6a, #4caf50);
            color: white;
        }

        .btn-start:hover:not(:disabled) {
            background: linear-gradient(135deg, #4caf50, #388e3c);
            transform: translateY(-2px);
            box-shadow: 0 6px 16px rgba(76, 175, 80, 0.4);
        }

        .btn-stop {
            background: linear-gradient(135deg, #ef5350, #e53935);
            color: white;
        }

        .btn-stop:hover:not(:disabled) {
            background: linear-gradient(135deg, #e53935, #c62828);
            transform: translateY(-2px);
            box-shadow: 0 6px 16px rgba(239, 83, 80, 0.4);
        }

        .btn-download {
            background: linear-gradient(135deg, #42a5f5, #2196f3);
            color: white;
        }

        .btn-download:hover {
            background: linear-gradient(135deg, #2196f3, #1976d2);
            transform: translateY(-2px);
            box-shadow: 0 6px 16px rgba(66, 165, 245, 0.4);
        }

        .control-info {
            display: flex;
            justify-content: space-around;
            gap: 20px;
            margin-top: 20px;
            flex-wrap: wrap;
        }

        .info-item {
            text-align: center;
            padding: 10px 15px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            flex: 1;
            min-width: 150px;
        }

        .info-label {
            font-size: 0.85rem;
            color: #bcaaa4;
            margin-bottom: 5px;
        }

        .info-value {
            font-size: 1.1rem;
            font-weight: 600;
            color: #ffccbc;
        }

        .footer {
            text-align: center;
            margin-top: 30px;
            padding: 20px;
            color: #bcaaa4;
            font-size: 0.9rem;
        }

        @media (max-width: 768px) {
            h1 {
                font-size: 1.8rem;
            }
            
            .sensors-grid {
                grid-template-columns: 1fr;
            }
            
            .control-buttons {
                flex-direction: column;
            }
            
            .btn {
                width: 100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Flow Sensors Dashboard</h1>
        
        <div class="sensors-grid">
            <!-- Sensor 1 -->
            <div class="sensor-card" id="sensor1">
                <div class="sensor-header">
                    <span class="sensor-title">SENSOR 1</span>
                </div>
                <div class="sensor-info">
                    <span class="info-badge">Temp: <span id="temp1">--</span> °C</span>
                    <span class="info-badge">Status: <span class="status-ok" id="status1">OK</span></span>
                </div>
                <div class="metrics">
                    <div class="metric-row">
                        <span class="metric-label">Mean(mL/min): </span><span id="mean1">--</span>
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS(mL/min): </span><span id="rms1">--</span>
                    </div>
                </div>
                <table class="data-table">
                    <thead>
                        <tr>
                            <th>Number</th>
                            <th>Flow (mL/min)</th>
                            <th>Temp (°C)</th>
                        </tr>
                    </thead>
                    <tbody id="data1"></tbody>
                </table>
            </div>

            <!-- Sensor 2 -->
            <div class="sensor-card" id="sensor2">
                <div class="sensor-header">
                    <span class="sensor-title">SENSOR 2</span>
                </div>
                <div class="sensor-info">
                    <span class="info-badge">Temp: <span id="temp2">--</span> °C</span>
                    <span class="info-badge">Status: <span class="status-ok" id="status2">OK</span></span>
                </div>
                <div class="metrics">
                    <div class="metric-row">
                        <span class="metric-label">Mean(mL/min): </span><span id="mean2">--</span>
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS(mL/min): </span><span id="rms2">--</span>
                    </div>
                </div>
                <table class="data-table">
                    <thead>
                        <tr>
                            <th>Number</th>
                            <th>Flow (mL/min)</th>
                            <th>Temp (°C)</th>
                        </tr>
                    </thead>
                    <tbody id="data2"></tbody>
                </table>
            </div>

            <!-- Sensor 3 -->
            <div class="sensor-card" id="sensor3">
                <div class="sensor-header">
                    <span class="sensor-title">SENSOR 3</span>
                </div>
                <div class="sensor-info">
                    <span class="info-badge">Temp: <span id="temp3">--</span> °C</span>
                    <span class="info-badge">Status: <span class="status-ok" id="status3">OK</span></span>
                </div>
                <div class="metrics">
                    <div class="metric-row">
                        <span class="metric-label">Mean(mL/min): </span><span id="mean3">--</span>
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS(mL/min): </span><span id="rms3">--</span>
                    </div>
                </div>
                <table class="data-table">
                    <thead>
                        <tr>
                            <th>Number</th>
                            <th>Flow (mL/min)</th>
                            <th>Temp (°C)</th>
                        </tr>
                    </thead>
                    <tbody id="data3"></tbody>
                </table>
            </div>

            <!-- Sensor 4 -->
            <div class="sensor-card" id="sensor4">
                <div class="sensor-header">
                    <span class="sensor-title">SENSOR 4</span>
                </div>
                <div class="sensor-info">
                    <span class="info-badge">Temp: <span id="temp4">--</span> °C</span>
                    <span class="info-badge">Status: <span class="status-ok" id="status4">OK</span></span>
                </div>
                <div class="metrics">
                    <div class="metric-row">
                        <span class="metric-label">Mean(mL/min): </span><span id="mean4">--</span>
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS(mL/min): </span><span id="rms4">--</span>
                    </div>
                </div>
                <table class="data-table">
                    <thead>
                        <tr>
                            <th>Number</th>
                            <th>Flow (mL/min)</th>
                            <th>Temp (°C)</th>
                        </tr>
                    </thead>
                    <tbody id="data4"></tbody>
                </table>
            </div>
        </div>

        <div class="controls">
            <h2 class="controls-header">CONTROLS</h2>
            <div class="control-buttons">
                <button class="btn btn-start" id="btnStart" onclick="startMonitoring()">Start</button>
                <button class="btn btn-stop" id="btnStop" onclick="stopMonitoring()" disabled>Stop</button>
                <a class="btn btn-download" id="btnDownload" href="/log.csv" style="display:none;">Download CSV</a>
            </div>
            <div class="control-info">
                <div class="info-item">
                    <div class="info-label">Auto poll</div>
                    <div class="info-value" id="autopoll">on</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Last update</div>
                    <div class="info-value" id="lastupdate">--</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Interval</div>
                    <div class="info-value" id="interval">)HTML" STR(POLL_INTERVAL_MS) R"HTML( ms</div>
                </div>
            </div>
        </div>

        <div class="footer">
            Served by ESP8266
        </div>
    </div>

    <script>
        const NUM_SENSORS = 4;
        const MAX_ROWS = 10;
        let sensorData = [[], [], [], []];
        let updateInterval;
        let metricsTick = 0;

        function startMonitoring() {
            fetch('/start', { method: 'POST' })
                .then(() => {
                    document.getElementById('autopoll').textContent = 'on';
                    document.getElementById('btnStart').disabled = true;
                    document.getElementById('btnStop').disabled = false;
                    document.getElementById('btnDownload').style.display = 'none';
                    if (!updateInterval) {
                        updateInterval = setInterval(updateData, )HTML" STR(POLL_INTERVAL_MS) R"HTML();
                    }
                    updateData();
                })
                .catch(e => console.error('Start error:', e));
        }

        function stopMonitoring() {
            fetch('/stop', { method: 'POST' })
                .then(() => {
                    document.getElementById('btnStart').disabled = false;
                    document.getElementById('btnStop').disabled = true;
                    updateData(); // one last refresh to reveal CSV button
                })
                .catch(e => console.error('Stop error:', e));
        }

        function updateData() {
            fetch('/api', { cache: 'no-store' })
                .then(response => response.json())
                .then(data => {
                    document.getElementById('lastupdate').textContent = new Date().toLocaleTimeString();
                    
                    // Update download button visibility
                    const dlBtn = document.getElementById('btnDownload');
                    if (data.run && data.run.csv_ready) {
                        dlBtn.style.display = 'inline-block';
                    } else {
                        dlBtn.style.display = 'none';
                    }

                    // Update all sensors
                    for (let i = 1; i <= NUM_SENSORS; i++) {
                        const sensor = data['s' + i];
                        if (!sensor) continue;

                        const card = document.getElementById('sensor' + i);
                        const statusEl = document.getElementById('status' + i);

                        // Status
                        statusEl.textContent = sensor.ok ? 'OK' : 'ERROR';
                        statusEl.className = sensor.ok ? 'status-ok' : 'status-error';

                        // Temperature
                        document.getElementById('temp' + i).textContent = sensor.temp_1s.toFixed(2);

                        // Rolling data table
                        sensorData[i-1].unshift({
                            flow: sensor.flow_1s,
                            temp: sensor.temp_1s
                        });
                        if (sensorData[i-1].length > MAX_ROWS) {
                            sensorData[i-1].pop();
                        }

                        const tbody = document.getElementById('data' + i);
                        tbody.innerHTML = sensorData[i-1].map((row, idx) => 
                            '<tr><td>' + (idx + 1) + '</td><td>' + 
                            row.flow.toFixed(2) + '</td><td>' + 
                            row.temp.toFixed(2) + '</td></tr>'
                        ).join('');

                        // Update metrics every 10 polls
                        if (metricsTick === 0) {
                            document.getElementById('mean' + i).textContent = sensor.mean10.toFixed(2);
                            document.getElementById('rms' + i).textContent = sensor.rms10.toFixed(2);
                        }
                    }

                    metricsTick = (metricsTick + 1) % 10;
                })
                .catch(e => console.error('Update error:', e));
        }

        // Start passive polling on page load (no recording until START is pressed)
        window.addEventListener('DOMContentLoaded', () => {
            updateData();
            updateInterval = setInterval(updateData, )HTML" STR(POLL_INTERVAL_MS) R"HTML();
        });
    </script>
</body>
</html>)HTML";

// HTTP Handler Functions
static void _handle_root() { 
    _server.send_P(200, "text/html", _PAGE_INDEX); 
}

static void _handle_api() {
    float f_1s[NUM_SENSORS], t_1s[NUM_SENSORS];
    float m10[NUM_SENSORS], r10[NUM_SENSORS], cv10[NUM_SENSORS];
    bool ok[NUM_SENSORS];
    bool rec, csv;
    
    get_ui_snapshot(f_1s, t_1s, m10, r10, cv10, ok, rec, csv);

    String json = "{";
    for (int i = 0; i < NUM_SENSORS; i++) {
        json += "\"s" + String(i + 1) + "\":{";
        json += "\"flow_1s\":" + String(f_1s[i], 3) + ",";
        json += "\"temp_1s\":" + String(t_1s[i], 3) + ",";
        json += "\"mean10\":" + String(m10[i], 3) + ",";
        json += "\"rms10\":" + String(r10[i], 3) + ",";
        json += "\"cv10\":" + String(cv10[i], 2) + ",";
        json += "\"ok\":" + String(ok[i] ? "true" : "false") + ",";
        json += "\"enabled\":" + String(get_sensor_enabled((uint8_t)(i + 1)) ? "true" : "false");
        json += "}";
        if (i < NUM_SENSORS - 1) json += ",";
    }
    
    json += ",\"run\":{";
    json += "\"recording\":" + String(rec ? "true" : "false") + ",";
    json += "\"csv_ready\":" + String(csv ? "true" : "false");
    json += "}}";
    
    _server.send(200, "application/json", json);
}

static void _handle_start() { 
    start_run(); 
    _server.send(200, "text/plain", "started"); 
}

static void _handle_stop() {  
    stop_run();  
    _server.send(200, "text/plain", "stopped"); 
}

static void _handle_log() {
    if (!stream_csv_to_client(_server)) {
        _server.send(404, "text/plain", "No CSV data available. Record data first.");
    }
}

static void _handle_sensor_toggle() {
    int sensor_id = _server.pathArg(0).toInt();
    String action = _server.pathArg(1);
    bool enable = (action == "on");
    
    if (sensor_id >= 1 && sensor_id <= NUM_SENSORS) {
        set_sensor_enabled((uint8_t)sensor_id, enable);
        _server.send(200, "text/plain", enable ? "enabled" : "disabled");
    } else {
        _server.send(400, "text/plain", "Invalid sensor ID");
    }
}

static void _handle_not_found() {
    _server.send(404, "text/plain", "404: Not Found");
}

// Setup and Loop Functions
inline void web_begin() {
    _server.on("/", HTTP_GET, _handle_root);
    _server.on("/api", HTTP_GET, _handle_api);
    _server.on("/start", HTTP_POST, _handle_start);
    _server.on("/stop", HTTP_POST, _handle_stop);
    _server.on("/log.csv", HTTP_GET, _handle_log);
    _server.on(UriRegex("/sensor/(\\d+)/(on|off)"), HTTP_POST, _handle_sensor_toggle);
    _server.onNotFound(_handle_not_found);
    _server.begin();
    Serial.println("[WEB] Server started on port 80");
}

inline void web_loop() { 
    _server.handleClient(); 
}