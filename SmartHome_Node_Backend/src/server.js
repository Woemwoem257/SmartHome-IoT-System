require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const path = require('path');

// Import các module tự xây dựng 
const { connectMQTT } = require('./services/mqttService');
const apiRoutes = require('./routes/api');

// 1. Khởi tạo Web Server
const app = express();
const PORT = process.env.PORT || 3000;

// 2. Thiết lập Middleware Pipeline
app.use(cors()); 
app.use(express.json());

// Nhúng bộ định tuyến API vào tiền tố /api/v1
app.use('/api/v1', apiRoutes);

// Mở cổng phục vụ thư mục public giao diện Web
app.use(express.static(path.join(__dirname, '../public')));

// 3. Khởi tạo Cầu nối CSDL (MongoDB Atlas)
const connectDB = async () => {
    try {
        const conn = await mongoose.connect(process.env.MONGODB_URI);
        console.log(`[Database] ✅ Kết nối thành công MongoDB Atlas: ${conn.connection.host}`);
    } catch (error) {
        console.error(`[Database] ❌ Lỗi kết nối CSDL: ${error.message}`);
        // Ép tiến trình Node.js dừng lập tức để tránh lỗi dây chuyền (Cascading Failure)
        process.exit(1); 
    }
};

// 4. Trình kích hoạt Hệ thống (System Bootstrap)
const startServer = async () => {
    await connectDB(); 
    connectMQTT();
    app.listen(PORT, () => {
        console.log(`[Server]   🚀 Node.js Backend đang lắng nghe tại cổng ${PORT}`);
    });
};

startServer();