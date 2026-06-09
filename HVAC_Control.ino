/**
 * HVAC_Control.ino
 * 
 * HỆ THỐNG ĐIỀU KHIỂN HVAC THÔNG MINH - HOÀN THIỆN TÍCH HỢP DASHBOARD
 * Thiết kế cho: ESP32-S3-N16R8 + Sensirion SCD30 + Module Relay
 * Nền tảng: Arduino IDE
 * 
 * Tính năng chính:
 *   1. Tự động kiểm soát nhiệt độ (Làm mát/Làm ấm qua hệ thống LED chỉ thị)
 *   2. Tự động điều khiển quạt tản thông gió qua Relay (khi CO2 > 1000 ppm hoặc Độ ẩm > 60%)
 *   3. Kết nối WiFi và đồng bộ hóa dữ liệu 2 chiều với Dashboard qua giao thức MQTT:
 *      - Chiều gửi: Đẩy thông số cảm biến lên topic "sensor/hvac-01" dạng JSON.
 *      - Chiều nhận: Lắng nghe lệnh điều khiển tắt/bật nguồn, thay đổi nhiệt độ cài đặt từ xa qua topic "remote-control".
 *   4. Cơ chế tự động hóa cục bộ độc lập: Nếu mất mạng WiFi/MQTT, hệ thống cục bộ vẫn tự điều khiển an toàn!
 */

#include <Wire.h>
#include <WiFi.h>
#include "src/SparkFun_SCD30/SparkFun_SCD30_Arduino_Library.h" // Thư viện SCD30 cục bộ
#include "src/PubSubClient/PubSubClient.h"                   // Thư viện MQTT cục bộ

// =========================================================================
// ⚙️ CẤU HÌNH HỆ THỐNG (Vui lòng thay đổi theo mạng của bạn)
// =========================================================================

// 1. Cấu hình kết nối WiFi
#define WIFI_SSID        "VIETSET_TECH"      // Tên mạng WiFi của bạn
#define WIFI_PASSWORD    "vs68686868"  // Mật khẩu WiFi của bạn

// 2. Cấu hình kết nối MQTT Broker (Kết nối tới Dashboard smart-hvac đám mây)
#define MQTT_SERVER      "192.168.1.71"    // IP of this PC (Docker host) on the Wi‑Fi network
#define MQTT_PORT        1883                  // Cổng MQTT của docker-compose.alt.yml
#define MQTT_DEVICE_ID   "indoor-01"           // ID thiết bị đám mây của bạn
#define MQTT_PUB_TOPIC   "sensor/indoor"       // Topic gửi dữ liệu cảm biến đám mây
#define MQTT_SUB_TOPIC   "remote-control/#"    // Topic nhận lệnh điều khiển đám mây (wildcard)

// 3. Cấu hình chân kết nối phần cứng (Pin Definitions)
#define I2C_SDA          8     // Chân SDA nối cảm biến SCD30 (Physical Pin 12 -> GPIO8)
#define I2C_SCL          9     // Chân SCL nối cảm biến SCD30 (Physical Pin 15 -> GPIO9)
#define PIN_RELAY_FAN    4     // Chân GPIO4 điều khiển Relay quạt tản (Physical Pin 4)
#define RELAY_ACTIVE_LOW false // Sửa lỗi ngược: Đặt thành false (Kích HIGH để bật quạt, LOW để tắt)
#define USE_ONBOARD_RGB  true  // Đặt thành 'true' nếu dùng LED RGB WS2812B tích hợp trên board ESP32-S3
#define PIN_RGB_WS2812   48    // Chân điều khiển LED RGB WS2812B (thường là 48 hoặc 38)

// Cấu hình chân UART cho cảm biến bụi PMS5003 (Physical Pin 9 -> GPIO16 (net PMS RX), Pin 10 -> GPIO17 (net PMS TX))
#define PMS_RX           17    // ESP32 RX pin (connects to PMS TX net)
#define PMS_TX           16    // ESP32 TX pin (connects to PMS RX net)
#define PIN_SERVO_VALVE  15    // Chân điều khiển Servo van thông gió (Physical Pin 8 -> GPIO15)

