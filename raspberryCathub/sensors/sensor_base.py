"""
Sensor Base Handler
Clase base para todos los manejadores de sensores
"""

import json
import time
import logging
from typing import Dict, Any, Optional
from datetime import datetime
from abc import ABC, abstractmethod

class SensorHandler(ABC):
    """
    Clase base abstracta para manejadores de sensores
    Define la interfaz común para todos los sensores
    """
    
    def __init__(self, sensor_type: str, config: Dict[str, Any]):
        self.sensor_type = sensor_type
        self.config = config
        self.is_active_flag = True
        self.last_update = None
        self.total_readings = 0
        self.error_count = 0
        self.start_time = time.time()
        
        logging.info(f"[{self.sensor_type.upper()}Handler] Inicializado")

    @abstractmethod
    def process_sensor_data(self, raw_data: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Procesa los datos crudos del sensor
        
        Args:
            raw_data: Datos recibidos del Arduino
            
        Returns:
            Datos procesados para enviar a MongoDB o None si hay error
        """
        pass

    def update_statistics(self, success: bool = True):
        """
        Actualiza las estadísticas del sensor
        
        Args:
            success: True si la lectura fue exitosa, False si hubo error
        """
        self.last_update = datetime.utcnow().isoformat()
        self.total_readings += 1
        
        if not success:
            self.error_count += 1

    def is_active(self) -> bool:
        """
        Indica si el sensor está activo
        
        Returns:
            True si está activo, False caso contrario
        """
        return self.is_active_flag

    def set_active(self, active: bool):
        """
        Establece el estado activo/inactivo del sensor
        
        Args:
            active: True para activar, False para desactivar
        """
        self.is_active_flag = active
        status = "activado" if active else "desactivado"
        logging.info(f"[{self.sensor_type.upper()}Handler] Sensor {status}")

    def get_uptime_seconds(self) -> float:
        """
        Retorna el tiempo que el sensor ha estado ejecutándose
        
        Returns:
            Tiempo de ejecución en segundos
        """
        return time.time() - self.start_time

    def get_error_rate(self) -> float:
        """
        Calcula la tasa de error del sensor
        
        Returns:
            Tasa de error como porcentaje (0.0 a 100.0)
        """
        if self.total_readings == 0:
            return 0.0
        
        return (self.error_count / self.total_readings) * 100.0

    def reset_statistics(self):
        """
        Reinicia las estadísticas del sensor
        """
        self.total_readings = 0
        self.error_count = 0
        self.start_time = time.time()
        logging.info(f"[{self.sensor_type.upper()}Handler] Estadísticas reiniciadas")

    def get_base_status(self) -> Dict[str, Any]:
        """
        Retorna información básica de estado común a todos los sensores
        
        Returns:
            Diccionario con información básica del sensor
        """
        return {
            "sensor_type": self.sensor_type,
            "is_active": self.is_active(),
            "uptime_seconds": self.get_uptime_seconds(),
            "total_readings": self.total_readings,
            "error_count": self.error_count,
            "error_rate_percent": self.get_error_rate(),
            "last_update": self.last_update
        }
