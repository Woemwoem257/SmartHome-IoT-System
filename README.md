# SmartHome-IoT-System 🏠

Một hệ thống nhà thông minh hoàn chỉnh sử dụng IoT, kết nối các thiết bị thông qua gateway ESP32 và điều khiển bằng STM32F412, với backend Node.js và AWS IoT.

## 📋 Tổng quan

**SmartHome-IoT-System** là một nền tảng IoT tích hợp cho tự động hóa nhà thông minh, gồm:

- **Firmware nhúng**: C/Assembly cho ESP32 Gateway và STM32 Controller
- **Backend**: Node.js với Express, MongoDB, AWS IoT SDK
- **Kiến trúc**: 3 thành phần chính liên tương tác với nhau

### Thành phần chính:

| Thành phần | Mô tả | Công nghệ |
|---|---|---|
| **ESP32 Gateway** | Gateway IoT trung tâm | C, RTOS, WiFi |
| **STM32 Controller** | Bộ điều khiển thiết bị | C, STM32F412, HAL |
| **Node Backend** | API Backend & Cloud | Node.js, Express, MongoDB, AWS IoT |

---

## 📁 Cấu trúc thư mục

```
SmartHome-IoT-System/
├── SmartHome_ESP32_Gateway/
│   └── ESP32_Gateway/          # Firmware chính cho ESP32
│       ├── main.c
│       ├── wifi_config.h
│       ├── mqtt_handler.c
│       └── ...
├── SmartHome_STM32_Controller/
│   ├── Core/                   # Mã lõi STM32F412
│   │   ├── Src/
│   │   └── Inc/
│   ├── Drivers/                # Driver HAL
│   ├── Middlewares/            # Middleware RTOS
│   ├── .cproject               # Cấu hình dự án Eclipse
│   ├── Stm32_Controller.ioc    # STM32CubeMX config
│   └── STM32F412RETX_*.ld     # Linker scripts
├── SmartHome_Node_Backend/
│   ├── src/                    # Mã nguồn Node.js
│   ├── public/                 # Static files (nếu có)
│   ├── package.json
│   └── .env                    # Configuration
├── archive/                    # Các tệp lưu trữ
└── .gitignore
```

### 📊 Thành phần ngôn ngữ lập trình

- **C**: 98.5% (Firmware nhúng cho ESP32 và STM32)
- **Python**: 0.5% (Utilities/Scripts)
- **HTML**: 0.3% (Frontend web)
- **Assembly**: 0.3% (Optimized routines)
- **C++**: 0.2%
- **Linker Script**: 0.1% (STM32 memory layout)
- **Other**: 0.1%

---

## 🚀 Các thành phần chi tiết

### 1. **SmartHome_ESP32_Gateway** 📡
**Mục đích**: Tạo gateway WiFi để kết nối thiết bị STM32 với cloud

**Chức năng chính**:
- Kết nối WiFi
- MQTT Client cho AWS IoT
- Nhận/gửi dữ liệu từ STM32 Controller
- Xử lý lệnh từ backend

**Công nghệ**:
- Microcontroller: ESP32
- Giao tiếp: UART/SPI với STM32
- Cloud: AWS IoT Core MQTT

**Để phát triển**:
```bash
# Cần Arduino IDE hoặc PlatformIO
# - ESP32 Board Support
# - AWS IoT Arduino Library
# - MQTT Client
```

---

### 2. **SmartHome_STM32_Controller** 🎮
**Mục đích**: Điều khiển trung tâm cho các thiết bị thông minh

**Thông số kỹ thuật**:
- **MCU**: STM32F412RET6 (ARM Cortex-M4, 100MHz)
- **RAM**: 256KB
- **Flash**: 512KB
- **Công cụ**: STM32CubeMX

**Cấu trúc**:
```
SmartHome_STM32_Controller/
├── Core/
│   ├── Src/                    # Implement chính
│   │   └── main.c
│   └── Inc/                    # Header files
├── Drivers/                    # STM32 HAL Drivers
├── Middlewares/                # RTOS (FreeRTOS)
├── .cproject                   # Eclipse IDE config
├── .mxproject                  # CubeMX metadata
└── Stm32_Controller.ioc        # Device config file
```

