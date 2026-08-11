from django.shortcuts import render, get_object_or_404, redirect
from django.http import JsonResponse
from django.views.generic import ListView
from django.contrib import messages
from .models import Room, Device, SensorData, LightStatus

# --- MAIN DASHBOARD VIEW ---
def home(request):
    """Home page view - Luồng hiển thị chính của IoT Dashboard"""
    rooms = Room.objects.all()
    devices = Device.objects.all()
    recent_data = SensorData.objects.order_by('-timestamp')[:10]
    
    context = {
        'rooms': rooms,
        'devices': devices,
        'recent_data': recent_data,
    }
    return render(request, 'myapp/home.html', context)

# --- CLASS-BASED VIEWS (Phục vụ hiển thị danh sách) ---
class RoomListView(ListView):
    model = Room
    template_name = 'myapp/room_list.html'
    context_object_name = 'rooms'

class DeviceListView(ListView):
    model = Device
    template_name = 'myapp/device_list.html'
    context_object_name = 'devices'

class SensorDataListView(ListView):
    """List sensor data"""
    model = SensorData
    template_name = 'myapp/sensor_data_list.html'
    context_object_name = 'sensor_data'
    ordering = ['-timestamp']
    paginate_by = 20

class LightStatusListView(ListView):
    model = LightStatus
    template_name = 'myapp/light_list.html'
    context_object_name = 'lights'

# --- FUNCTION-BASED VIEWS (Phục vụ thao tác điều khiển thiết bị) ---
def toggle_light(request, light_id):
    """Luồng xử lý tín hiệu điều khiển trạng thái bật/tắt đèn"""
    light = get_object_or_404(LightStatus, pk=light_id)
    light.toggle()
    return redirect('myapp:light_list')

# --- API ENDPOINTS (Luồng dữ liệu giao tiếp với IoT Hardware/Frontend) ---
def device_status_api(request):
    return JsonResponse({"status": "Operational"})

def sensor_data_api(request):
    return JsonResponse({"message": "Sensor data endpoint"})

def lights_api(request):
    return JsonResponse({"message": "Lights API endpoint"})

def light_control_api(request, light_id):
    return JsonResponse({"status": "Success", "light_id": light_id})