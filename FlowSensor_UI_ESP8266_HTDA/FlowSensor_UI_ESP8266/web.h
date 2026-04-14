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
<title>High Throughput Dissolution/Permeation Apparatus</title>
<style>
  :root { --bg:#000000; --card:#1a1a1a; --fg:#ffffff; --muted:#cccccc; --line:#333333; }
  *{box-sizing:border-box} 
  body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 system-ui,-apple-system,Segoe UI,Roboto;overflow-x:hidden}
  
  .banner{background:var(--card);padding:20px 30px;display:flex;justify-content:space-between;align-items:center;border-bottom:2px solid var(--line)}
  .banner-text{flex:1}
  .banner-text h1{margin:0;font-size:28px;font-weight:bold;color:#00ff00;line-height:1.2}
  .banner-text h2{margin:5px 0 0 0;font-size:20px;font-weight:600;color:#00aaff;line-height:1.2}
  .banner-text h3{margin:5px 0 0 0;font-size:16px;font-weight:400;color:var(--muted);line-height:1.2}
  .banner-logo{flex-shrink:0;margin-left:30px}
  .banner-logo img{height:80px;width:auto}
  
  .container{padding:20px;max-width:1400px;margin:0 auto}
  
  .plot-section{background:var(--card);border-radius:12px;padding:20px;margin-bottom:20px;border:1px solid var(--line)}
  .plot-title{font-size:24px;font-weight:700;text-align:center;margin-bottom:15px;color:var(--fg)}
  .legend{display:flex;justify-content:center;gap:30px;margin-bottom:15px;flex-wrap:wrap}
  .legend-item{display:flex;align-items:center;gap:8px;padding:8px 12px;background:#2a2a2a;border-radius:8px}
  .legend-color{width:20px;height:3px;border-radius:2px}
  .legend-text{font-size:14px;font-weight:600}
  .legend-mean{font-size:12px;color:var(--muted)}
  
  .plot-container{width:100%;height:300px;border:1px solid var(--line);border-radius:8px;background:#0a0a0a;margin:0 auto}
  .plot-canvas{width:100%;height:100%;border-radius:8px}
  
  .controls{position:fixed;bottom:20px;right:20px;background:var(--card);padding:15px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,.5);border:1px solid var(--line)}
  .controls button{background:#00aa00;color:#ffffff;border:1px solid #00aa00;border-radius:8px;padding:10px 15px;cursor:pointer;margin-right:10px;font-weight:600}
  .controls button:hover{background:#00cc00}
  .controls button:active{transform:translateY(1px)}
  .controls .btn-red{background:#cc0000;border-color:#cc0000}
  .controls .btn-red:hover{background:#ee0000}
  .controls a.download{display:inline-block;text-decoration:none;color:#ffffff;background:#0066cc;border:1px solid #0066cc;border-radius:8px;padding:10px 15px;font-weight:600}
  .controls a.download:hover{background:#0088ee}
</style>
</head>
<body>

<div class="banner">
  <div class="banner-text">
    <h1>Self-Driving Lab. #5</h1>
    <h2>Formulation</h2>
    <h3>High Throughput Dissolution/Permeation Apparatus for Automated Platform</h3>
  </div>
  <div class="banner-logo">
    <img src="logo.png" alt="Lab Logo">
  </div>
</div>

<div class="container">
  <div class="plot-section">
    <div class="plot-title">Flowrate vs Time</div>
    <div class="legend" id="flowrate-legend">
      <div class="legend-item">
        <div class="legend-color" style="background:#ff0000"></div>
        <div class="legend-text">Channel 1</div>
        <div class="legend-mean">Mean: <span id="ch1-mean">--</span> mL/min</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#00ff00"></div>
        <div class="legend-text">Channel 2</div>
        <div class="legend-mean">Mean: <span id="ch2-mean">--</span> mL/min</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#0080ff"></div>
        <div class="legend-text">Channel 3</div>
        <div class="legend-mean">Mean: <span id="ch3-mean">--</span> mL/min</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#ffff00"></div>
        <div class="legend-text">Channel 4</div>
        <div class="legend-mean">Mean: <span id="ch4-mean">--</span> mL/min</div>
      </div>
    </div>
    <div class="plot-container">
      <canvas id="flowrate-plot" class="plot-canvas" width="1200" height="300"></canvas>
    </div>
  </div>

  <div class="plot-section">
    <div class="plot-title">Temperature vs Time</div>
    <div class="legend" id="temperature-legend">
      <div class="legend-item">
        <div class="legend-color" style="background:#ff0000"></div>
        <div class="legend-text">Channel 1</div>
        <div class="legend-mean">Temp: <span id="ch1-temp">--</span> °C</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#00ff00"></div>
        <div class="legend-text">Channel 2</div>
        <div class="legend-mean">Temp: <span id="ch2-temp">--</span> °C</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#0080ff"></div>
        <div class="legend-text">Channel 3</div>
        <div class="legend-mean">Temp: <span id="ch3-temp">--</span> °C</div>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background:#ffff00"></div>
        <div class="legend-text">Channel 4</div>
        <div class="legend-mean">Temp: <span id="ch4-temp">--</span> °C</div>
      </div>
    </div>
    <div class="plot-container">
      <canvas id="temperature-plot" class="plot-canvas" width="1200" height="300"></canvas>
    </div>
  </div>
</div>

<div class="controls">
  <button id="btnStart">Start Recording</button>
  <button id="btnStop" class="btn-red" disabled>Stop Recording</button>
  <a id="btnDownload" class="download" href="/log.csv" style="display:none">Download CSV</a>
</div>

<script>
const setText=(id,v)=>document.getElementById(id).textContent=v;

// Data storage for each channel (30 seconds @ 1Hz = 30 points)
const MAX_POINTS = 30;
let channelData = [
  {flowValues: [], tempValues: [], times: []}, // Channel 1
  {flowValues: [], tempValues: [], times: []}, // Channel 2  
  {flowValues: [], tempValues: [], times: []}, // Channel 3
  {flowValues: [], tempValues: [], times: []}  // Channel 4
];

let interval=)HTML" STR(POLL_INTERVAL_MS) R"HTML(, timer=null;
let startTime = Date.now();

// Channel colors
const channelColors = ['#ff0000', '#00ff00', '#0080ff', '#ffff00'];

function addDataPoint(channelIndex, flowValue, tempValue) {
  const data = channelData[channelIndex];
  const currentTime = (Date.now() - startTime) / 1000; // seconds since start
  
  data.flowValues.push(flowValue);
  data.tempValues.push(tempValue);
  data.times.push(currentTime);
  
  // Keep only last 30 points
  if (data.flowValues.length > MAX_POINTS) {
    data.flowValues.shift();
    data.tempValues.shift();
    data.times.shift();
  }
}

function calculate30SecondMean(channelIndex, type) {
  const data = channelData[channelIndex];
  const values = type === 'flow' ? data.flowValues : data.tempValues;
  if (values.length === 0) return 0;
  
  const sum = values.reduce((a, b) => a + b, 0);
  return sum / values.length;
}

function drawMultiLinePlot(canvasId, dataType) {
  const canvas = document.getElementById(canvasId);
  const ctx = canvas.getContext('2d');
  
  // Clear canvas
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  
  // Plot settings
  const padding = 50;
  const plotWidth = canvas.width - 2 * padding;
  const plotHeight = canvas.height - 2 * padding;
  
  // Find overall min/max for scaling
  let allValues = [];
  let allTimes = [];
  for (let ch = 0; ch < 4; ch++) {
    const data = channelData[ch];
    const values = dataType === 'flow' ? data.flowValues : data.tempValues;
    allValues = allValues.concat(values);
    allTimes = allTimes.concat(data.times);
  }
  
  if (allValues.length === 0) return;
  
  const minVal = Math.min(...allValues) - 1;
  const maxVal = Math.max(...allValues) + 1;
  const valRange = maxVal - minVal || 1;
  
  const minTime = Math.min(...allTimes);
  const maxTime = Math.max(...allTimes);
  const timeRange = maxTime - minTime || 30;
  
  // Draw axes
  ctx.strokeStyle = '#666666';
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
  
  // Draw grid lines
  ctx.strokeStyle = '#333333';
  ctx.lineWidth = 0.5;
  for (let i = 1; i <= 5; i++) {
    const y = padding + (i * plotHeight / 6);
    ctx.beginPath();
    ctx.moveTo(padding, y);
    ctx.lineTo(canvas.width - padding, y);
    ctx.stroke();
  }
  
  // Draw axis labels
  ctx.fillStyle = '#cccccc';
  ctx.font = '12px Arial';
  ctx.fillText(maxVal.toFixed(1), 5, padding + 5);
  ctx.fillText(minVal.toFixed(1), 5, canvas.height - padding - 5);
  ctx.fillText('0s', padding - 10, canvas.height - padding + 15);
  ctx.fillText('30s', canvas.width - padding - 10, canvas.height - padding + 15);
  
  // Draw lines for each channel
  for (let ch = 0; ch < 4; ch++) {
    const data = channelData[ch];
    const values = dataType === 'flow' ? data.flowValues : data.tempValues;
    
    if (values.length < 2) continue;
    
    ctx.strokeStyle = channelColors[ch];
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    for (let i = 0; i < values.length; i++) {
      const x = padding + ((data.times[i] - minTime) / timeRange) * plotWidth;
      const y = canvas.height - padding - ((values[i] - minVal) / valRange) * plotHeight;
      
      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.stroke();
    
    // Draw data points
    ctx.fillStyle = channelColors[ch];
    for (let i = 0; i < values.length; i++) {
      const x = padding + ((data.times[i] - minTime) / timeRange) * plotWidth;
      const y = canvas.height - padding - ((values[i] - minVal) / valRange) * plotHeight;
      ctx.beginPath();
      ctx.arc(x, y, 3, 0, 2 * Math.PI);
      ctx.fill();
    }
  }
}

async function pull(){
  try{
    const r = await fetch('/api',{cache:'no-store'});
    const j = await r.json();

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

    // Update each channel data and legends
    for (let i = 1; i <= 4; i++) {
      const sensor = j[`s${i}`];
      
      // Add new data point
      addDataPoint(i-1, sensor.flow_1s, sensor.temp_1s);
      
      // Calculate and display 30-second means
      const meanFlow = calculate30SecondMean(i-1, 'flow');
      const meanTemp = calculate30SecondMean(i-1, 'temp');
      setText(`ch${i}-mean`, meanFlow.toFixed(2));
      setText(`ch${i}-temp`, meanTemp.toFixed(1));
    }

    // Redraw plots
    drawMultiLinePlot('flowrate-plot', 'flow');
    drawMultiLinePlot('temperature-plot', 'temp');

  }catch(e){ 
    console.error('Error fetching data:', e); 
  }
}

function startPolling(){ 
  if(timer) clearInterval(timer); 
  timer=setInterval(pull, interval);
}

startPolling();

// Recording controls
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
