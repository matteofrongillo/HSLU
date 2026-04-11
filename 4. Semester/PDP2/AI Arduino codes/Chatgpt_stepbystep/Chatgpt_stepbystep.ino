// ===================== INCLUDES =====================
#include "Seeed_AMG8833_driver.h"
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <setjmp.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

// ===================== CONSTANTS / GLOBALS =====================

// Motor driver settings
#define I2C_ADDRESS 0x0f

// --- CALIBRATION VALUES ---
// Replace these only after validation
float ambientTempC = 23.5;
float boardSideCm = 80.0;
char prototypePlacement[] = "CORNER"; // CORNER or BORDER

// Placement options
#define PLACEMENT_CORNER "CORNER"
#define PLACEMENT_BORDER "BORDER"

// Sweep angle (will depend on placement)
int SWEEP_ANGLE_DEG = 90; // CORNER -> 90°, BORDER -> 180°

// --- MOTOR CONFIGURATION ---
const int START_OFFSET_STEPS = 0;

// Base motor resolution (steps for 90 degrees)
const float BASE_STEPS_PER_90_DEG = 128.0;

int SWEEP_STEPS = 128;
int currentMotorPosition = 0;

// Derived conversion
float stepsPerDegree = BASE_STEPS_PER_90_DEG / 90.0;

// Ultrasonic sensor pin (Grove)
const int ultrasonicPin = 5;

// Optional reset button logic from original sketch
const int resetButtonPin = A1;
jmp_buf resetContext;

// Thermal camera
AMG8833 sensor;

#define ROWS 8
#define COLS 8

// ===================== HELPER FUNCTIONS =====================

void performReset() {
  Motor.stop(MOTOR1);
  Motor.stop(MOTOR2);

  while (digitalRead(resetButtonPin) == HIGH) {
    delay(10);
  }

  delay(500);
  longjmp(resetContext, 1);
}

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (digitalRead(resetButtonPin) == HIGH) {
      performReset();
    }
    delay(1);
  }
}

float getHotThreshold(float distanceCm, float boardSideLength, float ambientTempC) {
  if (distanceCm <= 0.45 * boardSideLength) {
    return 30.0;
  } else if (distanceCm < 0.9 * boardSideLength) {
    return ambientTempC + 38.72 * exp(-0.04661 * distanceCm);
  } else {
    return max(24.5, ambientTempC + 1.0);
  }
}

// --- MOTOR / ANGLE HELPERS ---

void updateSweepFromPlacement() {
  if (strcmp(prototypePlacement, PLACEMENT_BORDER) == 0) {
    SWEEP_ANGLE_DEG = 180;
  } else {
    SWEEP_ANGLE_DEG = 90;
  }

  SWEEP_STEPS = (int)(SWEEP_ANGLE_DEG * stepsPerDegree);
}

float motorPositionToDegrees(int positionSteps) {
  return positionSteps / stepsPerDegree;
}

// --- ULTRASONIC HELPERS ---
long readUltrasonicDuration() {
  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicPin, LOW);

  pinMode(ultrasonicPin, INPUT);
  return pulseIn(ultrasonicPin, HIGH, 30000);
}

float microsecondsToCentimeters(long duration) {
  if (duration == 0) return -1.0;
  return (duration * 0.0343) / 2.0;
}

// Single reading
float readDistanceCm() {
  long duration = readUltrasonicDuration();
  float distance = microsecondsToCentimeters(duration);

  if (distance <= 5.0 || distance >= boardSideCm) {
    return -1.0;
  }

  return distance;
}

// Averaged reading
float readDistanceCmAveraged(int samples) {
  float sum = 0.0;
  int validCount = 0;

  for (int i = 0; i < samples; i++) {
    float d = readDistanceCm();
    if (d > 0) {
      sum += d;
      validCount++;
    }
    delay(5);
  }

  if (validCount == 0) return -1.0;

  return sum / validCount;
}

// --- THERMAL HELPERS ---

float thermalPixels[ROWS * COLS];

void readThermalPixels() {
  sensor.read_pixel_temperature(thermalPixels);
}

float getMaxTemperature() {
  readThermalPixels();

  float maxTemp = thermalPixels[0];

  for (int i = 1; i < ROWS * COLS; i++) {
    if (thermalPixels[i] > maxTemp) {
      maxTemp = thermalPixels[i];
    }
  }

  return maxTemp;
}

// --- DETECTION / REPORTING HELPERS ---

void processDetectionAndReport() {
  float distance = readDistanceCmAveraged(3);
  float angleDeg = motorPositionToDegrees(currentMotorPosition);
  float maxTemp = getMaxTemperature();

  bool detected = false;
  float threshold = 0.0;

  if (distance > 0) {
    threshold = getHotThreshold(distance, boardSideCm, ambientTempC);
    if (maxTemp > threshold) {
      detected = true;
    }
  }

  Serial.print("Dist(cm): ");
  Serial.print(distance);
  Serial.print(" | Angle(deg): ");
  Serial.print(angleDeg);
  Serial.print(" | MaxTemp(C): ");
  Serial.print(maxTemp);
  Serial.print(" | Detected: ");
  Serial.println(detected ? "YES" : "NO");
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(resetButtonPin, INPUT);

  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);

  Wire.begin();

  Motor.begin(I2C_ADDRESS);
  Motor.stop(MOTOR1);
  Motor.stop(MOTOR2);

  sensor.init();

  updateSweepFromPlacement();

  delay(1000);

  if (START_OFFSET_STEPS != 0) {
    int direction = (START_OFFSET_STEPS > 0) ? 1 : -1;
    int stepsToMove = abs(START_OFFSET_STEPS);

    for (int i = 0; i < stepsToMove; i++) {
      Motor.StepperRun(direction, 0, 0);
      delay(10);
    }
    currentMotorPosition = START_OFFSET_STEPS;
  }
}

// ===================== MAIN LOOP =====================
void loop() {
  setjmp(resetContext);

  currentMotorPosition = 0;

  smartDelay(500);

  // ---------- Sweep Forward 0 -> X ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(1, 0, 0);
    currentMotorPosition++;

    processDetectionAndReport();

    if (digitalRead(resetButtonPin) == HIGH) {
      performReset();
    }

    delayMicroseconds(5000);
  }

  smartDelay(2000);

  // ---------- Sweep Backward X -> 0 ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(-1, 0, 0);
    currentMotorPosition--;

    processDetectionAndReport();

    if (digitalRead(resetButtonPin) == HIGH) {
      performReset();
    }

    delayMicroseconds(5000);
  }

  smartDelay(2000);
}
