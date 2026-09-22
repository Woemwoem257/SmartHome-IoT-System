const express = require('express');
const router = express.Router();
const apiController = require('../controllers/apiController');

// 1. TẠO Ổ KHÓA (MIDDLEWARE) BẰNG API KEY TĨNH
const verifyApiKey = (req, res, next) => {
    const clientKey = req.headers['x-api-key'];
    if (clientKey === 'IOT_BAO_VE_2026') {
        next(); // Đúng khóa -> Cho đi tiếp vào Controller
    } else {
        res.status(401).json({ success: false, message: 'Từ chối truy cập: Sai API Key!' });
    }
};

// 2. PUBLIC ROUTES (Mở tự do để giao diện Web load biểu đồ)
router.get('/sensor/latest', apiController.getLatestSensorData);
router.get('/sensor/history', apiController.getSensorHistory); 

// 3. PROTECTED ROUTES (Khóa chặt, bắt buộc phải gửi kèm API Key trong Header)
router.post('/device/control', verifyApiKey, apiController.controlDevice);
router.post('/alarm/clear', verifyApiKey, apiController.clearAlarm);

module.exports = router;