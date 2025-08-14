"""
Litterbox Presence Sensor - Sensor ultrasónico de presencia
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class LitterboxPresenceSensor(BaseSensor):
    """
    Sensor de presencia ultrasónico del arenero
    
    Procesa: {"sensor": "litterbox_presence", "distancia_cm": 25.4, "status": "READY"}
    Guarda: UN reading con distancia en cm
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 2):
        super().__init__(identifier, "litterbox_presence", interval_seconds)
        
        # ✅ CONFIGURACIÓN ESPECÍFICA
        self.presence_config = {
            "min_distance": 2,           # cm mínimo válido
            "max_distance": 200,         # cm máximo válido
            "detection_threshold": 30,   # cm - gato presente si < 30cm
            "too_close_threshold": 5     # cm - demasiado cerca
        }
        
        # ✅ ESTADÍSTICAS DE DETECCIÓN
        self.detection_count = 0
        self.last_detection_time = None

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para presencia del arenero"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "litterbox_presence"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa respuesta ultrasónica
        
        Espera: {
            "sensor": "litterbox_presence",
            "distancia_cm": 25.4,
            "status": "READY",
            "timestamp_ms": 12345
        }
        
        Returns:
            Distancia en cm o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "litterbox_presence":
                self.logger.warning(f"⚠️ Sensor incorrecto")
                return None
            
            # ✅ VERIFICAR STATUS
            status = arduino_response.get("status", "")
            if status != "READY":
                if status == "ERROR":
                    self.logger.error("❌ Error en sensor ultrasónico")
                return None
            
            # ✅ EXTRAER DISTANCIA
            distancia_cm = arduino_response.get("distancia_cm")
            if distancia_cm is None:
                self.logger.error("❌ 'distancia_cm' no encontrado")
                return None
            
            distance_value = float(distancia_cm)
            
            # ✅ VALIDAR
            if not self.is_valid_value(distance_value):
                return None
            
            # ✅ DETECTAR PRESENCIA
            if distance_value <= self.presence_config["detection_threshold"]:
                self.detection_count += 1
                self.last_detection_time = self.last_read_time
                self.logger.info(f"🐱 Gato detectado en arenero - {distance_value}cm")
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(distance_value)
            
            # ✅ GUARDAR
            if self.save_reading(distance_value):
                self.logger.debug(f"✅ Presencia guardada: {distance_value}cm")
            
            return distance_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando presencia: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de distancia"""
        if not isinstance(value, (int, float)):
            return False
        
        config = self.presence_config
        if not (config["min_distance"] <= value <= config["max_distance"]):
            self.logger.warning(f"⚠️ Distancia fuera de rango: {value}cm")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS DE PRESENCIA
    def is_cat_present(self) -> bool:
        """¿Hay gato presente?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.presence_config["detection_threshold"]

    def get_distance_cm(self) -> Optional[float]:
        """Distancia actual en cm"""
        return self.last_reading

    def get_detection_count(self) -> int:
        """Número total de detecciones"""
        return self.detection_count

    def is_too_close(self) -> bool:
        """¿Sensor demasiado cerca? (posible error)"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.presence_config["too_close_threshold"]