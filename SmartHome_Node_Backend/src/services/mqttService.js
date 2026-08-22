const awsIot = require('aws-iot-device-sdk');
const path = require('path');
const Telemetry = require('../models/Telemetry');

const connectMQTT = () => {
    const device = awsIot.device({
        keyPath: path.join(__dirname, '../../certs/private.pem.key'),
        certPath: path.join(__dirname, '../../certs/certificate.pem.crt'),
        caPath: path.join(__dirname, '../../certs/AmazonRootCA1.pem'),
        clientId: `nodejs_backend_${Math.random().toString(16).slice(2, 8)}`,
        host: 'a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com' // Kiểm tra lại Endpoint của bạn
    });

    device.on('connect', () => {
        console.log('[MQTT] ✅ Đã kết nối AWS IoT Core');
        device.subscribe('gateway/sensor/data');
    });

    device.on('message', async (topic, payload) => {
        if (topic === 'gateway/sensor/data') {
            try {
                const data = JSON.parse(payload.toString());
                
                // Kỹ thuật Defensive Programming: Kiểm định Payload trước khi tương tác DB
                if (typeof data.temperature !== 'number' || typeof data.humidity !== 'number') {
                    console.warn('[MQTT] ⚠️ Khước từ bản tin: Sai định dạng Schema Thiếu Nhiệt độ/Độ ẩm. Payload:', data);
                    return; // Ngắt luồng thực thi ngay lập tức
                }

                // Nếu hợp lệ, mới thực hiện lưu trữ
                await Telemetry.create({ temperature: data.temperature, humidity: data.humidity });
                console.log(`[MQTT] 💾 Lưu thành công: Nhiệt độ ${data.temperature}°C, Độ ẩm ${data.humidity}%`);
                
            } catch (error) {
                // Lỗi này giờ đây chỉ bắt các ngoại lệ rớt mạng DB hoặc JSON parse lỗi
                console.error('[MQTT] ❌ Lỗi ngoại lệ:', error.message);
            }
        }
    });
};

module.exports = { connectMQTT };