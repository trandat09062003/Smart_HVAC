// Full_Hardware_Test.ino
/*
 * Integration test for the HVAC_Control hardware.
 * This sketch verifies:
 *   - SCD30 sensor reading (CO2, temperature, humidity)
 *   - Relay control (fan) based on CO2 threshold
 *   - Servo valve actuation proportional to CO2 level
 *   - On‑board RGB LED (if available) indicating system state
 *
 * Pin definitions are taken from the main project (HVAC_Control.ino).
 */

#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
// ----- Pin definitions (match main project) -----
#define I2C_SDA          8
#define I2C_SCL          9
#define PIN_RELAY_FAN    4   // Relay controlling the fan
#define RELAY_ACTIVE_LOW false // HIGH = ON, LOW = OFF
#define PIN_SERVO_VALVE  15  // Servo controlling ventilation valve
#define USE_ONBOARD_RGB  true
#define PIN_RGB_WS2812   48

// ----- Thresholds -----
const float CO2_MAX = 800.0;          // ppm – turn fan ON
const float CO2_HYSTERESIS = 100.0;   // turn fan OFF when below (MAX - HYST)

SCD30 airSensor;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[Full Test] Starting hardware integration test");

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize SCD30
  if (!airSensor.begin(Wire)) {
    Serial.println("[Error] SCD30 not found! Check wiring.");
    while (1) { delay(1000); }
  }
  airSensor.setMeasurementInterval(2);
  Serial.println("[Info] SCD30 initialized.");

  // Initialize relay pin
  pinMode(PIN_RELAY_FAN, OUTPUT);
  digitalWrite(PIN_RELAY_FAN, RELAY_ACTIVE_LOW ? HIGH : LOW); // start OFF

  // Initialize servo PWM (LED C PWM channel 0)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_SERVO_VALVE, 50, 14);
  #else
    ledcSetup(0, 50, 14);
    ledcAttachPin(PIN_SERVO_VALVE, 0);
  #endif
  setServoAngle(0);

  // Initialise onboard RGB (if used)
  if (USE_ONBOARD_RGB) {
    neopixelWrite(PIN_RGB_WS2812, 0, 0, 0); // off
  }
}

// Helper to set servo angle (0‑90° range)
void setServoAngle(int angle) {
  angle = constrain(angle, 0, 90);
  float pulseMs = 0.5 + (angle / 90.0) * 2.0; // 0.5‑2.5ms
  int duty = (int)((pulseMs / 20.0) * 16383.0);
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_SERVO_VALVE, duty);
  #else
    ledcWrite(0, duty);
  #endif
}

void loop() {
  // Read sensor data
  if (airSensor.dataAvailable()) {
    float temperature = airSensor.getTemperature();
    float humidity    = airSensor.getHumidity();
    float co2         = airSensor.getCO2();

    Serial.printf("[Sensor] CO2: %.0f ppm | Temp: %.1f C | Hum: %.1f %%\n", co2, temperature, humidity);

    // ----- Fan relay logic -----
    static bool fanState = false;
    if (co2 > CO2_MAX) {
      fanState = true;
    } else if (co2 < (CO2_MAX - CO2_HYSTERESIS)) {
      fanState = false;
    }
    digitalWrite(PIN_RELAY_FAN, RELAY_ACTIVE_LOW ? (fanState ? LOW : HIGH) : (fanState ? HIGH : LOW));
    Serial.printf("[Relay] Fan %s\n", fanState ? "ON" : "OFF");

    // ----- Servo valve logic (0‑90° proportional to CO2 ratio) -----
    float ratio = (co2 - 600.0) / 600.0; // map 600‑1200 ppm to 0‑1
    ratio = constrain(ratio, 0.0, 1.0);
    int angle = (int)(ratio * 90.0);
    setServoAngle(angle);
    Serial.printf("[Servo] Angle %d° (ratio %.2f)\n", angle, ratio);

    // ----- RGB indication -----
    if (USE_ONBOARD_RGB) {
      if (!fanState) {
        // Green (system OK)
        neopixelWrite(PIN_RGB_WS2812, 0, 128, 0);
      } else {
        // Blue (fan active)
        neopixelWrite(PIN_RGB_WS2812, 0, 0, 128);
      }
    }

  } else {
    Serial.println("[Sensor] Waiting for new data...");
  }

  delay(2000); // match main READ_INTERVAL
}
