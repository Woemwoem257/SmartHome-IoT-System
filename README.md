# SmartHome IoT System 🏠

Hệ thống nhà thông minh tích hợp ba lớp chính: gateway ESP32, bộ điều khiển STM32 và backend Node.js. Dự án mô phỏng một kiến trúc IoT thực tế với truyền dữ liệu qua Wi‑Fi/MQTT, giao tiếp UART giữa gateway và controller, và API backend để quản lý thiết bị và truy xuất dữ liệu cảm biến.

## Tóm tắt

- ESP32 Gateway: thu thập dữ liệu, kết nối Wi‑Fi, giao tiếp MQTT với AWS IoT, nối với STM32 qua UART
- STM32 Controller: xử lý thiết bị/relay/sensor và đưa dữ liệu lên gateway
- Node.js Backend: API REST, kết nối MongoDB, bridge AWS IoT, nhận dữ liệu cảm biến và phát lệnh điều khiển

## Kiến trúc hệ thống

```text
┌──────────────────────────────┐
│        IoT Devices/          │
│   Sensors / Actuators        │
└──────────────▲───────────────┘
               │
               │ UART / Serial
               │
┌──────────────┴───────────────┐
│   STM32 Controller           │
│   - sensor/control logic     │
│   - HAL / device drivers     │
└──────────────▲───────────────┘
               │
               │ UART / Serial
               │
┌──────────────┴───────────────┐
│   ESP32 Gateway              │
│   - WiFi client              │
│   - MQTT client              │
│   - UART bridge              │
│   - HMI / command handling   │
└──────────────▲───────────────┘
               │
               │ MQTT / HTTPS
               │
┌──────────────┴───────────────┐
│   Node.js Backend            │
│   - Express API              │
│   - MongoDB                  │
│   - AWS IoT bridge           │
└──────────────────────────────┘
```

## Cấu trúc repository

```text
SmartHome-IoT-System/
├── SmartHome_ESP32_Gateway/
│   └── ESP32_Gateway/
│       ├── src/
│       │   ├── main.cpp
│       │   ├── wifi_manager.cpp
│       │   ├── uart_bridge.cpp
│       │   ├── hmi_manager.cpp
│       │   ├── aws_mqtt.cpp
│       │   └── CMakeLists.txt
│       ├── lib/
│       ├── include/
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
│   │   └── models/
│   ├── public/
│   ├── package.json
│   ├── package-lock.json
│   ├── ARCHITECTURE.md
│   └── .gitignore
├── archive/
├── README.md
├── .gitignore
└── LICENSE
```

## Thành phần chính

### 1) SmartHome_ESP32_Gateway

Đây là layer gateway/bridge trung tâm của hệ thống.

Chức năng chính:
- Kết nối Wi‑Fi
- Kết nối AWS IoT Core qua MQTT
- Giao tiếp UART với STM32 controller
- Nhận dữ liệu sensor và truyền lên cloud
- Gửi lệnh điều khiển xuống thiết bị

Cấu trúc firmware chính:
- `src/main.cpp`: entry point hệ thống
- `src/wifi_manager.cpp`: quản lý Wi‑Fi
- `src/aws_mqtt.cpp`: client MQTT AWS IoT
- `src/uart_bridge.cpp`: bridge UART giữa ESP32 và STM32
- `src/hmi_manager.cpp`: xử lý giao diện hoặc lệnh HMI

Yêu cầu phát triển:
- PlatformIO hoặc ESP-IDF
- ESP32 board support
- AWS IoT certificates và endpoint

### 2) SmartHome_STM32_Controller

Module điều khiển phần cứng dựa trên STM32F412, sử dụng project STM32CubeMX/CubeIDE.

Các thành phần cốt lõi:
- `Core/`: mã chính và mã phát sinh từ STM32CubeMX
- `Drivers/`: driver HAL
- `Middlewares/`: middleware / RTOS support
- `Stm32_Controller.ioc`: cấu hình thiết bị và peripheral
- `STM32F412RETX_FLASH.ld` / `STM32F412RETX_RAM.ld`: linker script

Yêu cầu phát triển:
- STM32CubeIDE hoặc STM32CubeMX
- ARM GCC toolchain
- ST-Link debugger

### 3) SmartHome_Node_Backend

Backend Node.js cung cấp API và tích hợp cloud.

Các file quan trọng:
- `src/server.js`: khởi động Express server và kết nối MongoDB
- `src/routes/api.js`: định tuyến API
- `src/controllers/apiController.js`: xử lý API điều khiển và truy xuất cảm biến
- `src/services/mqttService.js`: kết nối AWS IoT và publish/subscribe MQTT
- `src/models/...`: schema dữ liệu (ví dụ: Telemetry)

Công nghệ chính:
- Node.js
- Express
- MongoDB / Mongoose
- AWS IoT SDK
- CORS, dotenv

## Yêu cầu hệ thống

- Node.js v18+ cho backend
- npm
- MongoDB (local hoặc MongoDB Atlas)
- ESP32 + USB/UART cable
- STM32 board + ST-Link
- AWS IoT Core account với cert/key/CA

## Thiết lập nhanh

### 1. Clone repository

```bash
git clone https://github.com/Woemwoem257/SmartHome-IoT-System.git
cd SmartHome-IoT-System
```

### 2. Thiết lập backend

```bash
cd SmartHome_Node_Backend
npm install
```

Tạo file `.env` ở thư mục backend với cấu hình tương tự:

```env
PORT=3000
MONGODB_URI=mongodb://localhost:27017/smarthome
NODE_ENV=development
```

Khởi chạy server:

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

## Lưu ý bảo mật

- Không commit file `.env` hoặc cert/key của AWS IoT
- Sử dụng thư mục `certs/` hoặc biến môi trường để lưu chứng chỉ
- Không để endpoint, private key, certificate nằm trên repository public
- Validate payload ở backend trước khi lưu dữ liệu vào MongoDB

## Dữ liệu và giao tiếp

Hệ thống sử dụng các luồng dữ liệu sau:
- STM32 -> ESP32: UART/Serial
- ESP32 -> AWS IoT: MQTT
- ESP32 -> Backend: MQTT bridge
- Backend -> MongoDB: lưu telemetry
- Backend -> Device: gửi command qua MQTT topic

## Tình trạng dự án

Dự án đang ở dạng kiến trúc IoT prototype / lab project, tập trung vào:
- tích hợp phần cứng và firmware
- truyền dữ liệu IoT qua MQTT
- backend API phục vụ giám sát và điều khiển

## Đóng góp

1. Fork repository
2. Tạo branch cho tính năng mới
3. Commit thay đổi
4. Push lên branch của bạn
5. Mở pull request

## Giấy phép

Dự án đang sử dụng giấy phép ISC. Xem file `LICENSE` để biết chi tiết.

## Thông tin liên hệ

- GitHub: [Woemwoem257](https://github.com/Woemwoem257)
- Repo: [SmartHome-IoT-System](https://github.com/Woemwoem257/SmartHome-IoT-System)

## Ghi chú

README này được cập nhật để phản ánh đúng cấu trúc thư mục và triển khai hiện tại trong repository, thay vì mô tả chung mang tính ví dụ.
