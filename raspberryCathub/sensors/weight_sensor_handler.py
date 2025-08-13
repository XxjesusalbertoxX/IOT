"""
Weight Sensor Handler - Raspberry Pi
Maneja la comunicación y procesamiento de datos del sensor de peso del Arduino
"""

import json
import time
import logging
from typing import Dict, Any, Optional
from datetime import datetime
from .sensor_base import SensorHandler

class WeightSensorHandler(SensorHandler):
    """
    Handler especializado para el sensor de peso
    Procesa datos JSON del Arduino y los formatea para MongoDB
    """
    
    def __init__(self, config: Dict[str, Any]):
        super().__init__("weight", config)
        self.last_weight = 0.0
        self.last_stable_weight = 0.0
        self.weight_threshold = config.get("weight_threshold", 5.0)  # 5g threshold
        self.stability_timeout = config.get("stability_timeout", 10)  # 10 segundos
        self.last_stable_time = time.time()
        
        # Configuración de alertas
        self.min_weight_alert = config.get("min_weight_alert", 10.0)  # Peso mínimo para alerta
        self.max_weight_alert = config.get("max_weight_alert", 5000.0)  # Peso máximo para alerta
        
        logging.info(f"[WeightSensorHandler] Inicializado con threshold: {self.weight_threshold}g")

    def process_sensor_data(self, raw_data: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Procesa los datos del sensor de peso del Arduino
        
        Args:
            raw_data: Datos JSON del Arduino
            
        Returns:
            Dict con datos procesados para MongoDB o None si hay error
        """
        try:
            # Validar estructura de datos
            if not self._validate_weight_data(raw_data):
                return None
            
            weight_grams = float(raw_data.get("weight_grams", 0))
            is_stable = raw_data.get("is_stable", False)
            calibrated = raw_data.get("calibrated", False)
            arduino_timestamp = raw_data.get("timestamp", 0)
            
            # Actualizar estado interno
            self.last_weight = weight_grams
            
            # Verificar estabilidad
            weight_change = abs(weight_grams - self.last_stable_weight)
            current_time = time.time()
            
            if is_stable and weight_change > self.weight_threshold:
                self.last_stable_weight = weight_grams
                self.last_stable_time = current_time
                logging.info(f"[WeightSensorHandler] Nuevo peso estable: {weight_grams}g")
            
            # Crear documento para MongoDB
            processed_data = {
                "sensor_type": "weight",
                "device_id": self.config.get("device_id", "cathub_arduino_01"),
                "location": self.config.get("location", "food_bowl"),
                "timestamp": datetime.utcnow().isoformat(),
                "arduino_timestamp": arduino_timestamp,
                "data": {
                    "weight_grams": weight_grams,
                    "weight_stable": is_stable,
                    "calibrated": calibrated,
                    "weight_change": weight_change,
                    "last_stable_weight": self.last_stable_weight,
                    "time_since_stable": current_time - self.last_stable_time
                },
                "alerts": self._check_alerts(weight_grams, is_stable, calibrated),
                "metadata": {
                    "processor": "raspberry_pi",
                    "processing_time": datetime.utcnow().isoformat(),
                    "data_quality": "high" if calibrated else "medium"
                }
            }
            
            # Log para debugging
            if self.config.get("debug", False):
                logging.debug(f"[WeightSensorHandler] Datos procesados: {json.dumps(processed_data, indent=2)}")
            
            return processed_data
            
        except Exception as e:
            logging.error(f"[WeightSensorHandler] Error procesando datos: {e}")
            return None

    def _validate_weight_data(self, data: Dict[str, Any]) -> bool:
        """
        Valida la estructura de los datos del sensor de peso
        
        Args:
            data: Datos recibidos del Arduino
            
        Returns:
            True si los datos son válidos, False caso contrario
        """
        required_fields = ["sensor_type", "weight_grams", "is_stable"]
        
        for field in required_fields:
            if field not in data:
                logging.warning(f"[WeightSensorHandler] Campo requerido faltante: {field}")
                return False
        
        # Verificar que sea del tipo correcto
        if data.get("sensor_type") != "weight":
            logging.warning(f"[WeightSensorHandler] Tipo de sensor incorrecto: {data.get('sensor_type')}")
            return False
        
        # Validar rango de peso
        weight = float(data.get("weight_grams", 0))
        if weight < -1000 or weight > 10000:  # Rango razonable: -1kg a 10kg
            logging.warning(f"[WeightSensorHandler] Peso fuera de rango: {weight}g")
            return False
        
        return True

    def _check_alerts(self, weight: float, is_stable: bool, calibrated: bool) -> list:
        """
        Verifica condiciones de alerta basadas en el peso actual
        
        Args:
            weight: Peso actual en gramos
            is_stable: Si el peso está estable
            calibrated: Si el sensor está calibrado
            
        Returns:
            Lista de alertas activas
        """
        alerts = []
        
        # Alerta de calibración
        if not calibrated:
            alerts.append({
                "type": "calibration_error",
                "severity": "medium",
                "message": "Sensor de peso no calibrado",
                "timestamp": datetime.utcnow().isoformat()
            })
        
        # Alerta de peso bajo (posible plato vacío)
        if is_stable and weight < self.min_weight_alert:
            alerts.append({
                "type": "low_weight",
                "severity": "low",
                "message": f"Peso bajo detectado: {weight}g",
                "timestamp": datetime.utcnow().isoformat(),
                "threshold": self.min_weight_alert
            })
        
        # Alerta de peso alto (posible sobrecarga)
        if weight > self.max_weight_alert:
            alerts.append({
                "type": "weight_overload",
                "severity": "high",
                "message": f"Peso excesivo detectado: {weight}g",
                "timestamp": datetime.utcnow().isoformat(),
                "threshold": self.max_weight_alert
            })
        
        # Alerta de inestabilidad prolongada
        if not is_stable and (time.time() - self.last_stable_time) > self.stability_timeout:
            alerts.append({
                "type": "instability",
                "severity": "medium",
                "message": f"Peso inestable por más de {self.stability_timeout} segundos",
                "timestamp": datetime.utcnow().isoformat()
            })
        
        return alerts

    def get_current_weight(self) -> float:
        """
        Retorna el peso actual en gramos
        
        Returns:
            Peso actual en gramos
        """
        return self.last_weight

    def get_stable_weight(self) -> float:
        """
        Retorna el último peso estable en gramos
        
        Returns:
            Último peso estable en gramos
        """
        return self.last_stable_weight

    def is_weight_stable(self) -> bool:
        """
        Indica si el peso actual es considerado estable
        
        Returns:
            True si el peso está estable, False caso contrario
        """
        current_time = time.time()
        weight_change = abs(self.last_weight - self.last_stable_weight)
        return weight_change < self.weight_threshold and (current_time - self.last_stable_time) < 5.0

    def get_sensor_status(self) -> Dict[str, Any]:
        """
        Retorna el estado actual del sensor
        
        Returns:
            Diccionario con información del estado del sensor
        """
        current_time = time.time()
        return {
            "sensor_type": "weight",
            "status": "active" if self.is_active() else "inactive",
            "current_weight_grams": self.last_weight,
            "stable_weight_grams": self.last_stable_weight,
            "is_stable": self.is_weight_stable(),
            "time_since_stable_seconds": current_time - self.last_stable_time,
            "last_update": self.last_update,
            "total_readings": self.total_readings,
            "error_count": self.error_count
        }
