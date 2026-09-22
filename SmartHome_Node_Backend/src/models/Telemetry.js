const mongoose = require('mongoose');

const TelemetrySchema = new mongoose.Schema({
    device_id: { type: String, required: true, default: 'gateway_esp32_01', index: true },
    temperature: { type: Number, required: true },
    humidity: { type: Number, required: true },
    timestamp: { type: Date, default: Date.now }
});

// ============================================================================
// CHIẾN LƯỢC ĐÁNH CHỈ MỤC (INDEXING STRATEGY)
// ============================================================================

// 1. Time-series Index: Phục vụ truy vấn tải biểu đồ (Lấy N điểm dữ liệu mới nhất)
TelemetrySchema.index({ timestamp: -1 });

// 2. Compound Index: Cực kỳ quan trọng khi Frontend muốn vẽ biểu đồ riêng cho từng thiết bị
TelemetrySchema.index({ device_id: 1, timestamp: -1 });

// 3. TTL Index (Data Lifecycle): Tự động dọn rác các bản tin cũ hơn 30 ngày (2.592.000 giây)
TelemetrySchema.index({ timestamp: 1 }, { expireAfterSeconds: 2592000 });

module.exports = mongoose.model('Telemetry', TelemetrySchema);