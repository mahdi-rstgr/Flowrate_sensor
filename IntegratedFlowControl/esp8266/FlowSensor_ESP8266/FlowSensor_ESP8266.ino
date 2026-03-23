#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include "sensors.h"
#include "web.h"

// Wi-Fi Configuration
// UPDATE THESE WITH YOUR WIFI CREDENTIALS
const char* WIFI_SSID = "AC-IoT-Cudy-18DC";
const char* WIFI_PASS = "31185981";
const char* MDNS_HOST = "flowssensors";

// Global sensor enabled state (referenced by sensors.h and web.h)
bool sensor_enabled[NUM_SENSORS] = { true, true, true, true };

static void wifi_connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[wifi] Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }       
  Serial.println();
  Serial.printf("[wifi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  
  // Print prominent IP address banner
  Serial.println();
  Serial.println("================================================");
  Serial.println("          ESP8266 FLOW SENSOR READY");
  Serial.println("================================================");
  Serial.printf("     IP ADDRESS: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("     WEB ACCESS: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("     NETWORK: %s\n", WIFI_SSID);
  Serial.println("================================================");
  Serial.println();
  
  if (MDNS.begin(MDNS_HOST)) {
    Serial.printf("[mdns] mDNS available at: http://%s.local\n", MDNS_HOST);
  }
}

// Fast sampling 20 Hz
static const unsigned long SAMPLE_MS = 50;   // 20 Hz
static unsigned long last_sample_ms = 0;

// Periodic IP address reminder (every 60 seconds)
static unsigned long last_ip_reminder_ms = 0;
static const unsigned long IP_REMINDER_MS = 60000;  // 60 seconds

// Periodic storage cleanup check (every 5 minutes)
static unsigned long last_cleanup_check_ms = 0;
static const unsigned long CLEANUP_CHECK_MS = 300000;  // 5 minutes

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

// Binary record structure for efficient storage
struct SensorRecord {
  float time_s;                    // 4 bytes
  float flow[NUM_SENSORS];        // 16 bytes (4 sensors × 4 bytes)
  float temp[NUM_SENSORS];        // 16 bytes (4 sensors × 4 bytes) 
  uint8_t enabled_mask;           // 1 byte for enabled sensors bitmask
};  // Total: 37 bytes per record vs ~60 bytes for CSV

// Recording to Binary
static bool recording   = false;
static bool data_ready  = false;
static unsigned long run_start_ms  = 0;
static unsigned long last_record_ms = 0;
static const unsigned long RECORD_MS = 1000;  // 1.0 s

// Which sensors are recorded (snapshot at start_run)
static bool record_mask[NUM_SENSORS] = { true, true, true, true };

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

static void push_sample(FlowReading readings[]) {
  int next = (buf_idx + 1) % N10;
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    float f = readings[i].flow_ml_min;
    float t = readings[i].temp_c;
    bool ok = readings[i].ok;
    bool enabled = readings[i].enabled;

    if (buf_count == N10) {
      s_sum10[i]   -= s_flow_buf[i][next];
      s_sumsq10[i] -= (double)s_flow_buf[i][next] * s_flow_buf[i][next];
    }

    if (enabled && ok) {
      // Normal update
      s_flow_buf[i][next] = f;
      s_temp_buf[i][next] = t;
      s_sum10[i]   += f;
      s_sumsq10[i] += (double)f * f;
    } else {
      // For disabled or bad readings, hold last value; don't modify sums
      int prev = (buf_idx < 0) ? next : buf_idx;
      s_flow_buf[i][next] = s_flow_buf[i][prev];
      s_temp_buf[i][next] = s_temp_buf[i][prev];
    }

    s_ok[i] = ok;
  }

  if (buf_count < N10) buf_count++;
  buf_idx = next;
}

static void sample_20hz() {
  unsigned long now = millis();
  if (now - last_sample_ms < SAMPLE_MS) return;
  last_sample_ms = now;

  FlowReading readings[NUM_SENSORS];
  for (int i = 0; i < NUM_SENSORS; i++) {
    readings[i] = read_sensor((uint8_t)(i + 1));
  }

  push_sample(readings);
}

