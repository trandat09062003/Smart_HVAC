# 🌡️ Smart HVAC Control System - ESP32-S3 & SCD30 & Cloud/Local Dashboard

Dự án này là hệ thống điều khiển vi khí hậu và thông gió thông minh (HVAC) hoàn thiện, hiệu năng cao dành cho vi điều khiển **ESP32-S3-N16R8**, tích hợp cảm biến chất lượng không khí cao cấp **Sensirion SCD30** (đo CO2, Nhiệt độ và Độ ẩm thực tế), đóng ngắt quạt thông gió qua **Module Relay** và kết nối đồng bộ 2 chiều linh hoạt với cả **Dashboard cục bộ (Docker)** lẫn **Cloud Server (smart-hvac.io.vn)** qua giao thức **MQTT**.

Hệ thống được thiết kế theo tiêu chuẩn công nghiệp: tích hợp sẵn thư viện cần thiết (Zero-Library dependency cho biên dịch nhanh), tự động chạy chế độ cục bộ an toàn nếu mất mạng (Offline Fallback) và cơ chế điều khiển có khoảng trễ (Hysteresis) để tối ưu tuổi thọ thiết bị.

---

## 🏗️ 1. Sơ đồ kiến trúc toàn diện (System Architecture)

Hệ thống hỗ trợ song song hai mô hình vận hành: kết nối trực tiếp lên **Cloud Server riêng** hoặc chạy độc lập tại mạng nội bộ qua **Docker Compose**:
## 📌 2. Sơ đồ kết nối phần cứng (Wiring Diagram)

> [!IMPORTANT]
> Hãy ngắt tất cả nguồn điện cấp cho ESP32-S3 trước khi thực hiện cắm dây để bảo vệ các cổng GPIO nhạy cảm.

### 🔌 A. Cảm biến SCD30 với ESP32-S3 (Giao tiếp I2C)
| Chân SCD30 | Chân ESP32-S3 | Chức năng | Màu dây đề xuất |
| :--- | :--- | :--- | :--- |
| **VIN** | **3V3** | Cấp nguồn 3.3V | Đỏ |
| **GND** | **GND** | Đất chung | Đen |
| **SDA** | **GPIO8** | Đường truyền dữ liệu I2C SDA | Vàng |
| **SCL** | **GPIO9** | Đường truyền xung nhịp I2C SCL | Cam |

### 🎛️ B. Module Relay với ESP32-S3
| Chân Relay | Chân ESP32-S3 / Nguồn | Chức năng |
| :--- | :--- | :--- |
| **VCC** | **5V** (hoặc nguồn ngoài 5V) | Cấp nguồn cho cuộn hút của Relay |
| **GND** | **GND** | Đất chung |
| **IN1** | **GPIO4** | Tín hiệu điều khiển Quạt (Mặc định: Active-LOW) |

### 🌀 C. Đấu nối nguồn Quạt thông gió với Relay
* **Cực Dương (-)** của nguồn quạt $\rightarrow$ Nối trực tiếp vào **Dây âm** của Quạt.
* **Cực Âm (+)** của nguồn quạt $\rightarrow$ Nối vào chân **com** của Module Relay.
* Chân **No** của Module Relay $\rightarrow$ Nối vào **Dây +** của Quạt.

---

## ⚙️ 3. Cấu hình & Nạp mã nguồn (Firmware Upload)

Để đảm bảo quá trình biên dịch và nạp code không gặp lỗi trên môi trường Windows:

### ⚠️ Bước quan trọng đối với người dùng Windows & OneDrive:
Do đường dẫn chứa khoảng trắng hoặc ký tự đặc biệt của OneDrive (`OneDrive - Hanoi University of Science...`) dễ làm các công cụ biên dịch của Arduino IDE bị lỗi đường dẫn:
1. **Sao chép** toàn bộ thư mục dự án `HVAC_Control` ra một thư mục ngắn gọn ở ổ đĩa chính, ví dụ: **`C:\HVAC_Control`**.
2. Sử dụng Arduino IDE để mở file **`C:\HVAC_Control\HVAC_Control.ino`**.

### 🔧 Cấu hình các thông số trong file `HVAC_Control.ino`:
Mở file code chính và thay đổi các cấu hình mạng tại vùng cài đặt đầu file:

