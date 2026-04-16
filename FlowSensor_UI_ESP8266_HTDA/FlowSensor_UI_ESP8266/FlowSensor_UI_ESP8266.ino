#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include "sensors.h"
#include "web.h"

// Global sensor enabled array
bool sensor_enabled[NUM_SENSORS] = { true, true, true, true };

// Wi-Fi 
const char* WIFI_SSID = "AC-IoT-Cudy-18DC";
const char* WIFI_PASS = "31185981";
const char* MDNS_HOST = "HTDA-Flow";

static void wifi_connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[wifi] Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.println();
  Serial.printf("[wifi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  if (MDNS.begin(MDNS_HOST)) Serial.printf("[mdns] http://%s.local\n", MDNS_HOST);
}

// Fast sampling 20 Hz
static const unsigned long SAMPLE_MS = 50;   // 20 Hz
static unsigned long last_sample_ms = 0;

// Data buffers for 4 sensors
static const int N10 = 200; // 10 s @ 20 Hz
static float s_flow_buf[NUM_SENSORS][N10] = {0};
static float s_temp_buf[NUM_SENSORS][N10] = {0};
static int   buf_idx = -1;
static int   buf_count = 0;

// Sums for mean/RMS calculation
static double s_sum10[NUM_SENSORS]   = {0};
static double s_sumsq10[NUM_SENSORS] = {0};

// Sensor status
static bool s_ok[NUM_SENSORS] = {false};

// CV guard threshold (mL/min)
static const float CV_MEAN_EPS = 0.02f;

// Recording 10.0 s to CSV 
static bool recording = false;   
static bool csv_ready = false;
static unsigned long run_start_ms = 0;
static unsigned long last_record_ms = 0;
static const unsigned long RECORD_MS = 10000;  // 10.0 s

// helpers 
static inline int wrap(int i) { return (i + N10) % N10; }

static void reset_buffers() {
  buf_idx = -1; 
  buf_count = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    s_sum10[i]   = 0;
    s_sumsq10[i] = 0;
    s_ok[i]      = false;
    for (int j = 0; j < N10; j++) {
      s_flow_buf[i][j] = 0;
      s_temp_buf[i][j] = 0;
    }
  }
}

static void push_sample(float f[NUM_SENSORS], float t[NUM_SENSORS], bool ok[NUM_SENSORS]) {
  int next = (buf_idx + 1) % N10;
  if (buf_count == N10) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      s_sum10[i]   -= s_flow_buf[i][next];
      s_sumsq10[i] -= (double)s_flow_buf[i][next] * s_flow_buf[i][next];
    }
  } else {
    buf_count++;
  }

  for (int i = 0; i < NUM_SENSORS; i++) {
    s_flow_buf[i][next] = f[i];  
    s_temp_buf[i][next] = t[i];
    s_sum10[i]   += f[i];  
    s_sumsq10[i] += (double)f[i] * f[i];
    s_ok[i] = ok[i];
  }

  buf_idx = next;
}

static void sample_20hz() {
  unsigned long now = millis();
  if (now - last_sample_ms < SAMPLE_MS) return;
  last_sample_ms = now;

  float f[NUM_SENSORS], t[NUM_SENSORS];
  bool ok[NUM_SENSORS];

  // Read all 4 sensors
  for (int i = 0; i < NUM_SENSORS; i++) {
    FlowReading r = read_sensor(i + 1);
    
    // if a read fails, reuse previous value
    if (buf_idx >= 0 && !r.ok) {
      f[i] = s_flow_buf[i][buf_idx];
      t[i] = s_temp_buf[i][buf_idx];
    } else {
      f[i] = r.flow_ml_min;
      t[i] = r.temp_c;
    }
    ok[i] = r.ok;
  }

  push_sample(f, t, ok);
}

static void compute_1s_means(float f[NUM_SENSORS], float t[NUM_SENSORS]) {
  if (buf_count == 0) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      f[i] = t[i] = 0;
    }
    return; 
  }
  int n = min(buf_count, 20); // 1 s @ 20 Hz
  for (int i = 0; i < NUM_SENSORS; i++) {
    double sf = 0, st = 0;
    for (int j = 0; j < n; j++) {
      int idx = wrap(buf_idx - j);
      sf += s_flow_buf[i][idx]; 
      st += s_temp_buf[i][idx];
    }
    f[i] = sf / n; 
    t[i] = st / n;
  }
}

