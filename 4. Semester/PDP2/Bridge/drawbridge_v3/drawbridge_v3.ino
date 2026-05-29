#include <Grove_I2C_Motor_Driver.h>
#include <Servo.h>
#include <Ultrasonic.h>
#include <Wire.h>

#define I2C_ADDRESS 0x0f

#define ULTRASONIC_PIN  7
#define RED_LED         2
#define YELLOW_LED      3
#define GREEN_LED       4
#define SERVO_PIN       5   // Gate 1
#define SERVO_PIN_2     6   // Gate 2 (new)
#define RED_LED_2       8   // Traffic Light 2
#define YELLOW_LED_2    9   // Traffic Light 2
#define GREEN_LED_2     10  // Traffic Light 2
#define RED_LED_3       11  // Traffic Light 3
#define YELLOW_LED_3    12  // Traffic Light 3
#define GREEN_LED_3     13  // Traffic Light 3

// ── Timing constants (ms) ─────────────────────────────────────────────────────
#define DEBOUNCE_DELAY      10000
#define SERVO_OPEN_DELAY      200
#define SERVO_CLOSE_DELAY     200
#define BLINK_ON_TIME         200
#define BLINK_OFF_TIME        200
#define BLINK_COUNT             3
#define YELLOW_DELAY         2000
#define STEPS_PER_REV         512
// ─────────────────────────────────────────────────────────────────────────────

Ultrasonic ultrasonic(ULTRASONIC_PIN);
Servo myServo;
Servo myServo2;   // Gate 2 (new)

bool bridgeOpen       = false;
bool noDetectStarted  = false;
unsigned long noDetectTime = 0;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(YELLOW_LED_2, OUTPUT);
  pinMode(GREEN_LED_2, OUTPUT);
  pinMode(RED_LED_3, OUTPUT);
  pinMode(YELLOW_LED_3, OUTPUT);
  pinMode(GREEN_LED_3, OUTPUT);
  
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED_2, LOW);
  digitalWrite(YELLOW_LED_2, LOW);
  digitalWrite(GREEN_LED_2, HIGH);
  digitalWrite(RED_LED_3, HIGH);
  digitalWrite(YELLOW_LED_3, LOW);
  digitalWrite(GREEN_LED_3, LOW);

  myServo.attach(SERVO_PIN);
  myServo.write(180);       // Gate 1 closed at startup

  myServo2.attach(SERVO_PIN_2);
  myServo2.write(180);      // Gate 2 closed at startup

  Motor.begin(I2C_ADDRESS);
}

void loop() {
  long dist = ultrasonic.MeasureInCentimeters();

  if (!bridgeOpen) {
    if (dist > 0 && dist <= 10) {

      // 1. Green OFF, TL3 Red OFF
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(GREEN_LED_2, LOW);
      digitalWrite(RED_LED_3, LOW);

      // 2. Red blinks 3 times, TL3 Green blinks
      for (int i = 0; i < BLINK_COUNT; i++) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(RED_LED_2, HIGH);
        digitalWrite(GREEN_LED_3, HIGH);
        delay(BLINK_ON_TIME);
        digitalWrite(RED_LED, LOW);
        digitalWrite(RED_LED_2, LOW);
        digitalWrite(GREEN_LED_3, LOW);
        delay(BLINK_OFF_TIME);
      }

      // 3. Servo motor rotates
      myServo.write(90);
      myServo2.write(90);
      delay(SERVO_OPEN_DELAY);

      // 4. LED Always red, TL3 Green
      digitalWrite(RED_LED, HIGH);
      digitalWrite(RED_LED_2, HIGH);
      digitalWrite(GREEN_LED_3, HIGH);

      // 5. Stepper spins 90° clockwise
      int totalSteps = (90 * (long)STEPS_PER_REV) / 360;
      Motor.StepperRun(totalSteps);

      bridgeOpen      = true;
      noDetectStarted = false;
    }
  }

  else {
    if (dist == 0 || dist > 10) {
      if (!noDetectStarted) {
        noDetectStarted = true;
        noDetectTime    = millis();
      }

      if (millis() - noDetectTime >= DEBOUNCE_DELAY) {

        // 1. Stepper spins back 90° counter-clockwise
        int totalSteps = (90 * (long)STEPS_PER_REV) / 360;
        Motor.StepperRun(-totalSteps);

        // 2. LED Red + LED yellow for few seconds, TL3 Yellow
        digitalWrite(RED_LED, HIGH);
        digitalWrite(RED_LED_2, HIGH);
        digitalWrite(YELLOW_LED, HIGH);
        digitalWrite(YELLOW_LED_2, HIGH);
        digitalWrite(GREEN_LED_3, LOW);
        digitalWrite(YELLOW_LED_3, HIGH);
        delay(YELLOW_DELAY);

        // 3. Servo motors turn back
        myServo.write(180);
        myServo2.write(180);
        delay(SERVO_CLOSE_DELAY);

        // 4. LED Green light (and red/yellow off), TL3 Red
        digitalWrite(RED_LED, LOW);
        digitalWrite(RED_LED_2, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(YELLOW_LED_2, LOW);
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(GREEN_LED_2, HIGH);
        digitalWrite(YELLOW_LED_3, LOW);
        digitalWrite(RED_LED_3, HIGH);

        bridgeOpen      = false;
        noDetectStarted = false;
      }
    } else {
      noDetectStarted = false;
    }
  }
}