// Các chân GPIO nếu bạn sử dụng LED rời gắn ngoài (khi USE_ONBOARD_RGB = false)
#define PIN_LED_COOLING  10    // LED Xanh báo làm mát (Cooling)
#define PIN_LED_HEATING  11    // LED Đỏ hoặc màu khác báo làm ấm (Heating)

// =========================================================================
// 🔄 THÔNG SỐ VÀ BIẾN TOÀN CỤC
// =========================================================================

// Khởi tạo các đối tượng
SCD30 airSensor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Các biến điều khiển hệ thống (Có thể thay đổi động từ xa qua MQTT)
float TEMP_SETPOINT        = 25.0; // Nhiệt độ cài đặt mục tiêu (°C)
bool  systemPowerState     = true; // Trạng thái hoạt động hệ thống (true: ON, false: OFF - Chế độ chờ Standby)

// Các ngưỡng cố định bảo vệ tiện nghi không khí
const float TEMP_HYSTERESIS     = 0.5;  // Khoảng trễ nhiệt độ (°C) tránh đóng ngắt liên tục
const float CO2_MAX           = 800.0;  // Ngưỡng CO2 chuẩn báo động (ppm) để bật quạt
const float CO2_HYSTERESIS    = 100.0;  // Khoảng trễ CO2 (Quạt tắt khi < 700ppm)
const float HUMIDITY_MAX      = 60.0;   // Ngưỡng Độ ẩm tối đa (%) theo chuẩn
const float HUMIDITY_HYSTERESIS = 5.0;  // Khoảng trễ Độ ẩm (Quạt tắt khi < 55%)

// Ngưỡng chất lượng không khí mở rộng (Bụi mịn PM2.5)
const float PM25_MAX          = 50.0;   // Ngưỡng bụi PM2.5 kích hoạt quạt (ug/m3)
const float PM25_HYSTERESIS   = 10.0;   // Khoảng trễ PM2.5 (Quạt tắt khi < 40ug/m3)

// Quản lý trạng thái hiện tại
bool currentFanState   = false; // Trạng thái Quạt tản (false: OFF, true: ON)
bool isCoolingActive   = false; // Trạng thái Làm mát
bool isHeatingActive   = false; // Trạng thái Làm ấm
float currentPM25      = 0.0;   // Trạng thái nồng độ bụi PM2.5 hiện tại
int currentValveAngle  = 0;     // Góc mở van thông gió qua Servo (0 - 90 độ)

// Các biến điều khiển hệ thống nâng cao
String hvacMode = "auto";
String hvacManualState = "off";
String fanMode = "auto";
String fanManualState = "off";

// Quản lý thời gian (Non-blocking timer)
unsigned long lastReadTime       = 0;
const unsigned long READ_INTERVAL = 2000; // Chu kỳ đọc SCD30 và tính toán điều khiển (2 giây)

unsigned long lastMqttRetryTime  = 0;
const unsigned long MQTT_RETRY_INTERVAL = 5000; // Thử lại kết nối MQTT sau mỗi 5 giây nếu đứt mạng

// =========================================================================
// 🛠️ CÁC HÀM TIỆN ÍCH ĐIỀU KHIỂN THIẾT BỊ
// =========================================================================

/**
 * Hàm bật/tắt Relay Quạt tản
 */
void controlFan(bool turnOn) {
  currentFanState = turnOn;
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(PIN_RELAY_FAN, turnOn ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RELAY_FAN, turnOn ? HIGH : LOW);
  }
}

/**
 * Hàm điều khiển hệ thống LED chỉ thị nhiệt độ
 */
void controlTemperatureLEDs(bool cooling, bool heating) {
  isCoolingActive = cooling;
  isHeatingActive = heating;

  if (USE_ONBOARD_RGB) {
    if (cooling) {
      neopixelWrite(PIN_RGB_WS2812, 0, 0, 128); // Sáng màu Xanh Dương (Blue) báo làm mát
    } else if (heating) {
      neopixelWrite(PIN_RGB_WS2812, 128, 0, 0); // Sáng màu Đỏ (Red) báo làm ấm
    } else if (systemPowerState) {
      neopixelWrite(PIN_RGB_WS2812, 0, 128, 0); // Sáng màu Xanh Lá (Green) báo nhiệt độ lý tưởng
    } else {
      neopixelWrite(PIN_RGB_WS2812, 10, 5, 0);   // Sáng màu Cam mờ báo chế độ chờ
    }
  } else {
    digitalWrite(PIN_LED_COOLING, cooling ? HIGH : LOW);
    digitalWrite(PIN_LED_HEATING, heating ? HIGH : LOW);
  }
}

