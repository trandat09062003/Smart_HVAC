# Smart HVAC Control System

![Views](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Ftrandat09062003%2FSmart_HVAC&count_bg=%2379C0FF&title_bg=%23555555&icon=&icon_color=%23E5E5E5&title=views&edge_flat=false)

Hệ thống giám sát và điều khiển vi khí hậu (HVAC) cho phòng làm việc, xây dựng trên **ESP32-S3-N16R8**. Thiết bị đọc chất lượng không khí qua cảm biến **Sensirion SCD30** (CO₂, nhiệt độ, độ ẩm) và **Plantower PMS5003** (PM2.5), điều khiển quạt thông gió qua relay, van servo và hiển thị trạng thái qua LED RGB. Dữ liệu được đồng bộ hai chiều với Dashboard web qua **MQTT**, có thể chạy trên máy cục bộ (Docker) hoặc server cloud.

**Repository:** [github.com/trandat09062003/Smart_HVAC](https://github.com/trandat09062003/Smart_HVAC)

---

## Tổng quan

| Thành phần | Công nghệ |
|---|---|
| Vi điều khiển | ESP32-S3-N16R8 |
| Cảm biến khí | SCD30 (I2C) |
| Cảm biến bụi | PMS5003 (UART 9600 baud) |
| Điều khiển | Relay quạt, Servo van 0°–90°, LED WS2812 |
| Giao tiếp | WiFi, MQTT |
| Backend | Mosquitto + TimescaleDB + Python subscriber |
| Frontend | React + Vite + TypeScript |

Điều khiển tự động trên ESP32 dùng **luật ngưỡng (rule-based)** kèm hysteresis. Khi mất WiFi hoặc MQTT, firmware vẫn chạy logic cục bộ bình thường.

---

## Kiến trúc hệ thống

```
┌─────────────┐     publish      ┌──────────────┐     subscribe     ┌─────────────────┐
│  ESP32-S3   │ ───────────────► │   Mosquitto  │ ◄──────────────── │ mqtt-subscriber │
│ SCD30/PMS   │  sensor/indoor   │  (port 1883) │    sensor/#       │  + TimescaleDB  │
│ Relay/Servo │ ◄─────────────── │              │ ────────────────► │  REST API :5000 │
└─────────────┘ remote-control/# └──────────────┘                   └────────┬────────┘
                                                                              │
                                                                     ┌────────▼────────┐
                                                                     │  Dashboard Web  │
                                                                     │  (port 3000)    │
                                                                     └─────────────────┘
```

**Hai cách triển khai:**

1. **Cloud** — ESP32 trỏ `MQTT_SERVER` tới `smart-hvac.io.vn`, truy cập Dashboard tại [http://smart-hvac.io.vn](http://smart-hvac.io.vn).
2. **Local Docker** — Chạy `docker-compose up` trên máy tính, ESP32 trỏ `MQTT_SERVER` về IP LAN của máy đó, mở Dashboard tại `http://localhost:3000`.

Nếu cổng 3000/1883 đã bị chiếm, dùng `docker-compose.alt.yml` (Dashboard cổng **3005**, MQTT cổng **1885**).

---

## Cấu trúc thư mục

```
HVAC_Control/
├── HVAC_Control.ino          # Firmware chính
├── src/                      # Dashboard React + thư viện nhúng (SCD30, PubSubClient)
├── mqtt-subscriber/          # Nhận MQTT, ghi DB, phục vụ REST API
├── mosquitto/                # Cấu hình MQTT broker
├── tests/                    # Sketch kiểm tra phần cứng từng module
├── hardware/                 # Tài liệu thiết kế PCB (EasyEDA / KiCad / Altium)
├── libraries/                # Thư viện Arduino tham khảo (SCD30, TinyGSM)
├── docker-compose.yml        # Triển khai local (cổng 3000 / 1883)
└── docker-compose.alt.yml    # Triển khai local (cổng 3005 / 1885)
```

---

## Sơ đồ đấu nối phần cứng

> Ngắt nguồn ESP32 trước khi cắm dây.

### SCD30 (I2C)

| Chân SCD30 | ESP32-S3 | Ghi chú |
|---|---|---|
| VIN | 3V3 | |
| GND | GND | |
| SDA | GPIO8 | Kéo lên 4.7 kΩ |
| SCL | GPIO9 | Kéo lên 4.7 kΩ |

### PMS5003 (UART)

| Chân PMS5003 | ESP32-S3 | Ghi chú |
|---|---|---|
| VCC | 5V | |
| GND | GND | |
| TX | GPIO17 (RX2) | TX cảm biến → RX ESP |
| RX | GPIO16 (TX2) | RX cảm biến ← TX ESP |

### Relay quạt thông gió

| Chân Relay | ESP32-S3 / Nguồn |
|---|---|
| VCC | 5V |
| GND | GND |
| IN1 | GPIO4 |

Relay cấu hình **Active-HIGH** (`RELAY_ACTIVE_LOW = false`): HIGH = bật quạt.

Đấu nguồn quạt qua tiếp điểm COM/NO của relay (nguồn quạt tách riêng, không lấy từ 3.3V ESP).

### Servo van thông gió

| Chân Servo | ESP32-S3 |
|---|---|
| Signal | GPIO15 |
| VCC | 5V |
| GND | GND |

Góc quay 0° (đóng) đến 90° (mở tối đa), điều khiển bằng PWM 50 Hz.

### LED trạng thái

| Thành phần | GPIO | Ý nghĩa |
|---|---|---|
| WS2812 onboard | GPIO48 | Xanh dương = làm mát, Đỏ = làm ấm, Xanh lá = vùng comfort, Cam = standby |
| LED rời (tùy chọn) | GPIO10 / GPIO11 | Cool / Heat khi `USE_ONBOARD_RGB = false` |

Chi tiết thiết kế mạch in: xem [`hardware/hardware_design_guide.md`](hardware/hardware_design_guide.md).

---

## Cấu hình firmware

Mở `HVAC_Control.ino`, sửa khối cấu hình đầu file:

```cpp
#define WIFI_SSID        "TEN_WIFI_CUA_BAN"
#define WIFI_PASSWORD    "MAT_KHAU_WIFI"

#define MQTT_SERVER      "192.168.1.71"   // IP máy chạy Docker, hoặc "smart-hvac.io.vn"
#define MQTT_PORT        1883
#define MQTT_DEVICE_ID   "indoor-01"
#define MQTT_PUB_TOPIC   "sensor/indoor"
#define MQTT_SUB_TOPIC   "remote-control/#"
```

### Nạp code

1. Sao chép project ra đường dẫn ngắn (ví dụ `C:\HVAC_Control`) nếu đang nằm trong OneDrive — tránh lỗi biên dịch Arduino do khoảng trắng trong path.
2. Mở `HVAC_Control.ino` bằng Arduino IDE.
3. Chọn board **ESP32-S3 Dev Module**, cổng COM tương ứng.
4. Thư viện SCD30 và PubSubClient đã nhúng trong `src/`; không cần cài thêm qua Library Manager.
5. Upload, mở Serial Monitor **115200 baud**.

---

## Logic điều khiển tự động

Chu kỳ đọc cảm biến và tính toán: **2 giây**. Chỉ chạy khi `power = true`.

### Nhiệt độ (LED chỉ thị HVAC)

Setpoint mặc định **25°C**, hysteresis **±0.5°C** (biên kích hoạt ±0.25°C).

| Chế độ | Điều kiện | Hành động |
|---|---|---|
| `auto` | T > setpoint + 0.25°C | LED làm mát (xanh dương) |
| `auto` | T < setpoint − 0.25°C | LED làm ấm (đỏ) |
| `auto` | Trong vùng comfort | LED xanh lá |
| `cool` / `heat` / `off` | Manual | Giữ trạng thái theo lệnh Dashboard |

### Quạt thông gió (Relay)

| Chế độ | Bật quạt | Tắt quạt |
|---|---|---|
| `auto` | CO₂ > 800 ppm **hoặc** PM2.5 > 50 µg/m³ | CO₂ < 700 ppm **và** PM2.5 < 40 µg/m³ |
| `on` / `off` | Theo lệnh thủ công | Theo lệnh thủ công |

### Van servo

Mở tỷ lệ theo chỉ số xấu nhất giữa CO₂ và PM2.5:

- CO₂: 600–1200 ppm → 0–100%
- PM2.5: 25–100 µg/m³ → 0–100%
- Góc van = `max(co2Ratio, pm25Ratio) × 90°`

### Standby

Khi nhận `"power": false`: tắt quạt, đóng van (0°), tắt LED nhiệt, LED onboard chuyển cam mờ.

---

## Giao thức MQTT

### Telemetry (ESP32 → Broker)

Topic: `sensor/indoor`

```json
{
  "device_id": "indoor-01",
  "temperature": 25.20,
  "outdoor_temperature": 28.40,
  "humidity": 55.00,
  "co2": 650,
  "dust": 12.50,
  "valve_angle": 30
}
```

`outdoor_temperature` hiện được ước lượng từ nhiệt độ trong phòng (+3.2°C) khi chưa có cảm biến ngoài trời riêng.

### Điều khiển từ xa (Dashboard → ESP32)

Topic: `remote-control/indoor-01`

```json
{
  "device_id": "indoor-01",
  "power": true,
  "temp": 25.0,
  "operationMode": "auto",
  "fanPower": "auto"
}
```

| Trường | Giá trị hợp lệ |
|---|---|
| `operationMode` | `auto`, `cool`, `heat`, `off` |
| `fanPower` | `auto`, `on`, `off` |

---

## Chạy Dashboard local (Docker)

**Yêu cầu:** Docker Desktop đang chạy.

```bash
# Cổng mặc định: Dashboard 3000, MQTT 1883
docker-compose up -d --build

# Hoặc cổng thay thế: Dashboard 3005, MQTT 1885
docker-compose -f docker-compose.alt.yml up -d --build
```

| Container | Vai trò |
|---|---|
| `app` | Giao diện web React |
| `mosquitto` | MQTT broker |
| `timescaledb` | Lưu dữ liệu chuỗi thời gian |
| `mqtt-subscriber` | Nhận MQTT, REST API cổng 5000 |

Sau khi khởi động:

- Dashboard: `http://localhost:3000` (hoặc `:3005` nếu dùng file alt)
- Kiểm tra log: `docker logs mqtt-subscriber --tail 50 -f`
- Cấu hình ESP32: `MQTT_SERVER` = IP máy tính trong mạng LAN

Mở tường lửa Windows cho cổng MQTT (PowerShell Admin):

```powershell
New-NetFirewallRule -DisplayName "MQTT Broker Local" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 1883
```

---

## Kiểm tra phần cứng

Chạy lần lượt các sketch trong `tests/` trước khi nạp firmware chính:

| Sketch | Mục đích |
|---|---|
| `tests/test_scd30/test_scd30.ino` | Đọc CO₂, nhiệt độ, độ ẩm từ SCD30 |
| `tests/ReadHex_SCD30/ReadHex_SCD30.ino` | Debug raw I2C SCD30 |
| `tests/Relay_Test/Relay_Test.ino` | Bật/tắt relay GPIO4 mỗi 2 giây |
| `tests/Servo_Test/Servo_Test.ino` | Quét góc servo van GPIO15 |
| `tests/Full_Hardware_Test/Full_Hardware_Test.ino` | Kiểm tra tổng hợp toàn bộ ngoại vi |

---

## Xử lý sự cố

**Relay kêu nhưng quạt không quay**
- Cấp **5V** cho VCC module relay, không dùng 3.3V.
- Kiểm tra đấu nối COM/NO và nguồn quạt riêng.

**Không đọc được SCD30**
- Kiểm tra SDA/SCL (GPIO8/9), nguồn 3.3V.
- Chạy `test_scd30.ino`, xem Serial Monitor có báo địa chỉ I2C `0x61` không.

**PM2.5 luôn bằng 0**
- Đảo TX/RX nếu cần (GPIO16 ↔ GPIO17).
- PMS5003 cần vài giây khởi động; đợi 30s sau khi cấp nguồn.

**MQTT lỗi `rc = -2`**
- Sai SSID/mật khẩu WiFi, hoặc ESP32 không ping được broker.
- Với local: kiểm tra IP máy, tường lửa cổng 1883, container `mosquitto` đang chạy.

**Lỗi biên dịch do đường dẫn**
- Copy project ra `C:\HVAC_Control` rồi mở lại trong Arduino IDE.

---

## Công nghệ sử dụng

- **Firmware:** Arduino (C++), ESP32 Arduino Core
- **Dashboard:** React 19, Vite, Tailwind CSS, Recharts
- **Backend:** Python 3, Paho MQTT, PostgreSQL + TimescaleDB
- **Broker:** Eclipse Mosquitto

---

## Tác giả

Trần Đạt — [trandat09062003](https://github.com/trandat09062003)

Dự án phục vụ mục đích nghiên cứu và triển khai hệ thống giám sát chất lượng không khí trong nhà kết hợp điều khiển thông gió cơ bản.
