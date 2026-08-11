import os
import time
import json
import ssl
import tempfile
import django
import paho.mqtt.client as mqtt

# 1. Khởi tạo môi trường Django
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'server.settings') 
django.setup()

from dashboard.models import SensorData, DeviceStatus

# 2. Cấu hình thông số AWS IoT Core
AWS_ENDPOINT = "a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com"
PORT = 8883
CLIENT_ID = "Django_Server_Client"

TOPIC_SENSOR_SUB = "gateway/sensor/data"
TOPIC_STATUS_SUB = "gateway/status/#"
TOPIC_CONTROL_PUB = "gateway/control/device"

# ========================================================
# 3. NHÚNG CHỨNG CHỈ TRỰC TIẾP DẠNG CHUỖI
# ========================================================

AWS_CERT_CA = """-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----"""

AWS_CERT_CRT = """-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUSEcczKlIqcHZii0OmrMk2XI0fnwwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDcxODAwNTAz
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALpER5zJV+w2a3902En7
6G7iQbiuqmn5p82VOCCSR3NrCs7UwI3FINZnVWWnvRfeGjylSzpykYPcj3x9m36c
WBYwetZLexWKkuVWjfjjdAID9FyYRDxh97LPBYBwLmTTxUEvu5z+pIfoXk26cb4h
2VETTwLdGq7tnonVODn22no0JQlyw84B2X9FBtkTkP1cBQVaAv21mx+iGWW1TV0V
+f1/2wPn36DhMEZ6KNLQHG+/Vp4ctc587RmwuIj3Pw/yv6nPrknZQgihWeCEEhms
YMvWEYKI3kf8uWI0H8y/e+E6BO23YM9vOx6edyy5M12Uz2lonBn2KoWjNt14wOEX
AFcCAwEAAaNgMF4wHwYDVR0jBBgwFoAUcNJD/A7Sn/eDHkZsWEd+OEk0oyUwHQYD
VR0OBBYEFHS0qFK5vabv5rA4RAbUvQw/8mQ+MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQA/oXuCWYRCGjvllWN9UxWTrYyr
4WGRrK7x3z+/Mcn9D5tbqOHCqmMi5Zo1iWvH0wcK9JJFS6Cqa5Z5DIVhtZXnbAmD
Od7cXcYczOQBdeM/7pBrAdALCXRoZ0bNsVXg7CRRetInOmEXI7o9g8hptvYyO+FC
y6hpq2O3fdL5J27oDuy9+bPl4j5ohZ0PaOBRyRmcD5C7RNxv2ESoWjatVRzAPcfG
lU1gEl/HX9n2oqZMxgq5dXAaMCCGzq8HEPOYAfWaf7X6DMKyurKVfBiDlo98FXXj
kKo2LSLGqBk7+S4C0phfj8QTdqXw4AoN0rKjPK85UHl5bOc+7MpgjYh9i1lL
-----END CERTIFICATE-----"""

