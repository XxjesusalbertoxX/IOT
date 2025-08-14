"""
Sensor Manager - COORDINADOR SIMPLE del sistema
Solo orquesta sensores y componentes
"""

import time
import logging
from typing import Dict, Any
from sensors.feeder.weight_sensor import FeederWeightSensor
from sensors.feeder.ultrasonic1_sensor import FeederUltrasonic1Sensor
from sensors.feeder.ultrasonic2_sensor import FeederUltrasonic2Sensor
from sensors.litterbox.presence_sensor import LitterboxPresenceSensor
from sensors.litterbox.temperature_sensor import LitterboxTemperatureSensor
from sensors.litterbox.humidity_sensor import LitterboxHumiditySensor
from sensors.litterbox.gas_sensor import LitterboxGasSensor
from sensors.waterdispenser.water_level_sensor import WaterLevelSensor
from sensors.waterdispenser.ir_sensor import WaterIRSensor

from database.mongo_handler import MongoHandler
from database.local_storage import LocalStorage
from communication.arduino_serial import ArduinoSerial

class SensorManager:
    """
    COORDINADOR SIMPLE - Solo orquesta componentes con responsabilidades claras
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        
        # ✅ COMPONENTES CON RESPONSABILIDADES CLARAS
        self.arduino_serial = ArduinoSerial()
        self.mongo_handler = MongoHandler()
        self.local_storage = LocalStorage()
        
        # ✅ SENSORES ESPECIALIZADOS
        self.sensors = {
            # 🍽️ COMEDERO (3 sensores)
            "feeder_weight": FeederWeightSensor("WSR001", 20),
            "feeder_ultrasonic1": FeederUltrasonic1Sensor("ULT001", 15),
            "feeder_ultrasonic2": FeederUltrasonic2Sensor("ULT002", 15),
            
            # 📦 ARENERO (4 sensores)
            "litterbox_presence": LitterboxPresenceSensor("PRS001", 2),
            "litterbox_temperature": LitterboxTemperatureSensor("TEMP001", 5),
            "litterbox_humidity": LitterboxHumiditySensor("HUM001", 5),
            "litterbox_gas": LitterboxGasSensor("GAS001", 5),
            
            # 💧 BEBEDERO (2 sensores)
            "water_level": WaterLevelSensor("WTR001", 10),
            "water_ir": WaterIRSensor("IR001", 3),
        }
        
        self.initialized = False
        self.cycle_count = 0

    def initialize(self) -> bool:
        """Inicializa componentes con dependencias claras"""
        try:
            self.logger.info("🔧 Inicializando sistema...")
            
            # ✅ 1. CONECTAR COMPONENTES BÁSICOS
            self.arduino_serial.connect()
            self.mongo_handler.connect()
            self.local_storage.initialize()
            
            # ✅ 2. ESTABLECER RELACIONES ENTRE COMPONENTES
            self.mongo_handler.set_local_storage(self.local_storage)
            
            # ✅ 3. CONFIGURAR SENSORES CON HANDLERS
            for sensor in self.sensors.values():
                sensor.set_database_handlers(self.mongo_handler, self.local_storage)
            
            self.initialized = True
            self.logger.info("✅ Sistema inicializado con responsabilidades claras")
            return True
            
        except Exception as e:
            self.logger.error(f"❌ Error inicializando: {e}")
            return False

    def run_cycle(self):
        """Ejecuta un ciclo simple de monitoreo"""
        if not self.initialized:
            return
        
        current_time = time.time()
        
        # ✅ PROCESAR CADA SENSOR INDEPENDIENTEMENTE
        for sensor_name, sensor in self.sensors.items():
            try:
                if sensor.should_read(current_time):
                    self._process_sensor(sensor_name, sensor)
            except Exception as e:
                self.logger.error(f"❌ Error en sensor {sensor_name}: {e}")
        
        # ✅ SYNC OFFLINE OCASIONALMENTE
        if self.cycle_count % 1000 == 0:
            self._sync_offline()
        
        self.cycle_count += 1

    def _process_sensor(self, sensor_name: str, sensor):
        """
        Procesa un sensor individual con DELEGACIÓN CLARA
        """
        try:
            # ✅ 1. ARDUINO_SERIAL se encarga de comunicación
            command = sensor.get_arduino_command()
            response = self.arduino_serial.request_sensor_data(command)
            
            if not response:
                return
            
            # ✅ 2. SENSOR procesa datos y guarda (delegando a MongoHandler)
            value = sensor.process_data(response)
            if value is not None:
                # ✅ EL SENSOR YA GUARDÓ USANDO MongoHandler.save_sensor_reading()
                pass
                
        except Exception as e:
            self.logger.error(f"❌ Error procesando {sensor_name}: {e}")

    def _sync_offline(self):
        """
        Sincroniza datos offline - DELEGACIÓN A MongoHandler
        """
        try:
            if self.mongo_handler.is_connected():
                synced = self.mongo_handler.sync_offline_data(self.local_storage)
                if synced > 0:
                    self.logger.info(f"🔄 {synced} datos sincronizados")
        except Exception as e:
            self.logger.error(f"❌ Error sync: {e}")

    def get_status(self) -> Dict[str, Any]:
        """Estado simple del sistema"""
        return {
            "initialized": self.initialized,
            "cycle_count": self.cycle_count,
            "sensors": len(self.sensors),
            "arduino_connected": self.arduino_serial.is_connected() if hasattr(self.arduino_serial, 'is_connected') else False,
            "mongo_connected": self.mongo_handler.is_connected(),
            "components_status": {
                "mongo_handler": self.mongo_handler.get_status(),
                "local_storage": self.local_storage.get_status() if hasattr(self.local_storage, 'get_status') else {}
            }
        }