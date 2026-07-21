#include <Wire.h>

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // Wake up MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("Direct MPU6050 Raw Reader Started!");
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

  Serial.print("Accel X: "); Serial.print(AcX);
  Serial.print(" | Y: "); Serial.print(AcY);
  Serial.print(" | Z: "); Serial.print(AcZ);

  Serial.print("  ||  Gyro X: "); Serial.print(GyX);
  Serial.print(" | Y: "); Serial.print(GyY);
  Serial.print(" | Z: "); Serial.println(GyZ);

  delay(333);
}