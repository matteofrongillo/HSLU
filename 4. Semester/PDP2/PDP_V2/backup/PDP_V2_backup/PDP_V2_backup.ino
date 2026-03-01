// ===================== INCLUDES =====================
#include <Grove_I2C_Motor_Driver.h>
#include <Wire.h>
#include <stdint.h>
#include "Seeed_AMG8833_driver.h"

// ===================== CONSTANTS / GLOBALS =====================

// Motor driver
#define I2C_ADDRESS 0x0f

// Ultrasonic sensor pins
const int trigPin = 8;
const int echoPin = 9;

// Thermal camera
AMG8833 sensor;

#define ROWS 8
#define COLS 8

// ===================== HELPER FUNCTIONS =====================

// ---- Function to get HOTTEST pixel in middle columns (4 & 5 using 1-based index) ----
float getHottestMiddleColumn(float *pixels) {
    // start with a very low number so any real temp is higher
    float maxTemp = -1000.0;

    for (int row = 0; row < ROWS; row++) {
        int idx_col4 = 15+row;
        int idx_col5 = 23+row;

        if (pixels[idx_col4] > maxTemp) {
            maxTemp = pixels[idx_col4];
        }
        if (pixels[idx_col5] > maxTemp) {
            maxTemp = pixels[idx_col5];
        }
    }
    
    return maxTemp;   // hottest of those 16 pixels
}

// ---- Ultrasonic functions ----
long readUltrasonicDuration() {
  // Ensure a clean LOW pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);

  // Start the 10 µs trigger pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pulse in microseconds (30 ms timeout ~5 m)
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration;  // returns 0 if it timed out
}

float microsecondsToCentimeters(long duration) {
  // Speed of sound ≈ 0.0343 cm/µs
  // distance = (time * speed) / 2 (there and back)
  return (duration * 0.0343) / 2.0;
}

float AveDistance(long duration){
  // simple 3-sample average using the same raw duration
  float a = microsecondsToCentimeters(duration);
  float b = microsecondsToCentimeters(duration);
  float c = microsecondsToCentimeters(duration);
  return ((a + b + c) / 3.0);
}

// ===================== SETUP =====================
void setup() {
  // Use faster baud so we can stream lots of data
  Serial.begin(115200);

  Wire.begin();
  Motor.begin(I2C_ADDRESS);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Make sure trigger is low initially
  digitalWrite(trigPin, LOW);
  delay(2000);  // give ultrasonic time to settle

  // Init AMG8833 thermal camera
  sensor.init();
}

// ===================== MAIN LOOP =====================
void loop() {

  delay(500);  // small pause before each sweep

  // ---------- Sweep forward 0 -> 360 ----------
  for (int i = 0; i < 256; i++) {
    Motor.StepperRun(1, 0, 0);

    // --- Ultrasonic ---
    long duration = readUltrasonicDuration();
    float distanceCm;
    long angle = (long)map(i, 0, 256, 0, 180);

    if (duration == 0) {
      distanceCm = -1;  // indicate "no reading"
    } else {
      distanceCm = AveDistance(duration);
    }

    // --- Thermal camera HOTTEST middle-column pixel ---
    float pixels_temp[PIXEL_NUM] = {0};
    sensor.read_pixel_temperature(pixels_temp);
    float hottestMid = getHottestMiddleColumn(pixels_temp);

    // --- Output: angle , distance = hottestMid _
    Serial.print(angle);
    Serial.print(",");           // first separator
    Serial.print(distanceCm, 2);
    Serial.print("=");           // second separator
    Serial.print(hottestMid, 2);
    Serial.print("_");           // end-of-record

    delayMicroseconds(5000);
  }

  delay(3000);

  // ---------- Sweep backward 360 -> 0 ----------
  for (int i = 0; i <256; i++) {
    Motor.StepperRun(-1, 0, 0);

    // --- Ultrasonic ---
    long duration = readUltrasonicDuration();
    float distanceCm;
    long angle = (long)map(i, 0, 256, 180, 0);  // your 90° mapping

    if (duration == 0) {
      distanceCm = -1;  // indicate "no reading"
    } else {
      distanceCm = AveDistance(duration);
    }

    // --- Thermal camera HOTTEST middle-column pixel ---
    float pixels_temp[PIXEL_NUM] = {0};
    sensor.read_pixel_temperature(pixels_temp);
    float hottestMid = getHottestMiddleColumn(pixels_temp);

    // --- Output: angle , distance = hottestMid _
    Serial.print(angle);
    Serial.print(",");           
    Serial.print(distanceCm, 2);
    Serial.print("=");
    Serial.print(hottestMid, 2);
    Serial.print("_");

    delayMicroseconds(5000);
  }

  delay(3000);
}
