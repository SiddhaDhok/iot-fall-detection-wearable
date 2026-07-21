#define BLYNK_TEMPLATE_ID "PASTE_YOUR_TEMPLATE_ID_HERE"
#define BLYNK_TEMPLATE_NAME "IoT Fall and Faint Detection Device"
#define BLYNK_AUTH_TOKEN "PASTE_YOUR_AUTH_TOKEN_HERE"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <math.h>

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUZZER_PIN 25
#define BUTTON_PIN 27

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

const float DIP_THRESHOLD = 0.6;
const float IMPACT_THRESHOLD = 1.5;
const float STILL_LOW = 0.92;
const float STILL_HIGH = 1.08;
const unsigned long IMPACT_WINDOW_MS = 1000;
const unsigned long STILLNESS_DURATION_MS = 5500;
const unsigned long CANCEL_WINDOW_MS = 15000;

enum State { NORMAL, DIP_DETECTED, IMPACT_DETECTED, FALL_CONFIRMED, ALARM_ACTIVE, CANCELLED };
State currentState = NORMAL;

unsigned long dipTime = 0;
unsigned long impactTime = 0;
unsigned long stillnessStart = 0;
unsigned long alarmStartTime = 0;
int fallCount = 0;
bool blynkUpdatedForThisFall = false;

BlynkTimer timer;

void sendToBlynk(String status) {
  Blynk.virtualWrite(V0, status);
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT);

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
  display.println("Connecting WiFi...");
  display.display();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Fall Detection Ready");
  display.display();
  delay(1000);

  Blynk.virtualWrite(V0, "Normal");
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);

  Serial.println("Fall Detection + Blynk Started!");
}

void loop() {
  Blynk.run();

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
  bool buttonPressed = (digitalRead(BUTTON_PIN) == HIGH);

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
        currentState = NORMAL;
      }
      break;

    case IMPACT_DETECTED:
      if (magnitude >= STILL_LOW && magnitude <= STILL_HIGH) {
        if (stillnessStart == 0) stillnessStart = now;
        if (now - stillnessStart >= STILLNESS_DURATION_MS) {
          currentState = FALL_CONFIRMED;
          alarmStartTime = now;
          blynkUpdatedForThisFall = false;
          Serial.println(">> FALL CONFIRMED - starting cancel countdown");
        }
      } else {
        stillnessStart = 0;
      }
      if (now - impactTime > 8000 && currentState != FALL_CONFIRMED) {
        currentState = NORMAL;
        Serial.println(">> Reset - no sustained stillness");
      }
      break;

    case FALL_CONFIRMED:
      digitalWrite(BUZZER_PIN, HIGH);
      if (!blynkUpdatedForThisFall) {
        fallCount++;
        Blynk.virtualWrite(V0, "Fall Detected");
        Blynk.virtualWrite(V1, 1);
        Blynk.virtualWrite(V2, fallCount);
        blynkUpdatedForThisFall = true;
      }
      if (buttonPressed) {
        currentState = CANCELLED;
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println(">> Alarm CANCELLED by user");
      } else if (now - alarmStartTime >= CANCEL_WINDOW_MS) {
        currentState = ALARM_ACTIVE;
        Serial.println(">> No cancel - ALARM ACTIVE (Telegram fires here later)");
      }
      break;

    case ALARM_ACTIVE:
      digitalWrite(BUZZER_PIN, HIGH);
      Blynk.virtualWrite(V0, "ALARM ACTIVE");
      if (buttonPressed) {
        currentState = CANCELLED;
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println(">> Alarm CANCELLED by user (post-escalation)");
      }
      break;

    case CANCELLED:
      digitalWrite(BUZZER_PIN, LOW);
      Blynk.virtualWrite(V0, "Normal");
      Blynk.virtualWrite(V1, 0);
      if (!buttonPressed) {
        delay(500);
        currentState = NORMAL;
        Serial.println(">> Reset to NORMAL");
      }
      break;
  }

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
    case FALL_CONFIRMED: {
      display.println("FALL!");
      display.setTextSize(1);
      int secsLeft = (CANCEL_WINDOW_MS - (now - alarmStartTime)) / 1000;
      if (secsLeft < 0) secsLeft = 0;
      display.print("Cancel in: "); display.print(secsLeft); display.println("s");
      break;
    }
    case ALARM_ACTIVE: display.println("ALARM!"); break;
    case CANCELLED: display.println("Cancelled"); break;
  }
  display.display();

  delay(100);
}