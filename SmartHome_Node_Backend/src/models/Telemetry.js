const mongoose = require('mongoose');

const TelemetrySchema = new mongoose.Schema({
    device_id: { type: String, required: true, default: 'gateway_esp32_01', index: true },
    temperature: { type: Number, required: true },
    humidity: { type: Number, required: true },
    timestamp: { type: Date, default: Date.now }
});

module.exports = mongoose.model('Telemetry', TelemetrySchema);