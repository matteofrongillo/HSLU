#include "Seeed_AMG8833_driver.h"
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <setjmp.h>
#include <stdint.h>
#include <math.h>

// ----------------------------
// Calibration values
// ----------------------------
const float AMBIENT_TEMP_C = 23.5f;
const float BOARD_SIDE_CM = 90.0f;
const char PROTOTYPE_PLACEMENT[] = "CORNER";

// ----------------------------
// Hardware settings
// ----------------------------
const uint8_t ULTRASONIC_PIN = 7;      // Grove digital port D7
const uint8_t MOTOR_I2C_ADDRESS = 0x0F;
const unsigned long SERIAL_BAUD = 115200UL;

// Grove ultrasonic uses one signal pin for trigger + echo
const unsigned long ULTRASONIC_TIMEOUT_US = 30000UL;

// Motor driver settings
const int STEPPER_TYPE = 0;            // 0 = 4 phase stepper motor
const int STEPPER_MODE = 1;            // 1 = fine mode, 1 command = 1 step

// Library documentation states 512 steps = 360 degrees
const float MOTOR_STEPS_PER_REV = 512.0f;
const float DEGREES_PER_STEP = 360.0f / MOTOR_STEPS_PER_REV;

// CORNER -> 90 degrees
const float SWEEP_MAX_DEG = 90.0f;
const int SWEEP_MAX_STEPS = 128;       // 90 / 0.703125 = 128

const unsigned long STEP_SETTLE_MS = 20UL;

// ----------------------------
// Global objects and buffers
// ----------------------------
AMG8833 thermalSensor;
static float thermalPixels[PIXEL_NUM];

// ----------------------------
// Sweep state
// ----------------------------
int currentStepPosition = 0;           // 0 .. SWEEP_MAX_STEPS
int stepDirection = 1;                 // +1 outbound, -1 return
float currentAngleDeg = 0.0f;

// ----------------------------
// Threshold function (exact)
// ----------------------------
float getHotThreshold(float distanceCm, float boardSideLength, float ambientTempC) {
  if (distanceCm <= 0.45 * boardSideLength) {
    return 30.0;
  } else if (distanceCm < 0.9 * boardSideLength) {
    return ambientTempC + 38.72 * exp(-0.04661 * distanceCm);
  } else {
    return max(24.5, ambientTempC + 1.0);
  }
}

// ----------------------------
// Ultrasonic reading
// ----------------------------
float readUltrasonicDistanceCm() {
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);

  pinMode(ULTRASONIC_PIN, INPUT);
  unsigned long durationUs = pulseIn(ULTRASONIC_PIN, HIGH, ULTRASONIC_TIMEOUT_US);

  if (durationUs == 0UL) {
    return -1.0f;  // no echo
  }

  return (durationUs * 0.0343f) / 2.0f;
}

bool isDistanceValid(float distanceCm) {
  return (distanceCm > 5.0f) && (distanceCm < BOARD_SIDE_CM);
}

// ----------------------------
// Thermal reading
// ----------------------------
bool readMaxTemperature(float &maxTempC) {
  if (thermalSensor.read_pixel_temperature(thermalPixels) != 0) {
    return false;
  }

  maxTempC = thermalPixels[0];
  for (uint8_t i = 1; i < PIXEL_NUM; ++i) {
    if (thermalPixels[i] > maxTempC) {
      maxTempC = thermalPixels[i];
    }
  }
  return true;
}

// ----------------------------
// Serial reporting
// ----------------------------
void printDistanceField(float distanceCm) {
  if (distanceCm < 0.0f) {
    Serial.print("NO_ECHO");
  } else if (!isDistanceValid(distanceCm)) {
    Serial.print("OUT_OF_RANGE");
  } else {
    Serial.print(distanceCm, 1);
  }
}

void printStatusLine(float distanceCm, float angleDeg, float maxTempC, bool thermalOk, bool detected) {
  Serial.print("Distance(cm): ");
  printDistanceField(distanceCm);

  Serial.print(" | Angle(deg): ");
  Serial.print(angleDeg, 1);

  Serial.print(" | MaxTemp(C): ");
  if (thermalOk) {
    Serial.print(maxTempC, 1);
  } else {
    Serial.print("READ_ERROR");
  }

  Serial.print(" | Detection: ");
  Serial.println(detected ? "DETECTED" : "NO");
}

void printDetectedReport(float distanceCm, float angleDeg, float maxTempC) {
  Serial.print("TARGET DETECTED -> Distance(cm): ");
  Serial.print(distanceCm, 1);
  Serial.print(" | Angle(deg): ");
  Serial.print(angleDeg, 1);
  Serial.print(" | MaxTemp(C): ");
  Serial.println(maxTempC, 1);
}

// ----------------------------
// Motor control
// ----------------------------
void updateSweepDirection() {
  if (currentStepPosition >= SWEEP_MAX_STEPS) {
    stepDirection = -1;
  } else if (currentStepPosition <= 0) {
    stepDirection = 1;
  }
}

void moveOneMotorStep(int dir) {
  Motor.StepperRun(dir, STEPPER_TYPE, STEPPER_MODE);

  currentStepPosition += dir;

  if (currentStepPosition < 0) {
    currentStepPosition = 0;
  }
  if (currentStepPosition > SWEEP_MAX_STEPS) {
    currentStepPosition = SWEEP_MAX_STEPS;
  }

  currentAngleDeg = currentStepPosition * DEGREES_PER_STEP;
}

// ----------------------------
// Arduino setup
// ----------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  Wire.begin();

  Motor.begin(MOTOR_I2C_ADDRESS);

  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);

  if (thermalSensor.init()) {
    Serial.println("AMG8833 init failed");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("Thermal sweep scanner ready");
  Serial.print("AmbientTemp(C): ");
  Serial.println(AMBIENT_TEMP_C, 1);
  Serial.print("BoardSide(cm): ");
  Serial.println(BOARD_SIDE_CM, 1);
  Serial.print("Placement: ");
  Serial.println(PROTOTYPE_PLACEMENT);
  Serial.println("Serial Monitor baud: 115200");
}

// ----------------------------
// Arduino loop
// ----------------------------
void loop() {
  float distanceCm = readUltrasonicDistanceCm();
  float maxTempC = -1000.0f;
  bool thermalOk = readMaxTemperature(maxTempC);
  bool detected = false;

  if (isDistanceValid(distanceCm) && thermalOk) {
    float hotThreshold = getHotThreshold(distanceCm, BOARD_SIDE_CM, AMBIENT_TEMP_C);
    if (maxTempC > hotThreshold) {
      detected = true;
    }
  }

  printStatusLine(distanceCm, currentAngleDeg, maxTempC, thermalOk, detected);

  if (detected) {
    printDetectedReport(distanceCm, currentAngleDeg, maxTempC);
  }

  delay(STEP_SETTLE_MS);
  updateSweepDirection();
  moveOneMotorStep(stepDirection);
  delay(STEP_SETTLE_MS);
}
