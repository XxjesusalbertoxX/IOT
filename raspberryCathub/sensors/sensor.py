from repository.consultator import Consultation
from repository.mongo import Mongo
import os
import json
import time
from datetime import datetime

class Sensor:
    def __init__(self, id, valor_min, valor_max, unidad=""):
        self.id = id
        self.valor_min = valor_min
        self.valor_max = valor_max
        self.unidad = unidad
        self.estado = "desconocido"
        self.valor_actual = None
        self.ultima_lectura = None
        self.database = Mongo.get_collection("sensor_data") if Mongo.check_connection() else None
        self.pathOffline = os.path.join(os.getcwd(), "sensors_data", "sensors_data_offline.json")
        self.path = os.path.join(os.getcwd(), "sensors_data", f"{self.id}_data.json")
        
    def convertir_a_diccionario(self):
        """Convertir objeto sensor a diccionario para almacenamiento"""
        data = {}
        for key, value in self.__dict__.items():
            if isinstance(value, float):
                data[key] = round(value, 2)
            elif isinstance(value, str):
                data[key] = value.strip()
            elif key not in ['database']:  # Excluir objetos no serializables
                data[key] = value
        
        data['timestamp'] = datetime.now().isoformat()
        return data
    
    def leer_valor(self):
        """Método base para leer valor del sensor - debe ser implementado por subclases"""
        raise NotImplementedError("Subclases deben implementar leer_valor()")
    
    def validar_rango(self, valor):
        """Validar si el valor está dentro del rango esperado"""
        if valor is None:
            return False
        return self.valor_min <= valor <= self.valor_max
    
    def actualizar_estado(self, valor):
        """Actualizar estado basado en el valor leído"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        else:
            self.estado = "normal"
    
    def guardar_lectura(self, valor):
        """Guardar lectura en base de datos o archivo local"""
        self.valor_actual = valor
        self.ultima_lectura = datetime.now()
        self.actualizar_estado(valor)
        
        data = self.convertir_a_diccionario()
        
        # Intentar guardar en MongoDB
        if self.database:
            try:
                Mongo.insert_sensor_data(self.id, data)
                print(f"✅ Datos del sensor {self.id} guardados en MongoDB")
            except Exception as e:
                print(f"❌ Error guardando en MongoDB: {e}")
                self._guardar_offline(data)
        else:
            self._guardar_offline(data)
    
    def _guardar_offline(self, data):
        """Guardar datos localmente cuando no hay conexión a BD"""
        try:
            # Crear directorio si no existe
            os.makedirs(os.path.dirname(self.path), exist_ok=True)
            
            # Leer datos existentes
            if os.path.exists(self.path):
                with open(self.path, 'r') as f:
                    existing_data = json.load(f)
            else:
                existing_data = []
            
            # Agregar nueva lectura
            existing_data.append(data)
            
            # Mantener solo las últimas 1000 lecturas
            if len(existing_data) > 1000:
                existing_data = existing_data[-1000:]
            
            # Guardar datos actualizados
            with open(self.path, 'w') as f:
                json.dump(existing_data, f, indent=2)
                
            print(f"💾 Datos del sensor {self.id} guardados localmente")
            
        except Exception as e:
            print(f"❌ Error guardando datos offline: {e}")
    
    def obtener_estadisticas(self, ultimas_n_lecturas=10):
        """Obtener estadísticas de las últimas lecturas"""
        try:
            if os.path.exists(self.path):
                with open(self.path, 'r') as f:
                    data = json.load(f)
                
                if len(data) >= ultimas_n_lecturas:
                    ultimas_lecturas = data[-ultimas_n_lecturas:]
                    valores = [item.get('valor_actual') for item in ultimas_lecturas if item.get('valor_actual') is not None]
                    
                    if valores:
                        return {
                            'promedio': sum(valores) / len(valores),
                            'maximo': max(valores),
                            'minimo': min(valores),
                            'total_lecturas': len(valores)
                        }
            
            return None
            
        except Exception as e:
            print(f"Error calculando estadísticas: {e}")
            return None
    
    def __str__(self):
        return f"Sensor {self.id}: {self.valor_actual}{self.unidad} - Estado: {self.estado}"
