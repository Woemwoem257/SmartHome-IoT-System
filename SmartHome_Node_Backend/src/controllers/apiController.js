const Telemetry = require('../models/Telemetry');
const { publishControlCommand } = require('../services/mqttService');

// [GET] Lấy dữ liệu cảm biến mới nhất
const getLatestSensorData = async (req, res) => {
    try {
        // Truy vấn record cuối cùng dựa trên timestamp
        const latestData = await Telemetry.findOne().sort({ timestamp: -1 });
        
        if (!latestData) {
            return res.status(404).json({ success: false, message: 'Chưa có dữ liệu cảm biến' });
        }
        
        res.status(200).json({
            success: true,
            data: {
                temperature: latestData.temperature,
                humidity: latestData.humidity,
                timestamp: latestData.timestamp
            }
        });
    } catch (error) {
        console.error('[API] Lỗi truy xuất CSDL:', error);
        res.status(500).json({ success: false, message: 'Lỗi máy chủ nội bộ' });
    }
};

// [POST] Điều khiển thiết bị (Bật/Tắt Relay)
const controlDevice = (req, res) => {
    try {
        const { device_name, status } = req.body;
        
        if (!device_name || status === undefined) {
            return res.status(400).json({ success: false, message: 'Thiếu tham số điều khiển' });
        }

        // Cấu trúc Payload chuẩn bị đẩy xuống ESP32
        const commandPayload = {
            [device_name]: status // Ví dụ: { "relay_1": 1 }
        };

        // Bắn tín hiệu qua MQTT
        publishControlCommand(commandPayload);

        res.status(200).json({ success: true, message: 'Đã gửi lệnh điều khiển thành công' });
    } catch (error) {
        console.error('[API] Lỗi phát lệnh điều khiển:', error);
        res.status(500).json({ success: false, message: 'Lỗi máy chủ nội bộ' });
    }
};

module.exports = { getLatestSensorData, controlDevice };