
// ------------------------------------------------------------
// Processing sketch for visualising Arduino "radar" data
// - Reads angle + distance from serial (format: "angle,distance.")
// - Draws a 2D radar with sweeping line and object dots
// - Scale: 2 m diameter (= 1 m radius)
// ------------------------------------------------------------
import processing.serial.*;
import java.awt.event.KeyEvent;
import java.io.IOException;
Serial myPort;
// raw serial strings
String angle = "";
String distance = "";
String data = "";
String noObject; // (declared but currently unused)
// parsed numeric values
int iAngle, iDistance;
// index of comma separator in the incoming string
int index1 = 0;
// ---- SCALE SETTINGS ----
// We want a 2 m diameter → 1 m radius.
float maxRangeMeters = 1.0; // radius in meters
float maxRangeCm = maxRangeMeters * 100.0; // radius in
centimeters
void setup() {
    // Window size – adjust to your screen
    size(1920, 1080);
    smooth();
    // Open serial port from Arduino
    myPort = new Serial(this, "COM3", 9600);
    // Read bytes until '.' (end-of-packet marker from Arduino)
    myPort.bufferUntil('.');
}
void draw() {
    // MOTION BLUR BACKGROUND:
    // draw a translucent black rectangle each frame so old frames
    fade
    noStroke();
    fill(0, 15); // lower alpha = longer trails
    rect(0, 0, width, height); // do NOT use background(0);
    // Draw the static and dynamic parts of the radar
    drawRadar(); // circles + grid lines + range labels
    drawLine(); // sweeping line showing current angle
    drawObject(); // point for current distance reading
    drawText(); // info text at bottom (angle, distance,
    scale)
}
// -------------------------------------------------------------------
// Called automatically whenever a full serial message "...."
arrives.
// Format expected: "ANGLE,DISTANCE."
// e.g. "90,37."
// -------------------------------------------------------------------
void serialEvent(Serial myPort) {
    // Read up to the '.' terminator
    data = myPort.readStringUntil('.');
    // Ignore empty reads
    if (data == null) return;
    // Remove the final '.' character
    data = data.substring(0, data.length() - 1);
    // Find comma position
    index1 = data.indexOf(",");
    // Split into angle and distance substrings
    angle = data.substring(0, index1);
    distance = data.substring(index1 + 1);
    // Convert strings to integers
    iAngle = int(angle);
    iDistance = int(distance);
}
// -------------------------------------------------------------------
// Draws the radar grid:
// - Concentric circles (range rings)
// - Range labels in meters
// - Radial lines every 30°
// -------------------------------------------------------------------
void drawRadar() {
    pushMatrix();
    // Move origin to center of screen
    translate(width / 2, height / 2);
    // Radar size on screen (80% of smaller screen dimension)
    float radarDiameter = min(width, height) * 0.8;
    float radarRadius = radarDiameter / 2.0;
    // STROKE ONLY, NO FILL (for rings/lines)
    noFill();
    stroke(98, 245, 31);
    strokeWeight(2);
    int rings = 4; // 4 circles: 0.25m, 0.5m, 0.75m, 1.0m
    // Full 360° radar rings + labels
    for (int i = 1; i <= rings; i++) {
        float frac = i / (float)rings; // 0.25, 0.5, 0.75, 1.0
        float ringR = frac * radarRadius; // radius in pixels
        // Draw ring outline
        arc(0, 0, ringR * 2, ringR * 2, 0, TWO_PI);
        // Label each ring (in meters) on the top (90°)
        float distMeters = frac * maxRangeMeters;
        // Switch to fill for text
        fill(98, 245, 31);
        textAlign(CENTER, BOTTOM);
        textSize(20);
        text(nf(distMeters, 1, 2) + " m", 0, -ringR - 5);
        // Back to "noFill" for shapes
        noFill();
        stroke(98, 245, 31);
    }
    // 12 angle lines (every 30°)
    for (int a = 0; a < 360; a += 30) {
        float x = radarRadius * cos(radians(a));
        float y = radarRadius * sin(radians(a));
        line(0, 0, x, y);
    }
    popMatrix();
}
// -------------------------------------------------------------------
// Draws the sweeping radar line at current angle (iAngle).
// Length is full radar radius.
// -------------------------------------------------------------------
void drawLine() {
    pushMatrix();
    translate(width / 2, height / 2);
    stroke(30, 250, 60);
    strokeWeight(4);
    float radarDiameter = min(width, height) * 0.8;
    float radarRadius = radarDiameter / 2.0;
    // Line from center to edge at angle iAngle
    line(0,
         0,
         radarRadius * cos(radians(iAngle)),
         radarRadius * sin(radians(iAngle)));
    popMatrix();
}
// -------------------------------------------------------------------
// Draws the detected object as a point at the measured distance
// along the current angle, but only if within maxRangeCm (1 m).
// -------------------------------------------------------------------
void drawObject() {
    pushMatrix();
    translate(width / 2, height / 2);
    stroke(255, 10, 10);
    strokeWeight(7);
    float radarDiameter = min(width, height) * 0.8;
    float radarRadius = radarDiameter / 2.0;
    // Map sensor distance (cm) -> pixels, up to maxRangeCm (1 m)
    float pxDist = map(iDistance, 0, maxRangeCm, 0, radarRadius);
    // Only draw if within 1 m radius (2 m diameter)
    if (iDistance <= maxRangeCm) {
        float x = pxDist * cos(radians(iAngle));
        float y = pxDist * sin(radians(iAngle));
        // Drawing a point repeatedly is what creates the trail with
        motion blur
        point(x, y);
    }
    popMatrix();
}
// -------------------------------------------------------------------
// Draws HUD text at the bottom of the screen:
// - Current angle
// - Current distance in meters or "Out of Range"
// - Scale info
// -------------------------------------------------------------------
void drawText() {
    // Opaque bar at bottom so text doesn’t smear with blur
    noStroke();
    fill(0);
    rect(0, height - 120, width, 120);
    fill(98, 245, 31);
    textSize(28);
    textAlign(LEFT, CENTER);
    // Convert distance to meters for display
    float distMeters = iDistance / 100.0;
    String distLabel;
    // Decide what to show based on range
    if (iDistance > maxRangeCm) {
        distLabel = "Out of Range (> " + maxRangeMeters + " m)";
    } else {
        distLabel = nf(distMeters, 1, 2) + " m";
    }
    text("Angle: " + iAngle + "°", 20, height - 80);
    text("Distance: " + distLabel, 20, height - 40);
    // Scale info on right side
    textAlign(RIGHT, CENTER);
    text("RADAR SCALE: Diameter = 2.00 m", width - 40, height - 40);
}

