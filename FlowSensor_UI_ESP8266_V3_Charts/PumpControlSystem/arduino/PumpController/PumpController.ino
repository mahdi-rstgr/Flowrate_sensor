/*
 * Pump Controller Arduino Sketch (Updated: Direct Pulse Control)
 * Controls 4 pumps (X, Y, Z, A) using DRV8825 stepper drivers
 * Accepts JSON-style commands from Python or HTML/JS frontend
 * 
 * REPLACES AccelStepper with raw micros()-based pulse logic
 * for higher RPM and better flow at top speeds.
 */
 

#define X_STEP_PIN         2
#define X_DIR_PIN          5
#define X_ENABLE_PIN       8

#define Y_STEP_PIN         3
#define Y_DIR_PIN          6
#define Y_ENABLE_PIN       8

#define Z_STEP_PIN         4
#define Z_DIR_PIN          7
#define Z_ENABLE_PIN       8

#define A_STEP_PIN         12
#define A_DIR_PIN          13
#define A_ENABLE_PIN       8

#define STEPS_PER_REV      200.0
#define MICROSTEPPING      16
#define EFFECTIVE_STEPS_PER_REV (STEPS_PER_REV * MICROSTEPPING)

struct Pump {
  int stepPin;
  int dirPin;
  int enablePin;
  bool isRunning;
  float rpm;
  int duration;
  bool continuous;
  unsigned long startTime;
  float intervalMicros;
  unsigned long lastStepTime;
  char name;
};

Pump pumps[4] = {
  {X_STEP_PIN, X_DIR_PIN, X_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'X'},
  {Y_STEP_PIN, Y_DIR_PIN, Y_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'Y'},
  {Z_STEP_PIN, Z_DIR_PIN, Z_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'Z'},
  {A_STEP_PIN, A_DIR_PIN, A_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'A'}
};

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  for (int i = 0; i < 4; i++) {
    pinMode(pumps[i].stepPin, OUTPUT);
    pinMode(pumps[i].dirPin, OUTPUT);
    pinMode(pumps[i].enablePin, OUTPUT);
    digitalWrite(pumps[i].enablePin, HIGH);
    digitalWrite(pumps[i].dirPin, LOW);
    digitalWrite(pumps[i].stepPin, LOW);
  }

  Serial.println("Arduino Pump Controller Ready (Direct Pulse Mode)");
}

void loop() {
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }

  updatePumps();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else if (inChar != '\r') {
      inputString += inChar;
    }
  }
}

void processCommand(String command) {
  command.trim();
  if (command.startsWith("START")) handleStartCommand(command);
  else if (command.startsWith("STOP")) handleStopCommand(command);
  else if (command == "EMERGENCY") handleEmergencyStop();
  else if (command == "STATUS") handleStatusRequest();
  else Serial.println("ERROR: Unknown command");
}

void handleStartCommand(String command) {
  String parts[5];
  int partIndex = 0, startIndex = 0;
  for (int i = 0; i <= command.length(); i++) {
    if (i == command.length() || command.charAt(i) == ',') {
      if (partIndex < 5) parts[partIndex++] = command.substring(startIndex, i);
      startIndex = i + 1;
    }
  }

  if (partIndex != 5) {
    Serial.println("ERROR: Invalid START command format");
    return;
  }

  char pumpChar = parts[1].charAt(0);
  float rpm = parts[2].toFloat();
  int duration = parts[3].toInt();
  int continuous = parts[4].toInt();

  int pumpIndex = findPumpIndex(pumpChar);
  if (pumpIndex == -1) {
    Serial.println("ERROR: Invalid pump identifier");
    return;
  }

  if (rpm == 0 || abs(rpm) > 3000) {
    Serial.println("ERROR: RPM must be between -3000 and 3000");
    return;
  }

  if (continuous == 0 && duration <= 0) {
    Serial.println("ERROR: Duration must be positive");
    return;
  }

  startPump(pumpIndex, rpm, duration, continuous == 1);
  Serial.println("OK");
}

void handleStopCommand(String command) {
  int commaIndex = command.indexOf(',');
  if (commaIndex == -1 || commaIndex == command.length() - 1) {
    Serial.println("ERROR: Invalid STOP command");
    return;
  }

  char pumpChar = command.charAt(commaIndex + 1);
  int pumpIndex = findPumpIndex(pumpChar);
  if (pumpIndex == -1) {
    Serial.println("ERROR: Invalid pump");
    return;
  }

  stopPump(pumpIndex);
  Serial.println("OK");
}

void handleEmergencyStop() {
  for (int i = 0; i < 4; i++) stopPump(i);
  Serial.println("OK");
}

void handleStatusRequest() {
  Serial.print("{\"X\":"); Serial.print(pumps[0].isRunning ? 1 : 0);
  Serial.print(",\"Y\":"); Serial.print(pumps[1].isRunning ? 1 : 0);
  Serial.print(",\"Z\":"); Serial.print(pumps[2].isRunning ? 1 : 0);
  Serial.print(",\"A\":"); Serial.print(pumps[3].isRunning ? 1 : 0);
  Serial.println("}");
}

int findPumpIndex(char pumpChar) {
  for (int i = 0; i < 4; i++) if (pumps[i].name == pumpChar) return i;
  return -1;
}

void startPump(int pumpIndex, float rpm, int duration, bool continuous) {
  Pump* pump = &pumps[pumpIndex];

  stopPump(pumpIndex); // Safety: stop before restart

  digitalWrite(pump->dirPin, (rpm > 0) ? HIGH : LOW);
  rpm = abs(rpm);
  float stepsPerSec = (rpm / 60.0) * EFFECTIVE_STEPS_PER_REV;
  pump->intervalMicros = (1.0 / stepsPerSec) * 1e6;

  pump->rpm = rpm;
  pump->duration = duration;
  pump->continuous = continuous;
  pump->isRunning = true;
  pump->startTime = millis();
  pump->lastStepTime = micros();

  digitalWrite(pump->enablePin, LOW); // Enable motor

  Serial.print("Started pump ");
  Serial.print(pump->name);
  Serial.print(" @ ");
  Serial.print(rpm, 2);
  Serial.print(" RPM ");
  Serial.println((digitalRead(pump->dirPin) ? "(Forward)" : "(Reverse)"));
}

void stopPump(int pumpIndex) {
  Pump* pump = &pumps[pumpIndex];
  if (pump->isRunning) {
    pump->isRunning = false;
    digitalWrite(pump->enablePin, HIGH); // Disable motor
    Serial.print("Stopped pump ");
    Serial.println(pump->name);
  }
}

void updatePumps() {
  unsigned long nowMillis = millis();
  unsigned long nowMicros = micros();

  for (int i = 0; i < 4; i++) {
    Pump* pump = &pumps[i];
    if (!pump->isRunning) continue;

    if (!pump->continuous && (nowMillis - pump->startTime >= (unsigned long)(pump->duration * 1000))) {
      stopPump(i);
      continue;
    }

    if ((nowMicros - pump->lastStepTime) >= pump->intervalMicros) {
      digitalWrite(pump->stepPin, HIGH);
      delayMicroseconds(2);
      digitalWrite(pump->stepPin, LOW);
      pump->lastStepTime = nowMicros;
    }
  }
}