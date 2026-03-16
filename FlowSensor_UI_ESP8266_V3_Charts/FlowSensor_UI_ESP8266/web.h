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

// Enhanced HTML Dashboard with Chart.js scatter plots + tables
static const char _PAGE_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Flow Sensors Dashboard with Charts</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #f8f9fa 0%, #ffffff 50%, #f1f3f4 100%);
            min-height: 100vh;
            padding: 20px;
            color: #212529;
        }

        .container {
            max-width: 1600px;
            margin: 0 auto;
        }

        h1 {
            text-align: center;
            font-size: 2.5rem;
            margin-bottom: 30px;
            text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.1);
            letter-spacing: 1px;
            color: #1976d2;
        }

        .view-controls {
            text-align: center;
            margin-bottom: 30px;
        }

        .view-toggle {
            display: inline-flex;
            background: #ffffff;
            border-radius: 12px;
            padding: 4px;
            border: 1px solid rgba(0, 0, 0, 0.1);
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
        }

        .view-btn {
            padding: 10px 20px;
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #666666;
            cursor: pointer;
            transition: all 0.3s ease;
            font-weight: 500;
        }

        .view-btn.active {
            background: linear-gradient(135deg, #66bb6a, #4caf50);
            color: white;
            box-shadow: 0 4px 12px rgba(76, 175, 80, 0.3);
        }

        .sensors-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(380px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }

        .sensor-card {
            background: #ffffff;
            border: 1px solid rgba(0, 0, 0, 0.1);
            border-radius: 16px;
            padding: 20px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }

        .sensor-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 8px 30px rgba(0, 0, 0, 0.15);
        }

        .sensor-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 15px;
            border-bottom: 1px solid rgba(0, 0, 0, 0.1);
        }

        .sensor-title {
            font-size: 1.2rem;
            font-weight: 600;
            color: #1976d2;
        }

        .sensor-info {
            display: flex;
            gap: 15px;
            font-size: 0.9rem;
            margin-bottom: 15px;
            flex-wrap: wrap;
        }

        .info-badge {
            background: rgba(25, 118, 210, 0.1);
            padding: 5px 12px;
            border-radius: 20px;
            border: 1px solid rgba(25, 118, 210, 0.2);
            color: #1976d2;
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
            font-size: 1.1rem;
            font-weight: bold;
            margin: 8px 0;
            color: #1976d2;
        }

        .metric-label {
            font-size: 0.85rem;
            color: #666666;
            font-weight: normal;
        }

        /* Chart container */
        .chart-container {
            position: relative;
            height: 300px;
            margin-bottom: 20px;
            background: #f8f9fa;
            border-radius: 12px;
            padding: 10px;
            border: 1px solid rgba(0, 0, 0, 0.05);
        }

        .chart-title {
            text-align: center;
            color: #1976d2;
            font-size: 0.9rem;
            margin-bottom: 10px;
            font-weight: 500;
        }

        /* Table styles */
        .table-container {
            overflow-x: auto;
            border-radius: 12px;
            background: #f8f9fa;
            border: 1px solid rgba(0, 0, 0, 0.05);
        }

        .data-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 0.85rem;
        }

        .data-table th {
            background: rgba(25, 118, 210, 0.1);
            padding: 8px;
            text-align: left;
            font-weight: 600;
            color: #1976d2;
            border-bottom: 2px solid rgba(0, 0, 0, 0.1);
        }

        .data-table td {
            padding: 6px 8px;
            border-bottom: 1px solid rgba(0, 0, 0, 0.05);
        }

        .data-table tr:hover {
            background: rgba(25, 118, 210, 0.05);
        }

        .controls {
            background: #ffffff;
            border: 1px solid rgba(0, 0, 0, 0.1);
            border-radius: 16px;
            padding: 25px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
        }

        .controls-header {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 20px;
            color: #1976d2;
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
            background: rgba(25, 118, 210, 0.05);
            border-radius: 10px;
            flex: 1;
            min-width: 150px;
        }

        .info-label {
            font-size: 0.85rem;
            color: #666666;
            margin-bottom: 5px;
        }

        .info-value {
            font-size: 1.1rem;
            font-weight: 600;
            color: #1976d2;
        }

        .footer {
            text-align: center;
            margin-top: 30px;
            padding: 20px;
            color: #666666;
            font-size: 0.9rem;
        }

        /* Hide/show content based on view mode */
        .view-tables .chart-container {
            display: none;
        }

        .view-charts .table-container {
            display: none;
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

            .chart-container {
                height: 250px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>In-Line Pump-Viscometer</h1>
        
        <div class="view-controls">
            <div class="view-toggle">
                <button class="view-btn active" onclick="setViewMode('both')">Tables + Charts</button>
                <button class="view-btn" onclick="setViewMode('tables')">Tables Only</button>
                <button class="view-btn" onclick="setViewMode('charts')">Charts Only</button>
            </div>
        </div>
        
        <div class="sensors-grid" id="sensorsGrid">
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
                        <span class="metric-label">Mean: </span><span id="mean1">--</span> mL/min
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS: </span><span id="rms1">--</span> mL/min
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="chart1"></canvas>
                </div>
                <div class="table-container">
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
                        <span class="metric-label">Mean: </span><span id="mean2">--</span> mL/min
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS: </span><span id="rms2">--</span> mL/min
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="chart2"></canvas>
                </div>
                <div class="table-container">
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
                        <span class="metric-label">Mean: </span><span id="mean3">--</span> mL/min
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS: </span><span id="rms3">--</span> mL/min
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="chart3"></canvas>
                </div>
                <div class="table-container">
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
                        <span class="metric-label">Mean: </span><span id="mean4">--</span> mL/min
                    </div>
                    <div class="metric-row">
                        <span class="metric-label">RMS: </span><span id="rms4">--</span> mL/min
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="chart4"></canvas>
                </div>
                <div class="table-container">
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
            Acceleration Cosortium - SDL5 - Formulation • Mahdi Rastegar
        </div>
    </div>

    <script>
        const NUM_SENSORS = 4;
        const MAX_ROWS = 30;
        const MAX_CHART_POINTS = 50;
        let sensorData = [[], [], [], []];
        let chartData = [[], [], [], []];
        let updateInterval;
        let metricsTick = 0;
        let sampleCounter = 0;
        let charts = [];

        // Initialize charts
        function initCharts() {
            const chartColors = [
                'rgba(76, 175, 80, 1)',   // Green
                'rgba(33, 150, 243, 1)',  // Blue  
                'rgba(255, 152, 0, 1)',   // Orange
                'rgba(156, 39, 176, 1)'   // Purple
            ];

            for (let i = 1; i <= NUM_SENSORS; i++) {
                const ctx = document.getElementById('chart' + i).getContext('2d');
                const chart = new Chart(ctx, {
                    type: 'scatter',
                    data: {
                        datasets: [{
                            label: 'Flow (mL/min)',
                            data: [],
                            backgroundColor: chartColors[i-1],
                            borderColor: chartColors[i-1],
                            pointRadius: 4,
                            pointHoverRadius: 6,
                            tension: 0.1,
                            showLine: true
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        plugins: {
                            legend: {
                                labels: {
                                    color: '#1976d2'
                                }
                            }
                        },
                        scales: {
                            x: {
                                title: {
                                    display: true,
                                    text: 'Time (s)',
                                    color: '#1976d2'
                                },
                                grid: {
                                    color: 'rgba(0, 0, 0, 0.1)'
                                },
                                ticks: {
                                    color: '#666666'
                                }
                            },
                            y: {
                                title: {
                                    display: true,
                                    text: 'Flow Rate (mL/min)',
                                    color: '#1976d2'
                                },
                                grid: {
                                    color: 'rgba(0, 0, 0, 0.1)'
                                },
                                ticks: {
                                    color: '#666666'
                                }
                            }
                        },
                        interaction: {
                            intersect: false,
                            mode: 'nearest'
                        },
                        animation: {
                            duration: 300
                        }
                    }
                });
                charts.push(chart);
            }
        }

        // View mode control
        function setViewMode(mode) {
            document.querySelectorAll('.view-btn').forEach(btn => btn.classList.remove('active'));
            event.target.classList.add('active');
            
            const container = document.getElementById('sensorsGrid');
            container.className = 'sensors-grid view-' + mode;
        }

        function startMonitoring() {
            fetch('/start', { method: 'POST' })
                .then(() => {
                    document.getElementById('autopoll').textContent = 'on';
                    document.getElementById('btnStart').disabled = true;
                    document.getElementById('btnStop').disabled = false;
                    document.getElementById('btnDownload').style.display = 'none';
                    
                    // Reset charts and data
                    sampleCounter = 0;
                    sensorData = [[], [], [], []];
                    chartData = [[], [], [], []];
                    charts.forEach(chart => {
                        chart.data.datasets[0].data = [];
                        chart.update();
                    });
                    
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
                    sampleCounter++;
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

                        // Update table
                        const tbody = document.getElementById('data' + i);
                        tbody.innerHTML = sensorData[i-1].map((row, idx) => 
                            '<tr><td>' + (idx + 1) + '</td><td>' + 
                            row.flow.toFixed(3) + '</td><td>' + 
                            row.temp.toFixed(2) + '</td></tr>'
                        ).join('');

                        // Update chart data
                        const chartIndex = i - 1;
                        const chart = charts[chartIndex];
                        
                        // Add new point
                        chart.data.datasets[0].data.push({
                            x: sampleCounter,
                            y: sensor.flow_1s
                        });
                        
                        // Keep only last MAX_CHART_POINTS
                        if (chart.data.datasets[0].data.length > MAX_CHART_POINTS) {
                            chart.data.datasets[0].data.shift();
                        }
                        
                        chart.update('none'); // No animation for real-time updates

                        // Update metrics every 10 polls
                        if (metricsTick === 0) {
                            document.getElementById('mean' + i).textContent = sensor.mean10.toFixed(3);
                            document.getElementById('rms' + i).textContent = sensor.rms10.toFixed(3);
                        }
                    }

                    metricsTick = (metricsTick + 1) % 10;
                })
                .catch(e => console.error('Update error:', e));
        }

        // Start passive polling on page load
        window.addEventListener('DOMContentLoaded', () => {
            initCharts();
            updateData();
            updateInterval = setInterval(updateData, )HTML" STR(POLL_INTERVAL_MS) R"HTML();
        });
    </script>
</body>
</html>)HTML";

// HTTP Handler Functions (same as before)
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