// -----------------------------------------------------------------
Arduino code using Arduino IDE
// -----------------------------------------------------------------

// ------------------------------------------------------------
// Arduino sketch for ultrasonic "radar"
// - Sweeps a servo from 0° → 360° → 0°
// - At each step, measures distance with HC-SR04-style sensor
// - Sends "angle,distance." over Serial to Processing
// ------------------------------------------------------------
#include <Servo.h>
// Ultrasonic sensor pins
#define trigPin 8
#define echoPin 9
long duration; // time for echo to return (microseconds)
int distance; // measured distance (cm)
Servo myservo;
// ------------------------------------------------------------
// Trigger ultrasonic sensor and calculate distance in cm
// ------------------------------------------------------------
int calculateDistance() {
    // Ensure trigger is LOW for a short time
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Send 10 μs pulse to trigger pin
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Measure the length of the echo pulse
    duration = pulseIn(echoPin, HIGH);
    // Convert time to distance:
    // speed of sound ~ 0.034 cm/μs
    // divide by 2 (there and back)
    distance = duration * 0.034 / 2;
    return distance;
}
void setup() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    myservo.attach(11); // Servo signal pin
    Serial.begin(9600); // Must match Processing baud rate
}
void loop() {
    int i;
    // Sweep from 0° up to 360° (inclusive) in 1.8° steps
    for (i = 0; i <= 360; i += 1.8) {
        myservo.write(i); // Move servo to angle i
        delay(150); // Wait for servo to reach position
        calculateDistance();
        // Send "angle,distance." packet to Processing
        Serial.print(i);
        Serial.print(",");
        Serial.print(distance);
        Serial.print(".");
    }
    // Sweep back from 360° down to 0° in 1.8° steps
    for (i = 360; i >= 0; i -= 1.8) {
        myservo.write(i);
        delay(150);
        calculateDistance();
        Serial.print(i);
        Serial.print(",");
        Serial.print(distance);
        Serial.print(".");
    }
    // loop() repeats forever, so the sweep continues continuously
}
// ----------------------------------------------------------------
