#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void setup() {
  Wire.begin();
  Serial.begin(115200);

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
  display.println("MPU6050 + OLED OK");
  display.display();
  delay(1000);

  Serial.println("Magnitude Reader Started!");
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

  // Convert raw values to g-force (default sensitivity: 16384 LSB/g)
  float gx = AcX / 16384.0;
  float gy = AcY / 16384.0;
  float gz = AcZ / 16384.0;

  // Total acceleration magnitude, in g
  float magnitude = sqrt(gx * gx + gy * gy + gz * gz);

  Serial.print("X:"); Serial.print(gx, 2);
  Serial.print(" Y:"); Serial.print(gy, 2);
  Serial.print(" Z:"); Serial.print(gz, 2);
  Serial.print(" | Magnitude: "); Serial.println(magnitude, 2);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Accel (g):");
  display.setCursor(0, 12);
  display.print("X:"); display.print(gx, 2);
  display.print(" Y:"); display.print(gy, 2);
  display.print(" Z:"); display.print(gz, 2);

  display.setCursor(0, 32);
  display.setTextSize(2);
  display.print("Mag:"); display.print(magnitude, 2);
  display.setTextSize(1);

  display.display();
  delay(150);
}