#include <Ultrasonic.h>
#include <Grove_LED_Bar.h>
#include <Servo.h>
#include <Wire.h>
#include <Grove_I2C_Motor_Driver.h>

#define I2C_ADDRESS 0x0f

#define ULTRASONIC_PIN  7
#define RED_LED         3
#define GREEN_LED       2
#define SERVO_PIN       5

// ── Timing constants (ms) ─────────────────────────────────────────────────────
#define DEBOUNCE_DELAY      10   // how long object must be absent before closing starts
#define LED_BAR_STEP_DELAY  150  // delay between each LED bar step when unloading (10 steps = 1500ms total)
#define SERVO_OPEN_DELAY    1000 // time given to servo to physically reach 180°
#define SERVO_CLOSE_DELAY   1000 // time given to servo to physically return to 0°
#define BLINK_ON_TIME       200  // red LED on duration during warning blink (ms)
#define BLINK_OFF_TIME      200  // red LED off duration during warning blink (ms)
#define BLINK_COUNT         3    // number of warning blinks before servo activates
// ─────────────────────────────────────────────────────────────────────────────

Ultrasonic ultrasonic(ULTRASONIC_PIN);
Grove_LED_Bar bar(9, 8, 0);
Servo myServo;

bool bridgeOpen = false;
bool noDetectStarted = false;
unsigned long noDetectTime = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  bar.begin();
  bar.setLevel(0);
  Motor.begin(I2C_ADDRESS);
}

void loop() {
  long dist = ultrasonic.MeasureInCentimeters();

  if (!bridgeOpen) {
    if (dist > 0 && dist <= 10) {

      // 1. Green OFF immediately
      digitalWrite(GREEN_LED, LOW);

      // 2. Red LED blinks 3x as a warning
      for (int i = 0; i < BLINK_COUNT; i++) {
        digitalWrite(RED_LED, HIGH);
        delay(BLINK_ON_TIME);
        digitalWrite(RED_LED, LOW);
        delay(BLINK_OFF_TIME);
      }

      // 3. Red LED stays solid
      digitalWrite(RED_LED, HIGH);

      // 4. LED bar fills with a shadowing effect across the 4 Green, 3 Yellow, 3 Red LEDs
      for (int i = 1; i <= 10; i++) {
        // Creates a smooth brightness gradient from LED 1 (1.0) to LED 10 (0.1)
        float brightness = 1.0 - ((i - 1) * 0.1); 
        bar.setLed(i, brightness);
      }

      // 5. Servo opens to 180°
      myServo.write(180);
      delay(SERVO_OPEN_DELAY);

      // 6. Stepper motor spins opposite direction, waits, and returns
      // Formula: steps = (degrees / 360) * 512 
      int targetDegrees = 360 * 10; // 360 degrees * 10 times
      int totalSteps = (targetDegrees / 360) * 512;
      
      // Spin opposite direction (negative steps)
      int stepsRemaining = totalSteps;
      while (stepsRemaining > 0) {
        int stepsThisRun = min(stepsRemaining, 1024);
        Motor.StepperRun(-stepsThisRun);
        stepsRemaining -= stepsThisRun;
      }

      // Wait 2 seconds
      delay(2000);

      // Rotate back to initial position (positive steps)
      stepsRemaining = totalSteps;
      while (stepsRemaining > 0) {
        int stepsThisRun = min(stepsRemaining, 1024);
        Motor.StepperRun(stepsThisRun);
        stepsRemaining -= stepsThisRun;
      }

      bridgeOpen = true;
      noDetectStarted = false;
    }
  }

  else {
    if (dist == 0 || dist > 10) {
      if (!noDetectStarted) {
        noDetectStarted = true;
        noDetectTime = millis();
      }

      if (millis() - noDetectTime >= DEBOUNCE_DELAY) {

        // 1. LED bar unloads 10 → 0 while maintaining shadow gradient and a fading trail
        for (int level = 10; level >= -2; level--) {
          for (int i = 1; i <= 10; i++) {
            if (i <= level) {
              // Maintain original shadow gradient
              bar.setLed(i, 1.0 - ((i - 1) * 0.1));
            } else {
              // Fading shadow trail behind the unloading level
              int trail = i - level;
              if (trail == 1) bar.setLed(i, 0.05); // faint glow
              else if (trail == 2) bar.setLed(i, 0.01); // ultra faint glow
              else bar.setLed(i, 0.0); // completely off
            }
          }
          delay(LED_BAR_STEP_DELAY);
        }

        // 2. Servo closes to 0°
        myServo.write(0);
        delay(SERVO_CLOSE_DELAY);

        // 3. Red OFF + Green ON
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);

        bridgeOpen = false;
        noDetectStarted = false;
      }
    } else {
      noDetectStarted = false;
    }
  }
}