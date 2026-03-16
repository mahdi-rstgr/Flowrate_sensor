/*
 * Integrated Flow Control System - Pump Controller Arduino Sketch
 * Controls 4 pumps (1, 2, 3, 4) using DRV8825 stepper drivers
 * Accepts serial commands from Python backend
 * 
 * Updated for unified naming: Pump 1, Pump 2, Pump 3, Pump 4
 * (internally maps to X, Y, Z, A for compatibility)
 */

#define PUMP1_STEP_PIN         2
#define PUMP1_DIR_PIN          5
#define PUMP1_ENABLE_PIN       8

#define PUMP2_STEP_PIN         3
#define PUMP2_DIR_PIN          6
#define PUMP2_ENABLE_PIN       8

#define PUMP3_STEP_PIN         4
#define PUMP3_DIR_PIN          7
#define PUMP3_ENABLE_PIN       8

#define PUMP4_STEP_PIN         12
#define PUMP4_DIR_PIN          13
#define PUMP4_ENABLE_PIN       8

#define STEPS_PER_REV          200.0
#define MICROSTEPPING          16
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
  char legacyName;  // X,Y,Z,A for compatibility with Python backend
  int pumpNumber;   // 1,2,3,4 for user display
};

// Initialize pumps array with unified naming
Pump pumps[4] = {
  {PUMP1_STEP_PIN, PUMP1_DIR_PIN, PUMP1_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'X', 1},
  {PUMP2_STEP_PIN, PUMP2_DIR_PIN, PUMP2_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'Y', 2},
  {PUMP3_STEP_PIN, PUMP3_DIR_PIN, PUMP3_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'Z', 3},
  {PUMP4_STEP_PIN, PUMP4_DIR_PIN, PUMP4_ENABLE_PIN, false, 0, 0, false, 0, 0, 0, 'A', 4}
};

String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  // Initialize all pins
  for (int i = 0; i < 4; i++) {
    pinMode(pumps[i].stepPin, OUTPUT);
    pinMode(pumps[i].dirPin, OUTPUT);
    pinMode(pumps[i].enablePin, OUTPUT);
    digitalWrite(pumps[i].enablePin, HIGH);  // Disable motors initially
    digitalWrite(pumps[i].dirPin, LOW);
    digitalWrite(pumps[i].stepPin, LOW);
  }

  Serial.println("Integrated Flow Control - Pump Controller Ready");
  Serial.println("Pump Mapping: 1=X, 2=Y, 3=Z, 4=A");
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
  
  if (command.startsWith("START")) {
    handleStartCommand(command);
  } else if (command.startsWith("STOP")) {
    handleStopCommand(command);
  } else if (command == "EMERGENCY") {
    handleEmergencyStop();
  } else if (command == "STATUS") {
    handleStatusRequest();
  } else if (command == "INFO") {
    handleInfoRequest();
  } else {
    Serial.println("ERROR: Unknown command");
    Serial.println("Available commands: START, STOP, EMERGENCY, STATUS, INFO");
  }
}

void handleStartCommand(String command) {
  // Parse command: START,<pump>,<rpm>,<duration>,<continuous>
  // Example: START,X,500,30,0
  
  String parts[5];
  int partIndex = 0;
  int startIndex = 0;
  
  for (int i = 0; i <= command.length(); i++) {
    if (i == command.length() || command.charAt(i) == ',') {
      if (partIndex < 5) {
        parts[partIndex++] = command.substring(startIndex, i);
      }
      startIndex = i + 1;
    }
  }

  if (partIndex != 5) {
    Serial.println("ERROR: Invalid START command format");
    Serial.println("Usage: START,<pump>,<rpm>,<duration>,<continuous>");
    return;
  }

  char pumpChar = parts[1].charAt(0);
  float rpm = parts[2].toFloat();
  int duration = parts[3].toInt();
  int continuous = parts[4].toInt();

  int pumpIndex = findPumpIndexByLegacyName(pumpChar);
  if (pumpIndex == -1) {
    Serial.print("ERROR: Invalid pump identifier '");
    Serial.print(pumpChar);
    Serial.println("'. Use X, Y, Z, or A");
    return;
  }

  // Validate parameters
  if (rpm == 0 || abs(rpm) > 3000) {
    Serial.println("ERROR: RPM must be between -3000 and 3000 (excluding 0)");
    return;
  }

  if (continuous == 0 && duration <= 0) {
    Serial.println("ERROR: Duration must be positive for non-continuous operation");
    return;
  }

  startPump(pumpIndex, rpm, duration, continuous == 1);
  Serial.println("OK");
}

void handleStopCommand(String command) {
  // Parse command: STOP,<pump>
  // Example: STOP,X
  
  int commaIndex = command.indexOf(',');
  if (commaIndex == -1 || commaIndex == command.length() - 1) {
    Serial.println("ERROR: Invalid STOP command format");
    Serial.println("Usage: STOP,<pump>");
    return;
  }

  char pumpChar = command.charAt(commaIndex + 1);
  int pumpIndex = findPumpIndexByLegacyName(pumpChar);
  
  if (pumpIndex == -1) {
    Serial.print("ERROR: Invalid pump identifier '");
    Serial.print(pumpChar);
    Serial.println("'. Use X, Y, Z, or A");
    return;
  }

  stopPump(pumpIndex);
  Serial.println("OK");
}

