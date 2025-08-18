import time
import logging
import threading
from database.postgres_handler import PostgresHandler
from communication.arduino_serial import ArduinoSerial
from database.socket_handler import SocketHandler
from database.mqtt_handler import MQTTHandler
from database.mongo_handler import MongoHandler
from database.local_storage import LocalStorage
from typing import Optional, Dict, Any, List
from datetime import datetime

class DeviceManager:
    def __init__(self, device_code: str, serial_port: str = "/dev/ttyACM0"):
        self.logger = logging.getLogger(__name__)
        
        # ✅ HANDLERS
        self.db = PostgresHandler()
        self.arduino = ArduinoSerial(serial_port)
        self.socket_handler = SocketHandler()
        self.mqtt_handler = MQTTHandler()
        self.mongo_handler = MongoHandler()
        self.local_storage = LocalStorage()
        
        # ✅ CONECTAR MONGO CON LOCAL STORAGE
        self.mongo_handler.set_local_storage(self.local_storage)
        
        # ✅ PROPIEDADES DEL DISPOSITIVO
        self.device_code = device_code
        self.identifier: Optional[str] = None
        self.device_type: Optional[str] = None
        self.sensor_identifiers: List[str] = []
        self.food_amount: Optional[float] = None
        self.is_configured = False
        self.is_running = True
        
        # ✅ INTERVALOS
        self.mqtt_interval = 5    # 5 segundos para MQTT
        self.mongo_interval = 60  # 60 segundos para MongoDB
        
        # ✅ CONFIGURAR CALLBACKS
        self._setup_socket_callbacks()

    def _setup_socket_callbacks(self):
        """Configurar callbacks del socket"""
        self.socket_handler.set_device_identifier_created_callback(self._on_identifier_created)
        self.socket_handler.set_device_status_changed_callback(self._on_status_changed)
        self.socket_handler.set_feeder_food_assigned_callback(self._on_food_assigned)

    def _on_identifier_created(self, data: Dict):
        """🎯 EVENTO: Identifier creado - Solo consultar BD cuando recibo el evento"""
        self.logger.info("🎉 Evento: Identifier creado - consultando BD...")
        
        # Consultar BD ahora que SÉ que ya existe
        identifier = self.db.get_device_identifier(self.device_code)
        if identifier:
            self.identifier = identifier
            self.logger.info(f"✅ Identifier obtenido: {identifier}")
            
            # Obtener sensores
            self._fetch_sensors()
            
            # Configurar Arduino
            self._configure_arduino()
        else:
            self.logger.error("❌ Identifier no encontrado en BD después del evento")

    def _on_status_changed(self, data: Dict):
        """🎯 EVENTO: Status cambió (solo arenero) - Solo consultar BD cuando recibo el evento"""
        if self.get_device_type() == "litterbox" and self.identifier:
            self.logger.info("📊 Evento: Status cambió - consultando BD...")
            
            # Consultar status e intervalo AHORA que sé que cambió
            status = self.db.get_status_device_environment(self.identifier)
            interval = self.db.get_interval_motor(self.identifier)
            
            if status is not None and interval is not None:
                self.logger.info(f"✅ Nuevo status: {status}, Intervalo: {interval}")
                self._send_status_to_arduino(status, interval)
            else:
                self.logger.error("❌ No se pudo obtener status/intervalo de BD")

    def _on_food_assigned(self, data: Dict):
        """🎯 EVENTO: Comida asignada (solo feeder) - Solo consultar BD cuando recibo el evento"""
        if self.get_device_type() == "feeder" and self.identifier:
            sensor_id = data.get('sensor_id')
            self.logger.info(f"🍽️ Evento: Comida asignada - consultando BD para sensor {sensor_id}...")
            
            # Consultar configuración AHORA que sé que cambió
            if sensor_id:
                setting = self.db.get_device_sensor_setting(sensor_id)
                if setting and 'value' in setting:
                    self.food_amount = float(setting['value'])
                    self.logger.info(f"✅ Nueva cantidad: {self.food_amount}g")
                    self._send_food_to_arduino(self.food_amount)

    def get_device_type(self) -> str:
        """Determina el tipo de dispositivo basado en el código"""
        if self.device_code in ["A1B2C3D4"]:
            return "waterdispenser"
        elif self.device_code in ["E5F6G7H8"]:
            return "feeder" 
        elif self.device_code in ["I9J0K1L2"]:
            return "litterbox"
        return "unknown"

    def _fetch_sensors(self):
        """Obtener sensores desde BD"""
        try:
            device_env_id = self.db.get_device_environment_id(self.identifier)
            if not device_env_id:
                self.logger.error("❌ No se encontró device_environment_id")
                return

            sensors = self.db.get_all_identified_sensors(device_env_id)
            self.sensor_identifiers = [sensor['sensor_identifier'] for sensor in sensors]
            
            self.logger.info(f"✅ Sensores obtenidos: {self.sensor_identifiers}")
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo sensores: {e}")

    def _configure_arduino(self) -> bool:
        """Configurar Arduino con datos obtenidos"""
        if not self.identifier or not self.sensor_identifiers:
            self.logger.error("❌ No se puede configurar Arduino")
            return False

        device_type = self.get_device_type()
        
        # ✅ CONFIGURACIÓN BÁSICA
        basic_command = f"DEVICE:CONFIGURE:{self.identifier},{device_type}"
        if not self.arduino.send_command(basic_command):
            self.logger.error("❌ Error configuración básica")
            return False

        # ✅ CONFIGURAR SENSORES
        for i, sensor_id in enumerate(self.sensor_identifiers):
            sensor_command = f"SENSOR:SET_ID:{i},{sensor_id}"
            if not self.arduino.send_command(sensor_command):
                self.logger.error(f"❌ Error configurando sensor {sensor_id}")
                return False
            time.sleep(0.1)

        # ✅ CONFIGURACIÓN ESPECÍFICA POR TIPO
        if device_type == "litterbox":
            status = self.db.get_status_device_environment(self.identifier)
            interval = self.db.get_interval_motor(self.identifier)
            if status is not None and interval is not None:
                self._send_status_to_arduino(status, interval)

        elif device_type == "feeder":
            for sensor_id in self.sensor_identifiers:
                setting = self.db.get_device_sensor_setting(sensor_id)
                if setting and 'value' in setting:
                    self.food_amount = float(setting['value'])
                    self._send_food_to_arduino(self.food_amount)
                    break

        # ✅ CONECTAR MQTT
        if self.mqtt_handler.connect():
            self.mqtt_handler.subscribe_to_commands(self.identifier, self._handle_mqtt_command)
            self.mqtt_handler.publish_device_status(self.identifier, "online", device_type)

        # ✅ CONECTAR MONGO
        if self.mongo_handler.connect():
            self.logger.info("✅ MongoDB conectado")
            # Sincronizar datos offline
            synced = self.mongo_handler.sync_offline_data(self.local_storage)
            if synced > 0:
                self.logger.info(f"🔄 {synced} readings offline sincronizados")

        # ✅ INICIAR LOOPS DE DATOS
        self._start_data_loops()

        self.logger.info("✅ Arduino configurado exitosamente")
        self.is_configured = True
        return True

    def _start_data_loops(self):
        """Iniciar loops de recolección de datos"""
        
        # ✅ LOOP MQTT (cada 5 segundos)
        mqtt_thread = threading.Thread(target=self._mqtt_loop)
        mqtt_thread.daemon = True
        mqtt_thread.start()
        
        # ✅ LOOP MONGO (cada 60 segundos)
        mongo_thread = threading.Thread(target=self._mongo_loop)
        mongo_thread.daemon = True
        mongo_thread.start()

    def _mqtt_loop(self):
        """📡 Loop para enviar datos por MQTT cada 5 segundos"""
        while self.is_running and self.is_configured:
            try:
                # Leer datos del Arduino
                readings = self._read_arduino_sensors()
                
                if readings:
                    # ✅ PUBLICAR POR MQTT (tiempo real)
                    self.publish_sensor_readings(readings)
                    self.logger.debug(f"📡 Datos enviados por MQTT: {readings}")
                
                time.sleep(self.mqtt_interval)
                
            except Exception as e:
                self.logger.error(f"❌ Error en MQTT loop: {e}")
                time.sleep(5)

    def _mongo_loop(self):
        """💾 Loop para guardar en MongoDB cada 60 segundos"""
        while self.is_running and self.is_configured:
            try:
                # Leer datos del Arduino
                readings = self._read_arduino_sensors()
                
                if readings:
                    # ✅ GUARDAR EN MONGO (con fallback offline)
                    self._save_readings_to_mongo(readings)
                    self.logger.debug(f"💾 Datos guardados: {readings}")
                
                # ✅ INTENTAR SINCRONIZAR OFFLINE
                if self.mongo_handler.is_connected():
                    self.mongo_handler.sync_offline_data(self.local_storage)
                
                time.sleep(self.mongo_interval)
                
            except Exception as e:
                self.logger.error(f"❌ Error en Mongo loop: {e}")
                time.sleep(30)  # Esperar menos si hay error

    def _read_arduino_sensors(self) -> Optional[Dict[str, Any]]:
        """📥 Leer todos los sensores del Arduino"""
        try:
            # ✅ CORREGIDO: Comando unificado
            if self.arduino.send_command("SENSORS:READ_ALL"):
                response = self.arduino.read_response()
                if response:
                    return self._parse_arduino_response(response)
            return None
        except Exception as e:
            self.logger.error(f"❌ Error leyendo sensores: {e}")
            return None

    def _parse_arduino_response(self, response: str) -> Dict[str, Any]:
        """Parsear respuesta del Arduino"""
        try:
            # Si Arduino envía JSON
            import json
            return json.loads(response)
        except:
            # Si Arduino envía formato personalizado
            readings = {}
            lines = response.split('\n')
            for line in lines:
                if ':' in line:
                    key, value = line.split(':', 1)
                    try:
                        readings[key.strip()] = float(value.strip())
                    except:
                        readings[key.strip()] = value.strip()
            return readings

    def _save_readings_to_mongo(self, readings: Dict[str, Any]):
        """💾 Guardar readings en MongoDB usando la lógica implementada"""
        try:
            sensor_mappings = self._get_sensor_mappings()
            timestamp = datetime.now()
            device_id = self.mongo_handler.get_device_id(self.identifier)
            
            for reading_key, sensor_info in sensor_mappings.items():
                if reading_key in readings:
                    sensor_index = sensor_info.get('sensor_index', 0)
                    if sensor_index < len(self.sensor_identifiers):
                        sensor_id = self.sensor_identifiers[sensor_index]
                        
                        # ✅ DOCUMENTO PARA MONGO
                        document = {
                            "sensor_name": f"{self.get_device_type()}_{sensor_info['type']}",
                            "identifier": sensor_id,
                            "value": readings[reading_key],
                            "timestamp": timestamp,
                            "device_id": device_id,
                            "sensor_type": sensor_info['type'],
                            "unit": sensor_info.get('unit', ''),
                            "device_identifier": self.identifier
                        }
                        
                        # ✅ USAR TU LÓGICA IMPLEMENTADA
                        success = self.mongo_handler.save_sensor_reading(document)
                        
                        if success:
                            self.logger.debug(f"💾 Guardado: {sensor_id} = {readings[reading_key]}")
                        else:
                            self.logger.warning(f"⚠️ No se pudo guardar: {sensor_id}")
                            
        except Exception as e:
            self.logger.error(f"❌ Error guardando en Mongo: {e}")

    def _send_status_to_arduino(self, status: bool, interval: int):
        """Enviar status e intervalo al Arduino (solo arenero)"""
        command = f"LITTERBOX:CONFIG:{int(status)},{interval}"
        if self.arduino.send_command(command):
            self.logger.info(f"✅ Status enviado: {status}, intervalo: {interval}")

    def _send_food_to_arduino(self, amount: float):
        """Enviar cantidad de comida al Arduino (solo feeder)"""
        command = f"FEEDER:SET_FOOD:{amount}"
        if self.arduino.send_command(command):
            self.logger.info(f"✅ Comida enviada: {amount}g")

    def _handle_mqtt_command(self, topic: str, payload: Dict):
        """Manejar comandos recibidos por MQTT"""
        parts = topic.split('/')
        if len(parts) >= 5:
            command = parts[4]
            arduino_command = f"{command.upper()}:{payload.get('params', '')}"
            success = self.arduino.send_command(arduino_command)
            
            self.mqtt_handler.publish_command_response(
                self.identifier, command, success,
                "OK" if success else "ERROR"
            )

    def publish_sensor_readings(self, readings: Dict[str, Any]):
        """📡 Publicar lecturas por MQTT con topics únicos"""
        if not self.is_configured or not self.mqtt_handler.connected:
            return

        device_type = self.get_device_type()
        sensor_mappings = self._get_sensor_mappings()
        
        for reading_key, sensor_info in sensor_mappings.items():
            if reading_key in readings:
                sensor_index = sensor_info.get('sensor_index', 0)
                if sensor_index < len(self.sensor_identifiers):
                    sensor_id = self.sensor_identifiers[sensor_index]
                    
                    self.mqtt_handler.publish_sensor_data(
                        device_id=self.identifier,
                        sensor_id=sensor_id,
                        sensor_type=sensor_info['type'],
                        reading_value=readings[reading_key],
                        additional_data={
                            "unit": sensor_info.get('unit', ''),
                            "device_type": device_type
                        }
                    )

    def _get_sensor_mappings(self) -> Dict[str, Dict]:
        """Mapeo de lecturas a sensores según tipo de dispositivo"""
        device_type = self.get_device_type()
        
        if device_type == "litterbox":
            return {
                "distance": {"type": "ultrasonic", "sensor_index": 0, "unit": "cm"},
                "temperature": {"type": "temperature", "sensor_index": 1, "unit": "°C"},
                "humidity": {"type": "humidity", "sensor_index": 1, "unit": "%"},
                "gas_ppm": {"type": "gas", "sensor_index": 2, "unit": "ppm"}
            }
        elif device_type == "feeder":
            return {
                "weight": {"type": "weight", "sensor_index": 0, "unit": "g"},
                "cat_distance": {"type": "ultrasonic1", "sensor_index": 1, "unit": "cm"},
                "food_distance": {"type": "ultrasonic2", "sensor_index": 2, "unit": "cm"}
            }
        elif device_type == "waterdispenser":
            return {
                "water_level": {"type": "water_level", "sensor_index": 0, "unit": ""},
                "cat_drinking": {"type": "ir_sensor", "sensor_index": 1, "unit": "bool"}
            }
        
        return {}

    def start(self) -> bool:
        """🚀 Iniciar dispositivo completo"""
        self.logger.info(f"🚀 Iniciando dispositivo: {self.device_code}")
        
        # 1. Conectar Arduino
        if not self.arduino.connect():
            self.logger.error("❌ No se pudo conectar con Arduino")
            return False

        # 2. Conectar socket
        if not self.socket_handler.connect():
            self.logger.error("❌ No se pudo conectar al socket")
            return False

        # 3. Escuchar eventos específicos de nuestro código
        self.socket_handler.listen_to_device_creation(self.device_code)

        # 4. Verificar si ya existe (consulta única inicial)
        identifier = self.db.get_device_identifier(self.device_code)
        if identifier:
            self.identifier = identifier
            self.logger.info(f"✅ Identifier ya existe: {identifier}")
            self._fetch_sensors()
            self._configure_arduino()

        # 5. Mantener socket escuchando
        socket_thread = threading.Thread(target=self.socket_handler.wait_for_events)
        socket_thread.daemon = True
        socket_thread.start()

        return True

    def stop(self):
        """🛑 Detener dispositivo completamente"""
        self.is_running = False
        
        # ✅ ÚLTIMA SINCRONIZACIÓN
        if hasattr(self, 'mongo_handler') and self.mongo_handler.is_connected():
            self.mongo_handler.sync_offline_data(self.local_storage)
        
        # ✅ PUBLICAR STATUS OFFLINE
        if self.mqtt_handler.connected and self.identifier:
            self.mqtt_handler.publish_device_status(self.identifier, "offline", self.get_device_type())
        
        # ✅ DESCONECTAR TODO
        self.socket_handler.disconnect()
        self.mqtt_handler.disconnect()
        self.mongo_handler.disconnect()
        self.arduino.disconnect()
        
        self.logger.info("🛑 Dispositivo detenido completamente")