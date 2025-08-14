"""
Water Dispenser IR Sensor - Sensor infrarrojo de presencia
"""

import logging
import time
from typing import Dict, Any, Optional
from sensors.base_sensor import BaseSensor

class WaterIRSensor(BaseSensor):
    """
    Sensor IR del bebedero - Detección de presencia de gato
    
    Procesa: {"sensor": "water_ir", "gato_presente": true, "duracion_deteccion_ms": 2500}
    Guarda: UN reading booleano (True/False)
    """
    
    def __init__(self, identifier: str, interval_seconds: int = 3):
        super().__init__(identifier, "water_ir", interval_seconds)
        
        # ✅ CONFIGURACIÓN IR
        self.ir_config = {
            "detection_timeout": 5000,    # ms - timeout de detección
            "min_detection_time": 100,    # ms - tiempo mínimo de detección válida
            "debounce_time": 500         # ms - tiempo de debounce
        }
        
        # ✅ ESTADÍSTICAS DE DETECCIÓN
        self.detection_count = 0
        self.total_drinking_time = 0  # ms acumulado
        self.last_detection_time = None
        self.current_detection_start = None

    def get_arduino_command(self) -> Dict[str, str]:
        """Comando para sensor IR"""
        return {
            "type": "REQUEST_SENSOR_DATA",
            "sensor": "water_ir"
        }

    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[bool]:
        """
        Procesa respuesta del sensor IR
        
        Espera: {
            "sensor": "water_ir",
            "gato_presente": true,
            "duracion_deteccion_ms": 2500,
            "timestamp_ms": 12345
        }
        
        Returns:
            True si gato presente, False si no, None si error
        """
        try:
            # ✅ VERIFICAR SENSOR
            if arduino_response.get("sensor") != "water_ir":
                self.logger.warning("⚠️ Sensor incorrecto para water_ir")
                return None
            
            # ✅ EXTRAER PRESENCIA
            gato_presente = arduino_response.get("gato_presente")
            if gato_presente is None:
                self.logger.error("❌ 'gato_presente' no encontrado")
                return None
            
            # ✅ EXTRAER DURACIÓN (OPCIONAL)
            duracion_ms = arduino_response.get("duracion_deteccion_ms", 0)
            
            # ✅ CONVERTIR A BOOLEANO
            cat_detected = bool(gato_presente)
            
            # ✅ VALIDAR
            if not self.is_valid_value(cat_detected):
                return None
            
            # ✅ PROCESAR DETECCIÓN
            if cat_detected:
                self._handle_cat_detected(duracion_ms)
            else:
                self._handle_cat_not_detected()
            
            # ✅ ACTUALIZAR ESTADO
            self.update_reading_state(cat_detected)
            
            # ✅ GUARDAR BOOLEANO
            if self.save_reading(cat_detected):
                self.logger.debug(f"✅ IR guardado: {cat_detected}")
            
            return cat_detected
            
        except Exception as e:
            self.logger.error(f"❌ Error procesando IR: {e}")
            return None

    def is_valid_value(self, value: bool) -> bool:
        """Valida valor booleano"""
        return isinstance(value, bool)

    def _handle_cat_detected(self, duration_ms: int):
        """Maneja detección de gato"""
        current_time = time.time()
        
        # Primera detección o nueva sesión
        if self.current_detection_start is None:
            self.current_detection_start = current_time
            self.detection_count += 1
            self.logger.info(f"🐱 Gato empezó a beber (detección #{self.detection_count})")
        
        # Actualizar tiempo total si hay duración
        if duration_ms >= self.ir_config["min_detection_time"]:
            self.total_drinking_time += duration_ms
            self.last_detection_time = current_time
            
            # Log para duraciones significativas
            if duration_ms > 1000:  # > 1 segundo
                self.logger.info(f"🐱 Gato bebiendo - duración: {duration_ms}ms")

    def _handle_cat_not_detected(self):
        """Maneja cuando no hay detección"""
        if self.current_detection_start is not None:
            # Sesión terminada
            session_duration = time.time() - self.current_detection_start
            self.logger.info(f"🐱 Gato terminó de beber - sesión: {session_duration:.1f}s")
            self.current_detection_start = None

    # ✅ MÉTODOS ESPECÍFICOS
    def is_cat_present(self) -> bool:
        """¿Gato presente actualmente?"""
        return self.last_reading is True

    def get_detection_count(self) -> int:
        """Número total de detecciones"""
        return self.detection_count

    def get_total_drinking_time_ms(self) -> int:
        """Tiempo total de bebida acumulado (ms)"""
        return self.total_drinking_time

    def get_total_drinking_time_seconds(self) -> float:
        """Tiempo total de bebida acumulado (segundos)"""
        return self.total_drinking_time / 1000.0

    def is_currently_drinking(self) -> bool:
        """¿Está bebiendo actualmente?"""
        return self.current_detection_start is not None

    def get_current_session_duration(self) -> Optional[float]:
        """Duración de la sesión actual (segundos)"""
        if self.current_detection_start is None:
            return None
        return time.time() - self.current_detection_start

    def get_last_detection_time(self) -> Optional[float]:
        """Timestamp de la última detección"""
        return self.last_detection_time

    def get_drinking_statistics(self) -> Dict[str, Any]:
        """Estadísticas de bebida"""
        return {
            "total_detections": self.detection_count,
            "total_drinking_time_ms": self.total_drinking_time,
            "total_drinking_time_seconds": self.get_total_drinking_time_seconds(),
            "currently_drinking": self.is_currently_drinking(),
            "current_session_duration": self.get_current_session_duration(),
            "last_detection_time": self.last_detection_time
        }

    def reset_statistics(self):
        """Resetea estadísticas de detección"""
        self.detection_count = 0
        self.total_drinking_time = 0
        self.last_detection_time = None
        self.current_detection_start = None
        self.logger.info("🔄 Estadísticas IR reseteadas")