static void compute_1s_means(float f_1s[], float t_1s[]) {
  if (buf_count == 0) {
    for (int i = 0; i < NUM_SENSORS; i++) {
      f_1s[i] = 0;
      t_1s[i] = 0;
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
    f_1s[i] = sf / n; 
    t_1s[i] = st / n;
  }
}

static void compute_10s_metrics(float mean10[], float rms10[], float cv10[]) {
  int n = buf_count; 
  if (n == 0) { 
    for (int i = 0; i < NUM_SENSORS; i++) {
      mean10[i] = 0;
      rms10[i]  = 0;
      cv10[i]   = 0;
    }
    return; 
  }
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    double m = s_sum10[i] / n;
    double r = sqrt(max(0.0, s_sumsq10[i] / n));

    if (m < CV_MEAN_EPS) {
      cv10[i] = 0.0;
    } else {
      cv10[i] = 100.0 * sqrt(max(0.0, r*r - m*m)) / m;
    }

    mean10[i] = m; 
    rms10[i]  = r;
  }
}

// Storage check - stop if less than 10% free space
static bool check_storage_available() {
  FSInfo fs_info;
  LittleFS.info(fs_info);
  size_t used = fs_info.usedBytes;
  size_t total = fs_info.totalBytes;
  float percent_used = (float)used / total * 100.0;
  
  if (percent_used > 90.0) {
    Serial.printf("[storage] WARNING: %.1f%% full, stopping recording\n", percent_used);
    return false;
  }
  return true;
}

// Storage cleanup - remove old data files when storage gets full
void cleanup_storage() {
  Serial.println("[storage] Cleaning up storage...");
  
  FSInfo fs_info;
  LittleFS.info(fs_info);
  size_t used_before = fs_info.usedBytes;
  size_t total = fs_info.totalBytes;
  float percent_before = (float)used_before / total * 100.0;
  
  Serial.printf("[storage] Before cleanup: %.1f%% full (%.1fKB/%.1fKB)\n", 
                percent_before, used_before/1024.0, total/1024.0);
  
  // Remove main data file
  if (LittleFS.exists("/data.bin")) {
    LittleFS.remove("/data.bin");
    Serial.println("[storage] Removed /data.bin");
  }
  
  // Remove any backup/temp files that might exist
  if (LittleFS.exists("/data_backup.bin")) {
    LittleFS.remove("/data_backup.bin");
    Serial.println("[storage] Removed /data_backup.bin");
  }
  
  if (LittleFS.exists("/log.csv")) {
    LittleFS.remove("/log.csv");
    Serial.println("[storage] Removed /log.csv");
  }
  
  // Clear any other files in root directory (be careful to preserve system files)
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String filename = dir.fileName();
    if (filename.startsWith("/data") || filename.startsWith("/log") || 
        filename.endsWith(".csv") || filename.endsWith(".bin")) {
      LittleFS.remove(filename);
      Serial.printf("[storage] Removed %s\n", filename.c_str());
    }
  }
  
  // Check storage after cleanup
  LittleFS.info(fs_info);
  size_t used_after = fs_info.usedBytes;
  float percent_after = (float)used_after / total * 100.0;
  size_t freed = used_before - used_after;
  
  Serial.printf("[storage] After cleanup: %.1f%% full (%.1fKB/%.1fKB) - Freed %.1fKB\n", 
                percent_after, used_after/1024.0, total/1024.0, freed/1024.0);
                
  data_ready = false;  // No data available after cleanup
}

// Check for storage cleanup needs - call this periodically
void auto_cleanup_if_needed() {
  FSInfo fs_info;
  LittleFS.info(fs_info);
  size_t used = fs_info.usedBytes;
  size_t total = fs_info.totalBytes;
  float percent_used = (float)used / total * 100.0;
  
  // Auto-cleanup when storage gets to 85% to prevent hitting 90% limit
  if (percent_used > 85.0) {
    Serial.printf("[storage] Auto-cleanup triggered at %.1f%% full\n", percent_used);
    cleanup_storage();
  }
}

// Recording 1.0 s to LittleFS (Binary Format)
void start_run() {
  // Check storage availability
  if (!check_storage_available()) {
    Serial.println("[run] Cannot start - storage nearly full");
    return;
  }

  // Snapshot which sensors are currently enabled for this run
  for (int i = 0; i < NUM_SENSORS; i++) {
    record_mask[i] = sensor_enabled[i];
  }

  // Create/clear binary data file
  LittleFS.remove("/data.bin");
  File f = LittleFS.open("/data.bin", "w");
  if (f) {
    f.close();
  }

  data_ready  = false;
  recording   = true;
  run_start_ms = millis();
  last_record_ms = 0;

  Serial.println("[run] START recording (binary format)");
}

void stop_run() {
  recording = false;
  data_ready = true; // binary file has data
  Serial.println("[run] STOP recording; Binary data ready");
}

