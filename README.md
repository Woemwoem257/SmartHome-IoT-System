# SmartHome IoT System

Một hệ thống nhà thông minh tích hợp giữa phần cứng, gateway IoT, backend Node.js và giao diện dashboard realtime. Dự án này mô phỏng kiến trúc IoT thực tế với các lớp: cảm biến/thiết bị, gateway, cloud MQTT, backend API và web dashboard.

Repository: https://github.com/Woemwoem257/SmartHome-IoT-System

## Tổng quan

SmartHome IoT System gồm các thành phần chính:

- STM32 Controller: xử lý cảm biến, điều khiển relay/MOSFET và logic báo động
- ESP32 Gateway: bridge giữa STM32 và AWS IoT Core qua UART + MQTT
- Node.js Backend: API REST, MQTT bridge, lưu dữ liệu telemetry và điều khiển thiết bị
- Web Dashboard: hiển thị dữ liệu realtime, lịch sử cảm biến và trạng thái thiết bị

Kiến trúc cơ bản:

```text
STM32 Controller / Sensors
        │
        │ UART
        ▼
ESP32 Gateway
        │
        │ MQTT
        ▼
AWS IoT Core
        │
        │ MQTT / REST
        ▼
Node.js Backend
        │
        ├── MongoDB (Telemetry)
        ├── REST API
        └── Dashboard / Real-time Updates
```

## Tính năng chính

- Thu thập nhiệt độ và độ ẩm
- Điều khiển relay và MOSFET
- Hỗ trợ lệnh báo động / alarm clear
- Đồng bộ trạng thái thiết bị qua MQTT ACK
- Hiển thị dữ liệu realtime trên dashboard
- Lưu lịch sử sensor vào MongoDB
- Tích hợp API REST cho việc giám sát và điều khiển

## Cấu trúc repository

```text
SmartHome-IoT-System/
├── README.md
├── LICENSE
├── .gitignore
├── SmartHome_ESP32_Gateway/
│   └── ESP32_Gateway/
│       ├── src/
│       │   ├── main.cpp
│       │   ├── wifi_manager.cpp
│       │   ├── uart_bridge.cpp
│       │   ├── aws_mqtt.cpp
│       │   ├── hmi_manager.cpp
│       │   └── ...
│       ├── include/
│       ├── lib/
│       ├── platformio.ini
│       ├── sdkconfig.*
│       └── CMakeLists.txt
├── SmartHome_STM32_Controller/
│   ├── Core/
│   ├── Drivers/
│   ├── Middlewares/
│   ├── .cproject
│   ├── .mxproject
│   ├── .project
│   ├── Stm32_Controller.ioc
│   ├── STM32F412RETX_FLASH.ld
│   └── STM32F412RETX_RAM.ld
├── SmartHome_Node_Backend/
│   ├── src/
│   │   ├── server.js
│   │   ├── routes/
│   │   ├── controllers/
│   │   ├── services/
│   │   ├── models/
│   │   └── ...
│   ├── public/
│   ├── package.json
│   ├── package-lock.json
│   ├── ARCHITECTURE.md
│   └── .gitignore
├── archive/
└── ...
```

## 1) SmartHome_ESP32_Gateway

Lớp gateway đóng vai trò trung tâm của hệ thống.

Chức năng chính:
- Kết nối Wi‑Fi
- Kết nối AWS IoT Core qua MQTT
- Giao tiếp UART với STM32
- Nhận dữ liệu cảm biến và gửi lên cloud
- Gửi lệnh điều khiển xuống thiết bị

Các file quan trọng:
- `src/main.cpp`: entry point hệ thống
- `src/wifi_manager.cpp`: quản lý Wi‑Fi
- `src/aws_mqtt.cpp`: client MQTT AWS IoT
- `src/uart_bridge.cpp`: bridge UART giữa ESP32 và STM32
- `src/hmi_manager.cpp`: xử lý giao diện hoặc tác vụ HMI

Yêu cầu phát triển:
- PlatformIO hoặc ESP-IDF
- ESP32 board support
- AWS IoT certificate và endpoint

## 2) SmartHome_STM32_Controller

Module điều khiển phần cứng dựa trên STM32F412, sử dụng project STM32CubeMX / CubeIDE.

Các thành phần chính:
- `Core/`: mã chính và mã sinh bởi STM32CubeMX
- `Drivers/`: driver HAL
- `Middlewares/`: middleware và hỗ trợ RTOS
- `Stm32_Controller.ioc`: cấu hình board và peripheral
- `STM32F412RETX_FLASH.ld` / `STM32F412RETX_RAM.ld`: linker script

Yêu cầu phát triển:
- STM32CubeIDE hoặc STM32CubeMX
- ARM GCC toolchain
- ST-Link debugger

## 3) SmartHome_Node_Backend

Backend Node.js cung cấp API và tích hợp cloud.

Các file quan trọng:
- `src/server.js`: khởi động Express server và kết nối MongoDB
- `src/routes/api.js`: định tuyến API
- `src/controllers/apiController.js`: xử lý API điều khiển và truy xuất dữ liệu
- `src/services/mqttService.js`: kết nối AWS IoT và publish/subscribe MQTT
- `src/models/...`: schema dữ liệu (ví dụ: `Telemetry`)

Công nghệ chính:
- Node.js
- Express.js
- MongoDB / Mongoose
- AWS IoT SDK
- dotenv, CORS

## Yêu cầu hệ thống

- Node.js v18+
- npm
- MongoDB (local hoặc MongoDB Atlas)
- ESP32 + cáp UART
- STM32 board + ST-Link
- AWS IoT Core account với cert/key/CA

