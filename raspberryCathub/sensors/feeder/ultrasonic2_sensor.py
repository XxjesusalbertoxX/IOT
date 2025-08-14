"""
Feeder Ultrasonic Sensor 2 - Segundo sensor ultrasónico del comedero
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class FeederUltrasonic2Sensor(BaseSensor):
    """
    Segundo sensor ultrasónico del comedero
    
    Extrae SOLO sensor2 del comando "feeder_level"
    Procesa: {"sensor": "feeder_level", "altura_contenedor_cm": 45.6, "sensor2_status": "READY"}
    Guarda: UN reading con altura del contenedor en cm
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 15):
        super().__init__(identifier, "feeder_ultrasonic2", interval_seconds)
        
        # ✅ CONFIGURACIÓN ULTRASÓNICO 2
        self.ultrasonic_config = {
            "min_distance": 2,        # cm mínimo válido
            "max_distance": 100,      # cm máximo válido
            "expected_height": 50,    # cm altura esperada del contenedor
            "height_tolerance": 5     # cm tolerancia en altura
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para nivel del comedero (compartido)"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "feeder_level"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa SOLO el segundo ultrasónico
        
        Espera: {
            "sensor": "feeder_level",
            "nivel_comida_cm": 12.3,
            "altura_contenedor_cm": 45.6,
            "sensor1_status": "READY",
            "sensor2_status": "READY",
            "timestamp_ms": 12345
        }
        
        Returns:
            Altura del contenedor en cm o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "feeder_level":
                self.logger.warning("⚠️ Sensor incorrecto para feeder_level")
                return None
            
            # ✅ VERIFICAR STATUS DEL SENSOR 2
            sensor2_status = arduino_response.get("sensor2_status", "ERROR")
            if sensor2_status != "READY":
                if sensor2_status == "ERROR":
                    self.logger.error("❌ Sensor2 ultrasónico con error")
                elif sensor2_status == "TIMEOUT":
                    self.logger.warning("⏰ Sensor2 timeout")
                return None
            
            # ✅ EXTRAER ALTURA DEL CONTENEDOR (es del sensor2)
            altura_contenedor_cm = arduino_response.get("altura_contenedor_cm")
            if altura_contenedor_cm is None:
                self.logger.warning("⚠️ 'altura_contenedor_cm' no disponible")
                return None
            
            height_value = float(altura_contenedor_cm)
            
            # ✅ VALIDAR
            if not self.is_valid_value(height_value):
                return None
            
            # ✅ VERIFICAR ALTURA ESPERADA
            expected = self.ultrasonic_config["expected_height"]
            tolerance = self.ultrasonic_config["height_tolerance"]
            
            if abs(height_value - expected) > tolerance:
                self.logger.warning(
                    f"⚠️ Altura inesperada: {height_value}cm "
                    f"(esperada: {expected}±{tolerance}cm)"
                )
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(height_value)
            
            # ✅ GUARDAR
            if self.save_reading(height_value):
                self.logger.debug(f"✅ Ultrasónico2 guardado: {height_value}cm")
            
            return height_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando ultrasónico2: {e}")
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
            self.logger.warning(f"⚠️ Altura fuera de rango: {value}cm")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS
    def get_container_height_cm(self) -> Optional[float]:
        """Altura actual del contenedor en cm"""
        return self.last_reading

    def is_container_height_normal(self) -> bool:
        """¿Altura del contenedor es normal?"""
        if self.last_reading is None:
            return False
        
        expected = self.ultrasonic_config["expected_height"]
        tolerance = self.ultrasonic_config["height_tolerance"]
        
        return abs(self.last_reading - expected) <= tolerance

    def get_height_difference(self) -> Optional[float]:
        """Diferencia con altura esperada"""
        if self.last_reading is None:
            return None
        
        expected = self.ultrasonic_config["expected_height"]
        return self.last_reading - expected

    def get_container_status(self) -> str:
        """Estado del contenedor"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_container_height_normal():
            return "normal"
        else:
            diff = self.get_height_difference()
            if diff and diff > 0:
                return "too_tall"
            else:
                return "too_short"