#include "Seeed_AMG8833_driver.h"
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <setjmp.h>
#include <stdint.h>
#include <math.h>

// ===================== CALIBRATION =====================
const float ambientTempC = 23.5;
const float boardSideLength = 90.0;
const int maxAngle = 90;

// ===================== MOTOR =====================
#define STEPS_PER_REV 2048
#define DEG_PER_STEP (360.0 / STEPS_PER_REV)

float currentAngle = 0;
int direction = 1;

// ===================== GROVE ULTRASONIC =====================
const int ultrasonicPin = 2; // Grove D2 (single SIG pin)

// ===================== AMG8833 =====================
AMG8833 sensor;
float pixels[64];

// ===================== THRESHOLD =====================
float getHotThreshold(float distanceCm, float boardSideLength, float ambientTempC) {
  if (distanceCm <= 0.45 * boardSideLength) {
    return 30.0;
  } else if (distanceCm < 0.9 * boardSideLength) {
    return ambientTempC + 38.72 * exp(-0.04661 * distanceCm);
  } else {
    return max(24.5, ambientTempC + 1.0);
  }
}

// ===================== GROVE ULTRASONIC READ =====================
float readDistanceCM() {
  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);
  delayMicroseconds(2);

  digitalWrite(ultrasonicPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicPin, LOW);

  pinMode(ultrasonicPin, INPUT);
  long duration = pulseIn(ultrasonicPin, HIGH, 30000);

  float distance = duration * 0.034 / 2.0;
  return distance;
}

// ===================== AMG INIT =====================
bool initAMG() {
  for (int i = 0; i < 10; i++) {
    if (sensor.init() == 0) {
      return true;
    }
    delay(200);
  }
  return false;
}

float readMaxTemp() {
  sensor.read_pixel_temperature(pixels);

  float maxTemp = pixels[0];
  for (int i = 1; i < 64; i++) {
    if (pixels[i] > maxTemp) {
      maxTemp = pixels[i];
    }
  }
  return maxTemp;
}

// ===================== STEPPER =====================
const uint8_t stepSequence[4] = {0x1, 0x2, 0x4, 0x8};
int stepIndex = 0;

void stepMotor(int dir) {
  Motor.StepperRun(dir > 0 ? 8 : -8);
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(1000);

  Motor.begin(0x0f);

  if (!initAMG()) {
    Serial.println("AMG8833 init failed!");
    while (1);
  }

  Serial.println("AMG8833 initialized");
}

// ===================== LOOP =====================
void loop() {

  // Move motor
  stepMotor(direction);

  currentAngle += direction * DEG_PER_STEP;

  if (currentAngle >= maxAngle) {
    direction = -1;
    currentAngle = maxAngle;
  } else if (currentAngle <= 0) {
    direction = 1;
    currentAngle = 0;
  }

  // Read sensors
  float distance = readDistanceCM();
  float maxTemp = readMaxTemp();

  bool validDistance = (distance > 5.0 && distance < boardSideLength);
  bool detected = false;

  if (validDistance) {
    float threshold = getHotThreshold(distance, boardSideLength, ambientTempC);
    if (maxTemp > threshold) {
      detected = true;
    }
  }

  // Serial output
  Serial.print("Angle: ");
  Serial.print(currentAngle);
  Serial.print(" deg | Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Max Temp: ");
  Serial.print(maxTemp);
  Serial.print(" C | Status: ");

  if (detected) {
    Serial.println("TARGET DETECTED");
  } else {
    Serial.println("No target");
  }

  delay(50);
}