**Linker Scripts**:
- `STM32F412RETX_FLASH.ld` - Program từ Flash
- `STM32F412RETX_RAM.ld` - Debug từ RAM

**Để build & debug**:
```bash
# Yêu cầu:
# - STM32CubeIDE (hoặc Eclipse + ARM toolchain)
# - ST-Link debugger
# - STM32F412 Board Support Package

# Build:
make clean && make

# Upload:
# Sử dụng ST-Link Utility hoặc IDE
```

---

### 3. **SmartHome_Node_Backend** 🖥️
**Mục đích**: API Server quản lý hệ thống, lưu trữ dữ liệu, xử lý lệnh

**Dependencies**:
```json
{
  "express": "^5.2.1",              // Web Framework
  "mongoose": "^9.9.3",              // MongoDB ODM
  "aws-iot-device-sdk": "^2.2.16",   // AWS IoT Connection
  "cors": "^2.8.6",                  // CORS middleware
  "dotenv": "^17.4.2",               // Environment config
  "nodemon": "^3.1.14"               // Dev auto-reload
}
```

**Cấu trúc**:
```
SmartHome_Node_Backend/
├── src/                        # Source code
│   ├── routes/                 # API endpoints
│   ├── models/                 # MongoDB schemas
│   ├── controllers/            # Business logic
│   ├── middleware/             # Custom middleware
│   ├── config/                 # Configuration
│   └── index.js               # Entry point
├── public/                     # Static assets
├── package.json
├── .env                        # Environment variables
└── .gitignore
```

**Để chạy**:
```bash
# Cài dependencies
npm install

# Development (với nodemon auto-reload)
npm run dev

# Production
npm start

# Test
npm test
```

**Environment Variables** (`.env`):
```env
PORT=3000
MONGODB_URI=mongodb://localhost:27017/smarthome
AWS_IOT_ENDPOINT=<your-aws-iot-endpoint>
AWS_IOT_KEY=<path-to-private-key>
AWS_IOT_CERT=<path-to-certificate>
NODE_ENV=development
```

---

## 🔌 Giao tiếp giữa các thành phần

```
┌─────────────────────────────────────────────────────────┐
│                    AWS IoT Cloud                         │
└─────────────────────────────────────────────────────────┘
                          ↑↓
                        MQTT
                          ↑↓
    ┌──────────────────────────────────────────┐
    │     Node.js Backend (Express API)       │
    │  - API Endpoints                        │
    │  - MongoDB Database                     │
    │  - AWS IoT Bridge                       │
    └──────────────────────────────────────────┘
              ↑↓ HTTP/REST
              ↑↓
    ┌──────────────────────────────────────────┐
    │       ESP32 WiFi Gateway                │
    │  - WiFi Connection                      │
    │  - MQTT Client                          │
    │  - Serial/UART Bridge                   │
    └──────────────────────────────────────────┘
              ↑↓ UART/SPI
              ↑↓
    ┌──────────────────────────────────────────┐
    │     STM32F412 Controller                │
    │  - Device Control Logic                 │
    │  - Sensor Reading                       │
    │  - Actuator Control                     │
    └──────────────────────────────────────────┘
              ↑↓
        IoT Devices/Sensors
```

---

## 🛠️ Công cụ & Yêu cầu phát triển

### Chung:
- Git
- GitHub (quản lý mã nguồn)

### ESP32 Gateway:
- Arduino IDE hoặc PlatformIO
- ESP32 Board Support
- USB Cable (Type-A to Micro-B)

### STM32 Controller:
- **STM32CubeIDE** (tích hợp Eclipse + ARM toolchain)
- **STM32CubeMX** (Code Generator)
- **ST-Link V2/V3** Debugger
- **ARM GNU Toolchain** (arm-none-eabi-gcc)