static void compute_10s_metrics(float mean[NUM_SENSORS], float rms[NUM_SENSORS], float cv[NUM_SENSORS]) {
  int n = buf_count; 
  if (n == 0) { 
    for (int i = 0; i < NUM_SENSORS; i++) {
      mean[i] = rms[i] = cv[i] = 0;
    }
    return; 
  }
  for (int i = 0; i < NUM_SENSORS; i++) {
    double m = s_sum10[i] / n;
    double r = sqrt(max(0.0, s_sumsq10[i] / n));

    if (m < CV_MEAN_EPS) cv[i] = 0.0;
    else                 cv[i] = 100.0 * sqrt(max(0.0, r*r - m*m)) / m;

    mean[i] = m; 
    rms[i] = r;
  }
}

// Recording 10.0 s to LittleFS 
void start_run() {
  LittleFS.remove("/last_run.csv");
  File f = LittleFS.open("/last_run.csv", "w");
  if (f) {
    f.println("time_s,s1_flow_ml_min,s1_temp_c,s2_flow_ml_min,s2_temp_c,s3_flow_ml_min,s3_temp_c,s4_flow_ml_min,s4_temp_c");
    f.close();
  }
  // do NOT stop streaming; just reset CSV state
  csv_ready = false;
  recording = true;
  run_start_ms = millis();
  last_record_ms = 0;
  Serial.println("[run] START recording");
}

void stop_run() {
  recording = false;
  csv_ready = true; // file has data
  Serial.println("[run] STOP recording; CSV ready");
}

static void record_if_due() {
  if (!recording) return;
  unsigned long now = millis();
  if (now - last_record_ms < RECORD_MS) return;
  last_record_ms = now;

  // Calculate mean over 10-second interval (200 samples @ 20Hz)
  int n = min(buf_count, 200); // ~10.0 s @ 20 Hz
  if (n == 0) return;
  
  double f[NUM_SENSORS] = {0}, t[NUM_SENSORS] = {0};
  for (int i = 0; i < n; i++) {
    int idx = wrap(buf_idx - i);
    for (int s = 0; s < NUM_SENSORS; s++) {
      f[s] += s_flow_buf[s][idx]; 
      t[s] += s_temp_buf[s][idx];
    }
  }
  
  // Average the values
  for (int s = 0; s < NUM_SENSORS; s++) {
    f[s] /= n; 
    t[s] /= n;
  }

  float t_s = (now - run_start_ms) / 1000.0f;

  File file = LittleFS.open("/last_run.csv", "a");
  if (file) {
    file.printf("%.3f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", 
                t_s, f[0], t[0], f[1], t[1], f[2], t[2], f[3], t[3]);
    file.close();
  }
}

// API snapshot for web.h 
void get_ui_snapshot(
  float s_flow_1s[NUM_SENSORS], float s_temp_1s[NUM_SENSORS], 
  float s_mean10[NUM_SENSORS], float s_rms10[NUM_SENSORS], float s_cv10[NUM_SENSORS], 
  bool s_ok_out[NUM_SENSORS],
  bool& is_recording, bool& is_csv_ready
) {
  compute_1s_means(s_flow_1s, s_temp_1s);
  compute_10s_metrics(s_mean10, s_rms10, s_cv10);
  for (int i = 0; i < NUM_SENSORS; i++) {
    s_ok_out[i] = s_ok[i];
  }
  is_recording = recording; is_csv_ready = csv_ready;
}

bool stream_csv_to_client(ESP8266WebServer& server) {
  File f = LittleFS.open("/last_run.csv", "r");
  if (!f) return false;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/csv", "");
  while (f.available()) {
    uint8_t buf[512];
    size_t n = f.read(buf, sizeof(buf));
    server.sendContent_P((const char*)buf, n);
    yield();
  }
  f.close();
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[boot] ESP8266 Flow Dashboard");

  wifi_connect();
  if (!LittleFS.begin()) { LittleFS.format(); LittleFS.begin(); }
  sensors_begin();
  sensors_start();
  web_begin();  // routes for UI and CSV
}

void loop() {
  sample_20hz();      // always sampling/streaming
  record_if_due();    // only when recording == true
  web_loop();
}