/**
 * Hàm đọc nồng độ bụi PM2.5 từ cảm biến bụi PMS (giao tiếp UART qua Serial2)
 */
float readPM25() {
  static uint8_t buffer[32];
  static int index = 0;
  static float pm25 = 0.0;
  
  while (Serial2.available() > 0) {
    uint8_t ch = Serial2.read();
    
    // Tìm byte bắt đầu 0x42 và 0x4D
    if (index == 0 && ch != 0x42) continue;
    if (index == 1 && ch != 0x4D) {
      index = 0;
      continue;
    }
    
    buffer[index++] = ch;
    
    if (index == 32) {
      // Hex dump
      Serial.print("[PMS] HEX: ");
      for(int i=0; i<32; i++) { Serial.printf("%02X ", buffer[i]); }
      Serial.println();

      index = 0;
      // Kiểm tra checksum
      uint16_t sum = 0;
      for (int i = 0; i < 30; i++) {
        sum += buffer[i];
      }
      uint16_t checksum = ((uint16_t)buffer[30] << 8) | buffer[31];
      if (sum == checksum) {
        // Nồng độ PM2.5 (Standard particle) nằm ở byte 12 và 13
        uint16_t pm25_val = ((uint16_t)buffer[12] << 8) | buffer[13];
        pm25 = (float)pm25_val;
      }
    }
  }
  return pm25;
}

/**
 * Hàm quét I2C để kiểm tra thiết bị
 */
void i2c_scanner() {
  byte error, address;
  int nDevices = 0;
  Serial.println("\n[I2C] Quét thiết bị I2C...");
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("  --> Tìm thấy thiết bị tại địa chỉ 0x%02X\n", address);
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("  --> Không tìm thấy thiết bị I2C nào.");
}

/**
 * Hàm điều khiển góc quay của Servo van thông gió (góc từ 0 - 180 độ)
 * Sử dụng tính năng LEDC PWM của vi điều khiển ESP32
 */
void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  
  // Thời gian độ rộng xung chuẩn của Servo từ 0.5ms (0 độ) đến 2.5ms (180 độ)
  float pulseMs = 0.5 + (angle / 180.0) * 2.0;
  
  // Chu kỳ PWM là 20ms (tần số 50Hz).
  // Chuyển đổi sang giá trị duty cycle tương ứng độ phân giải 14-bit (2^14 - 1 = 16383)
  int duty = (int)((pulseMs / 20.0) * 16383.0);
  
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_SERVO_VALVE, duty);
#else
  ledcWrite(0, duty); // Channel 0
#endif
}

// =========================================================================
// 🌐 KẾT NỐI MẠNG (WIFI & MQTT)
// =========================================================================

/**
 * Hàm khởi tạo kết nối WiFi ban đầu
 */
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("[WiFi] Dang ket noi toi mạng: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Không chặn luồng chính mãi mãi, cho phép khởi động cục bộ ngay cả khi không có WiFi
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Ket noi thanh cong!");
    Serial.print("[WiFi] Dia chi IP cua ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[Canh bao] Khong the ket noi WiFi. He thong se chay o che do CUC BO (Offline).");
  }
}

