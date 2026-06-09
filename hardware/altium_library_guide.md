# Hướng dẫn Cài đặt & Sử dụng Thư viện Altium - Khuê Nguyễn Creator

Chào bạn! Thư viện **Altium-Libraly** của tác giả **nguyenkhue2608 (Khuê Nguyễn Creator)** đã được tải xuống và cài đặt thành công tại đường dẫn chuẩn theo yêu cầu của bạn:
📂 **`C:\Users\Public\Documents\Altium\Altium-Libraly`**

Tài liệu này sẽ hướng dẫn bạn cấu trúc thư viện và cách thiết lập chi tiết để sử dụng trong **Altium Designer** (đặc biệt là phiên bản **AD24** có sẵn trên máy của bạn).

---

## 🗂️ 1. Cấu trúc Thư mục Thư viện đã tải
Thư viện được chia thành các nhóm linh kiện cực kỳ trực quan và khoa học. Mỗi nhóm bao gồm các file nguyên lý (`.SchLib`) và sơ đồ chân mạch in chân thực (`.PcbLib` hỗ trợ 3D):

| Tên Thư mục | Loại Linh kiện Chứa bên trong |
| :--- | :--- |
| **`01_Passive_Components`** | Tụ điện (Capacitor), Điện trở (Resistor), Cuộn cảm (Inductor), Cầu chì (Fuse)... |
| **`02_Active_Components`** | IC, Transistor, Diode, Thạch anh (Crystal), Mosfet... |
| **`03_Electromechanical_Components`**| Domino, Connector cắm, Header, Relay, Công tắc (Switch), Đế pin... |
| **`04_Optoelectronic_Components`** | LED đơn, LED RGB, Màn hình hiển thị (Display), Optocoupler... |
| **`05_RF_Components`** | Ăng-ten (Antenna), Các Module RF (ESP32, Bluetooth, Lora...)... |
| **`06_Sensor`** | Cảm biến các loại và các Module cảm biến tích hợp... |
| **`07_Power_Components`** | IC Nguồn (Regulator), Module Nguồn Buck/Boost... |
| **`08_MCU`** | Vi điều khiển và các Kit phát triển (Arduino, ESP32 Dev Kit...). |
| **`09_Audio`** | Các Module âm thanh, Jack âm thanh, Còi chíp (Buzzer)... |
| 📄 **`AltiumLib_KhueNguyenCreator.LibPkg`** | **File quản lý gói thư viện tổng (Library Package)** |

---

## ⚙️ 2. Hướng dẫn Biên dịch thành Thư viện tích hợp (`.IntLib`)
Altium Designer khuyến khích sử dụng định dạng **Integrated Library (`.IntLib`)** vì nó tự động liên kết Schematic Symbol và PCB Footprint lại thành một file duy nhất, tránh tình trạng mất liên kết chân khi vẽ.

Hãy làm theo các bước dưới đây để biên dịch gói `.LibPkg` thành `.IntLib`:

### Bước 1: Mở gói thư viện trong Altium Designer
1. Khởi động phần mềm **Altium Designer**.
2. Chọn **File** ➡️ **Open** (hoặc nhấn `Ctrl + O`).
3. Tìm đến đường dẫn: `C:\Users\Public\Documents\Altium\Altium-Libraly`.
4. Chọn file **`AltiumLib_KhueNguyenCreator.LibPkg`** và nhấn **Open**.
5. Gói thư viện sẽ xuất hiện trong bảng quản lý dự án **Projects** ở thanh bên trái của bạn.

