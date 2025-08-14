"""
Feeder Ultrasonic Sensor 1 - Primer sensor ultrasónico del comedero
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class FeederUltrasonic1Sensor(BaseSensor):
    """
    Primer sensor ultrasónico del comedero
    
    Extrae SOLO sensor1 del comando "feeder_level"
    Procesa: {"sensor": "feeder_level", "nivel_comida_cm": 12.3, "sensor1_status": "READY"}
    Guarda: UN reading con distancia en cm
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 15):
        super().__init__(identifier, "feeder_ultrasonic1", interval_seconds)
        
        # ✅ CONFIGURACIÓN ULTRASÓNICO
        self.ultrasonic_config = {
            "min_distance": 2,      # cm mínimo válido
            "max_distance": 100,    # cm máximo válido
            "container_height": 50, # cm altura total contenedor
            "empty_threshold": 5,   # cm - contenedor vacío
            "low_threshold": 10,    # cm - comida baja
            "full_threshold": 45    # cm - contenedor lleno
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para nivel del comedero"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "feeder_level"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa SOLO el primer ultrasónico
        
        Espera: {
            "sensor": "feeder_level",
            "nivel_comida_cm": 12.3,
            "altura_contenedor_cm": 45.6,
            "sensor1_status": "READY",
            "sensor2_status": "READY", 
            "timestamp_ms": 12345
        }
        
        Returns:
            Distancia en cm o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "feeder_level":
                self.logger.warning("⚠️ Sensor incorrecto para feeder_level")
                return None
            
            # ✅ VERIFICAR STATUS DEL SENSOR 1
            sensor1_status = arduino_response.get("sensor1_status", "ERROR")
            if sensor1_status != "READY":
                if sensor1_status == "ERROR":
                    self.logger.error("❌ Sensor1 ultrasónico con error")
                elif sensor1_status == "TIMEOUT":
                    self.logger.warning("⏰ Sensor1 timeout")
                return None
            
            # ✅ EXTRAER NIVEL DE COMIDA (es del sensor1)
            nivel_comida_cm = arduino_response.get("nivel_comida_cm")
            if nivel_comida_cm is None:
                self.logger.error("❌ 'nivel_comida_cm' no encontrado")
                return None
            
            distance_value = float(nivel_comida_cm)
            
            # ✅ VALIDAR
            if not self.is_valid_value(distance_value):
                return None
            
            # ✅ LOG NIVELES CRÍTICOS
            if distance_value <= self.ultrasonic_config["empty_threshold"]:
                self.logger.error(f"🍽️ CONTENEDOR VACÍO: {distance_value}cm")
            elif distance_value <= self.ultrasonic_config["low_threshold"]:
                self.logger.warning(f"🍽️ COMIDA BAJA: {distance_value}cm")
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(distance_value)
            
            # ✅ GUARDAR
            if self.save_reading(distance_value):
                self.logger.debug(f"✅ Ultrasónico1 guardado: {distance_value}cm")
            
            return distance_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando ultrasónico1: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de distancia"""
        if not isinstance(value, (int, float)):
            return False
        
        config = self.ultrasonic_config
        if not (config["min_distance"] <= value <= config["max_distance"]):
            self.logger.warning(f"⚠️ Distancia fuera de rango: {value}cm")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS
    def get_distance_cm(self) -> Optional[float]:
        """Distancia actual en cm"""
        return self.last_reading

    def is_container_empty(self) -> bool:
        """¿Contenedor vacío?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.ultrasonic_config["empty_threshold"]

    def is_food_low(self) -> bool:
        """¿Comida baja?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.ultrasonic_config["low_threshold"]

    def is_container_full(self) -> bool:
        """¿Contenedor lleno?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.ultrasonic_config["full_threshold"]

    def get_food_percentage(self) -> Optional[float]:
        """Porcentaje de comida (0-100%)"""
        if self.last_reading is None:
            return None
        
        # Lógica invertida: menos distancia = más comida
        total_height = self.ultrasonic_config["container_height"]
        food_height = total_height - self.last_reading
        
        if food_height <= 0:
            return 0.0
        
        percentage = (food_height / total_height) * 100.0
        return max(0.0, min(100.0, percentage))

    def get_food_status(self) -> str:
        """Estado de la comida"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_container_empty():
            return "empty"
        elif self.is_food_low():
            return "low"
        elif self.is_container_full():
            return "full"
        else:
            return "normal"