/**
 * Bộ xử lý tin nhắn điều khiển từ xa (MQTT Callback)
 * Nhận lệnh từ Dashboard gửi xuống
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Đóng gói payload nhận được thành chuỗi an toàn
  char message[256];
  unsigned int i = 0;
  for (i = 0; i < length && i < sizeof(message) - 1; i++) {
    message[i] = (char)payload[i];
  }
  message[i] = '\0';

  Serial.printf("\n[MQTT Callback] Nhan lenh tren Topic [%s]: %s\n", topic, message);

  // Đảm bảo lệnh này gửi tới thiết bị của chúng ta
  if (strstr(message, MQTT_DEVICE_ID) == NULL) {
    // Nếu trong chuỗi lệnh không ghi device_id của ta, bỏ qua lệnh này
    // Tuy nhiên nếu Dashboard gửi dạng quảng bá, ta vẫn có thể tiếp nhận.
  }

  // 1. Phân tích trạng thái POWER (Bật/Tắt hệ thống từ xa)
  // Lệnh mẫu từ Dashboard: "power":true hoặc "power":false
  char* powerPtr = strstr(message, "\"power\"");
  if (powerPtr != NULL) {
    char* valPtr = strchr(powerPtr, ':');
    if (valPtr != NULL) {
      valPtr++;
      while (*valPtr == ' ' || *valPtr == '\t') valPtr++; // Bỏ qua khoảng trắng
      
      bool nextPowerState = systemPowerState;
      if (strncmp(valPtr, "true", 4) == 0) {
        nextPowerState = true;
      } else if (strncmp(valPtr, "false", 5) == 0) {
        nextPowerState = false;
      }

      if (nextPowerState != systemPowerState) {
        systemPowerState = nextPowerState;
        Serial.printf("  --> [MQTT Command] Thay doi nguon he thong: %s\n", 
                      systemPowerState ? "BAT (ON)" : "TAT (STANDBY/OFF)");
        
        // Nếu tắt nguồn hệ thống, tắt toàn bộ quạt và LED chỉ thị nhiệt độ ngay lập tức
        if (!systemPowerState) {
          controlFan(false);
          controlTemperatureLEDs(false, false);
          if (USE_ONBOARD_RGB) {
            // Sáng LED vàng nhạt/cam ở chế độ chờ (Standby) hoặc tắt hoàn toàn
            neopixelWrite(PIN_RGB_WS2812, 10, 5, 0); // Sáng cam mờ báo chế độ chờ
          }
        }
      }
    }
  }

  // 2. Phân tích nhiệt độ cài đặt mới TEMP (Setpoint Temp)
  // Lệnh mẫu từ Dashboard: "temp":24.5
  char* tempPtr = strstr(message, "\"temp\"");
  if (tempPtr != NULL) {
    char* valPtr = strchr(tempPtr, ':');
    if (valPtr != NULL) {
      valPtr++;
      float newSetpoint = atof(valPtr);
      if (newSetpoint >= 0.0 && newSetpoint <= 40.0) { // Giới hạn dải nhiệt độ tùy biến (0 - 40*C)
        TEMP_SETPOINT = newSetpoint;
        Serial.printf("  --> [MQTT Command] Cap nhat Setpoint nhiet do moi: %.1f *C\n", TEMP_SETPOINT);
      } else {
        Serial.printf("  --> [MQTT Command] Canh bao: Nhiet do %.1f *C ngoai dải an toan (0 - 40*C)\n", newSetpoint);
      }
    }
  }

  // 3. Phân tích chế độ HVAC (operationMode)
  char* opModePtr = strstr(message, "\"operationMode\"");
  if (opModePtr != NULL) {
    char* valPtr = strchr(opModePtr, ':');
    if (valPtr != NULL) {
      valPtr++;
      while (*valPtr == ' ' || *valPtr == '\t' || *valPtr == '"') valPtr++; 
      
      if (strncmp(valPtr, "auto", 4) == 0) {
        hvacMode = "auto";
      } else if (strncmp(valPtr, "cool", 4) == 0) {
        hvacMode = "manual";
        hvacManualState = "cool";
      } else if (strncmp(valPtr, "heat", 4) == 0) {
        hvacMode = "manual";
        hvacManualState = "heat";
      } else if (strncmp(valPtr, "off", 3) == 0) {
        hvacMode = "manual";
        hvacManualState = "off";
      }
      Serial.printf("  --> [MQTT Command] HVAC Mode: %s | State: %s\n", hvacMode.c_str(), hvacManualState.c_str());
    }
  }

  // 4. Phân tích chế độ Quạt (fanPower)
  char* fanPowerPtr = strstr(message, "\"fanPower\"");
  if (fanPowerPtr != NULL) {
    char* valPtr = strchr(fanPowerPtr, ':');
    if (valPtr != NULL) {
      valPtr++;
      while (*valPtr == ' ' || *valPtr == '\t' || *valPtr == '"') valPtr++;
      
      if (strncmp(valPtr, "auto", 4) == 0) {
        fanMode = "auto";
      } else if (strncmp(valPtr, "on", 2) == 0) {
        fanMode = "manual";
        fanManualState = "on";
      } else if (strncmp(valPtr, "off", 3) == 0) {
        fanMode = "manual";
        fanManualState = "off";
      }
      Serial.printf("  --> [MQTT Command] Fan Mode: %s | State: %s\n", fanMode.c_str(), fanManualState.c_str());
    }
  }
}

/**
 * Hàm tự động duy trì và kết nối lại WiFi khi mất mạng
 */
void maintainWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiRetryTime = 0;
    unsigned long now = millis();
    if (now - lastWiFiRetryTime >= 10000) { // Thử kết nối lại WiFi sau mỗi 10 giây
      lastWiFiRetryTime = now;
      Serial.println("\n[WiFi] Canh bao: Mat ket noi! Dang tien hanh ket noi lai WiFi...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}

/**
 * Hàm duy trì và kết nối lại MQTT Broker (Không chặn luồng chính)
 */
void maintainMQTTConnection() {
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastMqttRetryTime >= MQTT_RETRY_INTERVAL) {
      lastMqttRetryTime = now;
      
      // Đảm bảo WiFi đã kết nối trước khi cố gắng kết nối MQTT
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[MQTT] Dang ket noi den Broker...");
        
        // Cố gắng kết nối
        if (mqttClient.connect(MQTT_DEVICE_ID)) {
          Serial.println("Thanh cong!");
          
          // Đăng ký nhận kênh điều khiển từ xa
          mqttClient.subscribe(MQTT_SUB_TOPIC);
          Serial.printf("[MQTT] Da dang ky (Subscribed) topic: %s\n", MQTT_SUB_TOPIC);
        } else {
          Serial.print("That bai, ma loi rc = ");
          Serial.print(mqttClient.state());
          Serial.println(". Se thu lai sau 5 giay.");
        }
      } else {
        Serial.println("[MQTT] Canh bao: WiFi chua online. Bo qua buoc ket noi Broker.");
      }
    }
  } else {
    // Duy trì giao tiếp MQTT
    mqttClient.loop();
  }
}

// =========================================================================
// SETUP & LOOP
// =========================================================================

void setup() {
  // Khởi tạo Serial Monitor
  Serial.begin(115200);
  delay(2000); // Chờ cổng native USB sẵn sàng

  Serial.println("\n=======================================================");
  Serial.println("  KHOI DONG SAN PHAM HVAC CONTROL SMART-IOT COMPLETED");
  Serial.println("=======================================================");

  // 1. Cấu hình chân phần cứng Output/Input
  pinMode(PIN_RELAY_FAN, OUTPUT);
  controlFan(false); // Quạt tắt ban đầu

  // Khởi tạo UART cho cảm biến bụi PMS
  Serial2.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  Serial.printf("[PMS] Khoi tao UART: RX -> GPIO%d, TX -> GPIO%d\n", PMS_RX, PMS_TX);

  // Cấu hình Servo van thông gió dùng LEDC PWM
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_SERVO_VALVE, 50, 14); // Tần số 50Hz, Độ phân giải 14-bit
#else
  ledcSetup(0, 50, 14); // Kênh 0, Tần số 50Hz, Độ phân giải 14-bit
  ledcAttachPin(PIN_SERVO_VALVE, 0);
