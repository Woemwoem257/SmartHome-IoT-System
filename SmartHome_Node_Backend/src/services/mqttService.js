const awsIot = require('aws-iot-device-sdk');
const path = require('path');
const Telemetry = require('../models/Telemetry');

let mqttClient = null;

// NHẬN THAM SỐ io TỪ TRẠM ĐIỀU PHỐI (server.js)
const connectMQTT = (io) => {
    const device = awsIot.device({
        keyPath: path.join(__dirname, '../../certs/private.pem.key'),
        certPath: path.join(__dirname, '../../certs/certificate.pem.crt'),
        caPath: path.join(__dirname, '../../certs/AmazonRootCA1.pem'),
        clientId: `nodejs_backend_${Math.random().toString(16).slice(2, 8)}`,
        host: 'a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com' 
    });

    mqttClient = device;

    device.on('connect', () => {
        console.log('[MQTT] ✅ Đã kết nối AWS IoT Core');
        device.subscribe('gateway/sensor/data');
        // Mở rộng: Subscribe thêm topic phản hồi trạng thái từ ESP32
        device.subscribe('gateway/control/ack'); 
    });

    device.on('message', async (topic, payload) => {
        try {
            const data = JSON.parse(payload.toString());
            
            if (topic === 'gateway/sensor/data') {
                if (typeof data.temperature !== 'number' || typeof data.humidity !== 'number') {
                    console.warn('[MQTT] ⚠️ Khước từ bản tin: Sai định dạng Schema.');
                    return;
                }

                // 1. PHÁT DỮ LIỆU TRỰC TIẾP LÊN TRÌNH DUYỆT QUA WEBSOCKET (Độ trễ < 50ms)
                if (io) {
                    io.emit('device:sensor_update', data);
                }

                // 2. LƯU XUỐNG MONGODB ĐỂ PHỤC VỤ LỊCH SỬ
                await Telemetry.create({ temperature: data.temperature, humidity: data.humidity });
                console.log(`[MQTT] 💾 Lưu thành công: Nhiệt độ ${data.temperature}°C, Độ ẩm ${data.humidity}%`);
            }
            else if (topic === 'gateway/control/ack') {
                // Đẩy trạng thái nút bấm/relay thực tế lên giao diện
                if (io) {
                    io.emit('device:state_sync', data);
                }
            }
        } catch (error) {
            console.error('[MQTT] ❌ Lỗi ngoại lệ:', error.message);
        }
    });
};

const publishControlCommand = (payload) => {
    if (!mqttClient) {
        throw new Error("MQTT Client chưa được khởi tạo");
    }
    const topic = 'gateway/control/actuator';
    mqttClient.publish(topic, JSON.stringify(payload), { qos: 1 });
    console.log(`[MQTT] 📤 Đã gửi lệnh điều khiển:`, payload);
};

module.exports = { connectMQTT, publishControlCommand };