#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUZZER_PIN 25

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

// Thresholds based on your real test data
const float DIP_THRESHOLD = 0.6;       // below this = possible free-fall
const float IMPACT_THRESHOLD = 1.5;    // above this = possible impact
const float STILL_LOW = 0.85;
const float STILL_HIGH = 1.15;
const unsigned long IMPACT_WINDOW_MS = 1000;   // must see impact within 1s of dip
const unsigned long STILLNESS_DURATION_MS = 2000; // must stay still for 2s after impact

enum State { NORMAL, DIP_DETECTED, IMPACT_DETECTED, FALL_CONFIRMED };
State currentState = NORMAL;

unsigned long dipTime = 0;
unsigned long impactTime = 0;
unsigned long stillnessStart = 0;

void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (1) delay(10);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Fall Detection Ready");
  display.display();
  delay(1000);

  Serial.println("Fall Detection Started!");
}

void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Tmp = Wire.read() << 8 | Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  float gx = AcX / 16384.0;
  float gy = AcY / 16384.0;
  float gz = AcZ / 16384.0;
  float magnitude = sqrt(gx * gx + gy * gy + gz * gz);

  unsigned long now = millis();

  // State machine
  switch (currentState) {
    case NORMAL:
      if (magnitude < DIP_THRESHOLD) {
        currentState = DIP_DETECTED;
        dipTime = now;
        Serial.println(">> Dip detected");
      }
      break;

    case DIP_DETECTED:
      if (magnitude > IMPACT_THRESHOLD && (now - dipTime) < IMPACT_WINDOW_MS) {
        currentState = IMPACT_DETECTED;
        impactTime = now;
        stillnessStart = 0;
        Serial.println(">> Impact detected, checking stillness...");
      } else if (now - dipTime > IMPACT_WINDOW_MS) {
        currentState = NORMAL; // dip wasn't followed by impact in time, reset
      }
      break;

    case IMPACT_DETECTED:
      if (magnitude >= STILL_LOW && magnitude <= STILL_HIGH) {
        if (stillnessStart == 0) stillnessStart = now;
        if (now - stillnessStart >= STILLNESS_DURATION_MS) {
          currentState = FALL_CONFIRMED;
          Serial.println(">> FALL CONFIRMED");
        }
      } else {
        stillnessStart = 0; // movement detected, reset stillness timer
      }
      // Safety timeout: if no stillness confirmed within 4s of impact, reset
      if (now - impactTime > 4000 && currentState != FALL_CONFIRMED) {
        currentState = NORMAL;
      }
      break;

    case FALL_CONFIRMED:
      digitalWrite(BUZZER_PIN, HIGH);
      break;
  }

  // Serial debug
  Serial.print("Mag:"); Serial.print(magnitude, 2);
  Serial.print(" State:"); Serial.println(currentState);

  // OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Mag: "); display.println(magnitude, 2);

  display.setCursor(0, 16);
  display.setTextSize(2);
  switch (currentState) {
    case NORMAL: display.println("Normal"); break;
    case DIP_DETECTED: display.println("Dip..."); break;
    case IMPACT_DETECTED: display.println("Impact!"); break;
    case FALL_CONFIRMED: display.println("FALL!"); break;
  }
  display.display();

  delay(100);
}