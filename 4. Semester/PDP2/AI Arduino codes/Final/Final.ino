// ===================== INCLUDES =====================
#include "Seeed_AMG8833_driver.h"
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <stdint.h>
#include <math.h>

// ===================== CONSTANTS / GLOBALS =====================

// Motor driver settings
#define I2C_ADDRESS 0x0f

// --- CALIBRATION VALUES ---
// Replace these only after validation
#define AMBIENT_TEMP_C 23.50
#define BOARD_SIDE_CM 100.00
#define MOTOR_ROTATION_ANGLE_DEG 90.0

float ambientTempC = AMBIENT_TEMP_C;
float boardSideCm = BOARD_SIDE_CM;
float motorRotationAngleDeg = MOTOR_ROTATION_ANGLE_DEG;

// --- MOTOR CONFIGURATION ---
// Rotation range will be defined by the user later
const int START_OFFSET_STEPS = 0;

// Approximate conversion between steps and angle
// Update this once the rotation range is defined
float stepsPerDegree = 128.0 / 90.0;
int SWEEP_STEPS = (int)(motorRotationAngleDeg * stepsPerDegree);

int currentMotorPosition = 0;

// Ultrasonic sensor pin (Grove)
const int ultrasonicPin = 2;

// Thermal camera
AMG8833 sensor;

#define ROWS 8
#define COLS 8
#define TOTAL_PIXELS 64

// ===================== HELPER FUNCTIONS =====================

// --- REQUIRED THRESHOLD FUNCTION ---
float getHotThreshold(float distanceCm, float boardSideLength, float ambientTempC) {
  if (distanceCm <= 0.372 * boardSideLength) {
    return 30.0;
  } else if (distanceCm < 0.743 * boardSideLength) {
    return ambientTempC + 42.33 * exp(-0.05038 * distanceCm);
  } else {
    return max(24.5, ambientTempC + 1.0);
  }
}

// --- MOTOR / ANGLE HELPERS ---
int angleToSteps(float angleDeg) {
  return (int)(angleDeg * stepsPerDegree);
}

float stepsToAngle(int motorPositionSteps) {
  return motorPositionSteps / stepsPerDegree;
}

float getCurrentMotorAngleDeg() {
  return stepsToAngle(currentMotorPosition);
}

// TODO:
// - keep 0° -> X° -> 0° sweep behavior

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

float readDistanceCm() {
  long duration = readUltrasonicDuration();
  return microsecondsToCentimeters(duration);
}

float readAverageDistanceCm(int samples) {
  float sum = 0.0;
  int validSamples = 0;

  for (int i = 0; i < samples; i++) {
    float distanceCm = readDistanceCm();

    if (distanceCm > 0.0) {
      sum += distanceCm;
      validSamples++;
    }

    delay(10);
  }

  if (validSamples == 0) return -1.0;
  return sum / validSamples;
}

bool isValidDistanceCm(float distanceCm) {
  return (distanceCm > 5.0 && distanceCm < boardSideCm);
}

// TODO:
// - apply valid-range rule 5 < x < d in detection logic

// --- THERMAL HELPERS ---
void readThermalPixels(float *pixelsTemp) {
  sensor.read_pixel_temperature(pixelsTemp);
}

float readMaxTemperatureC() {
  float pixelsTemp[TOTAL_PIXELS];

  readThermalPixels(pixelsTemp);

  float maxTemp = pixelsTemp[0];

  for (int i = 1; i < TOTAL_PIXELS; i++) {
    if (pixelsTemp[i] > maxTemp) {
      maxTemp = pixelsTemp[i];
    }
  }

  return maxTemp;
}

// --- DETECTION / REPORTING HELPERS ---
bool isTargetDetected(float distanceCm, float maxTempC) {
  if (!isValidDistanceCm(distanceCm)) {
    return false;
  }

  float hotThreshold = getHotThreshold(distanceCm, boardSideCm, ambientTempC);
  return (maxTempC > hotThreshold);
}

void printDetectionStatus(float distanceCm, float motorAngleDeg, float maxTempC) {
  bool detected = isTargetDetected(distanceCm, maxTempC);

  Serial.print("Distance (cm): ");
  if (isValidDistanceCm(distanceCm)) {
    Serial.print(distanceCm, 2);
  } else {
    Serial.print("invalid");
  }

  Serial.print(" | Angle (deg): ");
  Serial.print(motorAngleDeg, 2);

  Serial.print(" | Max Temp (C): ");
  Serial.print(maxTempC, 2);

  Serial.print(" | Detection: ");
  if (detected) {
    Serial.println("YES");
  } else {
    Serial.println("NO");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);

  Wire.begin();

  Motor.begin(I2C_ADDRESS);
  Motor.stop(MOTOR1);
  Motor.stop(MOTOR2);

  sensor.init();

  SWEEP_STEPS = angleToSteps(motorRotationAngleDeg);
  if (SWEEP_STEPS < 0) {
    SWEEP_STEPS = -SWEEP_STEPS;
  }

  currentMotorPosition = 0;

  delay(1000);

  // Optional start offset
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
  currentMotorPosition = 0;

  delay(500);

  // ---------- Sweep Forward 0 -> X ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(1, 0, 0);
    currentMotorPosition++;

    float currentAngleDeg = getCurrentMotorAngleDeg();
    float distanceCm = readAverageDistanceCm(3);
    float maxTempC = readMaxTemperatureC();

    printDetectionStatus(distanceCm, currentAngleDeg, maxTempC);

    delayMicroseconds(5000);
  }

  delay(2000);

  // ---------- Sweep Backward X -> 0 ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(-1, 0, 0);
    currentMotorPosition--;

    float currentAngleDeg = getCurrentMotorAngleDeg();
    float distanceCm = readAverageDistanceCm(3);
    float maxTempC = readMaxTemperatureC();

    printDetectionStatus(distanceCm, currentAngleDeg, maxTempC);

    delayMicroseconds(5000);
  }

  delay(2000);
}