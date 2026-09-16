# SmartHome Node Backend - Architecture Documentation

**Last Updated**: September 16, 2026
**Status**: Production Ready v1.0

---

## 📋 Table of Contents

1. [System Architecture](#system-architecture)
2. [Event-Driven Architecture Diagram](#event-driven-architecture-diagram)
3. [API Endpoints](#api-endpoints)
4. [Database Schema](#database-schema)
5. [MQTT Topics & Payloads](#mqtt-topics--payloads)
6. [Data Flow Diagrams](#data-flow-diagrams)
7. [Error Handling Strategy](#error-handling-strategy)
8. [Performance Considerations](#performance-considerations)

---

## 🏗️ System Architecture

### Architecture Type: **Event-Driven Microservices**

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Application Layer                            │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────────┐ │
│  │  Express.js    │  │  REST API Routes │  │  Static Web Assets  │ │
│  │  HTTP Server   │  │  /api/v1/*       │  │  (Dashboard UI)     │ │
│  └────────┬────────┘  └────────┬─────────┘  └──────────┬──────────┘ │
└───────────┼──────────────────────┼────────────────────────┼──────────┘
            │                      │                        │
            ↓                      ↓                        ↓
┌─────────────────────────────────────────────────────────────────────┐
│                        Business Logic Layer                          │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │ Controllers (apiController.js)                                   ││
│  │ - getLatestSensorData()                                          ││
│  │ - controlDevice()                                                ││
│  └─────────────────────────────────────────────────────────────────┘│
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │ Services Layer                                                   ││
│  │ ├─ mqttService.js (AWS IoT Core connection)                     ││
│  │ ├─ database.js (MongoDB connection pooling)                     ││
│  │ └─ validation.js (Input sanitization)                           ││
│  └─────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────┘
            │                      │
            ↓                      ↓
┌──────────────────────────────────────────────────────────────────────┐
│                      Integration Layer                               │
│  ┌──────────────────────┐  ┌─────────────────────────────────────────┐
│  │ AWS IoT Core MQTT    │  │ MongoDB Atlas                           │
│  │ Broker               │  │ Database                                │
│  │ (Persistent Queue)   │  │ (Persistent Storage)                    │
│  └──────────────────────┘  └─────────────────────────────────────────┘
│         ↑↓                          ↑↓                                │
│      Topics:                   Collections:                          │
│  - gateway/sensor/data      - Telemetry                              │
│  - gateway/control/actuator - DeviceStatus                           │
│  - gateway/control/ack      - Users                                  │
└──────────────────────────────────────────────────────────────────────┘
            │                           │
            ↓                           ↓
┌──────────────────────────────────────────────────────────────────────┐
│                      Device Layer                                    │
│  ┌──────────────────────┐  ┌─────────────────────────────────────────┐
│  │ ESP32 Gateway        │  │ STM32F412 Controller                    │
│  │ (WiFi + MQTT Client) │  │ (UART/SPI Bridge)                       │
│  └──────────────────────┘  └─────────────────────────────────────────┘
│         ↑↓                          ↑↓                                │
│      Firmware:               Firmware:                               │
│  - Subscribe to Topics   - Control Logic                             │
│  - Publish Sensor Data   - Sensor Reading                            │
│  - Handle Commands       - Actuator Control                          │
└──────────────────────────────────────────────────────────────────────┘
            │                           │
            ↓                           ↓
┌──────────────────────────────────────────────────────────────────────┐
│                   Physical IoT Devices                               │
│  Sensors (Temperature, Humidity, Motion)                             │
│  Actuators (Relays, Motors, Lights)                                  │
└────────────────────────────────────��─────────────────────────────────┘
```

---

## 🔄 Event-Driven Architecture Diagram

### Message Flow Pattern

```
┌─────────────────────────────────────────────────────────────────────┐
│                      EVENT SOURCES                                  │
├─────────────────────────────────────────────────────────────────────┤
│  1. Device Sensors              2. User Actions (Web UI)            │
│     └─ Publish via ESP32            └─ REST API Calls              │
└─────────────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────────────┐
│                  AWS IoT Core MQTT Broker                           │
│  ┌─────────────────────────────────────────────────────────────────┐
│  │ Topics:                                                         │
│  │ • gateway/sensor/data        (Telemetry)                       │
│  │ • gateway/status/#           (Device Status)                   │
│  │ • gateway/control/actuator   (Command Input)                   │
│  │ • gateway/control/ack        (Command Ack)                     │
│  └─────────────────────────────────────────────────────────────────┘
└─────────────────────────────────────────────────────────────────────┘
                  ↓                           ↓
        ┌─────────────────┐        ┌──────────────────┐
        │ MQTT Subscriber │        │ MQTT Publisher   │
        │ (Ingress)       │        │ (Egress)         │
        └────────┬────────┘        └────────┬─────────┘
                 ↓                          ↑
        ┌──────────────────────────────────────────┐
        │   mqttService.js                         │
        │   Event Handlers:                        │
        │   - device.on('message', ...)            │
        │   - device.on('connect', ...)            │
        │   - device.on('error', ...)              │
        └────────┬─────────────────────────────────┘
                 ↓
        ┌──────────────────────────────────────────┐
        │   Controllers & Business Logic            │
        │   - Parse & Validate Payload              │
        │   - Enrich with Metadata                  │
        │   - Trigger Side Effects                  │
        └────────┬─────────────────────────────────┘
                 ↓
        ┌──────────────────────────────────────────┐
        │   MongoDB Persistence Layer               │
        │   - Write Telemetry                       │
        │   - Update DeviceStatus                   │
        │   - Log Events                            │
        └──────────────────────────────────────────┘
                 ↓
        ┌──────────────────────────────────────────┐
        │   WebSocket Events to Frontend Dashboard  │
        │   - Real-time Notifications               │
        │   - Live Sensor Updates                   │
        └──────────────────────────────────────────┘
```

---

## 📡 API Endpoints

### Base URL
```
http://localhost:3000/api/v1
```

### 1. Get Latest Sensor Data

**Endpoint**: `GET /sensor/latest`

**Request**:
```http
GET /api/v1/sensor/latest HTTP/1.1
Host: localhost:3000
```

**Response** (200 OK):
```json
{
  "success": true,
  "data": {
    "temperature": 28.5,
    "humidity": 65.2,
    "timestamp": "2026-09-16T10:30:00.000Z"
  }
}
```

**Error Response** (404 Not Found):
```json
{
  "success": false,
  "message": "Chưa có dữ liệu cảm biến"
}
```

**Error Response** (500 Internal Server Error):
```json
{
  "success": false,
  "message": "Lỗi máy chủ nội bộ"
}
```

**Implementation**:
- Uses `Telemetry.findOne().sort({ timestamp: -1 })`
- Retrieves most recent sensor reading
- O(1) lookup with proper indexing on `timestamp`

---

### 2. Control Device (Command)

**Endpoint**: `POST /device/control`

**Request**:
```http
POST /api/v1/device/control HTTP/1.1
Host: localhost:3000
Content-Type: application/json

{
  "device_name": "relay_1",
  "status": 1
}
```

**Request Parameters**:
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| device_name | string | Yes | Device identifier (e.g., "relay_1", "pump_1") |
| status | number | Yes | Command (0 = OFF, 1 = ON) |

**Response** (200 OK):
```json
{
  "success": true,
  "message": "Đã gửi lệnh điều khiển thành công"
}
```

**Error Response** (400 Bad Request):
```json
{
  "success": false,
  "message": "Thiếu tham số điều khiển"
}
```

**Payload Format Sent to MQTT**:
```javascript
{
  "relay_1": 1  // device_name: status
}
```

**MQTT Topic**: `gateway/control/actuator`

**MQTT QoS**: 1 (Ensure At-Least-Once delivery)

---

## 🗄️ Database Schema

### Collection: `Telemetry`

**Purpose**: Store sensor readings time-series data

```javascript
{
  _id: ObjectId,
  temperature: Number,        // °C
  humidity: Number,           // %
  device_id: String,          // Sensor identifier
  location: String,           // "living_room", "bedroom", "kitchen"
  timestamp: Date,            // ISO 8601 format
  __v: Number                 // Mongoose version key
}
```

**Indexes**:
```javascript
// Single field indexes
db.telemetries.createIndex({ timestamp: -1 })
db.telemetries.createIndex({ device_id: 1, timestamp: -1 })

// TTL index (auto-delete after 30 days)
db.telemetries.createIndex(
  { timestamp: 1 },
  { expireAfterSeconds: 2592000 }
)
```

**Sample Document**:
```json
{
  "_id": ObjectId("66e77c1a2f3b8c9d0e1f2g3h"),
  "temperature": 28.5,
  "humidity": 65.2,
  "device_id": "sensor_1",
  "location": "living_room",
  "timestamp": ISODate("2026-09-16T10:30:00.000Z")
}
```

---

### Collection: `DeviceStatus` (Recommended)

**Purpose**: Track real-time device state

```javascript
{
  _id: ObjectId,
  device_id: String,          // "relay_1", "pump_1", etc.
  status: Number,             // 0 (OFF) or 1 (ON)
  last_command: Date,         // When control was last issued
  last_ack: Date,             // When device acknowledged command
  is_synced: Boolean,          // Matches real device state
  battery_level: Number,      // For wireless devices (optional)
  signal_strength: Number,    // RSSI (optional)
  location: String,           // Physical location
  updated_at: Date
}
```

---

## 📨 MQTT Topics & Payloads

### 1. Telemetry Topic (Device → Backend)

**Topic**: `gateway/sensor/data`

**QoS**: 0 (Best Effort - acceptable for sensor data)

**Payload Format**:
```json
{
  "temperature": 28.5,
  "humidity": 65.2,
  "device_id": "sensor_1",
  "timestamp": "2026-09-16T10:30:00Z"
}
```

**Publishing Source**: ESP32 Gateway (subscribes from STM32 Controller via UART)

**Subscriber**: Node Backend (mqttService.js)

---

### 2. Control Command Topic (Backend → Device)

**Topic**: `gateway/control/actuator`

**QoS**: 1 (Ensure At-Least-Once delivery)

**Payload Format**:
```json
{
  "relay_1": 1,
  "pump_1": 0,
  "fan_1": 1
}
```

**Publishing Source**: Node Backend (via REST API request)

**Subscriber**: ESP32 Gateway → forwards to STM32 via UART

---

### 3. Acknowledgement Topic (Device → Backend)

**Topic**: `gateway/control/ack`

**QoS**: 1

**Payload Format**:
```json
{
  "device_name": "relay_1",
  "status": 1,
  "is_executed": true,
  "timestamp": "2026-09-16T10:30:00Z",
  "error_code": null
}
```

**Publishing Source**: ESP32 Gateway (after receiving confirmation from STM32)

**Subscriber**: Node Backend (for state synchronization)

---

### 4. Status Topic (Device → Backend)

**Topic Pattern**: `gateway/status/#` (Wildcard subscription)

**Examples**:
- `gateway/status/esp32/connection`
- `gateway/status/stm32/heartbeat`
- `gateway/status/gateway/uptime`

**Payload Format**:
```json
{
  "device": "esp32",
  "status": "connected",
  "uptime_seconds": 3600,
  "free_memory": 45000,
  "rssi": -65,
  "timestamp": "2026-09-16T10:30:00Z"
}
```

---

## 📊 Data Flow Diagrams

### Flow 1: Sensor Reading → Database → Dashboard

```
STM32 Sensor        ESP32 Gateway          AWS IoT            Node Backend
    │                    │                   │                      │
    │─ UART/SPI ─────────→│                   │                      │
    │  (raw data)         │                   │                      │
    │                     │─ MQTT Pub ───────→│                      │
    │                     │  (formatted)      │─ Message Handler ──→│
    │                     │                   │                      │
    │                     │                   │  ┌──────────────────┤
    │                     │                   │  │ Parse JSON       │
    │                     │                   │  │ Validate Schema  │
    │                     │                   │  │ Create Telemetry │
    │                     │                   │  └──────────────────┤
    │                     │                   │                      │
    │                     │                   │  MongoDB
    │                     │                   │      │
    │                     │                   │      ↓
    │                     │                   │  db.telemetries.insertOne()
    │                     │                   │      │
    │                     │                   │      ↓
    │                     │                   │  ┌──────────────────┐
    │                     │                   │  │ Broadcasting to  │
    │                     │                   │  │ WebSocket Clients│
    │                     │                   │  └──────────────────┘
```

**Timeline**:
1. **T+0ms**: Sensor measurement captured
2. **T+50ms**: STM32 formats and sends via UART
3. **T+100ms**: ESP32 publishes to MQTT
4. **T+200ms**: Backend receives and parses
5. **T+250ms**: MongoDB write completes
6. **T+300ms**: WebSocket broadcast to dashboards

---

### Flow 2: User Command → Device Execution

```
Web Dashboard      Node Backend           AWS IoT            ESP32 Gateway
      │                  │                   │                      │
      │─ REST POST ─────→│                   │                      │
      │  (control)       │                   │                      │
      │                  │─ MQTT Pub ───────→│─ Forward ──────────→│
      │                  │  (command)        │   (via UART)        │
      │                  │                   │                      │
      │                  │                   │  ┌──────────────────┐
      │                  │                   │  │ Route to STM32   │
      │                  │                   │  │ via UART/SPI     │
      │                  │                   │  └──────────────────┘
      │                  │                   │                      │
      │                  │  ← MQTT Ack ─────│←─ (state changed)    │
      │                  │  (confirmation)  │                      │
      │                  │                   │                      │
      │  ← WebSocket ───│                   │                      │
      │  (status)       │                   │                      │
      │                  │                   │                      │
      ✓ GUI Updates      ✓ DB Synced        ✓ Device Executed    ✓ Confirmed
```

**Timeline**:
1. **T+0ms**: User clicks toggle in Web UI
2. **T+50ms**: REST API `/device/control` invoked
3. **T+100ms**: MQTT publish to `gateway/control/actuator`
4. **T+200ms**: ESP32 receives and sends to STM32
5. **T+300ms**: STM32 executes (relay switches)
6. **T+400ms**: ESP32 publishes acknowledgement
7. **T+450ms**: Backend updates DeviceStatus
8. **T+500ms**: WebSocket broadcasts to dashboard
9. **T+550ms**: Dashboard UI reflects new state

---

## ⚠️ Error Handling Strategy

### MQTT Connection Errors

```javascript
// Handle network failures with exponential backoff
let reconnectAttempts = 0;
const MAX_RECONNECT_ATTEMPTS = 10;
const BASE_BACKOFF_MS = 1000;

device.on('error', (error) => {
    console.error(`[MQTT] Connection error: ${error.message}`);
    
    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        const backoffMs = Math.min(
            BASE_BACKOFF_MS * Math.pow(2, reconnectAttempts),
            30000  // Max 30 seconds
        );
        
        reconnectAttempts++;
        console.log(`[MQTT] Retrying in ${backoffMs}ms...`);
        
        setTimeout(() => {
            device.connect();
        }, backoffMs);
    } else {
        console.error('[MQTT] Max reconnection attempts reached. Manual intervention required.');
        // Alert monitoring system
        triggerAlert('MQTT_CONNECTION_CRITICAL');
    }
});
```

### Database Errors

```javascript
// Wrap all DB operations with timeout
const executeWithTimeout = (promise, timeoutMs = 5000) => {
    return Promise.race([
        promise,
        new Promise((_, reject) => 
            setTimeout(() => reject(new Error('DB Query Timeout')), timeoutMs)
        )
    ]);
};

// Usage
try {
    await executeWithTimeout(
        Telemetry.create({ temperature, humidity }),
        5000
    );
} catch (error) {
    if (error.message === 'DB Query Timeout') {
        console.error('[DB] Query exceeded 5 second limit');
        // Fall back to offline queue
        offlineQueue.push({ temperature, humidity, timestamp: Date.now() });
    } else {
        console.error('[DB] Insert failed:', error.message);
    }
}
```

### Input Validation Errors

```javascript
// Centralized error responses
const handleValidationError = (res, field, reason) => {
    res.status(400).json({
        success: false,
        error: 'VALIDATION_ERROR',
        field: field,
        message: reason,
        timestamp: new Date().toISOString()
    });
};

const controlDevice = (req, res) => {
    const { device_name, status } = req.body;
    
    // Type validation
    if (typeof device_name !== 'string') {
        return handleValidationError(res, 'device_name', 'Must be string');
    }
    
    if (![0, 1].includes(status)) {
        return handleValidationError(res, 'status', 'Must be 0 or 1');
    }
    
    // ... continue with execution
};
```

---

## 🚀 Performance Considerations

### 1. Concurrent Message Handling

**Problem**: MQTT messages can arrive faster than DB can persist

**Solution**: Use Message Queue (p-queue)

```javascript
const PQueue = require('p-queue');
const queue = new PQueue({ 
    concurrency: 5,           // Max 5 parallel DB writes
    timeout: 30000,           // 30 second operation timeout
    interval: 1000,           // Time window for rate limiting
    intervalCap: 100          // Max 100 operations per interval
});

device.on('message', async (topic, payload) => {
    await queue.add(async () => {
        const data = JSON.parse(payload.toString());
        await Telemetry.create(data);
    });
});
```

### 2. Database Connection Pooling

```javascript
// mongoose auto-manages connection pool
// Default pool size: 10 connections
const options = {
    maxPoolSize: 20,          // For high-throughput
    minPoolSize: 5,           // Maintain minimum connections
    socketTimeoutMS: 45000,
    serverSelectionTimeoutMS: 5000
};

mongoose.connect(process.env.MONGODB_URI, options);
```

### 3. Query Optimization

```javascript
// ❌ Inefficient: Fetches entire document
const data = await Telemetry.findOne().sort({ timestamp: -1 });

// ✅ Optimized: Select only needed fields
const data = await Telemetry
    .findOne()
    .sort({ timestamp: -1 })
    .select('temperature humidity timestamp')
    .lean()  // Return plain JS objects instead of Mongoose docs
    .exec();
```

### 4. Caching Strategy

```javascript
const NodeCache = require('node-cache');
const cache = new NodeCache({ stdTTL: 60 });  // 60 second TTL

const getLatestSensorData = async (req, res) => {
    const cacheKey = 'latest_sensor';
    
    // Try cache first
    let data = cache.get(cacheKey);
    
    if (!data) {
        // Cache miss: fetch from DB
        data = await Telemetry.findOne().sort({ timestamp: -1 }).lean();
        cache.set(cacheKey, data);  // Cache for next 60s
    }
    
    res.json({ success: true, data, cached: !!data });
};
```

---

## 📚 References

- [AWS IoT Core Documentation](https://docs.aws.amazon.com/iot/)
- [Mongoose Documentation](https://mongoosejs.com/)
- [MQTT Specification](http://mqtt.org/)
- [Express.js Best Practices](https://expressjs.com/en/advanced/best-practice-performance.html)
- [Node.js Event Loop](https://nodejs.org/en/docs/guides/blocking-vs-non-blocking/)

---

**Document Version**: 1.0
**Author**: Woemwoem257
**License**: ISC