```cpp
// 1. Cấu hình mạng WiFi của bạn
#define WIFI_SSID        "kata"         // Tên WiFi nhà bạn
#define WIFI_PASSWORD    "Katana3936@"     // Mật khẩu WiFi

// 2. Cấu hình máy chủ Cloud MQTT Broker
#define MQTT_SERVER      "smart-hvac.io.vn" // Tên miền máy chủ Cloud của bạn
#define MQTT_PORT        1883               // Cổng MQTT mặc định
#define MQTT_DEVICE_ID   "indoor-01"        // ID thiết bị trên Cloud
#define MQTT_PUB_TOPIC   "sensor/indoor"    // Kênh đẩy dữ liệu cảm biến
#define MQTT_SUB_TOPIC   "remote-control/#" // Kênh nghe lệnh điều khiển từ xa
```

### 💾 Thư viện sử dụng:
Mã nguồn đã được tích hợp sẵn các thư viện quan trọng cục bộ trong thư mục `src/` (bao gồm `PubSubClient` phục vụ MQTT và thư viện `SparkFun_SCD30` phục vụ cảm biến). Bạn chỉ cần cài duy nhất một thư viện sau trên Arduino IDE (nếu chưa có):
* Mở **Tools** $\rightarrow$ **Manage Libraries...**
* Tìm kiếm và cài đặt: **`SparkFun SCD30 Arduino Library`**.

### ⚡ Tiến hành nạp code:
* Chọn đúng mạch **ESP32-S3 Dev Module** (hoặc mạch ESP32-S3 tương thích của bạn).
* Chọn đúng cổng COM và nhấn nút **Upload** (Mũi tên chỉ sang phải).
* Mở **Serial Monitor** với tốc độ **`115200`** baud để giám sát quá trình khởi tạo và truyền nhận dữ liệu.

---

## 🧪 3.5. Chương trình kiểm tra phần cứng riêng biệt (Hardware Test)

Trước khi chạy chương trình chính phức tạp, bạn hãy nạp các chương trình kiểm tra nhỏ này để kiểm tra xem Relay và LED đã được đấu nối đúng cách và hoạt động tốt chưa.

### 🌀 A. Chương trình kiểm tra Quạt tản (Relay Test)
Mở file **`tests/Relay_Test/Relay_Test.ino`** hoặc copy đoạn code này nạp vào ESP32-S3. Chương trình sẽ tự động **BẬT quạt trong 5 giây** và **TẮT quạt trong 5 giây** liên tục:

```cpp
#define PIN_RELAY_FAN    4     // Chân GPIO4 điều khiển Relay quạt tản
#define RELAY_ACTIVE_LOW true  // Mức THẤP (LOW) để Bật, mức CAO (HIGH) để Tắt

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  
  // Trạng thái ban đầu: TẮT quạt
  digitalWrite(PIN_RELAY_FAN, RELAY_ACTIVE_LOW ? HIGH : LOW);
}

void loop() {
  // 1. BẬT QUẠT (Kích Relay)
  Serial.println("Relay -> DANG BAT (ON) - Quat se quay...");
  digitalWrite(PIN_RELAY_FAN, RELAY_ACTIVE_LOW ? LOW : HIGH);
  delay(5000); // Đợi 5 giây
  
  // 2. TẮT QUẠT (Ngắt Relay)
  Serial.println("Relay -> DANG TAT (OFF) - Quat se dung...");
  digitalWrite(PIN_RELAY_FAN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  delay(5000); // Đợi 5 giây
}
```
* **Kỳ vọng**: Bạn sẽ nghe thấy tiếng "cạch" phát ra từ Relay và quạt tản sẽ quay 5 giây rồi dừng 5 giây. Nếu có tiếng cạch của Relay nhưng quạt không quay, hãy kiểm tra lại nguồn ngoài nuôi quạt và cách đấu COM/NO như ở **Mục 2**.

### 💡 B. Chương trình kiểm tra LED nhiệt độ (LED Test)
Mở file **`tests/LED_Test/LED_Test.ino`** để kiểm tra đồng thời cả LED Onboard WS2812 (GPIO48) và 2 LED rời bên ngoài (Xanh dương ở GPIO10, Đỏ ở GPIO11) nhấp nháy chuyển trạng thái làm mát và làm ấm liên tục sau mỗi 2 giây.

---

## 🖥️ 4. Hướng dẫn truy cập Dashboard chi tiết

Dự án hỗ trợ cả việc kết nối lên Cloud toàn cầu và chạy thử nghiệm Dashboard nội bộ bằng Docker.

