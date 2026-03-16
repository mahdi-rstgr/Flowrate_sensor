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

// Minimal HTML for ESP8266 integration (will be accessed via Python backend)
static const char _PAGE_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Flow Sensors</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: #f5f5f5;
            margin: 20px;
            color: #333;
        }
        .status {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 { color: #2196F3; }
        .sensor { margin: 10px 0; }
        .running { color: #4CAF50; }
        .stopped { color: #F44336; }
    </style>
</head>
<body>
    <h1>🌊 ESP8266 Flow Sensors</h1>
    <div class="status">
        <p>This ESP8266 provides flow sensor data to the integrated system.</p>
        <p>Access the unified interface via the Python backend.</p>
        <p><strong>API Endpoint:</strong> <a href="/api">/api</a></p>
        <p><strong>Status:</strong> <span id="status">Loading...</span></p>
        
        <div id="sensors"></div>
        
        <div style="margin-top: 20px;">
            <button onclick="startRecording()" style="background: #4CAF50; color: white; padding: 10px; border: none; border-radius: 4px;">Start Recording</button>
            <button onclick="stopRecording()" style="background: #F44336; color: white; padding: 10px; border: none; border-radius: 4px; margin-left: 10px;">Stop Recording</button>
            <a href="/log.csv" style="background: #2196F3; color: white; padding: 10px; border: none; border-radius: 4px; text-decoration: none; margin-left: 10px;">Download CSV</a>
        </div>
    </div>

    <script>
        async function updateData() {
            try {
                const response = await fetch('/api');
                const data = await response.json();
                
                document.getElementById('status').textContent = 
                    data.run?.recording ? 'Recording' : 'Not Recording';
                document.getElementById('status').className = 
                    data.run?.recording ? 'running' : 'stopped';
                
                let html = '<h3>Sensor Data:</h3>';
                ['s1', 's2', 's3', 's4'].forEach((key, i) => {
                    const sensor = data[key];
                    if (sensor) {
                        html += `
                            <div class="sensor">
                                <strong>Sensor ${i+1}:</strong> 
                                ${sensor.flow_1s?.toFixed(1) || '--'} mL/min, 
                                ${sensor.temp_1s?.toFixed(1) || '--'}°C 
                                (${sensor.ok ? 'OK' : 'Error'}, ${sensor.enabled ? 'Enabled' : 'Disabled'})
                            </div>
                        `;
                    }
                });
                document.getElementById('sensors').innerHTML = html;
            } catch (error) {
                console.error('Error updating data:', error);
            }
        }

        async function startRecording() {
            try {
                await fetch('/start', { method: 'POST' });
                updateData();
            } catch (error) {
                console.error('Error starting recording:', error);
            }
        }

        async function stopRecording() {
            try {
                await fetch('/stop', { method: 'POST' });
                updateData();
            } catch (error) {
                console.error('Error stopping recording:', error);
            }
        }

        // Update every second
        setInterval(updateData, 1000);
        updateData();
    </script>
</body>
</html>)HTML";

// === HTTP Request Handlers ===

static void handle_index() {
  _server.send_P(200, "text/html", _PAGE_INDEX);
}

static void handle_api_data() {
  float flow_1s[NUM_SENSORS], temp_1s[NUM_SENSORS];
  float mean10[NUM_SENSORS], rms10[NUM_SENSORS], cv10[NUM_SENSORS];
  bool ok[NUM_SENSORS];
  bool is_recording, is_csv_ready;

  get_ui_snapshot(flow_1s, temp_1s, mean10, rms10, cv10, ok, is_recording, is_csv_ready);

  String json = "{";
  
  // Individual sensor data
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (i > 0) json += ",";
    int sn = i + 1;
    json += "\"s" + String(sn) + "\":{";
    json += "\"flow_1s\":" + String(flow_1s[i], 3) + ",";
    json += "\"temp_1s\":" + String(temp_1s[i], 2) + ",";
    json += "\"mean10\":" + String(mean10[i], 3) + ","; 
    json += "\"rms10\":" + String(rms10[i], 3) + ",";
    json += "\"cv10\":" + String(cv10[i], 2) + ",";
    json += "\"ok\":" + String(ok[i] ? "true" : "false") + ",";
    json += "\"enabled\":" + String(sensor_enabled[i] ? "true" : "false");
    json += "}";
  }
  
  // System status
  json += ",\"run\":{";
  json += "\"recording\":" + String(is_recording ? "true" : "false") + ",";
  json += "\"csv_ready\":" + String(is_csv_ready ? "true" : "false");
  json += "}";
  
  json += "}";
  
  _server.send(200, "application/json", json);
}

static void handle_start_recording() {
  start_run();
  _server.send(200, "text/plain", "Recording started");
}

static void handle_stop_recording() {
  stop_run();
  _server.send(200, "text/plain", "Recording stopped");
}

static void handle_download_csv() {
  if (!stream_csv_to_client(_server)) {
    _server.send(404, "text/plain", "No data available");
  }
}

static void handle_toggle_sensor() {
  String uri = _server.uri();
  
  // Parse URI: /sensor/{id}/{action}
  int firstSlash = uri.indexOf('/', 1);
  int secondSlash = uri.indexOf('/', firstSlash + 1);
  
  if (firstSlash == -1 || secondSlash == -1) {
    _server.send(400, "text/plain", "Invalid URL format");
    return;
  }
  
  String sensorIdStr = uri.substring(firstSlash + 1, secondSlash);
  String action = uri.substring(secondSlash + 1);
  
  uint8_t sensorId = sensorIdStr.toInt();
  bool enable = (action == "on");
  
  if (sensorId >= 1 && sensorId <= NUM_SENSORS) {
    set_sensor_enabled(sensorId, enable);
    String response = "Sensor " + String(sensorId) + (enable ? " enabled" : " disabled");
    _server.send(200, "text/plain", response);
  } else {
    _server.send(400, "text/plain", "Invalid sensor ID");
  }
}

static void handle_not_found() {
  _server.send(404, "text/plain", "Not Found");
}

// === Setup and Loop Functions ===

inline void web_begin() {
  // Route handlers
  _server.on("/", HTTP_GET, handle_index);
  _server.on("/api", HTTP_GET, handle_api_data);
  _server.on("/start", HTTP_POST, handle_start_recording);
  _server.on("/stop", HTTP_POST, handle_stop_recording);
  _server.on("/log.csv", HTTP_GET, handle_download_csv);
  
  // Sensor toggle endpoint (regex pattern)
  _server.on(UriRegex("/sensor/([1-4])/(on|off)"), HTTP_POST, handle_toggle_sensor);
  
  _server.onNotFound(handle_not_found);
  _server.begin();
  
  Serial.println("[web] HTTP server started on port 80");
}

inline void web_loop() {
  _server.handleClient();
}