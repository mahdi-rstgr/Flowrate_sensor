#pragma once
#include <Arduino.h>
#include <Wire.h>

// I2C Pin Configuration
#define SDA_PIN        4          // ESP8266 D2
#define SCL_PIN        5          // ESP8266 D1

// Sensor / Mux Configuration
#define USE_TCA9548A   1
#define TCA_ADDR       0x70
#define NUM_SENSORS    4          // Using MUX channels 0, 1, 2, 3

// Sensor I2C Address
#define SLF3X_ADDR     0x08       // All sensors share the same address

// Scale factors from datasheet
#define FLOW_SCALE     500.0f
#define TEMP_SCALE     200.0f

// Sensor State Structure
struct FlowReading {
  float flow_ml_min;
  float temp_c;
  bool  ok;
  bool  enabled;   // reflects current toggle state
};

// Global sensor enabled array - defined in main .ino file
extern bool sensor_enabled[NUM_SENSORS];

// --- Low-level I2C and CRC functions ---

// CRC-8 for Sensirion (poly 0x31, init 0xFF)
static uint8_t sensirion_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// Select mux channel 0–7
static void _tca_select(uint8_t ch) {
  if (ch >= NUM_SENSORS) return; // Safety check
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << ch);
  Wire.endTransmission();
}

// --- Sensor Control Functions ---

inline void sensors_begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // 400 kHz I2C
}

inline bool sensors_start() {
  bool ok = true;
  // Send "start continuous measurement (water)" command 0x3608 to all sensors
  for (uint8_t ch = 0; ch < NUM_SENSORS; ch++) {
    _tca_select(ch);
    delay(2);
    Wire.beginTransmission(SLF3X_ADDR);
    Wire.write(0x36); Wire.write(0x08);
    ok &= (Wire.endTransmission() == 0);
    delay(5);
  }
  return ok;
}

inline bool sensors_stop() {
  bool ok = true;
  // Send "stop continuous measurement" command 0x3F99 to all sensors
  for (uint8_t ch = 0; ch < NUM_SENSORS; ch++) {
    _tca_select(ch);
    delay(2);
    Wire.beginTransmission(SLF3X_ADDR);
    Wire.write(0x3F); Wire.write(0xF9);
    ok &= (Wire.endTransmission() == 0);
    delay(5);
  }
  return ok;
}

// Read flow & temperature from a specific sensor index (1-based)
inline FlowReading read_sensor(uint8_t sensor_index) {
  FlowReading r{0, 0, false, false};

  if (sensor_index < 1 || sensor_index > NUM_SENSORS) {
    return r;
  }

  r.enabled = sensor_enabled[sensor_index - 1];

  // If sensor is disabled, return immediately (ok=false, enabled=false)
  if (!r.enabled) {
    return r;
  }

  uint8_t ch = sensor_index - 1;
  _tca_select(ch);

  // We read first 2 words (flow, temp) = 2*2 bytes + 2 CRC = 6 bytes
  const uint8_t BYTES_TO_READ = 6;
  uint8_t raw[BYTES_TO_READ];

  uint8_t received = Wire.requestFrom((int)SLF3X_ADDR, (int)BYTES_TO_READ);
  if (received != BYTES_TO_READ) {
    return r; // NACK or not enough data
  }

  for (uint8_t i = 0; i < BYTES_TO_READ; i++) {
    raw[i] = Wire.read();
  }

  // Check CRC for flow word (raw[0], raw[1], raw[2])
  if (sensirion_crc8(&raw[0], 2) != raw[2]) {
    return r;
  }
  // Check CRC for temperature word (raw[3], raw[4], raw[5])
  if (sensirion_crc8(&raw[3], 2) != raw[5]) {
    return r;
  }

  // Convert to signed 16-bit
  int16_t flow_raw = (int16_t)((raw[0] << 8) | raw[1]);
  int16_t temp_raw = (int16_t)((raw[3] << 8) | raw[4]);

  // Apply scale factors
  r.flow_ml_min = (float)flow_raw / FLOW_SCALE;
  r.temp_c      = (float)temp_raw / TEMP_SCALE;
  r.ok          = true;

  return r;
}

// Toggle from web UI
inline void set_sensor_enabled(uint8_t sensor_index, bool enabled) {
  if (sensor_index >= 1 && sensor_index <= NUM_SENSORS) {
    sensor_enabled[sensor_index - 1] = enabled;
  }
}

inline bool get_sensor_enabled(uint8_t sensor_index) {
  if (sensor_index >= 1 && sensor_index <= NUM_SENSORS) {
    return sensor_enabled[sensor_index - 1];
  }
  return false;
}