### 🌐 PHƯƠNG ÁN A: Truy cập Dashboard qua Cloud Server (Khuyên dùng)
Khi thiết bị ESP32-S3 của bạn đã nạp code thành công và kết nối WiFi, dữ liệu sẽ được đẩy trực tiếp lên Cloud Server của bạn.

1. **Địa chỉ truy cập**: Mở trình duyệt Web của bạn (Chrome, Edge, Firefox) và truy cập địa chỉ:
   ```text
   http://smart-hvac.io.vn
   ```
2. **Đồng bộ thiết bị**: 
   * Trên giao diện Dashboard Cloud, chọn ID thiết bị là **`indoor-01`**.
   * Hệ thống sẽ tự động cập nhật các đồ thị trực quan về Nhiệt độ trong phòng, Nhiệt độ giả lập ngoài trời, Độ ẩm không khí, Nồng độ khí CO2 và chỉ số bụi mịn PM2.5.
3. **Điều khiển từ xa (Remote Control)**:
   * Bạn có thể điều khiển trực tiếp trên giao diện Dashboard Cloud (nút Power ON/OFF, chọn chế độ Cool/Heat/Fan, thay đổi nhiệt độ mục tiêu).
   * Lệnh điều khiển dạng JSON sẽ được đẩy xuống broker `smart-hvac.io.vn` trên topic `remote-control/indoor-01`.
   * Mạch ESP32-S3 của bạn sẽ lập tức nhận lệnh thông qua hàm `mqttCallback()` để bật/tắt hệ thống hoặc thay đổi nhiệt độ cài đặt cục bộ.

---

### 💻 PHƯƠNG ÁN B: Vận hành & Truy cập Dashboard cục bộ (Local Docker)
Nếu muốn tự vận hành toàn bộ hệ thống cơ sở dữ liệu và giao diện ngay trên máy tính cá nhân của mình, bạn hãy sử dụng nền tảng Docker đã được thiết lập sẵn.

#### ⚙️ Khởi chạy cụm dịch vụ cục bộ:
1. Đảm bảo máy tính của bạn đã cài đặt và đang chạy ứng dụng **Docker Desktop**.
2. Mở cửa sổ dòng lệnh (PowerShell hoặc Command Prompt) tại thư mục dự án (`C:\HVAC_Control`).
3. Gõ lệnh khởi chạy (Docker sẽ tự động tải các image, biên dịch và chạy ngầm 4 dịch vụ):
   ```bash
   docker-compose up -d --build
   ```
4. Kiểm tra các dịch vụ đang chạy bằng lệnh:
   ```bash
   docker ps
   ```
   Bạn sẽ thấy 4 container hoạt động bình thường:
   * `hvac_control-app-1` (Cổng 3000) - Giao diện Web.
   * `mqtt-subscriber` - Bộ điều phối dữ liệu Python.
   * `timescaledb` - Cơ sở dữ liệu chuỗi thời gian PostgreSQL.
   * `mosquitto` (Cổng 1883) - MQTT Broker cục bộ.

#### 🔗 Truy cập Dashboard Cục bộ:
1. Mở trình duyệt Web và truy cập đường dẫn:
   ```text
   http://localhost:3000
   ```
2. **Thay đổi cấu hình trên ESP32 để chạy Local**:
   Nếu muốn ESP32 gửi dữ liệu về máy tính cá nhân thay vì Cloud, hãy mở file `HVAC_Control.ino` lên và đổi `MQTT_SERVER` thành **Địa chỉ IP máy tính của bạn** trong mạng WiFi nội bộ (Ví dụ: `192.168.1.15`), sau đó nạp lại code.
3. **Xem logs nhận dữ liệu cục bộ**:
   Để kiểm tra xem dữ liệu từ thiết bị gửi về máy tính đã được ghi nhận vào cơ sở dữ liệu hay chưa, chạy lệnh:
   ```bash
   docker logs mqtt-subscriber --tail 50 -f
   ```

---

## 🛠️ 5. Hướng dẫn xử lý sự cố thường gặp (Troubleshooting)

### 🔴 Lỗi 1: Quạt không quay khi Relay đã kích hoạt (Đèn báo trên Relay sáng đỏ)
* **Nguyên nhân**: Cuộn hút của Module Relay cần nguồn điện 5V ổn định để đóng tiếp điểm COM sang NO. Nếu cấp nguồn 3.3V từ ESP32, rơ-le chỉ sáng đèn led báo hiệu chứ không có tiếng cạch đóng khóa cơ học.
* **Khắc phục**: 
  1. Đấu dây **VCC** của Module Relay vào chân **5V** (hoặc chân **5V/VBUS** trên ESP32-S3).
  2. Đảm bảo rút jum kết nối giữa chân `JD-VCC` và `VCC` ra nếu bạn dùng nguồn cấp ngoài cô lập, hoặc cắm chặt jum này nếu muốn dùng chung nguồn 5V của vi điều khiển.

