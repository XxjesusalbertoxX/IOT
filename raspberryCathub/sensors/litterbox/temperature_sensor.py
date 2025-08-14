"""
Litterbox Temperature Sensor - Sensor DHT22 temperatura
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class LitterboxTemperatureSensor(BaseSensor):
    """
    Sensor de temperatura del arenero - DHT22
    
    Extrae SOLO temperatura del comando "litterbox_environment"
    Guarda: UN reading con temperatura en °C
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 5):
        super().__init__(identifier, "litterbox_temperature", interval_seconds)
        
        # ✅ CONFIGURACIÓN DE TEMPERATURA
        self.temp_config = {
            "min_valid": -20,      # °C mínima válida
            "max_valid": 50,       # °C máxima válida
            "optimal_min": 18,     # °C óptima mínima
            "optimal_max": 26,     # °C óptima máxima
            "alert_min": 10,       # °C alerta frío
            "alert_max": 35        # °C alerta calor
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para ambiente (compartido con humedad/gas)"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "litterbox_environment"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa SOLO la temperatura del ambiente
        
        Espera: {
            "sensor": "litterbox_environment",
            "temperatura_c": 22.5,
            "humedad_pct": 65.2,
            "gas_ppm": 45,
            "gas_analog": 234,
            "timestamp_ms": 12345
        }
        
        Returns:
            Temperatura en °C o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "litterbox_environment":
                self.logger.warning("⚠️ Sensor incorrecto para temperatura")
                return None
            
            # ✅ EXTRAER SOLO TEMPERATURA
            temperatura_c = arduino_response.get("temperatura_c")
            if temperatura_c is None:
                self.logger.error("❌ 'temperatura_c' no encontrada")
                return None
            
            temp_value = float(temperatura_c)
            
            # ✅ VALIDAR
            if not self.is_valid_value(temp_value):
                return None
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(temp_value)
            
            # ✅ GUARDAR
            if self.save_reading(temp_value):
                self.logger.debug(f"✅ Temperatura guardada: {temp_value}°C")
            
            return temp_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando temperatura: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de temperatura"""
        if not isinstance(value, (int, float)):
            return False
        
        config = self.temp_config
        if not (config["min_valid"] <= value <= config["max_valid"]):
            self.logger.warning(f"⚠️ Temperatura fuera de rango: {value}°C")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS DE TEMPERATURA
    def get_temperature_celsius(self) -> Optional[float]:
        """Temperatura actual en °C"""
        return self.last_reading

    def is_too_hot(self) -> bool:
        """¿Temperatura muy alta?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.temp_config["alert_max"]

    def is_too_cold(self) -> bool:
        """¿Temperatura muy baja?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.temp_config["alert_min"]

    def is_optimal_temperature(self) -> bool:
        """¿Temperatura en rango óptimo?"""
        if self.last_reading is None:
            return False
        
        config = self.temp_config
        return config["optimal_min"] <= self.last_reading <= config["optimal_max"]

    def get_temperature_status(self) -> str:
        """Estado de la temperatura"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_too_cold():
            return "too_cold"
        elif self.is_too_hot():
            return "too_hot"
        elif self.is_optimal_temperature():
            return "optimal"
        else:
            return "normal"