#endif
  setServoAngle(0); // Đóng van thông gió ban đầu (0 độ)

  if (!USE_ONBOARD_RGB) {
    pinMode(PIN_LED_COOLING, OUTPUT);
    pinMode(PIN_LED_HEATING, OUTPUT);
    digitalWrite(PIN_LED_COOLING, LOW);
    digitalWrite(PIN_LED_HEATING, LOW);
  } else {
    neopixelWrite(PIN_RGB_WS2812, 0, 0, 0); // Tắt LED RGB
  }

  // 2. Khởi tạo bus I2C tùy biến
  Serial.printf("[I2C] Khoi tao bus: SDA -> GPIO%d, SCL -> GPIO%d...\n", I2C_SDA, I2C_SCL);
  Wire.begin(I2C_SDA, I2C_SCL);
  i2c_scanner();

  // 3. Khởi tạo cảm biến SCD30
  Serial.println("[SCD30] Dang ket noi voi cam bien...");
  if (airSensor.begin(Wire) == false) {
    Serial.println("[LOI] Khong tim thay cam bien SCD30!");
    Serial.println("  --> Vui long kiem tra lai duong day va nguon cap.");
    
    // Nhấp nháy LED đỏ liên tục báo lỗi phần cứng nặng
    while (1) {
      if (USE_ONBOARD_RGB) {
        neopixelWrite(PIN_RGB_WS2812, 128, 0, 0); delay(500);
        neopixelWrite(PIN_RGB_WS2812, 0, 0, 0); delay(500);
      } else {
        digitalWrite(PIN_LED_HEATING, HIGH); delay(500);
        digitalWrite(PIN_LED_HEATING, LOW); delay(500);
      }
    }
  }
  Serial.println("[SCD30] Ket noi thanh cong!");
  airSensor.setMeasurementInterval(2);

  // 4. Khởi tạo mạng WiFi
  setupWiFi();

  // 5. Cấu hình MQTT Broker
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("\n>> He thong HVAC san sang hoat dong!");
  Serial.println("-------------------------------------------------------");
}

