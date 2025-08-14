"""
Water Dispenser Level Sensor - Sensor de nivel de agua
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class WaterLevelSensor(BaseSensor):
    """
    Sensor de nivel de agua del bebedero
    
    Procesa: {"sensor": "water_level", "nivel_analogico": 456, "nivel_categoria": "WET", "agua_detectada": true}
    Guarda: UN reading con valor analógico
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 10):
        super().__init__(identifier, "water_level", interval_seconds)
        
        # ✅ CONFIGURACIÓN NIVEL DE AGUA
        self.water_config = {
            "analog_range": {
                "min": 0,        # Valor analógico mínimo
                "max": 1023      # Valor analógico máximo
            },
            "level_thresholds": {
                "dry": 100,      # < 100 = DRY
                "damp": 300,     # 100-300 = DAMP
                "wet": 700,      # 300-700 = WET
                "flood": 700     # > 700 = FLOOD
            },
            "categories": ["DRY", "DAMP", "WET", "FLOOD"]
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para nivel de agua"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "water_level"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa respuesta del nivel de agua
        
        Espera: {
            "sensor": "water_level",
            "nivel_analogico": 456,
            "nivel_categoria": "WET",
            "agua_detectada": true,
            "timestamp_ms": 12345
        }
        
        Returns:
            Valor analógico o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "water_level":
                self.logger.warning("⚠️ Sensor incorrecto para water_level")
                return None
            
            # ✅ EXTRAER VALOR ANALÓGICO
            nivel_analogico = arduino_response.get("nivel_analogico")
            if nivel_analogico is None:
                self.logger.error("❌ 'nivel_analogico' no encontrado")
                return None
            
            analog_value = float(nivel_analogico)
            
            # ✅ EXTRAER INFORMACIÓN ADICIONAL
            categoria = arduino_response.get("nivel_categoria", "UNKNOWN")
            agua_detectada = arduino_response.get("agua_detectada", False)
            
            # ✅ VALIDAR
            if not self.is_valid_value(analog_value):
                return None
            
            # ✅ LOG SEGÚN CATEGORÍA
            if categoria == "DRY":
                self.logger.warning(f"🏜️ AGUA SECA: {analog_value}")
            elif categoria == "DAMP":
                self.logger.info(f"💧 Agua baja: {analog_value}")
            elif categoria == "FLOOD":
                self.logger.error(f"🌊 INUNDACIÓN: {analog_value}")
            else:
                self.logger.debug(f"💧 Agua {categoria}: {analog_value}")
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(analog_value)
            
            # ✅ GUARDAR VALOR ANALÓGICO
            if self.save_reading(analog_value):
                self.logger.debug(f"✅ Water level guardado: {analog_value}")
            
            return analog_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando water level: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida valor analógico"""
        if not isinstance(value, (int, float)):
            return False
        
        range_config = self.water_config["analog_range"]
        if not (range_config["min"] <= value <= range_config["max"]):
            self.logger.warning(f"⚠️ Valor analógico fuera de rango: {value}")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS
    def get_analog_value(self) -> Optional[float]:
        """Valor analógico actual"""
        return self.last_reading

    def get_water_category(self) -> str:
        """Categoría del agua basada en valor analógico"""
        if self.last_reading is None:
            return "UNKNOWN"
        
        thresholds = self.water_config["level_thresholds"]
        value = self.last_reading
        
        if value <= thresholds["dry"]:
            return "DRY"
        elif value <= thresholds["damp"]:
            return "DAMP"
        elif value <= thresholds["wet"]:
            return "WET"
        else:
            return "FLOOD"

    def is_water_dry(self) -> bool:
        """¿Agua seca?"""
        return self.get_water_category() == "DRY"

    def is_water_low(self) -> bool:
        """¿Agua baja?"""
        category = self.get_water_category()
        return category in ["DRY", "DAMP"]

    def is_water_normal(self) -> bool:
        """¿Agua normal?"""
        return self.get_water_category() == "WET"

    def is_water_flood(self) -> bool:
        """¿Inundación?"""
        return self.get_water_category() == "FLOOD"

    def needs_refill(self) -> bool:
        """¿Necesita rellenado?"""
        return self.is_water_low()

    def needs_emergency_action(self) -> bool:
        """¿Necesita acción de emergencia?"""
        return self.is_water_dry() or self.is_water_flood()

    def get_water_percentage(self) -> Optional[float]:
        """Porcentaje de agua aproximado (0-100%)"""
        if self.last_reading is None:
            return None
        
        thresholds = self.water_config["level_thresholds"]
        value = self.last_reading
        
        if value <= thresholds["dry"]:
            return 0.0
        elif value <= thresholds["damp"]:
            # 0% - 25%
            range_size = thresholds["damp"] - thresholds["dry"]
            progress = (value - thresholds["dry"]) / range_size
            return progress * 25.0
        elif value <= thresholds["wet"]:
            # 25% - 75%
            range_size = thresholds["wet"] - thresholds["damp"]
            progress = (value - thresholds["damp"]) / range_size
            return 25.0 + (progress * 50.0)
        else:
            # 75% - 100%+
            return min(100.0, 75.0 + ((value - thresholds["wet"]) / 100.0))

    def get_water_status(self) -> str:
        """Estado del agua"""
        category = self.get_water_category()
        return category.lower()