### Bước 2: Tiến hành Biên dịch (Compile)
1. Trong bảng điều khiển **Projects**, click chuột phải vào tên file **`AltiumLib_KhueNguyenCreator.LibPkg`**.
2. Chọn **Compile Integrated Library AltiumLib_KhueNguyenCreator.LibPkg** từ menu chuột phải.
3. Altium Designer sẽ quét toàn bộ các file `.SchLib` và `.PcbLib` trong các thư mục con và tự động biên dịch.
4. Quá trình biên dịch hoàn tất sẽ tạo ra một file tích hợp duy nhất là **`AltiumLib_KhueNguyenCreator.IntLib`**.
5. File đầu ra này sẽ nằm trong thư mục: 
   `C:\Users\Public\Documents\Altium\Altium-Libraly\Project Outputs for AltiumLib_KhueNguyenCreator\`

---

## 🔌 3. Hướng dẫn Add Thư viện vào Altium Designer để sử dụng
Để bắt đầu lôi linh kiện ra vẽ sơ đồ nguyên lý trong dự án của bạn, hãy thực hiện cài đặt thư viện vào phần mềm theo 1 trong 2 cách sau:

### Cách 1: Cài đặt file tích hợp `.IntLib` (Khuyên dùng)
1. Trong giao diện Altium Designer, mở panel **Components** (thường nằm ở góc phải màn hình, hoặc mở qua menu **View** ➡️ **Panels** ➡️ **Components**).
2. Click vào biểu tượng menu **3 dấu gạch ngang (hamburger menu)** ở góc trên bên phải của panel Components ➡️ Chọn **File-based Libraries Preferences...**.
3. Tại bảng hiện ra, chọn tab **Installed**.
4. Click nút **Install...** bên phải.
5. Tìm đến thư mục chứa file đã biên dịch: 
   `C:\Users\Public\Documents\Altium\Altium-Libraly\Project Outputs for AltiumLib_KhueNguyenCreator\`
6. Chọn file **`AltiumLib_KhueNguyenCreator.IntLib`** và nhấn **Open**.
7. Nhấn **Close** để hoàn tất. Bây giờ bạn đã có thể tìm kiếm và kéo thả mọi linh kiện từ thư viện này trong panel **Components**!

### Cách 2: Tìm kiếm trực tiếp bằng Search Path (Dành cho nhà phát triển muốn chỉnh sửa linh kiện)
Nếu bạn muốn chỉnh sửa trực tiếp các ký hiệu nguyên lý hoặc chân footprint mà không cần biên dịch lại liên tục:
1. Mở bảng **File-based Libraries Preferences...** (như cách 1).
2. Chọn tab **Search Path**.
3. Click chọn **Add...** để thêm đường dẫn thư mục gốc: `C:\Users\Public\Documents\Altium\Altium-Libraly`.
4. Check chọn mục **Include Sub-folders** để Altium tự tìm các thư mục con từ `01` đến `09`.
5. Nhấn **OK** và **Apply**.


---

## 🔍 4. Vị trí các Linh kiện Trọng điểm cho Dự án Smart HVAC
Để hỗ trợ bạn thiết kế bo mạch **Smart HVAC Edge Controller** nhanh nhất, tôi đã quét hệ thống và lập bản đồ định vị các linh kiện cốt lõi của bạn trong bộ thư viện vừa tải:

1. **ESP32-S3-WROOM-1 (Module SoC Điều khiển chính):**
   * **Schematic Symbol (.SchLib):** Nằm trong `08_MCU\KIT.SchLib` (hoặc `08_MCU\MCU.SchLib`).
   * **PCB Footprint (.PcbLib):** Nằm trong `08_MCU\KIT.PcbLib` (hoặc `08_MCU\MCU.PcbLib`).
2. **Relay cơ học Omron 5V (Điều khiển đóng ngắt AC quạt):**
   * **Schematic Symbol (.SchLib):** Nằm trong `03_Electromechanical_Components\Relay.SchLib`.
   * **PCB Footprint (.PcbLib):** Nằm trong `03_Electromechanical_Components\Relay.PcbLib`.
3. **IC Nguồn Buck AP63203 & LDO AP2112 (Khối Hạ áp):**
   * **Schematic Symbol (.SchLib):** Nằm trong `07_Power_Components\Regulator.SchLib` (hoặc `07_Power_Components\Power_Module.SchLib`).
   * **PCB Footprint (.PcbLib):** Nằm trong `07_Power_Components\Regulator.PcbLib` (hoặc `07_Power_Components\Power_Module.PcbLib`).
4. **Cảm biến CO2 & Nhiệt ẩm SCD30:**
   * **Schematic Symbol (.SchLib):** Nằm trong `06_Sensor\Sensor.SchLib` (hoặc `06_Sensor\SensorModule.SchLib`).
   * **PCB Footprint (.PcbLib):** Nằm trong `06_Sensor\Sensor.PcbLib` (hoặc `06_Sensor\SensorModule.PcbLib`).
5. **Optocoupler EL817 (Cách ly quang bảo vệ chân MCU):**
   * **Schematic Symbol (.SchLib):** Nằm trong `04_Optoelectronic_Components\Optoelectronic.SchLib`.
   * **PCB Footprint (.PcbLib):** Nằm trong `04_Optoelectronic_Components\Optoelectronic.PcbLib`.
6. **LED chỉ thị WS2812B-B (RGB đa sắc):**
   * **Schematic Symbol (.SchLib):** Nằm trong `04_Optoelectronic_Components\Led.SchLib`.
   * **PCB Footprint (.PcbLib):** Nằm trong `04_Optoelectronic_Components\Led.PcbLib`.

---


## 💡 Lời khuyên thiết thực khi thiết kế mạch
* **Kiểm tra kỹ chân linh kiện (Footprint):** Mặc dù thư viện của anh Khuê Nguyễn rất chất lượng và được đông đảo cộng đồng điện tử Việt Nam tin dùng, bạn vẫn nên đối chiếu kích thước chân (pitch, pad size, 3D model) với Datasheet của linh kiện thực tế trong dự án **Smart HVAC Edge Controller** trước khi đặt in PCB (ví dụ: chân cắm cổng USB-C, cuộn cảm L1, hay Relay Omron).
* **Cập nhật thư viện:** Nếu tác giả có cập nhật mới trên GitHub, bạn chỉ cần mở PowerShell tại thư mục này và chạy lệnh `git pull` (nếu đã kết nối Git) hoặc tải đè file ZIP mới rồi biên dịch lại file `.IntLib` là xong!

*Chúc bạn thiết kế được bo mạch Smart HVAC Controller hoạt động ổn định và có tính thẩm mỹ cao nhất!*