## Thiết lập nhanh

### 1. Clone repository

```bash
git clone https://github.com/Woemwoem257/SmartHome-IoT-System.git
cd SmartHome-IoT-System
```

### 2. Cài đặt backend

```bash
cd SmartHome_Node_Backend
npm install
```

Tạo file `.env` ở thư mục backend với cấu hình tương tự:

```env
PORT=3000
MONGODB_URI=mongodb://localhost:27017/smarthome
NODE_ENV=development

AWS_IOT_HOST=your-aws-iot-endpoint
AWS_IOT_CERT_PATH=./certs/certificate.pem.crt
AWS_IOT_KEY_PATH=./certs/private.pem.key
AWS_IOT_CA_PATH=./certs/AmazonRootCA1.pem
```

Khởi chạy backend:

```bash
node src/server.js
# hoặc
npx nodemon src/server.js
```

### 3. Thiết lập ESP32 Gateway

```bash
cd SmartHome_ESP32_Gateway/ESP32_Gateway
```

Mở project trong PlatformIO hoặc IDE hỗ trợ ESP32, sau đó:
- cấu hình Wi‑Fi SSID/password
- cấu hình AWS IoT endpoint và certificate
- build + upload firmware

### 4. Thiết lập STM32 Controller

```bash
cd SmartHome_STM32_Controller
```

Mở project bằng:
- STM32CubeIDE
- hoặc STM32CubeMX + ARM toolchain

Build và flash bằng ST-Link.

## API Backend

### Lấy dữ liệu cảm biến mới nhất

```http
GET /api/v1/sensor/latest
```

### Lấy lịch sử cảm biến

```http
GET /api/v1/sensor/history
```

### Gửi lệnh điều khiển thiết bị

```http
POST /api/v1/device/control
Content-Type: application/json
```

Payload ví dụ:

```json
{
  "device_name": "relay1",
  "status": 1
}
```

### Gửi lệnh giải trừ báo động

```http
POST /api/v1/alarm/clear
Content-Type: application/json
```

Payload ví dụ:

```json
{
  "alarm_clear": 1
}
```

## MQTT topics

### Topic nhận dữ liệu cảm biến

```text
gateway/sensor/data
```

### Topic gửi lệnh điều khiển

```text
gateway/control/actuator
```

### Topic ACK trạng thái thiết bị

```text
gateway/control/ack
```

Ví dụ ACK:

```json
{
  "relay1": 1
}
```

## Luồng dữ liệu

### 1. Sensor flow

```text
STM32 -> ESP32 (UART)
ESP32 -> AWS IoT MQTT (gateway/sensor/data)
AWS IoT -> Node.js Backend
Backend -> MongoDB
Backend -> Web Dashboard
```

### 2. Control flow

```text
Web / API
   -> Node.js Backend
   -> MQTT topic gateway/control/actuator
   -> ESP32
   -> UART
   -> STM32
   -> ACK
   -> ESP32
   -> MQTT gateway/control/ack
   -> Backend
   -> Frontend update
```

## Bảo mật và lưu ý quan trọng

Dự án hiện đang ở mức prototype / MVP. Trước khi triển khai lên Internet, cần bổ sung:

- Authentication cho API
- Authorization theo role
- JWT hoặc session
- Helmet
- Rate limiting
- HTTPS
- CORS giới hạn domain
- Validate request
- Logging / audit trail
- Không lưu certificate / key trong repository public

> Không nên public server hoặc mở điều khiển thiết bị ra Internet nếu chưa có xác thực và kiểm soát quyền truy cập.

## Roadmap

### Giai đoạn 1: Hoàn thiện chức năng
- Sửa danh sách thiết bị
- Thêm trạng thái alarm
- Thêm API `alarm/clear`
- Xử lý ACK từ thiết bị
- Chuẩn hóa payload điều khiển

### Giai đoạn 2: Cải thiện dashboard
- Hiển thị gateway online/offline
- Hiển thị STM32 online/offline
- Hiển thị trạng thái thực tế của thiết bị
- Thêm lịch sử cảnh báo
- Tối ưu giao diện responsive

### Giai đoạn 3: Bảo mật
- Login / logout
- JWT hoặc session cookie
- Phân quyền user
- Rate limit
- HTTPS

### Giai đoạn 4: Triển khai public
- VPS / cloud hosting
- Nginx / Caddy
- Domain
- SSL bằng Let's Encrypt
- PM2 hoặc Docker
- Backup dữ liệu

## Tình trạng dự án

Dự án hiện đang ở dạng prototype IoT / lab project, tập trung vào:
- tích hợp phần cứng và firmware
- truyền dữ liệu IoT qua MQTT
- backend API phục vụ giám sát và điều khiển
- dashboard realtime cho demo trong mạng LAN/local

## Đóng góp

1. Fork repository
2. Tạo branch mới
3. Commit thay đổi
4. Push branch
5. Mở Pull Request

## Giấy phép

Dự án này được phân phối theo giấy phép ISC. Xem file `LICENSE` để biết chi tiết.

## Thông tin liên hệ

- GitHub: [Woemwoem257](https://github.com/Woemwoem257)
- Repository: [SmartHome-IoT-System](https://github.com/Woemwoem257/SmartHome-IoT-System)

## Ghi chú

README này được cập nhật để phản ánh đúng cấu trúc thư mục và triển khai hiện tại trong repository, đồng thời nhấn mạnh những điểm cần hoàn thiện trước khi triển khai thực tế.
