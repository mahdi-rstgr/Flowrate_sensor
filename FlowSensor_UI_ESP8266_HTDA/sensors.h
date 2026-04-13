#pragma once
#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN        4          // ESP8266 D2
#define SCL_PIN        5          // ESP8266 D1

#define USE_TCA9548A   1
#define TCA_ADDR       0x70
#define S1_MUX_CH      0
#define S2_MUX_CH      1

#define SLF3X_ADDR1    0x08
#define SLF3X_ADDR2    0x08

#define FLOW_SCALE     500.0f
#define TEMP_SCALE     200.0f

struct FlowReading {
  float flow_ml_min;
  float temp_c;
  bool  ok;
};

static uint8_t _crc8(uint8_t msb, uint8_t lsb) {
  uint8_t crc = 0xFF;
  crc ^= msb;
  for (int i = 0; i < 8; ++i) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  crc ^= lsb;
  for (int i = 0; i < 8; ++i) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  return crc;
}

#if USE_TCA9548A
static void _tca_select(uint8_t ch) { Wire.beginTransmission(TCA_ADDR); Wire.write(1 << ch); Wire.endTransmission(); }
#endif

inline void sensors_begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
#if USE_TCA9548A
  _tca_select(S1_MUX_CH);
#endif
}

inline bool sensors_start() {
  bool ok = true;
  auto startOne = [&](uint8_t addr, uint8_t ch){
#if USE_TCA9548A
    (void)addr; _tca_select(ch);
#endif
    Wire.beginTransmission(SLF3X_ADDR1);
    Wire.write(0x36); Wire.write(0x08);
    ok &= (Wire.endTransmission() == 0);
    delay(5);
  };
#if USE_TCA9548A
  startOne(SLF3X_ADDR1, S1_MUX_CH);
  startOne(SLF3X_ADDR2, S2_MUX_CH);
#else
  startOne(SLF3X_ADDR1, 0);
  startOne(SLF3X_ADDR2, 0);
#endif
  return ok;
}

inline bool sensors_stop() {
  bool ok = true;
  auto stopOne = [&](uint8_t addr, uint8_t ch){
#if USE_TCA9548A
    (void)addr; _tca_select(ch);
#endif
    Wire.beginTransmission(SLF3X_ADDR1);
    Wire.write(0x3F); Wire.write(0xF9);
    ok &= (Wire.endTransmission() == 0);
    delay(5);
  };
#if USE_TCA9548A
  stopOne(SLF3X_ADDR1, S1_MUX_CH);
  stopOne(SLF3X_ADDR2, S2_MUX_CH);
#else
  stopOne(SLF3X_ADDR1, 0);
  stopOne(SLF3X_ADDR2, 0);
#endif
  return ok;
}

inline FlowReading read_sensor(uint8_t sensor_index) {
  FlowReading r{0,0,false};
#if USE_TCA9548A
  _tca_select(sensor_index == 1 ? S1_MUX_CH : S2_MUX_CH);
  const uint8_t addr = SLF3X_ADDR1;
#else
  const uint8_t addr = (sensor_index == 1) ? SLF3X_ADDR1 : SLF3X_ADDR2;
#endif

  Wire.requestFrom((int)addr, 9);
  if (Wire.available() != 9) return r;

  uint8_t fMSB = Wire.read(), fLSB = Wire.read(), fCRC = Wire.read();
  uint8_t tMSB = Wire.read(), tLSB = Wire.read(), tCRC = Wire.read();
  uint8_t flMSB = Wire.read(), flLSB = Wire.read(), flCRC = Wire.read(); (void)flMSB; (void)flLSB; (void)flCRC;

  if (_crc8(fMSB, fLSB) != fCRC || _crc8(tMSB, tLSB) != tCRC) return r;

  int16_t fraw = (int16_t)((fMSB << 8) | fLSB);
  int16_t traw = (int16_t)((tMSB << 8) | tLSB);
  r.flow_ml_min = fraw / FLOW_SCALE;
  r.temp_c      = traw / TEMP_SCALE;
  r.ok = true;
  return r;
}