AWS_CERT_PRIVATE = """-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAukRHnMlX7DZrf3TYSfvobuJBuK6qafmnzZU4IJJHc2sKztTA
jcUg1mdVZae9F94aPKVLOnKRg9yPfH2bfpxYFjB61kt7FYqS5VaN+ON0AgP0XJhE
PGH3ss8FgHAuZNPFQS+7nP6kh+heTbpxviHZURNPAt0aru2eidU4OfbaejQlCXLD
zgHZf0UG2ROQ/VwFBVoC/bWbH6IZZbVNXRX5/X/bA+ffoOEwRnoo0tAcb79Wnhy1
znztGbC4iPc/D/K/qc+uSdlCCKFZ4IQSGaxgy9YRgojeR/y5YjQfzL974ToE7bdg
z287Hp53LLkzXZTPaWicGfYqhaM23XjA4RcAVwIDAQABAoIBADUZn/yvXXRGc1DL
g5lbygBKWggHKye490BhSLXoXZwmqNcbyaTgzKKypKKtNffm5j3htVd9L8SSjQkl
Wb23Xlk9CteqfvU+IvBkQ2bvmLO2YuQ+uD0qhI2h/OSHloJB6NrTe72ezlK8SJ4K
B8D3L1ewlHaxh5jUvrx2X2gO2mu5kuyp4CKshxBNSZQVtWMyGjnhGjnRxPfhc3So
RYR7O0QdYCKBFq7rXUaXNWvmOLy7d2/u6/y3EioL/T2AoaNvJamhzNYF5HZ/pr27
LdpXWgXhvmSy4LZLCTYOjn97CPewtpiMfCNOw+MjSAHGyed+E6ZkD6vHBNw15bWn
zC4LB0ECgYEA7FRrtQytogngjA91jJsia4ZGwVHAOD+Q//LKAaSe0bDF9bBlSPsv
QbMG8RyYg1AFUR8MB887VUl3inkUnfA5/IfFX+XlCYGzu5CRSCuETL2v53Mz1syO
Yeam02ZzV2xiGRutPVt2V3Gk18FgYNWbpzjEY+SfrEQnodJmhK3o1KECgYEAycUk
6u4LT3CeibRK5HxuR3YDdNLN+zjhG/783Bdie+8MaFlmJ1RmD0UwisYMWFANRjhz
yhsAqNR7kEl90dWtbdpX79/al0JmxVTJUVDIaukyepiOkr/8sB8bEYi11fWwxREY
sdKdhRAKVmmmktbKBOZyj8SrCpPuys2xEw3aOfcCgYB1ZDHkSxnsxY8+w2cWovDk
DF0VJjCfQCQcn1NsoYMqke6jbi3BpNQChJVMK3IuuA8MDqoBLxU/9jBlHmP5WBzV
rfODAUVWBZFr+BHSkZs0K303MOhtKEsJonL3y4o7wOCxrCfPtKECKrdBXRMsxq58
0BvtquENxwUQwtydx21CQQKBgEMN9ZrtyWx5LPbuqAUPJUcyfSuR80qOOKGpODec
veoaI7F6JGzlq5VflSZJc0jWMdMoZ7K/DpUNKJNnGR1nOd/MNHVPm8GxG55w8wbH
JhQBs/jyQk+a5ktRyxkkqemVYU3cxKw0Jo1WK1lUeztJjpHaVDBbV8yIJU/Y1ARV
EjU5AoGBAOQMmQFl9gTSlaEuVKK7JDI4gtRV6hIC5YB0aLlHAOVyoa4SdRYaVxue
psb/zwVYMMJ+Makjt8XzSGw1YzbRlZMXd3M4qSb2bQolWKkhtdEdcNj5hGMUWAMn
EIAS7C7kWgVGXdeeml0Tx+d5yvlTYafJRVBBkDzqnqSO/go/s2PJ
-----END RSA PRIVATE KEY-----"""

# Hàm hỗ trợ tạo file tạm thời từ chuỗi
def create_temp_cert_file(cert_string):
    fd, path = tempfile.mkstemp(suffix=".pem", text=True)
    with os.fdopen(fd, 'w') as f:
        f.write(cert_string.strip())
    return path

# ========================================================

# 4. Các hàm Callback xử lý sự kiện MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Kết nối thành công tới AWS IoT Core!")
        client.subscribe(TOPIC_SENSOR_SUB, qos=1)
        client.subscribe(TOPIC_STATUS_SUB, qos=1)
        print(f"[MQTT] Đã subscribe: {TOPIC_SENSOR_SUB} & {TOPIC_STATUS_SUB}")
    else:
        print(f"[MQTT] Kết nối thất bại. Mã lỗi (rc): {rc}")

def on_disconnect(client, userdata, rc):
    print("[MQTT] Đã ngắt kết nối. Đang thử kết nối lại...")
    while True:
        try:
            client.reconnect()
            break
        except Exception as e:
            print(f"[MQTT] Lỗi kết nối lại: {e}. Thử lại sau 5 giây...")
            time.sleep(5)

