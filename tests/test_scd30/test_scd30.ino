#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>

SCD30 airSensor;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("=== SCD30 Test Sketch ===");
  Wire.begin();
  if (!airSensor.begin(Wire)) {
    Serial.println("[ERROR] Không tìm thấy cảm biến SCD30! Kiểm tra kết nối I2C.");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("[INFO] SCD30 khởi tạo thành công.");
  airSensor.setMeasurementInterval(2);
}

void loop() {
  if (airSensor.dataAvailable()) {
    float temperature = airSensor.getTemperature();
    float humidity = airSensor.getHumidity();
    float co2 = airSensor.getCO2();
    Serial.printf("[SCD30] Temp: %.2f *C | Humidity: %.2f %% | CO2: %.0f ppm\n", temperature, humidity, co2);
  } else {
    Serial.println("[SCD30] Đang chờ dữ liệu...");
  }
  delay(2000);
}
