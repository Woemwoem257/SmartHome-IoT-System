#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h> // Gọi lõi đồ họa LVGL
#include "hmi_manager.h"


QueueHandle_t actuator_queue = NULL;
SemaphoreHandle_t xGuiSemaphore = NULL;

// Các thư viện Middleware cũ
#include "wifi_manager.h"
#include "uart_bridge.h"
#include "aws_mqtt.h"

#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

// Kích thước vật lý của màn hình
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

const char *TAG = "app_main";

// --- KHỞI TẠO OBJECTS LỚP VẬT LÝ ---
TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSPI(HSPI); 
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

TaskHandle_t HMITaskHandle = NULL;

/* =========================================================================
 * 1. CALLBACK ĐẨY KHUNG HÌNH (DISPLAY FLUSH)
 * Nhận vùng nhớ đồ họa từ LVGL và chuyển thẳng xuống thanh ghi màn hình
 * ========================================================================= */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // Bơm mảng màu tĩnh từ RAM xuống SPI
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    // Báo cáo cho LVGL biết tiến trình DMA đã hoàn tất
    lv_disp_flush_ready(disp_drv);
}

/* =========================================================================
 * 2. CALLBACK ĐỌC CẢM ỨNG (INPUT READ)
 * Polling tọa độ từ XPT2046 và truyền vào bộ định tuyến sự kiện của LVGL
 * ========================================================================= */
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR; // Trạng thái: Đang nhấn
        
        // Mapping tọa độ thô sang ma trận pixel 240x320
        data->point.x = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
        data->point.y = map(p.y, 200, 3800, 0, SCREEN_HEIGHT);
    } else {
        data->state = LV_INDEV_STATE_REL; // Trạng thái: Nhả tay
    }
}

/* =========================================================================
 * 3. HMI TASK (FREE-RTOS THREAD CHO LVGL)
 * Cô lập luồng đồ họa, không cho phép block tiến trình mạng (Wi-Fi/AWS)
 * ========================================================================= */
void HMI_Task(void *pvParameters) {
    // 3.1. Khởi tạo lõi LVGL
    lv_init();

    // 3.2. Cấp phát bộ đệm render (Dùng 10 dòng màn hình để tiết kiệm RAM)
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SCREEN_WIDTH * 10];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

    // 3.3. Đăng ký Display Driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // 3.4. Đăng ký Input Driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // --- XÂY DỰNG GIAO DIỆN TỪ MODULE QUẢN LÝ ---
    HmiManager::build_ui();

    // 3.5. Vòng lặp duy trì sự sống HMI
    while (1) {
        // Cố gắng lấy khóa (chờ tối đa 10 ticks = 10ms)
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler(); // Chỉ render đồ họa khi đã cầm chắc Mutex
            
            // Xử lý xong phải trả khóa ngay lập tức để luồng UART có thể lấy!
            xSemaphoreGive(xGuiSemaphore); 
        }
        
        vTaskDelay(pdMS_TO_TICKS(33)); // Giải phóng CPU cho RTOS Tick
    }
}

void Actuator_Task(void *pvParameters) {
    ControlMsg_t msg;
    while (1) {
        if (xQueueReceive(actuator_queue, &msg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI("ACTUATOR", "Nhan lenh HMI -> Thiet bi: %d, Trang thai: %d", msg.device_id, msg.state);
            
            char cmd[64];
            
            // Xây dựng Key JSON tương ứng với DeviceID
            if (msg.device_id == DEV_RELAY_1) {
                snprintf(cmd, sizeof(cmd), "{\"relay1\":%d}\r\n", msg.state);
            }else if (msg.device_id == DEV_RELAY_2) {
                snprintf(cmd, sizeof(cmd), "{\"relay2\":%d}\r\n", msg.state); 
            }else if (msg.device_id == DEV_MOSFET_1) {
                snprintf(cmd, sizeof(cmd), "{\"mosfet1\":%d}\r\n", msg.state);
            }else if (msg.device_id == DEV_MOSFET_2) {
                snprintf(cmd, sizeof(cmd), "{\"mosfet2\":%d}\r\n", msg.state);
            } else if (msg.device_id == DEV_ALARM_CLEAR) {
                snprintf(cmd, sizeof(cmd), "{\"alarm_clear\":%d}\r\n", msg.state);
            }
            
            // Điểm xuất dữ liệu duy nhất ra STM32
            UartBridge::send_command(cmd);
        }
    }
}

extern "C" void app_main() 
{ 
    initArduino();
    // Tạo Queue TRƯỚC KHI tạo bất kỳ Task nào
    actuator_queue = xQueueCreate(10, sizeof(ControlMsg_t));
    
    // Tạo luồng Actuator (Ưu tiên cao hơn HMI để xử lý phần cứng ngay lập tức)
    xTaskCreatePinnedToCore(Actuator_Task, "Actuator", 4096, NULL, 6, NULL, 1);

    ESP_LOGI(TAG, "Middleware Gateway Started"); 

    // --- KHỞI TẠO TẦNG VẬT LÝ ---
    vTaskDelay(pdMS_TO_TICKS(100)); 
    tft.init();
    tft.setRotation(2);
    tft.invertDisplay(true); // Đảo màu panel
    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(2);

    // --- KÍCH HOẠT HMI TASK ---
    // Trước khi khởi tạo cho HMI_Task thì bọc hàm lại
    xGuiSemaphore = xSemaphoreCreateMutex();
    // Cấp vùng nhớ 8KB Stack để LVGL thoải mái render các Animation phức tạp
    xTaskCreatePinnedToCore(HMI_Task, "HMI_Task", 8192, NULL, 5, &HMITaskHandle, 1);

    // --- KÍCH HOẠT TẦNG MẠNG VÀ MIDDLEWARE ---
    WiFiManager::init();
    UartBridge::init();

    esp_sntp_setoperatingmode((esp_sntp_operatingmode_t)SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com");      
    esp_sntp_init();

    time_t now; struct tm timeinfo;
    time(&now); localtime_r(&now, &timeinfo);
    
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now); localtime_r(&now, &timeinfo);
    }
    
    AwsMqtt::init();
}