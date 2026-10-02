#include "aws_mqtt.h"
#include <mqtt_client.h>
#include <esp_log.h>
#include <esp_event.h>
#include <string.h>
#include "aws_certs.h"
#include "uart_bridge.h"
#include <ArduinoJson.h>
#include "device_control.h"

static const char *TAG = "AWS_MQTT";

static esp_mqtt_client_handle_t s_client = NULL;

// Hàm xử lý sự kiện MQTT bất đồng bộ
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(TAG, "Ket noi thanh cong den AWS IoT Core");
            // Sau khi kết nối, tiến hành Subscribe các topic điều khiển
            esp_mqtt_client_subscribe(s_client, "gateway/control/actuator", 1);
            break;
            
        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGE(TAG, "Mat ket noi MQTT");
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

            if (strncmp(event->topic, "gateway/control/actuator", event->topic_len) == 0) {
                StaticJsonDocument<256> doc;
                DeserializationError error = deserializeJson(doc, event->data, event->data_len);

                if (!error) {
                    const char* keys[] = {"relay1", "relay2", "mosfet1", "mosfet2", "alarm_clear"};
                    DeviceID_t ids[] = {DEV_RELAY_1, DEV_RELAY_2, DEV_MOSFET_1, DEV_MOSFET_2, DEV_ALARM_CLEAR};
                    int num_devices = sizeof(keys) / sizeof(keys[0]);

                    for (int i = 0; i < num_devices; i++) {
                        if (doc.containsKey(keys[i])) {
                            ControlMsg_t msg;
                            msg.device_id = ids[i];
                            msg.state = doc[keys[i]];
                            
                            // ĐẨY VÀO HÀNG ĐỢI THAY VÌ GỌI UART TRỰC TIẾP
                            if (xQueueSend(actuator_queue, &msg, 0) != pdTRUE) {
                                ESP_LOGE(TAG, "Hang doi Actuator da day! Bo qua lenh cho %s", keys[i]);
                            }
                        }
                    }
                } else {
                    ESP_LOGE(TAG, "Loi Parse JSON tu AWS: %s", error.c_str());
                }
            }
            break;
        }
            
        case MQTT_EVENT_ERROR:
        /// ĐÁNH THỨC GIAO DIỆN NẾU CÓ LỖI TLS/TCP ĐỂ TRÁNH TREO MÀN HÌNH MÃI MÃI
            ESP_LOGW(TAG, "Da danh thuc HMI Task do MQTT loi.");

            
            ESP_LOGE(TAG, "Loi MQTT/TLS");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Loi Transport: %s", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
          
        default:
            break;
    }
}

void AwsMqtt::init() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    
    // Cấu trúc API của ESP-IDF v4.4
    mqtt_cfg.uri = "mqtts://a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com:8883";
    mqtt_cfg.client_id = "Django_Server_Client";
    mqtt_cfg.cert_pem = (const char *)aws_root_ca_pem;
    mqtt_cfg.client_cert_pem = (const char *)certificate_pem_crt;
    mqtt_cfg.client_key_pem = (const char *)private_pem_key;
    // THÊM DÒNG NÀY: Ép AWS và ESP32 chỉ "hỏi thăm" nhau mỗi 2 phút một lần
    mqtt_cfg.keepalive = 120;
    
    ESP_LOGI(TAG, "Dang khoi tao AWS MQTT Client...");
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    
    // Đăng ký Event Handler và khởi động Client
    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    
    // 2. ĐÓNG BĂNG GIAO DIỆN NGAY TRƯỚC KHI KÍCH HOẠT KẾT NỐI mTLS
    ESP_LOGW(TAG, "Dong bang HMI de tap trung dien nang cho AWS mTLS...");


    esp_mqtt_client_start(s_client);
}

void AwsMqtt::publish(const std::string& topic, const std::string& payload) {
    if (s_client != NULL) {
        int msg_id = esp_mqtt_client_publish(s_client, topic.c_str(), payload.c_str(), payload.length(), 1, 0);
        ESP_LOGI(TAG, "Publish len %s, msg_id=%d", topic.c_str(), msg_id);
    }
}