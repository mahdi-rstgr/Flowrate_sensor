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
  .banner-text h1{margin:0;font-size:28px;font-weight:bold;color:#ff8800;line-height:1.2}
  .banner-text h2{margin:5px 0 0 0;font-size:20px;font-weight:600;color:#ffffff;line-height:1.2}
  .banner-text h3{margin:5px 0 0 0;font-size:16px;font-weight:400;color:#0088ff;line-height:1.2}
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
  
  .plots-row{display:flex;gap:20px;margin-bottom:20px}
  .plots-row .plot-section{flex:1;margin-bottom:0}
  
  .controls{position:fixed;bottom:20px;right:20px;background:var(--card);padding:15px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,.5);border:1px solid var(--line)}
  .controls button{background:#00aa00;color:#ffffff;border:1px solid #00aa00;border-radius:8px;padding:10px 15px;cursor:pointer;margin-right:10px;font-weight:600}
  .controls button:hover{background:#00cc00}
  .controls button:active{transform:translateY(1px)}
  .controls .btn-red{background:#cc0000;border-color:#cc0000}
  .controls .btn-red:hover{background:#ee0000}
  .controls a.download{display:inline-block;text-decoration:none;color:#ffffff;background:#0066cc;border:1px solid #0066cc;border-radius:8px;padding:10px 15px;font-weight:600}
  .controls a.download:hover{background:#0088ee}
  
  .recording-indicator{display:inline-block;width:12px;height:12px;background:#ff0000;border-radius:50%;margin-left:8px;animation:blink 1s infinite}
  .recording-indicator.hidden{display:none}
  @keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0.3}}
  
  .upload-box{position:fixed;bottom:20px;left:20px;background:var(--card);padding:15px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,.5);border:1px solid var(--line);max-width:300px}
  .upload-box.hidden{display:none}
  .close-btn{float:right;background:none;border:none;color:#ccc;font-size:16px;cursor:pointer;padding:0;margin-left:10px;font-weight:bold}
  .close-btn:hover{color:#fff}
</style>
</head>
<body>

<div class="banner">
  <div class="banner-text">
    <h1>Self-Driving Lab. #5</h1>
    <h2>Acceleration Consortium - Formulation</h2>
    <h3>High Throughput Dissolution/Permeation Apparatus for Automated Platform</h3>
  </div>
  <div class="banner-logo">
    <img src="logo.png" alt="Lab Logo" style="height:80px;width:auto;">
  </div>
</div>

<div class="container">
  <div class="plots-row">
    <div class="plot-section">
      <div class="plot-title">Flowrates (ml/min)</div>
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
        <canvas id="flowrate-plot" class="plot-canvas" width="600" height="300"></canvas>
      </div>
    </div>

    <div class="plot-section">
      <div class="plot-title">Temperature (°C)</div>
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
        <canvas id="temperature-plot" class="plot-canvas" width="600" height="300"></canvas>
      </div>
    </div>
  </div>
</div>

<div class="controls">
  <button id="btnStart">Start Recording<span id="recordingDot" class="recording-indicator hidden"></span></button>
  <button id="btnStop" class="btn-red" disabled>Stop Recording</button>
  <a id="btnDownload" class="download" href="/log.csv" style="display:none">Download CSV</a>
</div>

<div id="uploadBox" class="upload-box">
  <h4 style="margin:0 0 10px 0;color:var(--fg);font-size:14px;">Upload Logo<button class="close-btn" onclick="closeUploadBox()">&times;</button></h4>
  <form method="POST" action="/upload" enctype="multipart/form-data" style="margin:0;" onsubmit="handleUploadSubmit()">
    <input type="file" name="logo" accept="image/png" style="width:100%;margin-bottom:10px;padding:5px;background:#2a2a2a;border:1px solid #555;border-radius:4px;color:var(--fg);font-size:12px;">
    <input type="submit" value="Upload Logo" style="width:100%;background:#0066cc;color:#ffffff;border:1px solid #0066cc;border-radius:6px;padding:8px;cursor:pointer;font-weight:600;font-size:12px;">
  </form>
  <div style="font-size:11px;color:var(--muted);margin-top:8px;">PNG files only. Max 100KB.</div>
</div>

<script>
const setText=(id,v)=>document.getElementById(id).textContent=v;

// Data storage for each channel (60 points for side-by-side layout)
const MAX_POINTS = 60;
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
  
  // Keep only last 60 points
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
  const timeRange = maxTime - minTime || 60;
  
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
  
  // Draw grid lines (10 horizontal lines)
  ctx.strokeStyle = '#333333';
  ctx.lineWidth = 0.5;
  for (let i = 1; i <= 10; i++) {
    const y = padding + (i * plotHeight / 11);
    ctx.beginPath();
    ctx.moveTo(padding, y);
    ctx.lineTo(canvas.width - padding, y);
    ctx.stroke();
  }
  
  // Draw axis labels
  ctx.fillStyle = '#cccccc';
  ctx.font = '12px Arial';
  
  // Y-axis labels (show every other one - 5 total) - positioned after title
  for (let i = 0; i <= 10; i += 2) {
    const val = maxVal - ((maxVal - minVal) * i / 10);
    const y = padding + (i * plotHeight / 11);
    ctx.fillText(val.toFixed(1), 30, y + 4);
  }
  
  // Y-axis title - positioned on the far left
  ctx.save();
  ctx.translate(14, canvas.height / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.font = '14px Arial';
  ctx.textAlign = 'center';
  const yTitle = dataType === 'flow' ? 'Flowrate (ml/min)' : 'Temperature (°C)';
  ctx.fillText(yTitle, 0, 0);
  ctx.restore();
  
  // X-axis labels
  ctx.font = '12px Arial';
  ctx.textAlign = 'center';
  const startTime = Math.max(0, maxTime - 60);
  ctx.fillText(startTime.toFixed(0) + 's', padding, canvas.height - padding + 15);
  ctx.fillText(maxTime.toFixed(0) + 's', canvas.width - padding, canvas.height - padding + 15);
  
  // X-axis title
  ctx.fillText('Time', canvas.width / 2, canvas.height - 10);
  
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

    // Update button states and recording indicator
    const startBtn = document.getElementById('btnStart');
    const stopBtn  = document.getElementById('btnStop');
    const dlBtn    = document.getElementById('btnDownload');
    const recordingDot = document.getElementById('recordingDot');
    
    if (j.run.recording) { 
      startBtn.disabled = true;  
      stopBtn.disabled = false; 
      dlBtn.style.display = 'none';
      recordingDot.classList.remove('hidden');
    } else { 
      startBtn.disabled = false; 
      stopBtn.disabled = true;  
      dlBtn.style.display = j.run.csv_ready ? 'inline-block' : 'none';
      recordingDot.classList.add('hidden');
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

// Handle upload form submission
function handleUploadSubmit() {
  setTimeout(() => {
    closeUploadBox();
  }, 1000);
}

// Close upload box manually
function closeUploadBox() {
  document.getElementById('uploadBox').classList.add('hidden');
  localStorage.setItem('logoUploaded', 'true');
}

// Hide upload box if logo was previously uploaded
if (localStorage.getItem('logoUploaded') === 'true') {
  document.getElementById('uploadBox').classList.add('hidden');
}

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

static void _handle_logo(){
  File f = LittleFS.open("/logo.png", "r");
  if (f) {
    _server.streamFile(f, "image/png");
    f.close();
  } else {
    _server.send(404, "text/plain", "Logo not found");
  }
}

static void _handle_upload() {
  HTTPUpload& upload = _server.upload();
  static File uploadFile;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.endsWith(".png")) {
      return;
    }
    if (LittleFS.exists("/logo.png")) {
      LittleFS.remove("/logo.png");
    }
    uploadFile = LittleFS.open("/logo.png", "w");
    if (!uploadFile) {
      Serial.println("Failed to create logo file");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile && upload.currentSize > 0) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.println("Logo upload completed: " + String(upload.totalSize) + " bytes");
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) {
      uploadFile.close();
      if (LittleFS.exists("/logo.png")) {
        LittleFS.remove("/logo.png");
      }
    }
  }
}

static void _handle_upload_response() {
  _server.sendHeader("Location", "/");
  _server.send(303, "text/plain", "Upload complete. Redirecting...");
}

inline void web_begin() {
  _server.on("/", HTTP_GET, _handle_root);
  _server.on("/api", HTTP_GET, _handle_api);
  _server.on("/start", HTTP_POST, _handle_start);
  _server.on("/stop",  HTTP_POST, _handle_stop);
  _server.on("/log.csv", HTTP_GET, _handle_log);
  _server.on("/logo.png", HTTP_GET, _handle_logo);
  _server.on("/upload", HTTP_POST, _handle_upload_response, _handle_upload);
  _server.onNotFound([]{ _server.send(404, "text/plain", "Not found"); });
  _server.begin();
  Serial.println("[http] Server started on :80");
}

inline void web_loop() { _server.handleClient(); }
