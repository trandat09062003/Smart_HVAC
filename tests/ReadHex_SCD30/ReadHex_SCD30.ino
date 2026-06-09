// ReadHex_SCD30.ino
/*
 * Simple sketch to read raw I2C registers from the SCD30 sensor and
 * output their hexadecimal values to the Serial monitor. Useful for
 * debugging low‑level communication or verifying register contents.
 */

#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>

// Pin definitions – match the main project
#define I2C_SDA 8
#define I2C_SCL 9

SCD30 airSensor;

// Helper: print a byte in two‑digit hex
void printHexByte(uint8_t b) {
  if (b < 0x10) Serial.print('0');
  Serial.print(b, HEX);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[HexDump] Starting SCD30 register dump");

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!airSensor.begin(Wire)) {
    Serial.println("[Error] SCD30 not detected – check wiring!");
    while (1) delay(1000);
  }
  Serial.println("[Info] SCD30 initialized.");

  // Optional: set measurement interval (not required for register dump)
  airSensor.setMeasurementInterval(2);
}

void loop() {
  // The SCD30 has a set of command registers; we can read a few useful ones.
  // Here we read registers 0x00‑0x0F as an example.
  for (uint8_t reg = 0x00; reg <= 0x0F; ++reg) {
    uint16_t value = airSensor.readRegister(reg);
    Serial.print("[Reg 0x");
    printHexByte(reg);
    Serial.print("] = 0x");
    printHexByte(highByte(value));
    Serial.print(' ');
    printHexByte(lowByte(value));
    Serial.println();
    delay(50); // short pause to avoid I2C flooding
  }

  Serial.println("--- End of dump ---\n");
  delay(5000); // repeat every 5 seconds
}