def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        data = json.loads(payload)

        # Xử lý dữ liệu cảm biến
        if topic == TOPIC_SENSOR_SUB:
            # Lấy bản ghi gần nhất trong Database
            latest_record = SensorData.objects.order_by('-timestamp').first()
            
            # Lấy giá trị từ MQTT (nếu không có sẽ trả về None)
            temp_val = data.get("temperature")
            hum_val = data.get("humidity")
            
            # Nếu tin nhắn CHỈ có Nhiệt độ, mượn lại Độ ẩm cũ
            if temp_val is not None and hum_val is None:
                hum_val = latest_record.humidity if latest_record else 0.0
                
            # Nếu tin nhắn CHỈ có Độ ẩm, mượn lại Nhiệt độ cũ
            elif hum_val is not None and temp_val is None:
                temp_val = latest_record.temperature if latest_record else 0.0
                
            # Lưu vào Database với đầy đủ cả 2 thông số
            SensorData.objects.create(
                temperature=temp_val or 0.0,
                humidity=hum_val or 0.0
            )
            print(f"[DB] Đã lưu đầy đủ: Nhiệt độ {temp_val}°C - Độ ẩm {hum_val}%")

        # Xử lý dữ liệu trạng thái thiết bị
# Xử lý dữ liệu trạng thái thiết bị
        elif topic.startswith("gateway/status"):
            # Lấy key đầu tiên trong JSON làm tên thiết bị (ví dụ: "relay_2")
            # và giá trị của nó làm status (ví dụ: 0)
            try:
                # data.keys() sẽ trả về danh sách các key, ta lấy cái đầu tiên
                device_name = list(data.keys())[0]
                status_val = data[device_name]
                
                # Chuyển đổi trạng thái sang dạng số 0/1
                if isinstance(status_val, str):
                    status_val = 1 if status_val.upper() in ["ON", "1", "TRUE"] else 0
                else:
                    status_val = int(status_val)
                
                # Cập nhật DB
                DeviceStatus.objects.update_or_create(
                    device_name=device_name,
                    defaults={'status': status_val}
                )
                print(f"[DB] Đã cập nhật: {device_name} -> {'ON' if status_val == 1 else 'OFF'}")
            
            except Exception as e:
                print(f"[ERROR] Cấu trúc JSON không hợp lệ: {e}")

    except json.JSONDecodeError:
        print("[MQTT] Lỗi định dạng JSON.")
    except Exception as e:
        print(f"[MQTT] Lỗi xử lý message: {e}")

# 5. Thiết lập và chạy Client
def run_mqtt_client():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, CLIENT_ID)
    

    print("[SYSTEM] Đang nạp chứng chỉ vào bộ nhớ tạm...")
    ca_path = create_temp_cert_file(AWS_CERT_CA)
    cert_path = create_temp_cert_file(AWS_CERT_CRT)
    key_path = create_temp_cert_file(AWS_CERT_PRIVATE)

    try:
        client.tls_set(
            ca_certs=ca_path,
            certfile=cert_path,
            keyfile=key_path,
            tls_version=ssl.PROTOCOL_TLSv1_2
        )
    except Exception as e:
        print(f"[LỖI] Cấu hình bảo mật thất bại: {e}")
        return

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    print("[MQTT] Đang khởi tạo kết nối AWS...")
    client.connect(AWS_ENDPOINT, PORT, keepalive=60)
    
    # Xóa file tạm ngay sau khi TLS context đã đọc xong (để dọn dẹp)
    try:
        os.remove(ca_path)
        os.remove(cert_path)
        os.remove(key_path)
        print("[SYSTEM] Đã dọn dẹp file chứng chỉ tạm.")
    except Exception as e:
        pass

    client.loop_forever()

if __name__ == '__main__':
    run_mqtt_client()