### Node.js Backend:
- **Node.js** (v16+)
- **npm** hoặc **yarn**
- **MongoDB** (local hoặc Atlas)
- **AWS IoT Account** (Free Tier)
- **VS Code** hoặc **IDE khác**

---

## 📦 Cách cài đặt & sử dụng

### 1. Clone Repository
```bash
git clone https://github.com/Woemwoem257/SmartHome-IoT-System.git
cd SmartHome-IoT-System
```

### 2. Setup ESP32 Gateway
```bash
cd SmartHome_ESP32_Gateway/ESP32_Gateway
# Cấu hình WiFi, AWS IoT credentials
# Upload firmware bằng Arduino IDE
```

### 3. Setup STM32 Controller
```bash
cd SmartHome_STM32_Controller
# Mở trong STM32CubeIDE
# Generate code từ .ioc file
# Build project
# Upload bằng ST-Link
```

### 4. Setup Node.js Backend
```bash
cd SmartHome_Node_Backend
npm install
cp .env.example .env  # Thay đổi config
npm run dev
```

---

## 🔒 Bảo mật

- ⚠️ **Không commit** `.env` file chứa credentials
- ✅ Dùng AWS IoT certificates thay vì plain passwords
- ✅ Validate input trên backend
- ✅ HTTPS cho production
- ✅ Firmware signing cho OTA updates

---

## 📝 Ghi chú phát triển

### Những điểm cần chú ý:

1. **STM32CubeMX Code Generation**:
   - Chỉnh sửa code trong `Core/Src` khi cần
   - Để STM32CubeMX area không chạm vào
   - Tạo file riêng cho custom code

2. **AWS IoT Connection**:
   - Cần cấp chứng chỉ cho ESP32 & Backend
   - Tạo MQTT topics riêng
   - Test connection bằng AWS CLI

3. **MongoDB**:
   - Tạo collections cho devices, users, logs
   - Setup indexes cho performance
   - Backup dữ liệu định kỳ

4. **CORS & API Security**:
   - Cấu hình CORS cho production
   - Validate JWT tokens
   - Rate limiting trên API

---

## 🤝 Đóng góp

1. Fork repository
2. Tạo branch feature (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open Pull Request

---

## 📄 License

Dự án này được cấp phép dưới **ISC License** - xem `LICENSE` file để chi tiết.

---

## 👨‍💻 Tác giả

**Woemwoem257** - Lập trình viên full-stack IoT

---

## 📞 Liên hệ & Hỗ trợ

- 📧 Email: [email của bạn]
- 💬 Issues: GitHub Issues
- 📚 Documentation: [Link wiki hoặc docs]

---

## ✨ Công nghệ nổi bật

- **Embedded**: STM32F412, ESP32 (ARM Cortex-M4)
- **Backend**: Node.js + Express + MongoDB
- **Cloud**: AWS IoT Core
- **Protocol**: MQTT, UART, HTTP/REST
- **Real-time**: MQTT Publish/Subscribe

---

**Last Updated**: September 2026

---

### 🎯 Quick Links

| Tài nguyên | Link |
|---|---|
| [AWS IoT Docs](https://docs.aws.amazon.com/iot/) | AWS IoT Core Documentation |
| [STM32 Reference](https://www.st.com/en/microcontrollers/stm32f4-series.html) | STM32F412 Product Page |
| [ESP32 Docs](https://docs.espressif.com/projects/esp-idf/en/latest/) | Espressif IoT Development Framework |
| [Node.js Docs](https://nodejs.org/docs/) | Node.js Official Documentation |
| [Mongoose](https://mongoosejs.com/) | MongoDB Object Modeling |

---

### ⚡ Tính năng tiên tiến (Coming Soon)

- [ ] Web Dashboard & Mobile App
- [ ] Voice Control Integration
- [ ] ML-based Anomaly Detection
- [ ] OTA Firmware Updates
- [ ] Advanced Scheduling & Automation
- [ ] Multi-location Support
- [ ] Energy Consumption Analytics
