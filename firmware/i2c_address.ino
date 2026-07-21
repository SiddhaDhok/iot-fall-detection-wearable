#include <Wire.h>

void setup() {
  Wire.begin(); // uses default SDA=21, SCL=22 on most ESP32 boards
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("\nI2C Scanner starting...");
}

void loop() {
  int devicesFound = 0;

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }

  if (devicesFound == 0) {
    Serial.println("No I2C devices found. Check wiring.");
  } else {
    Serial.print(devicesFound);
    Serial.println(" device(s) found.");
  }

  delay(5000); // scan again every 5 seconds
}