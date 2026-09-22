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

// ============================================================================
// [GET] LẤY LỊCH SỬ DỮ LIỆU ĐỂ VẼ BIỂU ĐỒ (INITIAL LOAD)
// ============================================================================
const getSensorHistory = async (req, res) => {
    try {
        const historyData = await Telemetry.find()
            .sort({ timestamp: -1 })
            .limit(30)
            .select('temperature humidity timestamp -_id'); 

        // Đảo ngược mảng để Chart.js vẽ đúng từ trái (cũ) sang phải (mới)
        const chartReadyData = historyData.reverse();

        res.status(200).json({ success: true, data: chartReadyData });
    } catch (error) {
        console.error('[API] ❌ Lỗi truy vấn lịch sử:', error.message);
        res.status(500).json({ success: false, message: 'Lỗi truy xuất cơ sở dữ liệu' });
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

// ĐỪNG QUÊN XUẤT HÀM MỚI Ở ĐÂY
module.exports = { getLatestSensorData, getSensorHistory, controlDevice };