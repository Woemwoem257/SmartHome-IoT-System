const express = require('express');
const router = express.Router();
const apiController = require('../controllers/apiController');

router.get('/sensor/latest', apiController.getLatestSensorData);
router.post('/device/control', apiController.controlDevice);

module.exports = router;