static void record_if_due() {
  if (!recording) return;
  
  // Check storage every 10 seconds
  static unsigned long last_storage_check = 0;
  unsigned long now = millis();
  if (now - last_storage_check > 10000) {
    last_storage_check = now;
    if (!check_storage_available()) {
      stop_run();
      return;
    }
  }
  
  if (now - last_record_ms < RECORD_MS) return;
  last_record_ms = now;

  int n = min(buf_count, 20); // ~1.0 s @ 20 Hz
  if (n == 0) return;
  
  double f_avg[NUM_SENSORS] = {0};
  double t_avg[NUM_SENSORS] = {0};

  for (int i = 0; i < NUM_SENSORS; i++) {
    for (int j = 0; j < n; j++){
      int idx = wrap(buf_idx - j);
      f_avg[i] += s_flow_buf[i][idx]; 
      t_avg[i] += s_temp_buf[i][idx];
    }
    f_avg[i] /= n; 
    t_avg[i] /= n;
  }

  float t_s = (now - run_start_ms) / 1000.0f;

  // Create binary record
  SensorRecord record;
  record.time_s = t_s;
  record.enabled_mask = 0;
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    record.flow[i] = f_avg[i];
    record.temp[i] = t_avg[i];
    if (record_mask[i]) {
      record.enabled_mask |= (1 << i);
    }
  }

  // Write binary record
  File f = LittleFS.open("/data.bin", "a");
  if (f) {
    f.write((uint8_t*)&record, sizeof(record));
    f.close();
  }
}

// API snapshot for web.h 
void get_ui_snapshot(
  float s_flow_1s[], float s_temp_1s[], 
  float s_mean10[], float s_rms10[], float s_cv10[], 
  bool s_ok_out[],
  bool& is_recording, bool& is_csv_ready
) {
  compute_1s_means(s_flow_1s, s_temp_1s);
  compute_10s_metrics(s_mean10, s_rms10, s_cv10);
  for (int i = 0; i < NUM_SENSORS; i++) {
    s_ok_out[i] = s_ok[i];
  }
  is_recording = recording; 
  is_csv_ready = data_ready;
}

bool stream_csv_to_client(ESP8266WebServer& server) {
  File f = LittleFS.open("/data.bin", "r");
  if (!f) return false;
  
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Disposition", "attachment; filename=flow_data.csv");
  server.send(200, "text/csv", "");
  
  // Send CSV header
  String header = "time_s";
  for (int i = 0; i < NUM_SENSORS; i++) {
    int sn = i + 1;
    header += ",s" + String(sn) + "_flow_ml_min,s" + String(sn) + "_temp_c";
  }
  header += "\n";
  server.sendContent(header);
  
  // Read binary records and convert to CSV
  SensorRecord record;
  while (f.readBytes((char*)&record, sizeof(record)) == sizeof(record)) {
    String csvLine = String(record.time_s, 1);
    
    for (int i = 0; i < NUM_SENSORS; i++) {
      // Check if sensor was enabled when recorded
      if (record.enabled_mask & (1 << i)) {
        csvLine += "," + String(record.flow[i], 2) + "," + String(record.temp[i], 1);
      } else {
        csvLine += ",,";  // Empty cells for disabled sensor
      }
    }
    csvLine += "\n";
    server.sendContent(csvLine);
    yield(); // Allow ESP8266 to handle other tasks
  }
  f.close();
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[boot] Integrated Flow Control - ESP8266 Sensors");

  wifi_connect();
  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
  }
  sensors_begin();
  sensors_start();
  web_begin();
  reset_buffers();

  Serial.println("[setup] Flow sensors ready");
  Serial.printf("[setup] Access web interface at: http://%s\n", WiFi.localIP().toString().c_str());
  
  // Additional prominent IP display for backend connection
  Serial.println();
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println("  COPY THIS IP ADDRESS FOR BACKEND CONFIG:");
  Serial.printf("  >>> %s <<<\n", WiFi.localIP().toString().c_str());  
  Serial.println("  Add this IP to esp8266_handler.py if needed");
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println();
}

void loop() {
  sample_20hz();      // always sampling
  record_if_due();    // only when recording == true
  web_loop();
  
  unsigned long now = millis();
  
  // Periodic IP address reminder for easy backend configuration
  if (now - last_ip_reminder_ms > IP_REMINDER_MS) {
    last_ip_reminder_ms = now;
    Serial.println("--------------------------------------------");
    Serial.printf("ESP8266 IP: %s | Status: %s\n", 
                  WiFi.localIP().toString().c_str(),
                  WiFi.isConnected() ? "Connected" : "Disconnected");
    Serial.println("--------------------------------------------");
  }
  
  // Periodic storage cleanup check to prevent storage from getting full
  if (now - last_cleanup_check_ms > CLEANUP_CHECK_MS) {
    last_cleanup_check_ms = now;
    auto_cleanup_if_needed();
  }
}