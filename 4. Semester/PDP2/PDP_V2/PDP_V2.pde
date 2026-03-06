import processing.serial.*;
import java.awt.event.KeyEvent;
import java.io.IOException;

Serial myPort;

String angle   = "";
String distance = "";
String tempStr = "";
String data    = "";

int   iAngle, iDistance;
float fTemp;

int index1 = 0;
int index2 = 0;

// ---- SCALE SETTINGS ----
float maxRangeMeters = 1.0;                    // radius in meters
float maxRangeCm     = maxRangeMeters * 100.0; // 100 cm

void setup() {
  fullScreen();
  smooth();

  // FIX 1: MATCH BAUD RATE TO ARDUINO (115200)
  // Ensure the port name "/dev/ttyACM0" is correct for your system
  try {
    myPort = new Serial(this, "/dev/ttyACM0", 9600);
    myPort.bufferUntil('_');
  } catch (Exception e) {
    println("Error opening serial port: " + e.getMessage());
  }
}

void draw() {
  // motion-blur background
  noStroke();
  fill(0, 15);
  rect(0, 0, width, height);

  drawRadar();
  drawLine();
  drawObject();   // temperature-colored dot
  drawText();
}

// ---------------------------------------------------------
// SERIAL
// ---------------------------------------------------------
void serialEvent(Serial p) {
  // 1. Read the data until the terminator '_'
  data = p.readStringUntil('_');

  if (data != null) {
    try {
      // 2. Clean the string (remove whitespace and the trailing '_')
      data = trim(data);
      if (data.endsWith("_")) {
        data = data.substring(0, data.length() - 1);
      }

      // Format received: "Pos:179,Ang:125,37.60=21.75"
      // Strategy: Find indices of "Ang:", the comma after it, and the equals sign.

      int idxAngLabel = data.indexOf("Ang:");
      int idxEq       = data.indexOf("=");

      // proceed only if we found the key markers
      if (idxAngLabel != -1 && idxEq != -1) {

        // Find the comma specifically AFTER the "Ang:" label
        // This comma separates the Angle from the Distance
        int idxComma = data.indexOf(",", idxAngLabel);

        if (idxComma != -1) {
          // Extract substrings based on these positions
          // "Ang:" is 4 characters long, so we start reading 4 chars after the label starts
          String sAngle = data.substring(idxAngLabel + 4, idxComma);
          String sDist  = data.substring(idxComma + 1, idxEq);
          String sTemp  = data.substring(idxEq + 1);

          // Convert to numbers
          iAngle = int(trim(sAngle));

          // Distance comes in as "37.60" (float string).
          // We parse to float first, then cast to int to match your 'iDistance' variable type.
          iDistance = int(float(trim(sDist)));

          fTemp = float(trim(sTemp));
        }
      }
    } catch (Exception e) {
      // If a partial or corrupted line comes in, print it to console but don't crash
      println("Parse Error on data: [" + data + "]");
      e.printStackTrace();
    }
  }
}

// ---------------------------------------------------------
// DRAW RADAR
// ---------------------------------------------------------
void drawRadar() {
  pushMatrix();
  translate(width/2, height/2);

  float radarDiameter = min(width, height) * 0.8;
  float radarRadius   = radarDiameter / 2.0;

  noFill();
  stroke(98, 245, 31);
  strokeWeight(2);

  int rings = 4; // 0.25m, 0.5m, 0.75m, 1.0m

  for (int i = 1; i <= rings; i++) {
    float frac  = i / (float)rings;
    float ringR = frac * radarRadius;

    // ring outline
    arc(0, 0, ringR * 2, ringR * 2, 0, TWO_PI);

    // distance label (meters)
    float distMeters = frac * maxRangeMeters;
    fill(98, 245, 31);
    textAlign(CENTER, BOTTOM);
    textSize(20);
    text(nf(distMeters, 1, 2) + " m", 0, -ringR - 5);

    noFill();
    stroke(98, 245, 31);
  }

  // angle lines every 30°
  for (int a = 0; a < 360; a += 30) {
    float x = radarRadius * cos(radians(a));
    float y = radarRadius * sin(radians(a));
    line(0, 0, x, y);
  }

  popMatrix();
}

// ---------------------------------------------------------
// SWEEP LINE
// ---------------------------------------------------------
void drawLine() {
  pushMatrix();
  translate(width/2, height/2);

  stroke(30, 250, 60);
  strokeWeight(4);

  float radarDiameter = min(width, height) * 0.8;
  float radarRadius   = radarDiameter / 2.0;

  line(0, 0,
       radarRadius * cos(radians(iAngle)),
       radarRadius * sin(radians(iAngle)));

  popMatrix();
}

// ---------------------------------------------------------
// TEMP → COLOR
// Blue →  Red
// ---------------------------------------------------------
color tempToColor(float temp, float tCold, float tHot) {
  float t = constrain((temp - tCold) / (tHot - tCold), 0, 1);

  if (temp  < 30) {
    // blue → cyan
    float k = t / 0.5;
    return color(0, 0 , 255);
  } else {
    // orange → red
    float k = (t - 0.80) / 0.20;
    return color(255, 0, 0);
  }
}

// ---------------------------------------------------------
// OBJECT DOT (TEMP-COLORED)
// ---------------------------------------------------------
void drawObject() {
  pushMatrix();
  translate(width/2, height/2);

  float radarDiameter = min(width, height) * 0.8;
  float radarRadius   = radarDiameter / 2.0;

  float pxDist = map(iDistance, 0, maxRangeCm, 0, radarRadius);

  // only draw inside range
  if (iDistance <= maxRangeCm) {

    // temperature range for visualization (tune these!)
    float coldTemp = 15;  // <= 15°C → blue
    float hotTemp  = 40;  // >= 40°C → red

    color c = tempToColor(fTemp, coldTemp, hotTemp);

    stroke(c);
    strokeWeight(20);

    float x = pxDist * cos(radians(iAngle));
    float y = pxDist * sin(radians(iAngle));
    point(x, y);
  }

  popMatrix();
}

// ---------------------------------------------------------
// TEXT HUD
// ---------------------------------------------------------
void drawText() {

  // ---- Draw only a small corner box ----
  int boxW = 450;   // width of info box
  int boxH = 160;   // height of info box

  noStroke();
  fill(0, 180);      // semi-transparent black
  rect(20, height - boxH - 20, boxW, boxH, 20);  // rounded rectangle



  // ---- Draw text on top ----
  fill(98, 245, 31);
  textSize(28);
  textAlign(LEFT, CENTER);

  // float distMeters = iDistance / 100.0;
  float distMeters = iDistance;

  String distLabel;
  if (iDistance > maxRangeCm || iDistance < 0) {
    distLabel = "Out of Range";
  } else {
    distLabel = nf(distMeters, 1, 2) + " cm";
  }

int x = 40;
  int baseY = height - boxH;

  text("Angle: " + iAngle + "°",    x, baseY - 10);
  text("Distance: " + distLabel,   x, baseY + 40);
  text("Temp: " + nf(fTemp, 1, 2) + " °C", x, baseY + 90);

  float triggerTemp = 30.0; // Change this value to your "hot" threshold

  if (fTemp > triggerTemp) {
    fill(255, 0, 0);          // Red Color
    textSize(80);             // Bigger text
    textAlign(RIGHT, BOTTOM); // Aligns text to the bottom-right corner
    
    text("Penguin Detected!", width - 40, height - 80); 
  }
}
