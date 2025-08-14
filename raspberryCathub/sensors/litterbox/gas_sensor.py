"""
Litterbox Gas Sensor - Sensor MQ2 gas
"""

import logging
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class LitterboxGasSensor(BaseSensor):
    """
    Sensor de gas del arenero - MQ2
    
    Extrae SOLO gas del comando "litterbox_environment"
    Procesa: {"sensor": "litterbox_environment", "gas_ppm": 45, "gas_analog": 234}
    Guarda: UN reading con gas en PPM
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 5):
        super().__init__(identifier, "litterbox_gas", interval_seconds)
        
        # ✅ CONFIGURACIÓN DE GAS MQ2
        self.gas_config = {
            "min_valid": 0,          # ppm mínimo
            "max_valid": 10000,      # ppm máximo
            "normal_max": 100,       # ppm normal
            "warning_level": 500,    # ppm advertencia
            "danger_level": 1000,    # ppm peligro
            "critical_level": 2000   # ppm crítico
        }

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para ambiente (compartido)"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "litterbox_environment"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[float]:
        """
        Procesa SOLO el gas del ambiente
        
        Espera: {
            "sensor": "litterbox_environment",
            "temperatura_c": 22.5,
            "humedad_pct": 65.2,
            "gas_ppm": 45,
            "gas_analog": 234,
            "timestamp_ms": 12345
        }
        
        Returns:
            Gas en PPM o None
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "litterbox_environment":
                self.logger.warning("⚠️ Sensor incorrecto para gas")
                return None
            
            # ✅ EXTRAER SOLO GAS
            gas_ppm = arduino_response.get("gas_ppm")
            if gas_ppm is None:
                self.logger.error("❌ 'gas_ppm' no encontrado")
                return None
            
            gas_value = float(gas_ppm)
            
            # ✅ VALIDAR
            if not self.is_valid_value(gas_value):
                return None
            
            # ✅ LOG NIVELES CRÍTICOS
            if gas_value >= self.gas_config["critical_level"]:
                self.logger.error(f"☠️ GAS CRÍTICO: {gas_value}ppm")
            elif gas_value >= self.gas_config["danger_level"]:
                self.logger.error(f"☢️ GAS PELIGROSO: {gas_value}ppm")
            elif gas_value >= self.gas_config["warning_level"]:
                self.logger.warning(f"⚠️ Gas elevado: {gas_value}ppm")
            elif gas_value > self.gas_config["normal_max"]:
                self.logger.info(f"📊 Gas sobre normal: {gas_value}ppm")
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(gas_value)
            
            # ✅ GUARDAR
            if self.save_reading(gas_value):
                self.logger.debug(f"✅ Gas guardado: {gas_value}ppm")
            
            return gas_value
            
        except (ValueError, TypeError) as e:
            self.logger.error(f"❌ Error procesando gas: {e}")
            return None
        except Exception as e:
            self.logger.error(f"❌ Error en process_data: {e}")
            return None

    def is_valid_value(self, value: float) -> bool:
        """Valida rango de gas"""
        if not isinstance(value, (int, float)):
            return False
        
        config = self.gas_config
        if not (config["min_valid"] <= value <= config["max_valid"]):
            self.logger.warning(f"⚠️ Gas fuera de rango: {value}ppm")
            return False
        
        return True

    # ✅ MÉTODOS ESPECÍFICOS
    def get_gas_ppm(self) -> Optional[float]:
        """Gas actual en PPM"""
        return self.last_reading

    def is_gas_normal(self) -> bool:
        """¿Gas en nivel normal?"""
        if self.last_reading is None:
            return False
        return self.last_reading <= self.gas_config["normal_max"]

    def is_gas_warning(self) -> bool:
        """¿Gas en nivel de advertencia?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.gas_config["warning_level"]

    def is_gas_dangerous(self) -> bool:
        """¿Gas en nivel peligroso?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.gas_config["danger_level"]

    def is_gas_critical(self) -> bool:
        """¿Gas en nivel crítico?"""
        if self.last_reading is None:
            return False
        return self.last_reading >= self.gas_config["critical_level"]

    def get_gas_status(self) -> str:
        """Estado del gas"""
        if self.last_reading is None:
            return "unknown"
        elif self.is_gas_critical():
            return "critical"
        elif self.is_gas_dangerous():
            return "dangerous"
        elif self.is_gas_warning():
            return "warning"
        elif self.is_gas_normal():
            return "normal"
        else:
            return "elevated"

    def needs_emergency_stop(self) -> bool:
        """¿Necesita parada de emergencia?"""
        return self.is_gas_dangerous() or self.is_gas_critical()

    def needs_ventilation(self) -> bool:
        """¿Necesita ventilación?"""
        return self.is_gas_warning() or self.needs_emergency_stop()

    def get_gas_risk_level(self) -> int:
        """
        Nivel de riesgo del gas (0-4)
        0: Normal, 1: Elevado, 2: Warning, 3: Peligroso, 4: Crítico
        """
        if self.last_reading is None:
            return 0
        elif self.is_gas_critical():
            return 4
        elif self.is_gas_dangerous():
            return 3
        elif self.is_gas_warning():
            return 2
        elif self.is_gas_normal():
            return 0
        else:
            return 1