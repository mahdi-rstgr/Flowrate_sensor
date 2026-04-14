#pragma once
#include <ESP8266WebServer.h>
#include "sensors.h"

#define POLL_INTERVAL_MS 1000
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static ESP8266WebServer _server(80);

// Provided by the .ino
extern void get_ui_snapshot(
  float s_flow_1s[NUM_SENSORS], float s_temp_1s[NUM_SENSORS], 
  float s_mean10[NUM_SENSORS], float s_rms10[NUM_SENSORS], float s_cv10[NUM_SENSORS], 
  bool s_ok_out[NUM_SENSORS],
  bool& is_recording, bool& is_csv_ready
);
extern void start_run();
extern void stop_run();
extern bool stream_csv_to_client(ESP8266WebServer& server);

// HTML 
static const char _PAGE_INDEX[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Flow Sensors</title>
<style>
  :root { --bg:#4285f4; --card:#ffffff; --fg:#202124; --muted:#5f6368; --line:#dadce0; }
  *{box-sizing:border-box} body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 system-ui,-apple-system,Segoe UI,Roboto}
  header{padding:18px 20px;text-align:center;font-weight:800;letter-spacing:.3px;color:#ffffff}
  .container{display:flex;gap:16px;padding:16px;max-width:1400px;margin:0 auto}
  .channel{flex:1;background:var(--card);border-radius:16px;padding:16px;box-shadow:0 10px 30px rgba(0,0,0,.25)}
  .channel-title{font-size:24px;font-weight:800;text-align:center;color:var(--fg);margin-bottom:16px}
  .mean-value{font-size:20px;font-weight:600;text-align:center;margin-bottom:20px;color:#34a853}
  .plot-container{width:100%;height:200px;border:1px solid var(--line);border-radius:8px;background:#fafafa}
  .plot-canvas{width:100%;height:100%;border-radius:8px}
  .controls{position:fixed;bottom:20px;right:20px;background:var(--card);padding:12px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,.15)}
  button{background:#34a853;color:#ffffff;border:1px solid #34a853;border-radius:8px;padding:8px 12px;cursor:pointer;margin-right:8px}
  button:active{transform:translateY(1px)}
  .btn-red{background:#ea4335;border-color:#ea4335}
  a.download{display:inline-block;text-decoration:none;color:#ffffff;background:#34a853;border:1px solid #34a853;border-radius:8px;padding:8px 12px}
  .status{font-size:12px;color:var(--muted);text-align:center;margin-top:8px}
</style>
</head>
<body>
<header>Flow Rate Monitoring System</header>

<div class="container">
  <div class="channel">
    <div class="channel-title">Channel 1</div>
    <div class="mean-value">Mean: <span id="ch1-mean">--</span> mL/min</div>
    <div class="plot-container">
      <canvas id="plot1" class="plot-canvas" width="300" height="200"></canvas>
    </div>
    <div class="status">Status: <span id="ch1-status">--</span></div>
  </div>

  <div class="channel">
    <div class="channel-title">Channel 2</div>
    <div class="mean-value">Mean: <span id="ch2-mean">--</span> mL/min</div>
    <div class="plot-container">
      <canvas id="plot2" class="plot-canvas" width="300" height="200"></canvas>
    </div>
    <div class="status">Status: <span id="ch2-status">--</span></div>
  </div>

  <div class="channel">
    <div class="channel-title">Channel 3</div>
    <div class="mean-value">Mean: <span id="ch3-mean">--</span> mL/min</div>
    <div class="plot-container">
      <canvas id="plot3" class="plot-canvas" width="300" height="200"></canvas>
    </div>
    <div class="status">Status: <span id="ch3-status">--</span></div>
  </div>

  <div class="channel">
    <div class="channel-title">Channel 4</div>
    <div class="mean-value">Mean: <span id="ch4-mean">--</span> mL/min</div>
    <div class="plot-container">
      <canvas id="plot4" class="plot-canvas" width="300" height="200"></canvas>
    </div>
    <div class="status">Status: <span id="ch4-status">--</span></div>
  </div>
</div>

<div class="controls">
  <button id="btnStart" class="btn-green">Start</button>
  <button id="btnStop" class="btn-red" disabled>Stop</button>
  <a id="btnDownload" class="download" href="/log.csv" style="display:none">Download CSV</a>
</div>

<script>
const setText=(id,v)=>document.getElementById(id).textContent=v;

// Data storage for each channel (30 seconds @ 1Hz = 30 points)
const MAX_POINTS = 30;
let channelData = [
  {values: [], times: []}, // Channel 1
  {values: [], times: []}, // Channel 2  
  {values: [], times: []}, // Channel 3
  {values: [], times: []}  // Channel 4
];

let interval=)HTML" STR(POLL_INTERVAL_MS) R"HTML(, timer=null;

function addDataPoint(channelIndex, value, time) {
  const data = channelData[channelIndex];
  data.values.push(value);
  data.times.push(time);
  
  // Keep only last 30 points
  if (data.values.length > MAX_POINTS) {
    data.values.shift();
    data.times.shift();
  }
}

function calculate30SecondMean(channelIndex) {
  const data = channelData[channelIndex];
  if (data.values.length === 0) return 0;
  
  const sum = data.values.reduce((a, b) => a + b, 0);
  return sum / data.values.length;
}

function drawPlot(canvasId, channelIndex) {
  const canvas = document.getElementById(canvasId);
  const ctx = canvas.getContext('2d');
  const data = channelData[channelIndex];
  
  // Clear canvas
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  
  if (data.values.length < 2) return;
  
  // Plot settings
  const padding = 30;
  const plotWidth = canvas.width - 2 * padding;
  const plotHeight = canvas.height - 2 * padding;
  
  // Find min/max for scaling
  const minVal = Math.min(...data.values) - 1;
  const maxVal = Math.max(...data.values) + 1;
  const valRange = maxVal - minVal || 1;
  
  // Draw axes
  ctx.strokeStyle = '#dadce0';
  ctx.lineWidth = 1;
  
  // Y-axis
  ctx.beginPath();
  ctx.moveTo(padding, padding);
  ctx.lineTo(padding, canvas.height - padding);
  ctx.stroke();
  
  // X-axis
  ctx.beginPath();
  ctx.moveTo(padding, canvas.height - padding);
  ctx.lineTo(canvas.width - padding, canvas.height - padding);
  ctx.stroke();
  
  // Draw plot line
  ctx.strokeStyle = '#4285f4';
  ctx.lineWidth = 2;
  ctx.beginPath();
  
  for (let i = 0; i < data.values.length; i++) {
    const x = padding + (i / (MAX_POINTS - 1)) * plotWidth;
    const y = canvas.height - padding - ((data.values[i] - minVal) / valRange) * plotHeight;
    
    if (i === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  }
  ctx.stroke();
  
  // Draw value labels
  ctx.fillStyle = '#5f6368';
  ctx.font = '10px Arial';
  ctx.fillText(maxVal.toFixed(1), 5, padding);
  ctx.fillText(minVal.toFixed(1), 5, canvas.height - padding);
}

async function pull(){
  try{
    const r = await fetch('/api',{cache:'no-store'});
    const j = await r.json();
    const currentTime = Date.now();

    // Update button states
    const startBtn = document.getElementById('btnStart');
    const stopBtn  = document.getElementById('btnStop');
    const dlBtn    = document.getElementById('btnDownload');
    if (j.run.recording) { 
      startBtn.disabled = true;  
      stopBtn.disabled = false; 
      dlBtn.style.display = 'none'; 
    } else { 
      startBtn.disabled = false; 
      stopBtn.disabled = true;  
      dlBtn.style.display = j.run.csv_ready ? 'inline-block' : 'none'; 
    }

    // Update each channel
    for (let i = 1; i <= 4; i++) {
      const sensor = j[`s${i}`];
      
      // Add new data point
      addDataPoint(i-1, sensor.flow_1s, currentTime);
      
      // Calculate and display 30-second mean
      const mean30s = calculate30SecondMean(i-1);
      setText(`ch${i}-mean`, mean30s.toFixed(2));
      
      // Update status
      setText(`ch${i}-status`, sensor.ok ? 'Connected' : 'Error');
      
      // Draw plot
      drawPlot(`plot${i}`, i-1);
    }

  }catch(e){ 
    console.error('Error fetching data:', e); 
    for (let i = 1; i <= 4; i++) {
      setText(`ch${i}-status`, 'Connection Error');
    }
  }
}

function startPolling(){ 
  if(timer) clearInterval(timer); 
  timer=setInterval(pull, interval);
}

startPolling();

// Start/Stop recording controls
document.getElementById('btnStart').addEventListener('click', async ()=>{
  try{ await fetch('/start', {method:'POST'}); }catch(e){}
  pull();
});

document.getElementById('btnStop').addEventListener('click', async ()=>{
  try{ await fetch('/stop', {method:'POST'}); }catch(e){}
  pull();
});

// Initial data fetch
pull();
</script>
</body></html>)HTML";

// HTTP Handlers
static void _handle_root(){ _server.send_P(200, "text/html", _PAGE_INDEX); }

static void _handle_api(){
  float f_1s[NUM_SENSORS], t_1s[NUM_SENSORS];
  float m[NUM_SENSORS], r[NUM_SENSORS], cv[NUM_SENSORS];
  bool ok[NUM_SENSORS];
  bool rec, csv;
  
  get_ui_snapshot(f_1s, t_1s, m, r, cv, ok, rec, csv);

  String j = "{";
  for (int i = 0; i < NUM_SENSORS; i++) {
    j += "\"s" + String(i+1) + "\":{";
    j += "\"flow_1s\":" + String(f_1s[i],3) + ",\"temp_1s\":" + String(t_1s[i],3) + ",";
    j += "\"mean10\":" + String(m[i],3) + ",\"rms10\":" + String(r[i],3) + ",\"cv10\":" + String(cv[i],2) + ",";
    j += "\"ok\":" + String(ok[i] ? "true" : "false") + ",";
    j += "\"enabled\":" + String(get_sensor_enabled((uint8_t)(i + 1)) ? "true" : "false");
    j += "}";
    if (i < NUM_SENSORS - 1) j += ",";
  }
  j += ",\"run\":{";
  j += "\"recording\":" + String(rec ? "true" : "false") + ",\"csv_ready\":" + String(csv ? "true" : "false");
  j += "}}";
  _server.send(200, "application/json", j);
}
  float f_1s[NUM_SENSORS], t_1s[NUM_SENSORS];
  float m[NUM_SENSORS], r[NUM_SENSORS], cv[NUM_SENSORS];
  bool ok[NUM_SENSORS];
  bool rec, csv;
  
  get_ui_snapshot(f_1s, t_1s, m, r, cv, ok, rec, csv);

  String j = "{";
  for (int i = 0; i < NUM_SENSORS; i++) {
    j += "\"s" + String(i+1) + "\":{";
    j += "\"flow_1s\":" + String(f_1s[i],3) + ",\"temp_1s\":" + String(t_1s[i],3) + ",";
    j += "\"mean10\":" + String(m[i],3) + ",\"rms10\":" + String(r[i],3) + ",\"cv10\":" + String(cv[i],2) + ",";
    j += "\"ok\":" + String(ok[i] ? "true" : "false") + ",";
    j += "\"enabled\":" + String(get_sensor_enabled((uint8_t)(i + 1)) ? "true" : "false");
    j += "}";
    if (i < NUM_SENSORS - 1) j += ",";
  }
  j += ",\"run\":{";
  j += "\"recording\":" + String(rec ? "true" : "false") + ",\"csv_ready\":" + String(csv ? "true" : "false");
  j += "}}";
  _server.send(200, "application/json", j);
}

static void _handle_start(){ start_run(); _server.send(200, "text/plain", "started"); }
static void _handle_stop(){  stop_run();  _server.send(200, "text/plain", "stopped"); }

static void _handle_log(){
  if (!stream_csv_to_client(_server)) {
    _server.send(404, "text/plain", "No CSV found. Start a run first.");
  }
}

inline void web_begin() {
  _server.on("/", HTTP_GET, _handle_root);
  _server.on("/api", HTTP_GET, _handle_api);
  _server.on("/start", HTTP_POST, _handle_start);
  _server.on("/stop",  HTTP_POST, _handle_stop);
  _server.on("/log.csv", HTTP_GET, _handle_log);
  _server.onNotFound([]{ _server.send(404, "text/plain", "Not found"); });
  _server.begin();
  Serial.println("[http] Server started on :80");
}

inline void web_loop() { _server.handleClient(); }
