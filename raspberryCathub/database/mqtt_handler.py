import paho.mqtt.client as mqtt
import json
import logging
from typing import Dict, Any, Optional

class MQTTHandler:
    def __init__(self, broker_host: str = "localhost", broker_port: int = 8883, 
                 username: str = "admin", password: str = "admin123"):
        self.logger = logging.getLogger(__name__)
        self.client = mqtt.Client()
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.username = username
        self.password = password
        self.connected = False
        
        self._setup_client()

    def _setup_client(self):
        """Configurar cliente MQTT"""
        # Configurar credenciales
        self.client.username_pw_set(self.username, self.password)
        
        # Configurar TLS/SSL para WSS
        self.client.tls_set()
        
        # Callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_publish = self._on_publish

    def _on_connect(self, client, userdata, flags, rc):
        """Callback cuando se conecta"""
        if rc == 0:
            self.logger.info("🔌 Conectado al broker MQTT")
            self.connected = True
        else:
            self.logger.error(f"❌ Error conectando al MQTT: {rc}")

    def _on_disconnect(self, client, userdata, rc):
        """Callback cuando se desconecta"""
        self.logger.warning("🔌 Desconectado del broker MQTT")
        self.connected = False

    def _on_publish(self, client, userdata, mid):
        """Callback cuando se publica un mensaje"""
        self.logger.debug(f"📤 Mensaje publicado: {mid}")

    def connect(self) -> bool:
        """Conectar al broker MQTT"""
        try:
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
            return True
        except Exception as e:
            self.logger.error(f"❌ Error conectando a MQTT: {e}")
            return False

    def disconnect(self):
        """Desconectar del broker"""
        if self.connected:
            self.client.loop_stop()
            self.client.disconnect()

    def publish_sensor_data(self, device_id: str, sensor_id: str, sensor_type: str, 
                           reading_value: Any, additional_data: Dict = None) -> bool:
        """
        Publica datos de sensores en topics específicos
        
        Topic pattern: cathub/device/{device_id}/sensor/{sensor_type}/{sensor_id}/data
        """
        if not self.connected:
            self.logger.error("❌ No conectado al broker MQTT")
            return False

        # Crear el topic específico
        topic = f"cathub/device/{device_id}/sensor/{sensor_type}/{sensor_id}/data"
        
        # Preparar payload
        payload = {
            "device_id": device_id,
            "sensor_id": sensor_id,
            "sensor_type": sensor_type,
            "reading": reading_value,
            "timestamp": int(time.time() * 1000),  # Timestamp en milisegundos
            "additional_data": additional_data or {}
        }
        
        try:
            result = self.client.publish(topic, json.dumps(payload), qos=1)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                self.logger.info(f"📤 Datos publicados en {topic}: {reading_value}")
                return True
            else:
                self.logger.error(f"❌ Error publicando en {topic}: {result.rc}")
                return False
        except Exception as e:
            self.logger.error(f"❌ Excepción publicando datos: {e}")
            return False

    def publish_device_status(self, device_id: str, status: str, device_type: str) -> bool:
        """
        Publica status del dispositivo
        
        Topic: cathub/device/{device_id}/status
        """
        topic = f"cathub/device/{device_id}/status"
        
        payload = {
            "device_id": device_id,
            "device_type": device_type,
            "status": status,
            "timestamp": int(time.time() * 1000)
        }
        
        try:
            result = self.client.publish(topic, json.dumps(payload), qos=1, retain=True)
            return result.rc == mqtt.MQTT_ERR_SUCCESS
        except Exception as e:
            self.logger.error(f"❌ Error publicando status: {e}")
            return False

    def publish_command_response(self, device_id: str, command: str, success: bool, 
                                response: str = None) -> bool:
        """
        Publica respuesta a comandos
        
        Topic: cathub/device/{device_id}/command/response
        """
        topic = f"cathub/device/{device_id}/command/response"
        
        payload = {
            "device_id": device_id,
            "command": command,
            "success": success,
            "response": response,
            "timestamp": int(time.time() * 1000)
        }
        
        try:
            result = self.client.publish(topic, json.dumps(payload), qos=1)
            return result.rc == mqtt.MQTT_ERR_SUCCESS
        except Exception as e:
            self.logger.error(f"❌ Error publicando respuesta: {e}")
            return False

    def subscribe_to_commands(self, device_id: str, callback_function) -> bool:
        """
        Suscribirse a comandos para un dispositivo específico
        
        Topic: cathub/device/{device_id}/command/+
        """
        topic = f"cathub/device/{device_id}/command/+"
        
        def on_message(client, userdata, message):
            try:
                payload = json.loads(message.payload.decode())
                callback_function(message.topic, payload)
            except Exception as e:
                self.logger.error(f"❌ Error procesando comando: {e}")
        
        self.client.on_message = on_message
        result = self.client.subscribe(topic, qos=1)
        
        return result[0] == mqtt.MQTT_ERR_SUCCESS