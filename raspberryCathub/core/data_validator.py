"""
Data Validator - Valida datos y decide acciones
"""

import logging
from typing import Dict, Any, List

class DataValidator:
    """
    Validador de datos de sensores
    Analiza valores y decide qué acciones tomar
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        
        # ✅ UMBRALES DE VALIDACIÓN
        self.thresholds = {
            "weight": {
                "low_food": 50,      # gramos - comida baja
                "empty": 10,         # gramos - comedero vacío
                "max_valid": 5000    # gramos - peso máximo válido
            },
            "presence": {
                "cat_detected": 30,  # cm - gato presente si < 30cm
                "too_close": 5       # cm - demasiado cerca (error?)
            },
            "temperature": {
                "too_hot": 35,       # °C - demasiado caliente
                "too_cold": 10       # °C - demasiado frío
            },
            "humidity": {
                "too_humid": 80,     # % - demasiado húmedo
                "too_dry": 20        # % - demasiado seco
            },
            "gas": {
                "alert_level": 500,  # ppm - nivel de alerta
                "danger_level": 1000 # ppm - nivel peligroso
            },
            "water_level": {
                "low_water": 200,    # valor analógico - agua baja
                "critical": 100      # valor crítico
            }
        }

    def validate_sensor_data(self, sensor_name: str, values: List[float], 
                           sensor_object=None) -> Dict[str, Any]:
        """
        Valida datos de un sensor y retorna acciones
        
        Args:
            sensor_name: Nombre del sensor
            values: Lista de valores a validar
            sensor_object: Objeto del sensor (opcional)
            
        Returns:
            Diccionario con resultado de validación y acciones
        """
        try:
            validation_result = {
                "valid": True,
                "warnings": [],
                "alerts": [],
                "actions": []
            }
            
            # Validar según tipo de sensor
            if sensor_name == "weight":
                validation_result = self._validate_weight(values[0], validation_result)
                
            elif "presence" in sensor_name:
                validation_result = self._validate_presence(sensor_name, values[0], validation_result)
                
            elif sensor_name == "environment":
                # Los sensores de ambiente retornan múltiples valores
                validation_result = self._validate_environment(values, validation_result)
                
            elif sensor_name == "water_level":
                validation_result = self._validate_water_level(values[0], validation_result)
            
            return validation_result
            
        except Exception as e:
            self.logger.error(f"❌ Error validando {sensor_name}: {e}")
            return {"valid": False, "error": str(e), "actions": []}

    def _validate_weight(self, weight_grams: float, result: Dict[str, Any]) -> Dict[str, Any]:
        """Valida datos de peso del comedero"""
        
        # Verificar rango válido
        if weight_grams > self.thresholds["weight"]["max_valid"]:
            result["valid"] = False
            result["alerts"].append(f"Peso inválido: {weight_grams}g (máximo: {self.thresholds['weight']['max_valid']}g)")
            return result
        
        # Comedero vacío - CRÍTICO
        if weight_grams <= self.thresholds["weight"]["empty"]:
            result["alerts"].append(f"COMEDERO VACÍO: {weight_grams}g")
            result["actions"].append({
                "type": "alert_empty_feeder",
                "priority": "critical",
                "message": f"Comedero vacío ({weight_grams}g)"
            })
        
        # Comida baja - ADVERTENCIA
        elif weight_grams <= self.thresholds["weight"]["low_food"]:
            result["warnings"].append(f"Comida baja: {weight_grams}g")
            result["actions"].append({
                "type": "alert_low_food", 
                "priority": "warning",
                "message": f"Comida baja ({weight_grams}g)"
            })
        
        return result

    def _validate_presence(self, sensor_name: str, distance_cm: float, 
                          result: Dict[str, Any]) -> Dict[str, Any]:
        """Valida sensores de presencia"""
        
        # Gato detectado
        if distance_cm <= self.thresholds["presence"]["cat_detected"]:
            
            if "litterbox" in sensor_name:
                result["actions"].append({
                    "type": "cat_in_litterbox",
                    "priority": "high", 
                    "message": f"Gato en arenero ({distance_cm}cm)",
                    "data": {"distance_cm": distance_cm}
                })
                
            elif "water" in sensor_name:
                result["actions"].append({
                    "type": "cat_drinking",
                    "priority": "info",
                    "message": f"Gato bebiendo ({distance_cm}cm)",
                    "data": {"distance_cm": distance_cm}
                })
        
        # Sensor demasiado cerca (posible error)
        if distance_cm <= self.thresholds["presence"]["too_close"]:
            result["warnings"].append(f"Sensor muy cerca: {distance_cm}cm")
        
        return result

    def _validate_environment(self, values: List[float], result: Dict[str, Any]) -> Dict[str, Any]:
        """Valida sensores de ambiente (temp, humedad, gas)"""
        
        # Asumir orden: [temperatura, humedad, gas]
        if len(values) >= 3:
            temp, humidity, gas = values[0], values[1], values[2]
            
            # Validar temperatura
            if temp > self.thresholds["temperature"]["too_hot"]:
                result["alerts"].append(f"Temperatura alta: {temp}°C")
                result["actions"].append({
                    "type": "high_temperature",
                    "priority": "warning",
                    "message": f"Temperatura alta ({temp}°C)"
                })
            
            # Validar humedad
            if humidity > self.thresholds["humidity"]["too_humid"]:
                result["warnings"].append(f"Humedad alta: {humidity}%")
                result["actions"].append({
                    "type": "high_humidity", 
                    "priority": "info",
                    "message": f"Humedad alta ({humidity}%)"
                })
            
            # Validar gas - MÁS CRÍTICO
            if gas > self.thresholds["gas"]["danger_level"]:
                result["alerts"].append(f"Gas peligroso: {gas}ppm")
                result["actions"].append({
                    "type": "dangerous_gas",
                    "priority": "critical",
                    "message": f"Nivel de gas peligroso ({gas}ppm)"
                })
            elif gas > self.thresholds["gas"]["alert_level"]:
                result["warnings"].append(f"Gas elevado: {gas}ppm")
                result["actions"].append({
                    "type": "elevated_gas",
                    "priority": "warning", 
                    "message": f"Gas elevado ({gas}ppm)"
                })
        
        return result

    def _validate_water_level(self, level: float, result: Dict[str, Any]) -> Dict[str, Any]:
        """Valida nivel de agua"""
        
        # Agua crítica
        if level <= self.thresholds["water_level"]["critical"]:
            result["alerts"].append(f"Agua crítica: {level}")
            result["actions"].append({
                "type": "critical_water_level",
                "priority": "critical",
                "message": f"Nivel de agua crítico ({level})"
            })
        
        # Agua baja
        elif level <= self.thresholds["water_level"]["low_water"]:
            result["warnings"].append(f"Agua baja: {level}")
            result["actions"].append({
                "type": "low_water_level",
                "priority": "warning",
                "message": f"Nivel de agua bajo ({level})"
            })
        
        return result