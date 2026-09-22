require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const path = require('path');
const http = require('http'); // Thư viện lõi để tạo HTTP Server
const { Server } = require('socket.io'); // Import Socket.io

const { connectMQTT } = require('./services/mqttService');
const apiRoutes = require('./routes/api');

const app = express();
const PORT = process.env.PORT || 3000;

// BỌC EXPRESS VÀ KHỞI TẠO SOCKET.IO
const server = http.createServer(app);
const io = new Server(server, {
    cors: { origin: "*", methods: ["GET", "POST"] }
});

app.use(cors()); 
app.use(express.json());

// Tiêm io vào Request để dùng trong các Controller API (nếu cần)
app.use((req, res, next) => {
    req.io = io;
    next();
});

app.use('/api/v1', apiRoutes);
app.use(express.static(path.join(__dirname, '../public')));

const connectDB = async () => {
    try {
        const conn = await mongoose.connect(process.env.MONGODB_URI);
        console.log(`[Database] ✅ Kết nối thành công MongoDB Atlas: ${conn.connection.host}`);
    } catch (error) {
        console.error(`[Database] ❌ Lỗi kết nối CSDL: ${error.message}`);
        process.exit(1); 
    }
};

// QUẢN LÝ VÒNG ĐỜI CLIENT WEBSOCKET
io.on('connection', (socket) => {
    console.log(`[WebSocket] 🟢 Trình duyệt vừa kết nối: ${socket.id}`);
    socket.on('disconnect', () => {
        console.log(`[WebSocket] 🔴 Trình duyệt ngắt kết nối: ${socket.id}`);
    });
});

const startServer = async () => {
    await connectDB(); 
    // TIÊM INSTANCE io VÀO MQTT SERVICE
    connectMQTT(io); 
    
    // THAY THẾ app.listen BẰNG server.listen ĐỂ CHẠY CẢ HTTP & WEBSOCKET
    server.listen(PORT, () => {
        console.log(`[Server]   🚀 Node.js + Socket.io đang lắng nghe tại cổng ${PORT}`);
    });
};

startServer();