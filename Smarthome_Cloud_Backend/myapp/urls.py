from django.urls import path
from . import views

app_name = 'myapp'

urlpatterns = [
    # Main pages
    path('', views.home, name='home'),
    path('rooms/', views.RoomListView.as_view(), name='room_list'),
    path('devices/', views.DeviceListView.as_view(), name='device_list'),
    path('sensor-data/', views.SensorDataListView.as_view(), name='sensor_data_list'),
    path('lights/', views.LightStatusListView.as_view(), name='light_list'),
    
    # Light control
    path('lights/toggle/<int:light_id>/', views.toggle_light, name='toggle_light'),
    
    # API endpoints
    path('api/devices/', views.device_status_api, name='device_status_api'),
    path('api/sensor-data/', views.sensor_data_api, name='sensor_data_api'),
    path('api/lights/', views.lights_api, name='lights_api'),
    path('api/lights/<int:light_id>/', views.light_control_api, name='light_control_api'),
]