#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include "sensors.h"
#include "web.h"

// Wi-Fi 
const char* WIFI_SSID = "NAME_OF_WIFI";
const char* WIFI_PASS = "YOUR_PASSWORD";
const char* MDNS_HOST = "xyz";

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

// Always stream: no 'running' gate anymore
static const int N10 = 200; // 10 s @ 20 Hz
static float s1_flow_buf[N10] = {0}, s2_flow_buf[N10] = {0};
static float s1_temp_buf[N10] = {0}, s2_temp_buf[N10] = {0};
static int   buf_idx = -1;
static int   buf_count = 0;

static double s1_sum10 = 0,  s2_sum10 = 0;
static double s1_sumsq10 = 0, s2_sumsq10 = 0;

static bool s1_ok = false, s2_ok = false;

// CV guard threshold (mL/min)
static const float CV_MEAN_EPS = 0.02f;

// Recording 0.5 s to CSV 
static bool recording = false;   
static bool csv_ready = false;
static unsigned long run_start_ms = 0;
static unsigned long last_record_ms = 0;
static const unsigned long RECORD_MS = 500;  // 0.5 s

// helpers 
static inline int wrap(int i) { return (i + N10) % N10; }

static void reset_buffers() {
  buf_idx = -1; buf_count = 0;
  s1_sum10 = s2_sum10 = 0;
  s1_sumsq10 = s2_sumsq10 = 0;
  s1_ok = s2_ok = false;
}

static void push_sample(float f1, float t1, bool ok1, float f2, float t2, bool ok2) {
  int next = (buf_idx + 1) % N10;
  if (buf_count == N10) {
    s1_sum10   -= s1_flow_buf[next];
    s1_sumsq10 -= (double)s1_flow_buf[next] * s1_flow_buf[next];
    s2_sum10   -= s2_flow_buf[next];
    s2_sumsq10 -= (double)s2_flow_buf[next] * s2_flow_buf[next];
  } else {
    buf_count++;
  }

  s1_flow_buf[next] = f1;  s1_temp_buf[next] = t1;
  s2_flow_buf[next] = f2;  s2_temp_buf[next] = t2;

  s1_sum10   += f1;  s1_sumsq10 += (double)f1 * f1;
  s2_sum10   += f2;  s2_sumsq10 += (double)f2 * f2;

  buf_idx = next;
  s1_ok = ok1; s2_ok = ok2;
}

static void sample_20hz() {
  unsigned long now = millis();
  if (now - last_sample_ms < SAMPLE_MS) return;
  last_sample_ms = now;

  FlowReading r1 = read_sensor(1);
  FlowReading r2 = read_sensor(2);

  // if a read fails, reuse previous value
  float f1 = s1_flow_buf[buf_idx < 0 ? 0 : buf_idx];
  float t1 = s1_temp_buf[buf_idx < 0 ? 0 : buf_idx];
  float f2 = s2_flow_buf[buf_idx < 0 ? 0 : buf_idx];
  float t2 = s2_temp_buf[buf_idx < 0 ? 0 : buf_idx];

  if (r1.ok) { f1 = r1.flow_ml_min; t1 = r1.temp_c; }
  if (r2.ok) { f2 = r2.flow_ml_min; t2 = r2.temp_c; }

  push_sample(f1, t1, r1.ok, f2, t2, r2.ok);
}

static void compute_1s_means(float& f1, float& t1, float& f2, float& t2) {
  if (buf_count == 0) { f1=t1=f2=t2=0; return; }
  int n = min(buf_count, 20); // 1 s @ 20 Hz
  double sf1=0, st1=0, sf2=0, st2=0;
  for (int i=0;i<n;i++) {
    int idx = wrap(buf_idx - i);
    sf1 += s1_flow_buf[idx]; st1 += s1_temp_buf[idx];
    sf2 += s2_flow_buf[idx]; st2 += s2_temp_buf[idx];
  }
  f1 = sf1 / n; t1 = st1 / n; f2 = sf2 / n; t2 = st2 / n;
}

static void compute_10s_metrics(float& mean1, float& rms1, float& cv1,
                                float& mean2, float& rms2, float& cv2) {
  int n = buf_count; 
  if (n == 0) { mean1=rms1=cv1=mean2=rms2=cv2=0; return; }
  double m1 = s1_sum10 / n;
  double r1 = sqrt(max(0.0, s1_sumsq10 / n));
  double m2 = s2_sum10 / n;
  double r2 = sqrt(max(0.0, s2_sumsq10 / n));

  if (m1 < CV_MEAN_EPS) cv1 = 0.0;
  else                  cv1 = 100.0 * sqrt(max(0.0, r1*r1 - m1*m1)) / m1;

  if (m2 < CV_MEAN_EPS) cv2 = 0.0;
  else                  cv2 = 100.0 * sqrt(max(0.0, r2*r2 - m2*m2)) / m2;

  mean1 = m1; rms1 = r1; mean2 = m2; rms2 = r2;
}

// Recording 0.5 s to LittleFS 
void start_run() {
  LittleFS.remove("/last_run.csv");
  File f = LittleFS.open("/last_run.csv", "w");
  if (f) {
    f.println("time_s,s1_flow_ml_min,s1_temp_c,s2_flow_ml_min,s2_temp_c");
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

  int n = min(buf_count, 10); // ~0.5 s @ 20 Hz
  if (n == 0) return;
  double f1=0,t1=0,f2=0,t2=0;
  for (int i=0;i<n;i++){
    int idx = wrap(buf_idx - i);
    f1 += s1_flow_buf[idx]; t1 += s1_temp_buf[idx];
    f2 += s2_flow_buf[idx]; t2 += s2_temp_buf[idx];
  }
  f1/=n; t1/=n; f2/=n; t2/=n;

  float t_s = (now - run_start_ms) / 1000.0f;

  File f = LittleFS.open("/last_run.csv", "a");
  if (f) {
    f.printf("%.3f,%.5f,%.5f,%.5f,%.5f\n", t_s, f1, t1, f2, t2);
    f.close();
  }
}

// API snapshot for web.h 
void get_ui_snapshot(
  float& s1_flow_1s, float& s1_temp_1s, float& s1_mean10, float& s1_rms10, float& s1_cv10, bool& ok1,
  float& s2_flow_1s, float& s2_temp_1s, float& s2_mean10, float& s2_rms10, float& s2_cv10, bool& ok2,
  bool& is_recording, bool& is_csv_ready
) {
  compute_1s_means(s1_flow_1s, s1_temp_1s, s2_flow_1s, s2_temp_1s);
  compute_10s_metrics(s1_mean10, s1_rms10, s1_cv10, s2_mean10, s2_rms10, s2_cv10);
  ok1 = s1_ok; ok2 = s2_ok;
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
