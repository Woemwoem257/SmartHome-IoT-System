from django.db import models

class SensorData(models.Model):
    temperature = models.FloatField(null=True, blank=True)
    humidity = models.FloatField(null=True, blank=True)
    timestamp = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"Nhiệt độ: {self.temperature}°C - Độ ẩm: {self.humidity}%"

class DeviceStatus(models.Model):
    device_name = models.CharField(max_length=50) # Ví dụ: relay_1, mosfet_1
    status = models.IntegerField(default=0)       # 0: OFF, 1: ON
    timestamp = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"{self.device_name}: {'ON' if self.status == 1 else 'OFF'}"