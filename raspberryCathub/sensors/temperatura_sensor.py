from sensors.sensor import Sensor
from repository.consultator import Consultation
import json
import random

class SensorTemperatura(Sensor):
    def __init__(self, id, valor_min=10, valor_max=40, proposito="ambiente"):
        super().__init__(id, valor_min, valor_max, unidad="°C")
        self.proposito = proposito  # ambiente, agua, etc.
        self.temp_ideal_min = 20
        self.temp_ideal_max = 25
    
    def leer_valor(self):
        """Leer temperatura"""
        try:
            response = Consultation.consultar_sensor_especifico(self.id)
            
            if response:
                data = json.loads(response)
                temperatura = data.get('temperatura_celsius', 0)
                
                if self.validar_rango(temperatura):
                    self.guardar_lectura(temperatura)
                    return temperatura
            
            return self._simular_lectura()
            
        except Exception as e:
            print(f"Error leyendo sensor de temperatura {self.id}: {e}")
            return self._simular_lectura()
    
    def _simular_lectura(self):
        """Simular lectura de temperatura"""
        if self.proposito == "agua":
            # Agua debe estar fresca
            temp = random.uniform(18, 24)
        else:
            # Temperatura ambiente
            temp = random.uniform(20, 28)
        
        temperatura = round(temp, 1)
        self.guardar_lectura(temperatura)
        return temperatura
    
    def temperatura_ideal(self):
        """Verificar si la temperatura está en rango ideal"""
        return (self.valor_actual is not None and 
                self.temp_ideal_min <= self.valor_actual <= self.temp_ideal_max)
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para temperatura"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        elif valor < 15:
            self.estado = "muy_frio"
        elif valor < self.temp_ideal_min:
            self.estado = "frio"
        elif valor > 30:
            self.estado = "muy_caliente"
        elif valor > self.temp_ideal_max:
            self.estado = "caliente"
        else:
            self.estado = "ideal"
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'proposito': self.proposito,
            'temperatura_celsius': self.valor_actual,
            'estado': self.estado,
            'temperatura_ideal': self.temperatura_ideal(),
            'rango_ideal': f"{self.temp_ideal_min}-{self.temp_ideal_max}°C",
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def __str__(self):
        return f"Temperatura {self.proposito} {self.id}: {self.valor_actual}°C - Estado: {self.estado}"
