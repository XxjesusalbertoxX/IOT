"""
Base Sensor - Clase base para TODOS los sensores
Contiene métodos comunes y define interfaz común
"""

import time
import logging
from abc import ABC, abstractmethod
from typing import Dict, Any, Optional, List, Union
from datetime import datetime

class BaseSensor(ABC):
    """
    Clase base abstracta para todos los sensores
    
    Responsabilidades comunes:
    1. Manejo de intervalos de lectura
    2. Estado interno básico
    3. Referencias a handlers de BD
    4. Interfaz común para todos los sensores
    
    NO SE RESPONSABILIZA DE:
    - Guardar datos (delegado a MongoHandler)
    - Lógica de conexión (delegado a handlers)
    """
    
    def __init__(self, identifier: str, sensor_name: str, interval_seconds: int = 10):
        self.identifier = identifier          # "WSR001", "PRS001", etc.
        self.sensor_name = sensor_name       # "feeder_weight", "litterbox_presence", etc.
        self.interval_seconds = interval_seconds
        self.logger = logging.getLogger(f"{self.__class__.__name__}-{identifier}")
        
        # ✅ ESTADO COMÚN
        self.last_reading = None
        self.last_read_time = 0
        self.device_id = None
        self.reading_count = 0
        self.initialized = False
        
        # ✅ REFERENCIAS A HANDLERS (se asignan después)
        self.mongo_handler = None
        self.local_storage = None
        
        self.logger.info(f"✅ {self.__class__.__name__} creado - {identifier}")

    def set_database_handlers(self, mongo_handler, local_storage):
        """
        Asigna handlers de base de datos
        
        Args:
            mongo_handler: MongoHandler instance
            local_storage: LocalStorage instance
        """
        self.mongo_handler = mongo_handler
        self.local_storage = local_storage
        
        # ✅ OBTENER device_id USANDO EL HANDLER
        if mongo_handler:
            self.device_id = mongo_handler.get_device_id(self.identifier)
            if self.device_id:
                self.logger.info(f"✅ Device ID: {self.device_id}")
            else:
                self.logger.warning(f"⚠️ Device ID no encontrado para {self.identifier}")
        
        self.initialized = True

    def should_read(self, current_time: float) -> bool:
        """
        Verifica si es momento de leer el sensor
        
        Args:
            current_time: Timestamp actual
            
        Returns:
            True si debe leer
        """
        return (current_time - self.last_read_time) >= self.interval_seconds

    def create_document(self, value: Union[float, int, bool, str]) -> Dict[str, Any]:
        """
        Crea documento para MongoDB - FORMATO INDIVIDUAL
        
        Args:
            value: Valor único del sensor
            
        Returns:
            Documento en formato MongoDB
        """
        return {
            "sensor_name": self.sensor_name,
            "identifier": self.identifier,
            "value": value,                    # ✅ VALOR ÚNICO
            "timestamp": datetime.utcnow(),
            "device_id": self.device_id or 1
        }

    def save_reading(self, value: Union[float, int, bool, str]) -> bool:
        """
        ✅ DELEGACIÓN PURA - Solo crea documento y delega al MongoHandler
        
        Args:
            value: Valor único del sensor
            
        Returns:
            True si MongoHandler lo guardó correctamente
        """
        if not self.initialized:
            self.logger.error("❌ Sensor no inicializado")
            return False
        
        try:
            # ✅ CREAR DOCUMENTO
            document = self.create_document(value)
            
            # ✅ DELEGACIÓN PURA AL MONGO_HANDLER
            if self.mongo_handler:
                return self.mongo_handler.save_sensor_reading(document)
            else:
                self.logger.error("❌ MongoHandler no asignado")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Error en save_reading: {e}")
            return False

    def get_status(self) -> Dict[str, Any]:
        """Estado común del sensor"""
        return {
            "sensor_name": self.sensor_name,
            "identifier": self.identifier,
            "interval_seconds": self.interval_seconds,
            "last_reading": self.last_reading,
            "last_read_time": self.last_read_time,
            "reading_count": self.reading_count,
            "device_id": self.device_id,
            "initialized": self.initialized,
            "time_until_next_read": max(0, self.interval_seconds - (time.time() - self.last_read_time))
        }

    def reset_reading_timer(self):
        """Resetea timer para forzar próxima lectura"""
        self.last_read_time = 0
        self.logger.info("🔄 Timer reseteado")

    def update_reading_state(self, value: Union[float, int, bool, str]):
        """
        Actualiza estado interno después de lectura exitosa
        
        Args:
            value: Valor leído
        """
        self.last_reading = value
        self.last_read_time = time.time()
        self.reading_count += 1

    # ✅ MÉTODOS ABSTRACTOS - CADA SENSOR DEBE IMPLEMENTAR
    @abstractmethod
    def get_arduino_command(self) -> Dict[str, str]:
        """
        Comando JSON para solicitar datos al Arduino
        
        Returns:
            Diccionario con comando
        """
        pass

    @abstractmethod
    def process_data(self, arduino_response: Dict[str, Any]) -> Optional[Union[float, int, bool, str]]:
        """
        Procesa respuesta del Arduino y retorna VALOR ÚNICO
        
        Args:
            arduino_response: Respuesta JSON del Arduino
            
        Returns:
            Valor único procesado o None si error
        """
        pass

    @abstractmethod
    def is_valid_value(self, value: Union[float, int, bool, str]) -> bool:
        """
        Valida si el valor está en rango/formato correcto
        
        Args:
            value: Valor a validar
            
        Returns:
            True si es válido
        """
        pass

    def __str__(self) -> str:
        """Representación string"""
        status = "inicializado" if self.initialized else "no inicializado"
        return f"{self.__class__.__name__}({self.identifier}, {status})"

    def __repr__(self) -> str:
        """Representación detallada"""
        return (f"{self.__class__.__name__}(identifier='{self.identifier}', "
                f"sensor_name='{self.sensor_name}', "
                f"interval={self.interval_seconds}, "
                f"device_id={self.device_id})")