### 🟡 Lỗi 2: Lỗi biên dịch `SparkFun_SCD30_Arduino_Library.h` hoặc `Adafruit_NeoPixel.h`
* **Nguyên nhân**: Thiếu thư viện hoặc đường dẫn chứa khoảng trắng.
* **Khắc phục**: 
  1. Di chuyển toàn bộ thư mục dự án ra ngoài ổ đĩa `C:\HVAC_Control`.
  2. Cài đặt các thư viện thông qua **Manage Libraries** trên Arduino IDE như hướng dẫn ở mục 3.

### 🔵 Lỗi 3: ESP32 báo lỗi kết nối MQTT Broker `rc = -2` liên tục
* **Nguyên nhân**: Mã lỗi `-2` (`MQTT_CONNECT_FAILED`) xảy ra khi thiết bị không thể thiết lập kết nối mạng TCP tới Broker. Thường do máy chủ MQTT bị chặn bởi tường lửa của hệ điều hành, hoặc ESP32 bị mất kết nối WiFi.
* **Khắc phục**:
  1. Đảm bảo tên WiFi (`WIFI_SSID`) và Mật khẩu chính xác 100%.
  2. Kiểm tra xem Cloud Server của bạn có mở cổng `1883` ra Internet công cộng chưa.
  3. Nếu kiểm tra với Broker cục bộ (Local), hãy chạy lệnh PowerShell dưới đây với quyền Administrator trên máy tính của bạn để mở tường lửa Windows cho cổng 1883:
     ```powershell
     New-NetFirewallRule -DisplayName "MQTT Broker Local" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 1883
     ```

---

## 📝 6. Nguyên lý tự động hóa cục bộ (Offline Smart Automation)

Hệ thống sở hữu khả năng tự động hóa cực kỳ thông minh và hoạt động độc lập ngay cả khi mất kết nối mạng (WiFi/MQTT):

* **Thuật toán kiểm soát chất lượng không khí (Quạt thông gió)**: 
  * **Khi ở chế độ Auto (Tự động):** Cứ mỗi 2 giây, vi điều khiển đọc cảm biến SCD30. Nếu **CO2 > 800 ppm** hoặc **Độ ẩm > 60%**, Relay quạt thông gió (GPIO4) sẽ lập tức được kích hoạt. Quạt chỉ tắt khi nồng độ CO2 đã hạ xuống dưới **700 ppm** và Độ ẩm dưới **55%** (Cơ chế khoảng trễ Hysteresis giúp quạt chạy bền bỉ, không bật tắt liên tục gây hỏng động cơ).
  * **Khi ở chế độ Manual (Thủ công):** Hệ thống sẽ bỏ qua các chỉ số cảm biến CO2/Độ ẩm và tuân thủ tuyệt đối lệnh BẬT (ON) / TẮT (OFF) quạt từ giao diện người dùng trên Dashboard.

* **Thuật toán kiểm soát nhiệt độ (HVAC - LED)**:
  * **Khi ở chế độ Auto:** 
    * Khi nhiệt độ môi trường vượt quá `TEMP_SETPOINT + 0.25°C` $\rightarrow$ Kích hoạt làm mát (LED Onboard sáng màu **Xanh Dương**, LED rời GPIO10 bật).
    * Khi nhiệt độ môi trường giảm dưới `TEMP_SETPOINT - 0.25°C` $\rightarrow$ Kích hoạt làm ấm (LED Onboard sáng màu **Đỏ**, LED rời GPIO11 bật).
  * **Khi ở chế độ Manual:** Đèn LED sẽ được giữ cố định ở trạng thái Làm mát (Cooling), Làm ấm (Heating) hoặc Tắt (Off) hoàn toàn dựa vào nút bấm bạn chọn trên Dashboard, bỏ qua việc so sánh nhiệt độ.
  * **Chế độ Standby (Chờ):** Nếu nhận lệnh tắt nguồn hệ thống (`"power":false`) từ Dashboard, hệ thống sẽ ngắt toàn bộ quạt, tắt LED điều khiển nhiệt độ và chuyển LED Onboard sang màu **Cam mờ** dịu nhẹ báo hiệu trạng thái chờ.
