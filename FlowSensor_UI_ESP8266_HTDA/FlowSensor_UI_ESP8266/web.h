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
  :root { --bg:#0f172a; --card:#111827; --fg:#e5e7eb; --muted:#94a3b8; --line:#1f2937; }
  *{box-sizing:border-box} body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.45 system-ui,-apple-system,Segoe UI,Roboto}
  header{padding:18px 20px;text-align:center;font-weight:800;letter-spacing:.3px}
  .grid{display:grid;gap:16px;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));padding:0 16px 24px;max-width:1200px;margin:0 auto}
  .card{background:var(--card);border-radius:16px;padding:16px 18px;box-shadow:0 10px 30px rgba(0,0,0,.25)}
  .label{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.12em;margin-bottom:8px}
  .bigline{font-size:26px;font-weight:800;margin:6px 0 6px}
  .row{display:flex;gap:12px;flex-wrap:wrap;align-items:center;margin-bottom:10px}
  .pill{font-size:12px;padding:6px 10px;background:#0b1220;border:1px solid var(--line);border-radius:999px;color:var(--muted)}
  table{width:100%;border-collapse:collapse;border:1px solid var(--line);border-radius:12px;overflow:hidden}
  thead{background:#0b1220;color:var(--muted)}
  th,td{padding:8px 10px;border-bottom:1px solid var(--line);font-variant-numeric:tabular-nums;text-align:center}
  tr:nth-child(even){background:#0d1322}
  button{background:#1f2937;color:#d1d5db;border:1px solid #374151;border-radius:10px;padding:8px 12px;cursor:pointer}
  button:active{transform:translateY(1px)}
  .btn-green{background:#065f46;border-color:#065f46}
  .btn-red{background:#7f1d1d;border-color:#7f1d1d}
  a.download{display:inline-block;text-decoration:none;color:#d1d5db;background:#1f2937;border:1px solid #374151;border-radius:10px;padding:8px 12px}
  footer{color:var(--muted);text-align:center;font-size:12px;padding:10px 0 22px}
</style>
</head>
<body>
<header>Flow Sensors</header>

<div class="grid">
  <div class="card">
    <div class="label">Sensor 1</div>
    <div class="row">
      <div class="pill">Temp: <span id="s1-temp">--</span> °C</div>
      <div class="pill">Status: <span id="s1-ok">--</span></div>
    </div>
    <div class="bigline">Mean(mL/min): <span id="s1-mean">--</span></div>
    <div class="bigline">RMS(mL/min):  <span id="s1-rms">--</span></div>
    <table>
      <thead><tr><th>Number</th><th>Flow (mL/min)</th><th>Temp (°C)</th></tr></thead>
      <tbody id="s1-body"></tbody>
    </table>
  </div>

  <div class="card">
    <div class="label">Sensor 2</div>
    <div class="row">
      <div class="pill">Temp: <span id="s2-temp">--</span> °C</div>
      <div class="pill">Status: <span id="s2-ok">--</span></div>
    </div>
    <div class="bigline">Mean(mL/min): <span id="s2-mean">--</span></div>
    <div class="bigline">RMS(mL/min):  <span id="s2-rms">--</span></div>
    <table>
      <thead><tr><th>Number</th><th>Flow (mL/min)</th><th>Temp (°C)</th></tr></thead>
      <tbody id="s2-body"></tbody>
    </table>
  </div>

  <div class="card">
    <div class="label">Sensor 3</div>
    <div class="row">
      <div class="pill">Temp: <span id="s3-temp">--</span> °C</div>
      <div class="pill">Status: <span id="s3-ok">--</span></div>
    </div>
    <div class="bigline">Mean(mL/min): <span id="s3-mean">--</span></div>
    <div class="bigline">RMS(mL/min):  <span id="s3-rms">--</span></div>
    <table>
      <thead><tr><th>Number</th><th>Flow (mL/min)</th><th>Temp (°C)</th></tr></thead>
      <tbody id="s3-body"></tbody>
    </table>
  </div>

  <div class="card">
    <div class="label">Sensor 4</div>
    <div class="row">
      <div class="pill">Temp: <span id="s4-temp">--</span> °C</div>
      <div class="pill">Status: <span id="s4-ok">--</span></div>
    </div>
    <div class="bigline">Mean(mL/min): <span id="s4-mean">--</span></div>
    <div class="bigline">RMS(mL/min):  <span id="s4-rms">--</span></div>
    <table>
      <thead><tr><th>Number</th><th>Flow (mL/min)</th><th>Temp (°C)</th></tr></thead>
      <tbody id="s4-body"></tbody>
    </table>
  </div>

  <div class="card">
    <div class="label">Controls</div>
    <div class="row" style="margin-top:8px">
      <button id="btnStart" class="btn-green">Start</button>
      <button id="btnStop" class="btn-red" disabled>Stop</button>
      <a id="btnDownload" class="download" href="/log.csv" style="display:none">Download CSV</a>
    </div>
    <div class="row">
      <div class="pill">Auto poll: <span id="auto">on</span></div>
      <div class="pill">Last update: <span id="ts">--</span></div>
      <div class="pill">Interval: <span id="iv">)HTML" STR(POLL_INTERVAL_MS) R"HTML(</span> ms</div>
    </div>
  </div>
</div>

<footer>Served by ESP8266</footer>

<script>
const setText=(id,v)=>document.getElementById(id).textContent=v;

const MAX_ROWS = 10;
let s1 = [], s2 = [], s3 = [], s4 = [];
const sensorBuffers = [s1, s2, s3, s4];

function addRow(buf, obj){ buf.unshift(obj); if(buf.length > MAX_ROWS) buf.pop(); }
function renderTable(tbodyId, buf){
  const tbody = document.getElementById(tbodyId);
  tbody.innerHTML = buf.map((r, i) => `
    <tr>
      <td>${i+1}</td>
      <td>${r.flow.toFixed(2)}</td>
      <td>${r.temp.toFixed(2)}</td>
    </tr>`).join('');
}

let interval=)HTML" STR(POLL_INTERVAL_MS) R"HTML(, timer=null;

// 10s metrics throttle 
let metricsTick = 0;

function clearUI(){
  for (let i = 1; i <= 4; i++) {
    setText(`s${i}-mean`,'--'); setText(`s${i}-rms`,'--'); setText(`s${i}-cv`,'--');
    setText(`s${i}-temp`,'--'); setText(`s${i}-ok`,'--');
  }
  s1=[]; s2=[]; s3=[]; s4=[]; 
  for (let i = 1; i <= 4; i++) {
    renderTable(`s${i}-body`, sensorBuffers[i-1]);
  }
  metricsTick = 0;
}

async function pull(){
  try{
    const r = await fetch('/api',{cache:'no-store'});
    const j = await r.json();
    setText('ts', new Date().toLocaleTimeString());

    // buttons state (recording only affects CSV)
    const startBtn = document.getElementById('btnStart');
    const stopBtn  = document.getElementById('btnStop');
    const dlBtn    = document.getElementById('btnDownload');
    if (j.run.recording) { startBtn.disabled = true;  stopBtn.disabled = false; dlBtn.style.display = 'none'; }
    else                 { startBtn.disabled = false; stopBtn.disabled = true;  dlBtn.style.display = j.run.csv_ready ? 'inline-block' : 'none'; }

    // 1 s badges + rolling tables for all 4 sensors
    for (let i = 1; i <= 4; i++) {
      const sensor = j[`s${i}`];
      setText(`s${i}-temp`, sensor.temp_1s.toFixed(2));
      setText(`s${i}-ok`, sensor.ok ? 'OK' : 'ERR');
      
      addRow(sensorBuffers[i-1], {flow: sensor.flow_1s, temp: sensor.temp_1s});
      renderTable(`s${i}-body`, sensorBuffers[i-1]);
    }

    // 10 s metrics — update every 10 polls (≈10 s at 1 Hz)
    metricsTick = (metricsTick + 1) % 10;
    if (metricsTick === 0) {
      for (let i = 1; i <= 4; i++) {
        const sensor = j[`s${i}`];
        setText(`s${i}-mean`, sensor.mean10.toFixed(2));
        setText(`s${i}-rms`,  sensor.rms10.toFixed(2));
        setText(`s${i}-cv`,   sensor.cv10.toFixed(2));
      }
    }

  }catch(e){ console.error(e); }
}

function startPolling(){ if(timer) clearInterval(timer); timer=setInterval(pull, interval); setText('auto','on'); }
startPolling();

// Start/Stop (only affects recording)
document.getElementById('btnStart').addEventListener('click', async ()=>{
  metricsTick = 0;
  try{ await fetch('/start', {method:'POST'}); }catch(e){}
  pull();
});
document.getElementById('btnStop').addEventListener('click', async ()=>{
  try{ await fetch('/stop', {method:'POST'}); }catch(e){}
  pull();
});
</script>
</body></html>)HTML";

// HTTP 
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
