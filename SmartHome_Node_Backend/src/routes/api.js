const express = require('express');
const router = express.Router();
const apiController = require('../controllers/apiController');

router.get('/sensor/latest', apiController.getLatestSensorData);
// THÊM DÒNG NÀY:
router.get('/sensor/history', apiController.getSensorHistory); 
router.post('/device/control', apiController.controlDevice);

module.exports = router;