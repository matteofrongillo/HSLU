// ===================== INCLUDES =====================
#include "Seeed_AMG8833_driver.h"
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <stdint.h>

// ===================== CONSTANTS / GLOBALS =====================

// Motor driver settings
#define I2C_ADDRESS 0x0f

// Temp in Celsius that triggers the pause
#define HOT_THRESHOLD 30.0

// --- MOTOR CONFIGURATION ---
// Adjust these values to match your physical setup
// 256 steps is approx 45 degrees on a standard 28BYJ-48 (depending on gearing)
// If you need exactly 180 degrees, you usually need ~1024 or ~2048 steps.
// For now, I kept your value of 256.

const int START_OFFSET_STEPS = 0; // Set this if you want it to move to a start
                                  // point relative to boot (e.g., -50 or 50)
const int SWEEP_STEPS =
    128; // The range of motion for the loop (128 steps for 90 degrees)
int currentMotorPosition = 0; // Tracks relative position from boot

// Ultrasonic sensor pin (Grove)
const int ultrasonicPin = 5; // D5

// Grove Button for Reset
const int resetButtonPin = A1;
void (*resetFunc)(void) = 0; // Software reset function

// Thermal camera
AMG8833 sensor;

#define ROWS 8
#define COLS 8

// ===================== HELPER FUNCTIONS =====================

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (digitalRead(resetButtonPin) == HIGH) {
      resetFunc();
    }
    delay(1);
  }
}

// ---- Function to get HOTTEST pixel in middle columns (4 & 5 using 1-based
// index) ----
float getHottestMiddleColumn(float *pixels) {
  float maxTemp = -1000.0;

  for (int row = 0; row < ROWS; row++) {
    // The sensor array is 0-63.
    // Row 0: indices 0-7. Middle cols (indices 3 and 4)
    // Adjusting your logic: 15+row and 23+row seem to target specific columns
    // based on your specific camera orientation. I will keep your original
    // indexing logic.
    int idx_col4 = 15 + row;
    int idx_col5 = 23 + row;

    if (pixels[idx_col4] > maxTemp)
      maxTemp = pixels[idx_col4];
    if (pixels[idx_col5] > maxTemp)
      maxTemp = pixels[idx_col5];
  }
  return maxTemp;
}

// ---- Ultrasonic functions ----
long readUltrasonicDuration() {
  pinMode(ultrasonicPin, OUTPUT);
  digitalWrite(ultrasonicPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicPin, LOW);

  pinMode(ultrasonicPin, INPUT);
  return pulseIn(ultrasonicPin, HIGH, 30000); // 30ms timeout
}

float microsecondsToCentimeters(long duration) {
  if (duration == 0)
    return -1.0; // Return -1 for timeout
  return (duration * 0.0343) / 2.0;
}

// Fixed Logic: Takes 3 distinct readings to average noise
float getAverageDistance() {
  float total = 0;
  int validReadings = 0;

  for (int k = 0; k < 3; k++) {
    long d = readUltrasonicDuration();
    float cm = microsecondsToCentimeters(d);

    // Only average valid readings
    if (cm > 0) {
      total += cm;
      validReadings++;
    }
    delay(5); // Small delay between pings
  }

  if (validReadings == 0)
    return -1.0;
  return total / validReadings;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(resetButtonPin, INPUT);

  Wire.begin();
  Motor.begin(I2C_ADDRESS);

  // Init AMG8833 thermal camera
  sensor.init();

  delay(1000);

  // --- GO TO STARTING POSITION ---
  // If you want the motor to start at a specific offset from where it was
  // turned on:
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
  smartDelay(500);

  // ---------- Sweep Forward 0 -> 180 ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(1, 0, 0);
    currentMotorPosition++;

    long angle = map(i, 0, SWEEP_STEPS, 0, 90);

    float distanceCm = getAverageDistance();

    float pixels_temp[PIXEL_NUM] = {0};
    sensor.read_pixel_temperature(pixels_temp);
    float hottestMid = getHottestMiddleColumn(pixels_temp);

    // --- NEW LOGIC: PAUSE IF HOT ---
    if (hottestMid > HOT_THRESHOLD) {
      smartDelay(150); // Sleep for 150ms to "stare" at the object
    }

    // --- RESET CHECK ---
    if (digitalRead(resetButtonPin) == HIGH) {
      resetFunc();
    }

    // --- CLEAN OUTPUT ---
    Serial.print("Pos:");
    Serial.print(currentMotorPosition);
    Serial.print(",Ang:");
    Serial.print(angle);
    Serial.print(",");
    Serial.print(distanceCm, 2);
    Serial.print("=");
    Serial.print(hottestMid, 2);
    Serial.print("_");

    delayMicroseconds(5000);
  }

  smartDelay(2000);

  // ---------- Sweep Backward 180 -> 0 ----------
  for (int i = 0; i < SWEEP_STEPS; i++) {
    Motor.StepperRun(-1, 0, 0);
    currentMotorPosition--;

    long angle = map(i, 0, SWEEP_STEPS, 90, 0);

    float distanceCm = getAverageDistance();

    float pixels_temp[PIXEL_NUM] = {0};
    sensor.read_pixel_temperature(pixels_temp);
    float hottestMid = getHottestMiddleColumn(pixels_temp);

    // --- NEW LOGIC: PAUSE IF HOT ---
    if (hottestMid > HOT_THRESHOLD) {
      smartDelay(150);
    }

    // --- RESET CHECK ---
    if (digitalRead(resetButtonPin) == HIGH) {
      resetFunc();
    }

    // --- CLEAN OUTPUT ---
    Serial.print("Pos:");
    Serial.print(currentMotorPosition);
    Serial.print(",Ang:");
    Serial.print(angle);
    Serial.print(",");
    Serial.print(distanceCm, 2);
    Serial.print("=");
    Serial.print(hottestMid, 2);
    Serial.print("_");

    delayMicroseconds(5000);
  }
  smartDelay(2000);
}