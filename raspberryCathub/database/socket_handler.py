import socketio
import logging
from typing import Dict, Callable

class SocketHandler:
    def __init__(self, server_url: str = "http://localhost:3000"):
        self.logger = logging.getLogger(__name__)
        self.sio = socketio.Client()
        self.server_url = server_url
        self.connected = False
        
        # Callbacks
        self.on_device_identifier_created_callback: Callable = None
        self.on_device_status_changed_callback: Callable = None
        self.on_feeder_food_assigned_callback: Callable = None
        
        self._setup_events()

    def _setup_events(self):
        """Configurar eventos del socket"""
        
        @self.sio.on('connect')
        def on_connect():
            self.logger.info("🔌 Conectado al servidor de sockets")
            self.connected = True

        @self.sio.on('disconnect')
        def on_disconnect():
            self.logger.warning("🔌 Desconectado del servidor de sockets")
            self.connected = False

        # ===== EVENTO ESPECÍFICO POR CÓDIGO DE DISPOSITIVO =====
        @self.sio.on('create/identifier/device')
        def on_device_identifier_created(data):
            """Cuando se crea un identifier para UN código específico"""
            self.logger.info(f"🆔 Identifier creado: {data}")
            if self.on_device_identifier_created_callback:
                self.on_device_identifier_created_callback(data)

        @self.sio.on('get/status/device')
        def on_device_status_changed(data):
            """Cambio de status de dispositivo"""
            self.logger.info(f"📊 Cambio de status: {data}")
            if self.on_device_status_changed_callback:
                self.on_device_status_changed_callback(data)

        @self.sio.on('get/device/sensor/setting')
        def on_feeder_food_assigned(data):
            """Asignación de comida al feeder"""
            self.logger.info(f"🍽️ Comida asignada: {data}")
            if self.on_feeder_food_assigned_callback:
                self.on_feeder_food_assigned_callback(data)

    def connect(self) -> bool:
        """Conectar al servidor de sockets"""
        try:
            self.sio.connect(self.server_url)
            return True
        except Exception as e:
            self.logger.error(f"❌ Error conectando al socket: {e}")
            return False

    def listen_to_device_creation(self, device_code: str):
        """
        ✅ ESCUCHAR ESPECÍFICAMENTE LA CREACIÓN DE UN DISPOSITIVO
        Estructura: create/identifier/device/{codigo}
        """
        room = f"create/identifier/device/{device_code}"
        self.sio.emit('join', {'room': room})
        self.logger.info(f"👂 Escuchando creación de identifier para: {device_code}")
        self.logger.info(f"🎯 Room: {room}")

    def disconnect(self):
        """Desconectar del servidor"""
        if self.connected:
            self.sio.disconnect()

    # Setters para callbacks
    def set_device_identifier_created_callback(self, callback: Callable):
        self.on_device_identifier_created_callback = callback

    def set_device_status_changed_callback(self, callback: Callable):
        self.on_device_status_changed_callback = callback

    def set_feeder_food_assigned_callback(self, callback: Callable):
        self.on_feeder_food_assigned_callback = callback

    def wait_for_events(self):
        """Mantener el socket escuchando"""
        if self.connected:
            self.sio.wait()