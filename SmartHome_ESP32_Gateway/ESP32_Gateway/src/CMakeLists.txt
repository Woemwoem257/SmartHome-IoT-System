# Khai báo đích danh từng file mã nguồn (Dùng GLOB là một lỗi sai khiến file không  được tìm thấy)
idf_component_register(SRCS "main.cpp" 
                            "wifi_manager.cpp" 
                            "uart_bridge.cpp" 
                            "aws_mqtt.cpp"
                            "hmi_manager.cpp"
                            "app_tasks.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES json mqtt nvs_flash)