void handleEmergencyStop() {
  Serial.println("EMERGENCY STOP ACTIVATED");
  for (int i = 0; i < 4; i++) {
    stopPump(i);
  }
  Serial.println("OK");
}

void handleStatusRequest() {
  // Return JSON status compatible with Python backend
  Serial.print("{\"X\":");
  Serial.print(pumps[0].isRunning ? 1 : 0);
  Serial.print(",\"Y\":");
  Serial.print(pumps[1].isRunning ? 1 : 0);
  Serial.print(",\"Z\":");
  Serial.print(pumps[2].isRunning ? 1 : 0);
  Serial.print(",\"A\":");
  Serial.print(pumps[3].isRunning ? 1 : 0);
  Serial.println("}");
}

void handleInfoRequest() {
  Serial.println("=== Integrated Flow Control - Pump Controller Info ===");
  Serial.println("Firmware: v1.0");
  Serial.println("Pumps: 4 (Stepper motors with DRV8825 drivers)");
  Serial.println("Control: Direct pulse generation");
  Serial.println("Max RPM: 3000");
  Serial.println("Pump Mapping:");
  
  for (int i = 0; i < 4; i++) {
    Serial.print("  Pump ");
    Serial.print(pumps[i].pumpNumber);
    Serial.print(" (");
    Serial.print(pumps[i].legacyName);
    Serial.print("): Step=");
    Serial.print(pumps[i].stepPin);
    Serial.print(", Dir=");
    Serial.print(pumps[i].dirPin);
    Serial.print(", Status=");
    Serial.println(pumps[i].isRunning ? "Running" : "Stopped");
  }
  
  Serial.println("Commands: START, STOP, EMERGENCY, STATUS, INFO");
  Serial.println("==========================================");
}

int findPumpIndexByLegacyName(char pumpChar) {
  for (int i = 0; i < 4; i++) {
    if (pumps[i].legacyName == pumpChar) {
      return i;
    }
  }
  return -1;
}

void startPump(int pumpIndex, float rpm, int duration, bool continuous) {
  Pump* pump = &pumps[pumpIndex];

  // Safety: stop pump before restart
  stopPump(pumpIndex);

  // Set rotation direction
  digitalWrite(pump->dirPin, (rpm > 0) ? HIGH : LOW);
  
  // Calculate step interval
  rpm = abs(rpm);
  float stepsPerSec = (rpm / 60.0) * EFFECTIVE_STEPS_PER_REV;
  pump->intervalMicros = (1.0 / stepsPerSec) * 1e6;

  // Configure pump parameters
  pump->rpm = rpm;
  pump->duration = duration;
  pump->continuous = continuous;
  pump->isRunning = true;
  pump->startTime = millis();
  pump->lastStepTime = micros();

  // Enable motor (LOW = enabled)
  digitalWrite(pump->enablePin, LOW);

  // Log pump start
  Serial.print("Started Pump ");
  Serial.print(pump->pumpNumber);
  Serial.print(" (");
  Serial.print(pump->legacyName);
  Serial.print(") @ ");
  Serial.print(rpm, 2);
  Serial.print(" RPM ");
  Serial.print((digitalRead(pump->dirPin) ? "(Forward)" : "(Reverse)"));
  
  if (continuous) {
    Serial.println(" [Continuous Mode]");
  } else {
    Serial.print(" [Duration: ");
    Serial.print(duration);
    Serial.println(" seconds]");
  }
}

void stopPump(int pumpIndex) {
  Pump* pump = &pumps[pumpIndex];
  
  if (pump->isRunning) {
    pump->isRunning = false;
    digitalWrite(pump->enablePin, HIGH); // Disable motor (HIGH = disabled)
    
    Serial.print("Stopped Pump ");
    Serial.print(pump->pumpNumber);
    Serial.print(" (");
    Serial.print(pump->legacyName);
    Serial.println(")");
  }
}

void updatePumps() {
  unsigned long nowMillis = millis();
  unsigned long nowMicros = micros();

  for (int i = 0; i < 4; i++) {
    Pump* pump = &pumps[i];
    
    if (!pump->isRunning) continue;

    // Check if duration has elapsed (for non-continuous mode)
    if (!pump->continuous && 
        (nowMillis - pump->startTime >= (unsigned long)(pump->duration * 1000))) {
      stopPump(i);
      continue;
    }

    // Generate step pulses
    if ((nowMicros - pump->lastStepTime) >= pump->intervalMicros) {
      digitalWrite(pump->stepPin, HIGH);
      delayMicroseconds(2);  // Minimum pulse width for DRV8825
      digitalWrite(pump->stepPin, LOW);
      pump->lastStepTime = nowMicros;
    }
  }
}