"""
Litterbox Humidity Sensor - Sensor DHT22 humedad
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class LitterboxHumiditySensor(BaseSensor):
    """
    Sensor de humedad del arenero - DHT22
    
    Extrae SOLO humedad del comando "litterbox_environment"
    Procesa: {"sensor": "litterbox_environment", "humedad_pct": 65.2}
    Guarda: UN reading con humedad en %
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 5):
        super().__init__(identifier, "litterbox_humidity", interval_seconds)
        
        # ✅ CONFIGURACIÓN DE HUMEDAD
        self.humidity_config = {
            "min_valid": 0,        # % mínima válida
            "max_valid": 100,      # % máxima válida
            "optimal_min": 40,     # % óptima mínima
            "optimal_max": 60,     # % óptima máxima
            "alert_min": 20,       # % alerta seco
            "alert_max": 80        # % alerta húmedo
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para ambiente (compartido)"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "litterbox_environment"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa SOLO la humedad del ambiente
        
        Espera: {
            "sensor": "litterbox_environment",
            "temperatura_c": 22.5,
            "humedad_pct": 65.2,
            "gas_ppm": 45,
            "gas_analog": 234,
            "timestamp_ms": 12345
        }
        
        Returns:
            Humedad en % o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "litterbox_environment":
                self.logger.warning("⚠️ Sensor incorrecto para humedad")
                return None
            
            # ✅ EXTRAER SOLO HUMEDAD
            humedad_pct = arduino_response.get("humedad_pct")
            if humedad_pct is None:
                self.logger.warning("⚠️ 'humedad_pct' no disponible (DHT22 error?)")
                return None
            
            humidity_value = float(humedad_pct)
            
            # ✅ VALIDAR
            if not self.is_valid_value(humidity_value):
                return None
            
            # ✅ LOG ALERTAS
            if humidity_value <= self.humidity_config["alert_min"]:
                self.logger.warning(f"🏜️ MUY SECO: {humidity_value}%")
            elif humidity_value >= self.humidity_config["alert_max"]:
                self.logger.warning(f"💧 MUY HÚMEDO: {humidity_value}%")
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(humidity_value)
            
            # ✅ GUARDAR
            if self.save_reading(humidity_value):
                self.logger.debug(f"✅ Humedad guardada: {humidity_value}%")
            
            return humidity_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando humedad: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de humedad"""
        if not isinstance(value, (int, float)):
            return False
        
        config = self.humidity_config
        if not (config["min_valid"] <= value <= config["max_valid"]):
            self.logger.warning(f"⚠️ Humedad fuera de rango: {value}%")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS
    def get_humidity_percentage(self) -> Optional[float]:
        """Humedad actual en %"""
        return self.last_reading

    def is_too_dry(self) -> bool:
        """¿Muy seco?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.humidity_config["alert_min"]

    def is_too_humid(self) -> bool:
        """¿Muy húmedo?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.humidity_config["alert_max"]

    def is_optimal_humidity(self) -> bool:
        """¿Humedad en rango óptimo?"""
        if self.last_reading is None:
            return False
        
        config = self.humidity_config
        return config["optimal_min"] <= self.last_reading <= config["optimal_max"]

    def get_humidity_status(self) -> str:
        """Estado de la humedad"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_too_dry():
            return "too_dry"
        elif self.is_too_humid():
            return "too_humid"
        elif self.is_optimal_humidity():
            return "optimal"
        else:
            return "normal"

    def needs_ventilation(self) -> bool:
        """¿Necesita ventilación?"""
        return self.is_too_humid()

    def needs_humidification(self) -> bool:
        """¿Necesita humidificación?"""
        return self.is_too_dry()