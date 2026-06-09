/**
 * Relay Test for channel 1 (GPIO4)
 * This sketch toggles the first relay (channel 1) on and off every 2 seconds.
 * It uses the same pin definition as the main project (PIN_RELAY_FAN = 4).
 */

#define RELAY_PIN 4               // GPIO4 – first channel of 4‑channel relay board
#define RELAY_ACTIVE_LOW false    // Set to false (HIGH = ON, LOW = OFF) as in main code

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(RELAY_PIN, OUTPUT);
  // Ensure relay starts OFF
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  Serial.println("[Relay Test] Started – toggling channel 1");
}

void loop() {
  // Turn ON
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
  Serial.println("[Relay] ON (channel 1)");
  delay(2000);
  // Turn OFF
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  Serial.println("[Relay] OFF (channel 1)");
  delay(2000);
}
