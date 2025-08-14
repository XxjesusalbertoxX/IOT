"""
Feeder Weight Sensor - Sensor de peso del comedero
Hereda de BaseSensor y especializado en datos HX711
"""

import logging
from typing import Dict, Any, Optional, Union
from sensors.base_sensor import BaseSensor

class FeederWeightSensor(BaseSensor):
    """
    Sensor de peso del comedero - HX711
    
    Procesa: {"sensor": "feeder_weight", "peso_gramos": 150.5, "status": "READY"}
    Guarda: UN reading con valor en gramos
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 20):
        super().__init__(identifier, "feeder_weight", interval_seconds)
        
        # ✅ CONFIGURACIÓN ESPECÍFICA DE PESO
        self.weight_config = {
            "min_valid": 0,        # gramos mínimos
            "max_valid": 5000,     # gramos máximos (5kg)
            "precision": 0.1,      # precisión HX711
            "empty_threshold": 10, # comedero vacío
            "low_threshold": 50    # comida baja
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para peso del comedero"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "feeder_weight"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa respuesta del Arduino
        
        Espera: {
            "sensor": "feeder_weight",
            "peso_gramos": 150.5,
            "status": "READY",
            "timestamp_ms": 12345
        }
        
        Returns:
            Peso en gramos o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "feeder_weight":
                self.logger.warning(f"⚠️ Sensor incorrecto: {arduino_response.get('sensor')}")
                return None
            
            # ✅ VERIFICAR STATUS
            status = arduino_response.get("status", "")
            if status != "READY":
                if status == "ERROR":
                    self.logger.error("❌ Arduino reporta error en HX711")
                elif status == "NOT_READY":
                    self.logger.warning("⚠️ HX711 calibrando...")
                return None
            
            # ✅ EXTRAER PESO
            peso_gramos = arduino_response.get("peso_gramos")
            if peso_gramos is None:
                self.logger.error("❌ 'peso_gramos' no encontrado")
                return None
            
            weight_value = float(peso_gramos)
            
            # ✅ VALIDAR
            if not self.is_valid_value(weight_value):
                return None
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(weight_value)
            
            # ✅ GUARDAR AUTOMÁTICAMENTE
            if self.save_reading(weight_value):
                self.logger.debug(f"✅ Peso guardado: {weight_value}g")
            
            return weight_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando peso: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de peso"""
        if not isinstance(value, (int, float)):
            return False
        
        if not (self.weight_config["min_valid"] <= value <= self.weight_config["max_valid"]):
            self.logger.warning(f"⚠️ Peso fuera de rango: {value}g")
            return False
        
        # Detectar cambios extremos
        if self.last_reading is not None:
            change = abs(value - self.last_reading)
            if change > 1000:  # 1kg cambio máximo
                self.logger.warning(f"⚠️ Cambio extremo: {self.last_reading}g → {value}g")
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS DEL SENSOR DE PESO
    def is_empty(self) -> bool:
        """¿Comedero vacío?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.weight_config["empty_threshold"]

    def is_low(self) -> bool:
        """¿Comida baja?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.weight_config["low_threshold"]

    def get_weight_grams(self) -> Optional[float]:
        """Peso actual en gramos"""
        return self.last_reading

    def get_weight_status(self) -> str:
        """Estado del peso (empty/low/normal)"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_empty():
            return "empty"
        elif self.is_low():
            return "low"
        else:
            return "normal"