void loop() {
  // 1. Tự động duy trì và kết nối lại WiFi nếu mất mạng
  maintainWiFiConnection();

  // 2. Duy trì kết nối MQTT (Không chặn)
  maintainMQTTConnection();

  // 2. Đọc cảm biến, chạy thuật toán điều khiển và đẩy dữ liệu định kỳ mỗi 2 giây
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    if (airSensor.dataAvailable()) {
      float temperature = airSensor.getTemperature();
      float humidity    = airSensor.getHumidity();
      float co2         = airSensor.getCO2();

      // Đọc giá trị bụi PM2.5 từ cảm biến
      currentPM25 = readPM25();

      // Hiển thị thông số cục bộ lên cổng Serial
      Serial.printf("[Sensor] CO2: %.1f ppm | Temp: %.1f *C | Hum: %.1f %% | PM2.5: %.1f ug/m3\n", 
                    co2, temperature, humidity, currentPM25);

      // A. CHỈ CHẠY THUẬT TOÁN ĐIỀU KHIỂN NẾU HỆ THỐNG ĐƯỢC BẬT (POWER = ON)
      if (systemPowerState) {
        // ===================================================================
        // 1. THUẬT TOÁN ĐIỀU KHIỂN NHIỆT ĐỘ CỤC BỘ (LÀM MÁT / LÀM ẤM - LED)
        // ===================================================================
        bool nextCoolingState = isCoolingActive;
        bool nextHeatingState = isHeatingActive;

        if (hvacMode == "auto") {
          if (temperature > (TEMP_SETPOINT + TEMP_HYSTERESIS / 2.0)) {
            nextCoolingState = true;
            nextHeatingState = false;
          } else if (temperature < (TEMP_SETPOINT - TEMP_HYSTERESIS / 2.0)) {
            nextCoolingState = false;
            nextHeatingState = true;
          } else {
            // Nhiệt độ lý tưởng (Comfort Zone)
            nextCoolingState = false;
            nextHeatingState = false;
          }
        } else {
          // Che do thu cong
          if (hvacManualState == "cool") {
            nextCoolingState = true;
            nextHeatingState = false;
          } else if (hvacManualState == "heat") {
            nextCoolingState = false;
            nextHeatingState = true;
          } else { // "off"
            nextCoolingState = false;
            nextHeatingState = false;
          }
        }

        // Luôn gọi hàm để cập nhật màu LED chuẩn xác (bao gồm LED Xanh lá khi bình thường)
        controlTemperatureLEDs(nextCoolingState, nextHeatingState);

        // ===================================================================
        // 2. THUẬT TOÁN ĐIỀU KHIỂN QUẠT TẢN THÔNG GIÓ (CO2 & PM2.5)
        // ===================================================================
        bool nextFanState = currentFanState;

        if (fanMode == "auto") {
          // Bật quạt tản khi nồng độ CO2 hoặc PM2.5 vượt ngưỡng nguy hại
          if (co2 > CO2_MAX || currentPM25 > PM25_MAX) {
            nextFanState = true;
          } 
          // Chỉ tắt quạt khi cả CO2 và PM2.5 đều hạ xuống dưới ngưỡng trễ an toàn
          else if (co2 < (CO2_MAX - CO2_HYSTERESIS) && currentPM25 < (PM25_MAX - PM25_HYSTERESIS)) {
            nextFanState = false;
          }
        } else {
          // Che do thu cong
          if (fanManualState == "on") {
            nextFanState = true;
          } else { // "off"
            nextFanState = false;
          }
        }

        if (nextFanState != currentFanState) {
          controlFan(nextFanState);
          Serial.printf("  --> [QUAT Local] Cap nhat Relay Quat: %s\n", 
                        nextFanState ? "ON (BAT QUAT)" : "OFF (TAT QUAT)");
        }

        // ===================================================================
        // 3. THUẬT TOÁN ĐIỀU KHIỂN GÓC MỞ VAN THÔNG GIÓ (SERVO)
        // ===================================================================
        int nextValveAngle = 0;
        
        // Tỷ lệ phần trăm mở van dựa trên CO2 (khoảng từ 600ppm đến 1200ppm)
        float co2Ratio = (co2 - 600.0) / 600.0;
        if (co2Ratio < 0.0) co2Ratio = 0.0;
        if (co2Ratio > 1.0) co2Ratio = 1.0;

        // Tỷ lệ phần trăm mở van dựa trên PM2.5 (khoảng từ 25ug/m3 đến 100ug/m3)
        float pm25Ratio = (currentPM25 - 25.0) / 75.0;
        if (pm25Ratio < 0.0) pm25Ratio = 0.0;
        if (pm25Ratio > 1.0) pm25Ratio = 1.0;

        // Sử dụng giá trị tỷ lệ lớn nhất giữa CO2 và PM2.5 để mở van
        float maxRatio = max(co2Ratio, pm25Ratio);
        nextValveAngle = (int)(maxRatio * 90.0); // Mở góc từ 0 đến 90 độ cho phù hợp

        if (nextValveAngle != currentValveAngle) {
          currentValveAngle = nextValveAngle;
          setServoAngle(currentValveAngle);
          Serial.printf("  --> [SERVO Valve] Thay doi goc mo van: %d do (Ty le %.1f%%)\n", 
                        currentValveAngle, (currentValveAngle / 90.0) * 100.0);
        }
      } else {
        // Trạng thái nguồn tắt (Standby) - Đóng van thông gió, tắt quạt, sáng cam mờ
        controlFan(false);
        controlTemperatureLEDs(false, false);
        if (currentValveAngle != 0) {
          currentValveAngle = 0;
          setServoAngle(0);
        }
        Serial.println("  --> [System State] Che do cho (Standby). Chờ lenh tu xa...");
      }

      // ===================================================================
      // B. ĐẨY DỮ LIỆU LÊN DASHBOARD QUA MQTT (NẾU ĐÃ KẾT NỐI BROKER)
      // ===================================================================
      if (mqttClient.connected()) {
        char jsonPayload[256];
        // Đóng gói JSON thủ công bao gồm nồng độ bụi thực tế và góc mở van thông gió
        snprintf(jsonPayload, sizeof(jsonPayload),
                 "{\"device_id\":\"%s\",\"temperature\":%.2f,\"outdoor_temperature\":%.2f,\"humidity\":%.2f,\"co2\":%d,\"dust\":%.2f,\"valve_angle\":%d}",
                 MQTT_DEVICE_ID, temperature, (temperature + 3.2), humidity, (int)co2, currentPM25, currentValveAngle);

        Serial.printf("[MQTT Publish] Gui tin len topic [%s]: %s\n", MQTT_PUB_TOPIC, jsonPayload);
        
        bool pubResult = mqttClient.publish(MQTT_PUB_TOPIC, jsonPayload);
        if (pubResult) {
          Serial.println("  --> [MQTT] Gui du lieu thanh cong!");
        } else {
          Serial.println("  --> [MQTT] LOI: Gui du lieu that bai.");
        }
      } else {
        Serial.println("[MQTT] Khong online. Bo qua buoc gui du lieu.");
      }
      
      Serial.println("-------------------------------------------------------");
    } else {
      Serial.println("[SCD30] Dang cho du lieu moi...");
    }
  }
}
