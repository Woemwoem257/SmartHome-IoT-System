#include "aws_mqtt.h"
#include <mqtt_client.h>
#include <esp_log.h>
#include <esp_event.h>
#include <string.h>
#include "aws_certs.h"

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
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Nhan tin nhan tu Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);
            // TƯƠNG LAI: Chuyển tiếp Payload này xuống UartBridge
            break;
            
        case MQTT_EVENT_ERROR:
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
    
    // Cấu hình mạng & Broker
    mqtt_cfg.broker.address.uri = "mqtts://a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com:8883"; // Port 8883 cho mTLS
    
// Nạp thẳng các hằng số chuỗi đã định nghĩa trong file aws_certs.h
    mqtt_cfg.broker.verification.certificate = aws_root_ca_pem;
    mqtt_cfg.credentials.authentication.certificate = certificate_pem_crt;
    mqtt_cfg.credentials.authentication.key = private_pem_key;
    
    ESP_LOGI(TAG, "Dang khoi tao AWS MQTT Client...");
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    
    // Đăng ký Event Handler và khởi động Client
    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
}

void AwsMqtt::publish(const std::string& topic, const std::string& payload) {
    if (s_client != NULL) {
        int msg_id = esp_mqtt_client_publish(s_client, topic.c_str(), payload.c_str(), payload.length(), 1, 0);
        ESP_LOGI(TAG, "Publish len %s, msg_id=%d", topic.c_str(), msg_id);
    }
}