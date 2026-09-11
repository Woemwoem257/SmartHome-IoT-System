#include "lcd_manager.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"       // Cung cấp định nghĩa gpio_num_t và GPIO_MODE_OUTPUT
#include "esp_err.h"           // Cung cấp macro ESP_ERROR_CHECK
#include "esp_lcd_panel_io.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"

#define LCD_MOSI 23
#define LCD_MISO 19
#define LCD_CLK  18
#define LCD_CS   5
#define LCD_DC   21
#define LCD_RST  22
#define TOUCH_CS 15 // T_CS nối với IO15 theo sơ đồ của bạn

void LcdManager_Init(void) {
    // 0. Khóa chốt cảm ứng XPT2046 (Bắt buộc nằm TRONG hàm)
    gpio_set_direction((gpio_num_t)TOUCH_CS, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)TOUCH_CS, 1);

    // 1. Khởi tạo SPI Bus
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = LCD_MOSI;
    buscfg.miso_io_num = LCD_MISO;
    buscfg.sclk_io_num = LCD_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 320 * 240 * 2 + 8; // Bộ đệm khung hình

    spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 2. Gắn thiết bị vào SPI Bus (Hạ xung nhịp xuống 10MHz để chống nhiễu cáp)
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = LCD_CS;
    io_config.dc_gpio_num = LCD_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = 5 * 1000 * 1000; // Tinh chỉnh: Giảm từ 20MHz xuống 10MHz xuống 5MHz
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle);

    // 3. Khởi tạo driver ILI9341
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST;
    panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR; 
    panel_config.bits_per_pixel = 16;

    esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);

    // 4. Trình tự đánh thức (Wake-up Sequence) CÓ ĐỘ TRỄ
    esp_lcd_panel_reset(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(100)); // Đợi IC reset

    esp_lcd_panel_init(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(100)); // Đợi IC khởi động ma trận hiển thị

    esp_lcd_panel_disp_on_off(panel_handle, true);
    vTaskDelay(pdMS_TO_TICKS(100)); // Đợi hiển thị ổn định

    // 5. Bơm màu đỏ (Red = 0xF800)
    uint16_t *color_map = (uint16_t *)heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_DMA);
    if (color_map) {
        for (int i = 0; i < 320 * 240; i++) {
            color_map[i] = 0xF800; // Mã màu đỏ 16-bit
        }
        
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 320, color_map);
    }
}