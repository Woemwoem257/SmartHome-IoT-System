import json
import ssl
import tempfile
import os
import paho.mqtt.client as mqtt

# THÊM 'render' VÀO DÒNG NÀY:
from django.shortcuts import render, redirect 
from django.contrib.auth.decorators import login_required
from .models import SensorData, DeviceStatus
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



def create_temp_cert(cert_string):
    fd, path = tempfile.mkstemp(suffix=".pem", text=True)
    with os.fdopen(fd, 'w') as f:
        f.write(cert_string.strip())
    return path

# ========================================================
# 2. HÀM HIỂN THỊ TRANG CHỦ (DASHBOARD)
# ========================================================
@login_required(login_url='/login/')
def home_dashboard(request):
    latest_sensor = SensorData.objects.order_by('-timestamp').first()
    
    # ÉP CỨNG: Khởi tạo/Lấy đúng 4 Relay và 4 Mosfet (ID 0-3) từ DB
    relays = [DeviceStatus.objects.get_or_create(device_name=f"relay_{i}", defaults={'status': 0})[0] for i in range(4)]
    mosfets = [DeviceStatus.objects.get_or_create(device_name=f"mosfet_{i}", defaults={'status': 0})[0] for i in range(4)]
    
    context = {
        'sensor': latest_sensor,
        'relays': relays,
        'mosfets': mosfets,
    }
    return render(request, 'dashboard/index.html', context)


@login_required(login_url='/login/')
def toggle_device(request, device_name):
    if request.method == "POST":
        current_status = int(request.POST.get('current_status', 0))
        new_status = 0 if current_status == 1 else 1 

        # Tách "relay_0" thành type="relay" và id=0
        try:
            dev_type, dev_id = device_name.split("_")
            dev_id = int(dev_id)
        except:
            dev_type, dev_id = device_name, 0

        ca_path = create_temp_cert(AWS_CERT_CA)
        cert_path = create_temp_cert(AWS_CERT_CRT)
        key_path = create_temp_cert(AWS_CERT_PRIVATE)

        try:
            client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, f"Web_Controller_{device_name}")
            client.tls_set(ca_certs=ca_path, certfile=cert_path, keyfile=key_path, tls_version=ssl.PROTOCOL_TLSv1_2)
            client.connect("a2b1ak1ocftwcb-ats.iot.ap-southeast-2.amazonaws.com", 8883, 60)
            
            # ĐÓNG GÓI BAO TRÙM: Gửi tất cả các trường hợp biến mà ESP có thể đang dùng
            payload = json.dumps({
                dev_type: dev_id,          # Ví dụ: {"relay": 0}
                "id": dev_id,              # Ví dụ: {"id": 0}
                "type": dev_type,          # Ví dụ: {"type": "relay"}
                "status": new_status,
                "command": new_status,
                "state": "ON" if new_status == 1 else "OFF"
            })
            
            payload = json.dumps({
                device_name: new_status 
            })
            
            client.publish("gateway/control/device", payload, qos=1)
            client.publish("gateway/control", payload, qos=1)
            client.disconnect()
        except Exception as e:
            print(f"Lỗi gửi lệnh: {e}")
        finally:
            os.remove(ca_path)
            os.remove(cert_path)
            os.remove(key_path)

    return redirect('home')
