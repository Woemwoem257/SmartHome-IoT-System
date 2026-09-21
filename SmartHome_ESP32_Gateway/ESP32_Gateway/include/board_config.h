#pragma once

namespace Board {
    // Cấu hình màn hình TFT & Touch
    constexpr int TouchCsPin  = 15;
    constexpr int TouchIrqPin = 27;
    constexpr int SpiSclkPin  = 14;
    constexpr int SpiMisoPin  = 12;
    constexpr int SpiMosiPin  = 13;

    constexpr int ScreenWidth  = 240;
    constexpr int ScreenHeight = 320;
    
    // (Sau này chúng ta sẽ gom các cấu hình UART từ uart_bridge.cpp sang đây)
}