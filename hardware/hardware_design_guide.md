# Cẩm nang Thiết kế Phần cứng - Smart HVAC Edge Controller
*Tài liệu hướng dẫn chi tiết sơ đồ nguyên lý (Schematic) và mạch in (PCB) cho hệ thống điều khiển HVAC thông minh*

Tài liệu này được biên soạn nhằm hướng dẫn bạn thiết kế phần cứng hoàn chỉnh cho **Edge Controller** của dự án HVAC_Control. Tài liệu này cung cấp đầy đủ thông tin kỹ thuật cần thiết để bạn vẽ mạch trên bất kỳ phần mềm EDA nào (**EasyEDA, KiCad, hoặc Altium**).

> [!TIP]
> Nếu bạn sử dụng **Altium Designer**, thư viện linh kiện chuẩn của Khuê Nguyễn Creator đã được cài đặt thành công tại `C:\Users\Public\Documents\Altium\Altium-Libraly`. Bạn hãy tham khảo ngay hướng dẫn cấu hình và bản đồ vị trí linh kiện chi tiết tại: [Hướng dẫn Cài đặt & Sử dụng Thư viện Altium](file:///c:/Users/DELL/OneDrive%20-%20Hanoi%20University%20of%20Science%20and%20Technology/Desktop/HVAC_Control/hardware/altium_library_guide.md).

---

## 🔌 1. Bảng Phân bổ Chân & Kết nối chi tiết (Netlist Pin Allocation)

Dưới đây là sơ đồ đấu nối chi tiết từ Module **ESP32-S3-WROOM-1** tới các khối chức năng trên mạch.

### A. Bảng kết nối tín hiệu chính
| Tên chân ESP32-S3 | Hướng (I/O) | Kết nối ngoại vi | Vai trò chức năng | Lưu ý kỹ thuật |
| :--- | :--- | :--- | :--- | :--- |
| **3V3** | Power Out | VCC của SCD30 & Đường 3.3V | Cấp nguồn vi điều khiển và cảm biến | Cần tụ lọc song song $10\mu\text{F} + 100\text{nF}$ sát chân cấp nguồn module. |
| **GND** | Ground | Ground chung hệ thống | Đất chung | Phủ đồng (GND plane) toàn bộ các lớp trống. |
| **EN (CHIP_PU)** | Input | Nút bấm Reset + Mạch RC | Thiết lập chân Reset cứng | Kéo lên 3.3V qua điện trở $10\text{ k}\Omega$, tụ $1\mu\text{F}$ xuống GND để trễ nguồn. |
| **GPIO0** | Input | Nút bấm BOOT (Strapping) | Vào chế độ nạp Firmware | Kéo lên 3.3V qua điện trở $10\text{ k}\Omega$ để chạy bình thường. |
| **GPIO4** | Output | Ngõ vào Optocoupler EL817 | Điều khiển đóng ngắt Relay | Chân kéo xuống qua điện trở $10\text{ k}\Omega$ chống nhiễu khi khởi động. |
| **GPIO8** | I/O | Chân SDA của cảm biến SCD30 | Đường truyền dữ liệu I2C | Kéo lên 3.3V bằng điện trở $4.7\text{ k}\Omega$. |
| **GPIO9** | I/O | Chân SCL của cảm biến SCD30 | Đường truyền xung nhịp I2C | Kéo lên 3.3V bằng điện trở $4.7\text{ k}\Omega$. |
| **GPIO10** | Output | Anode của LED Cooling | LED xanh báo trạng thái Làm mát | Nối tiếp điện trở hạn dòng $330\,\Omega$. |
| **GPIO11** | Output | Anode của LED Heating | LED đỏ báo trạng thái Làm ấm | Nối tiếp điện trở hạn dòng $330\,\Omega$. |
| **GPIO19** | I/O | Chân D- của cổng USB-C | Đường truyền USB Native D- | Nối tiếp điện trở hạn dòng $22\,\Omega$. |
| **GPIO20** | I/O | Chân D+ của cổng USB-C | Đường truyền USB Native D+ | Nối tiếp điện trở hạn dòng $22\,\Omega$. |
| **GPIO43 (TXD0)** | Output | Chân TX của cổng UART nạp | UART Debug | Đưa ra Pin Header 4-pin dự phòng. |
| **GPIO44 (RXD0)** | Input | Chân RX của cổng UART nạp | UART Debug | Đưa ra Pin Header 4-pin dự phòng. |
| **GPIO48** | Output | Chân DIN của LED WS2812B | LED RGB Onboard đa chỉ thị | Cần nối tiếp điện trở $330\,\Omega$ trước chân DIN. |

### B. Cấu hình chân Strapping cực kỳ quan trọng
ESP32-S3 sử dụng một số chân để xác định chế độ boot khi khởi động (Strapping Pins). Đảm bảo thiết kế phần cứng tuân thủ:
1. **GPIO0:** Quyết định chế độ nạp (nhấn giữ nút BOOT kéo xuống GND khi khởi động $\rightarrow$ Chế độ ROM Serial Bootloader).
2. **GPIO45:** Chọn điện áp nguồn cấp cho bộ nhớ Flash nội bộ. **Mặc định để hở (Float)** hoặc kéo xuống GND bằng trở $10\text{ k}\Omega$. Tuyệt đối không kéo lên 3.3V vì sẽ làm sai lệch nguồn cấp Flash nội, dẫn đến lỗi hỏng Flash vĩnh viễn.
3. **GPIO46:** Chọn chế độ log khởi động. **Mặc định kéo xuống GND** bằng trở $10\text{ k}\Omega$ hoặc để hở.

---

## 🛒 2. Danh mục linh kiện chi tiết (Bill of Materials - BOM)
Các linh kiện dưới đây được chuẩn hóa theo kích thước **0603** (cho điện trở/tụ điện) để vừa dễ hàn tay bằng mỏ hàn thường, vừa tương thích hoàn hảo với robot dán linh kiện (SMT) của JLCPCB.

| STT | Ký hiệu (Designator) | Giá trị / Model | Đóng gói (Package) | Mã linh kiện LCSC (Khuyên dùng) | Vai trò chức năng |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | U1 | ESP32-S3-WROOM-1-N16R8 | SMD-Module | `C2913202` | Bộ vi xử lý SoC điều khiển trung tâm |
| 2 | U2 | AP63203WU-7 | SOT-563 | `C3271135` | IC Buck DC-DC hạ áp đồng bộ 5V / 2A |
| 3 | U3 | AP2112K-3.3TRG1 | SOT-23-5 | `C347262` | IC LDO ổn áp nguồn 3.3V cho ESP32 và cảm biến |
| 4 | U4 | EL817S1(C)(TU)-F | SOP-4 | `C10363` | Optocoupler cách ly quang điều khiển Relay |
| 5 | Q1 | SS8050 | SOT-23 | `C83973` | Transistor NPN kích Relay |
| 6 | Q2 | AO3401A | SOT-23 | `C14894` | P-Channel MOSFET ngắt nguồn tự động |
| 7 | D1 | 1N4148W | SOD-123 | `C81598` | Diode dập xung ngược (Flyback Diode) cho cuộn dây Relay |
| 8 | D2 | SS34 | SMA | `C10511` | Diode Schottky ngăn dòng ngược từ USB-C |
| 9 | D3 | SMBJ24A | SMB | `C88082` | Diode TVS bảo vệ quá áp/chống sét cho ngõ vào DC |
| 10 | RL1 | G5LA-14-CF DC5 | THT-Relay | `C23485` | Relay cơ học Omron 5VDC / 10A đóng cắt AC 220V |
| 11 | CN1 | TYPE-C-31-M-12 | SMD-16Pin | `C165948` | Cổng cắm nguồn và nạp USB-C |
| 12 | CN2 | KF2EDGK-5.08-2P | THT-5.08mm | `C386121` | Domino Terminal cấp nguồn DC ngoài (9V - 24V) |
| 13 | CN3 | KF301-2P (or 3P) | THT-5.00mm | `C19098` | Domino Terminal chịu tải ngõ ra Relay (nối quạt) |
| 14 | D_RGB | WS2812B-B | SMD-5050 | `C114585` | LED RGB đa chỉ thị trạng thái |
| 15 | L1 | 4.7uH (Shielded) | SMD-5.0x5.0mm | `C2684814` | Cuộn cảm lọc nguồn cho IC Buck |
| 16 | R1, R2 | 5.1kΩ | 0603 | `C23186` | Điện trở CC1/CC2 trên cổng USB-C để nhận dạng nguồn PD |
| 17 | R3, R4 | 4.7kΩ | 0603 | `C25907` | Điện trở kéo lên (Pull-up) cho đường I2C SDA/SCL |
| 18 | R_G1 | 330Ω | 0603 | `C17630` | Điện trở hạn dòng chân kích Gate / LED đơn |
| 19 | C_IN | 10uF 50V (MLCC) | 0805 | `C284930` | Tụ lọc ngõ vào 24V cho Buck |
| 20 | C_OUT| 22uF 10V (MLCC) | 0805 | `C34320` | Tụ lọc ngõ ra 5V / 3.3V |

---

## 🎨 3. Sơ đồ mạch nguyên lý của các khối quan trọng (Schematic Details)

Dưới đây là sơ đồ chi tiết dạng mã ASCII vẽ trực quan giúp bạn dễ dàng tái tạo trong trình biên dịch Schematic.

### A. Mạch nguồn Buck hạ áp AP63203 (12V/24V sang 5V)
```text
DC_IN (9V-24V) ───[ FUSE 2A ]───┬───[ TVS SMBJ24A ]───[ C_IN 10uF ]─── VIN (Pin 1)
                                │
                                └───[ R_EN 100k ]─── EN (Pin 2)
                                
AP63203:
  ┌────────────────────────┐
  │ 1. VIN          SW 6   ├───[ L1 4.7uH ]───┬─── 5V Rail 
  │ 2. EN          GND 5   ├─── GND           ├───[ C_OUT 22uF ]─── GND
  │ 3. VOUT        BST 4   ├───[ C_BST 100nF]─┘
  └────────────────────────┘
```
*Lưu ý:* Chân BST (Bootstrap) cần nối một tụ $100\text{nF}$ trực tiếp sang chân SW trước cuộn cảm để tạo điện áp kích lái van MOSFET nội.

### B. Mạch kích hoạt Relay cách ly quang (Optocoupler Isolation)
```text
                  5V_SYS
                    │
                 [Cuộn dây Relay] ───┬───────────────────┐
                    │                │                   │
                    │              [ D1 1N4148 ]      [ Relay Coil - ]
                    │              (Ngược chiều 5V)      │
                    │                │                   │
                    ├────────────────┘                   │
                    │                                    │
                  Collector (C)                          │
                 ┌───────────┐                           │
       ┌─────────┤   Q1      ├───────────────────────────┘
       │         │  SS8050   │
     [R_B 1k]    └───┬───────┘
       │             │ Emitter (E)
       │             ├─── GND
       │             │
   Emitter (E)     [R_PD 10k]
 ┌───────────┐       │
 │   U4      ├───┬───┘
 │  EL817    │   GND
 └─────┬─────┘
   Collector (C) ──── 5V_SYS
   
           U4 Optocoupler Input:
           GPIO4 ───[ R_IN 330Ω ]─── Anode (Pin 1)
                                     Cathode (Pin 2) ─── GND
```
*Phân tích nguyên lý an toàn:* 
1. Khi `GPIO4` xuất mức `LOW` ($0\text{V}$), LED bên trong Optocoupler tắt $\rightarrow$ Phototransistor ngắt $\rightarrow$ Chân Base của transistor `Q1` bị kéo xuống GND hoàn toàn qua `R_PD` ($10\text{ k}\Omega$) $\rightarrow$ Transistor `Q1` đóng $\rightarrow$ Relay ngắt an toàn.
2. Khi `GPIO4` xuất mức `HIGH` ($3.3\text{V}$), LED phát sáng $\rightarrow$ Phototransistor dẫn thông dòng từ $5\text{V}$ qua `R_B` ($1\text{ k}\Omega$) vào cực Base `Q1` $\rightarrow$ `Q1` bão hòa $\rightarrow$ Cuộn dây Relay được nối đất và hút tiếp điểm đóng mạch quạt.

### C. Mạch chọn nguồn tự động (Auto-Power Select)
Đảm bảo khi bạn vừa cắm nguồn công nghiệp 24V vừa cắm cáp USB-C để nạp chương trình debug, dòng điện không bị xông chéo phá hỏng máy tính.
```text
USB_5V  ───────[ Diode SS34 ]───────┬──────> 5V_SYS (Cấp nguồn toàn mạch)
                                    │
              Gate (G)              │ Source (S)
            ┌───────────┐           │
DC_5V ──────┤   Q2      ├───────────┘
(Từ Buck)   │  AO3401   │
            └───┬───────┘
                │ Drain (D)
                ├───[ R_G 10k ]─── GND
                │
                └─── USB_5V (Nối trực tiếp cực Gate vào nguồn USB)
```
*Nguyên lý hoạt động:* 
* Khi cắm **USB-C**: Cực Gate của MOSFET kênh P (`Q2`) ở mức HIGH ($5\text{V}$), dẫn đến $V_{GS} = 0\text{V} \rightarrow$ MOSFET `Q2` ngắt hoàn toàn. Nguồn nuôi mạch in được lấy trực tiếp từ USB thông qua diode Schottky `D2` (`SS34`).
* Khi rút **USB-C**: Cực Gate của `Q2` bị kéo xuống GND thông qua điện trở `R_G` ($10\text{ k}\Omega$). Lúc này $V_{GS} \approx -5\text{V} \rightarrow$ MOSFET `Q2` dẫn hoàn toàn, cấp nguồn thông suốt từ ngõ ra IC Buck `DC_5V` sang `5V_SYS` với độ sụt áp gần như bằng 0 (chống tổn hao điện tốt hơn dùng diode thường).

---

## 📐 4. Thiết lập quy tắc vẽ mạch in (PCB Design Rules)

Khi vẽ mạch in PCB, bạn hãy cấu hình các thông số sau trong mục **Design Rules** của phần mềm vẽ mạch để đảm bảo mạch chạy ổn định nhất.

### A. Độ rộng đường mạch đề xuất (Trace Width)
* **Đường nguồn cấp công suất (24V, 5V, AC 220V tải quạt):** Thiết lập độ rộng tối thiểu **$1.0\text{mm} - 1.5\text{mm}$** ($40\text{mil} - 60\text{mil}$). Nếu dòng điện quạt lớn ($> 5\text{A}$), phủ thêm lớp thiếc (Exposed Copper) trên đường mạch AC để tăng khả năng chịu tải.
* **Đường nguồn 3.3V cấp cho IC/Cảm biến:** Thiết lập độ rộng **$0.5\text{mm} - 0.6\text{mm}$** ($20\text{mil} - 24\text{mil}$).
* **Đường tín hiệu điều khiển thường (GPIO, I2C):** Thiết lập độ rộng **$0.2\text{mm} - 0.25\text{mm}$** ($8\text{mil} - 10\text{mil}$).

### B. Khoảng cách an toàn và Chống nhiễu (Clearance & Creepage)
* **Khoảng cách mạch số (GND-Signal):** Khoảng cách an toàn tối thiểu giữa các đường dây cạnh nhau là **$0.152\text{mm}$** ($6\text{mil}$) đối với công nghệ chế tạo PCB giá rẻ phổ thông.
* **Khoảng cách AC sang DC:** Khoảng cách giữa các đường mạch dẫn điện xoay chiều AC 220V của Relay và mạch DC số 5V/3.3V tuyệt đối **không được nhỏ hơn $2.0\text{mm}$** (Khuyên dùng $3.0\text{mm} - 4.0\text{mm}$).
* **Rãnh khoét cách điện (Isolation Slot):** Vẽ một đường Cutout rỗng (khoét mạch trống không có phíp thủy tinh) chiều rộng **$2.0\text{mm}$** chạy dọc ngay dưới khu vực tiếp điểm COM-NO-NC của Relay để ngăn ngừa tuyệt đối phóng điện bề mặt do ẩm ẩm bụi bẩn.

### C. Đổ đồng GND (Ground Pouring)
* Thực hiện **Đổ đồng GND** (GND Copper Pour) ở cả 2 lớp Top và Bottom để tạo thành một màng chắn nhiễu điện từ.
* Sử dụng **Thermal Relief** cho các pad nối đất (GND pads) để tránh tản nhiệt nhanh khi hàn thủ công bằng tay.

---

## 🗂️ 5. Quy trình Kiểm tra DRC và Xuất tệp sản xuất (Pre-production Checklist)

Trước khi gửi file đi gia công tại nhà máy (ví dụ JLCPCB), hãy thực hiện kiểm tra kiểm soát chất lượng (Quality Control) qua checklist sau để tránh lỗi hỏng mạch phải đặt lại:

- [ ] **1. Kiểm tra Ăng-ten Wi-Fi:** Vùng ăng-ten của module ESP32-S3 có nằm nhô ra ngoài viền PCB hoặc không có bất kỳ đường dây đồng/mặt đồng GND nào phủ trực tiếp bên dưới không?
- [ ] **2. Tụ lọc nhiễu:** Tất cả các tụ gốm $100\text{nF}$ có được đặt cực kỳ gần (khoảng cách $< 2\text{mm}$) các chân cấp nguồn VDD/3V3 của ESP32 và cảm biến SCD30 chưa?
- [ ] **3. Trở kéo I2C:** Đã vẽ thêm 2 điện trở $4.7\text{ k}\Omega$ kéo lên đường 3.3V cho chân SDA (`GPIO8`) và SCL (`GPIO9`) chưa? (SCD30 truyền nhận dạng open-drain bắt buộc phải có trở kéo).
- [ ] **4. Đường viền bo (Keepout/Board Outline):** Đã đóng kín bo bằng một đường khép kín hoàn chỉnh chưa? Các góc mạch nên được bo tròn (Radius $R = 2.0\text{mm} - 3.0\text{mm}$) để tránh sắc nhọn gây nguy hiểm khi cầm nắm.
- [ ] **5. Ký hiệu chữ (Silkscreen):** 
  - Đã ký hiệu rõ cực dương/âm của nguồn DC vào chưa (Ví dụ: ghi chữ `VIN+ 9-24V` và `GND` rõ ràng)?
  - Đã ký hiệu chân đấu tiếp điểm Relay chưa (Ví dụ ghi rõ `RELAY_COM`, `RELAY_NO`) để tránh đấu nhầm chập điện lưới?
- [ ] **6. Định dạng tệp xuất:**
  - File mạch in xuất định dạng **Gerber RS-274X** kèm file khoan lỗ **Excellon Drill File**.
  - Đối với đặt dán linh kiện tự động: Xuất thêm file **BOM** (.csv) định nghĩa rõ LCSC Part Number và file **Pick & Place (Centroid File)** định vị tọa độ X-Y của linh kiện dán.

---

Chúc bạn thiết kế được bo mạch HVAC điều khiển thông minh cực kỳ đẹp mắt, bền bỉ và hoạt động tin cậy tuyệt đối! Nếu bạn gặp bất kỳ vướng mắc gì trong quá trình vẽ mạch hoặc chạy đường dây trên phần mềm EDA, hãy nhắn ngay cho tôi để nhận hỗ trợ hiệu chỉnh kịp thời.
