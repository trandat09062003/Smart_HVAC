/**
 * Simple Servo Test for HVAC Valve (channel 15)
 * Uses the same pin definitions as the main project.
 */

#define SERVO_PIN 15               // GPIO15 – matches PCB design
#define SERVO_LEDC_CH 0            // LEDC channel 0
#define SERVO_LEDC_HZ 50           // 50 Hz PWM for hobby servos
#define SERVO_LEDC_RES 14          // 14‑bit resolution

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  float pulseMs = 0.5 + (angle / 180.0) * 2.0;
  int duty = (int)((pulseMs / 20.0) * 16383.0);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(SERVO_PIN, duty);
#else
  ledcWrite(SERVO_LEDC_CH, duty);
#endif
}

void setup() {
  Serial.begin(115200);
  delay(500);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(SERVO_PIN, SERVO_LEDC_HZ, SERVO_LEDC_RES);
#else
  ledcSetup(SERVO_LEDC_CH, SERVO_LEDC_HZ, SERVO_LEDC_RES);
  ledcAttachPin(SERVO_PIN, SERVO_LEDC_CH);
#endif
  setServoAngle(0);
}

void loop() {
  for (int a = 0; a <= 180; a += 30) {
    setServoAngle(a);
    Serial.printf("[Servo] Angle: %d°\n", a);
    delay(800);
  }
  delay(1500);
  for (int a = 180; a >= 0; a -= 30) {
    setServoAngle(a);
    Serial.printf("[Servo] Angle: %d°\n", a);
    delay(800);
  }
  delay(3000);
}
