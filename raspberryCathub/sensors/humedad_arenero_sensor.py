from sensors.sensor import Sensor
from repository.consultator import Consultation
import json
import random

class SensorHumedadArenero(Sensor):
    def __init__(self, id, valor_min=0, valor_max=100):
        super().__init__(id, valor_min, valor_max, unidad="%")
        self.nivel_critico = 70  # 70% de humedad es crítico (necesita limpieza)
        self.nivel_normal = 40   # Hasta 40% es normal
        self.requiere_limpieza = False
    
    def leer_valor(self):
        """Leer humedad del arenero"""
        try:
            response = Consultation.consultar_sensor_especifico(self.id)
            
            if response:
                data = json.loads(response)
                humedad = data.get('humedad_porcentaje', 0)
                
                if self.validar_rango(humedad):
                    self.guardar_lectura(humedad)
                    return humedad
            
            return self._simular_lectura()
            
        except Exception as e:
            print(f"Error leyendo sensor de humedad del arenero {self.id}: {e}")
            return self._simular_lectura()
    
    def _simular_lectura(self):
        """Simular lectura de humedad del arenero"""
        if hasattr(self, '_ultima_humedad_simulada'):
            # Simular incremento gradual de humedad por uso
            if random.random() < 0.3:  # 30% probabilidad de aumento
                incremento = random.uniform(1, 5)
                self._ultima_humedad_simulada = min(100, self._ultima_humedad_simulada + incremento)
            elif random.random() < 0.1:  # 10% probabilidad de limpieza/secado
                self._ultima_humedad_simulada = random.uniform(20, 35)
        else:
            self._ultima_humedad_simulada = random.uniform(25, 60)
        
        humedad = round(self._ultima_humedad_simulada, 1)
        self.guardar_lectura(humedad)
        return humedad
    
    def necesita_limpieza(self):
        """Verificar si necesita limpieza basado en humedad"""
        return self.valor_actual is not None and self.valor_actual >= self.nivel_critico
    
    def esta_en_buen_estado(self):
        """Verificar si el arenero está en buen estado"""
        return self.valor_actual is not None and self.valor_actual <= self.nivel_normal
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para humedad del arenero"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        elif valor >= self.nivel_critico:
            self.estado = "critico_necesita_limpieza"
            self.requiere_limpieza = True
        elif valor >= 55:
            self.estado = "humedo"
        elif valor <= self.nivel_normal:
            self.estado = "normal"
            self.requiere_limpieza = False
        else:
            self.estado = "aceptable"
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'humedad_porcentaje': self.valor_actual,
            'estado': self.estado,
            'necesita_limpieza': self.necesita_limpieza(),
            'esta_en_buen_estado': self.esta_en_buen_estado(),
            'nivel_critico': self.nivel_critico,
            'nivel_normal': self.nivel_normal,
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def __str__(self):
        alert = " ⚠️ NECESITA LIMPIEZA" if self.necesita_limpieza() else ""
        return f"Humedad arenero {self.id}: {self.valor_actual}% - Estado: {self.estado}{alert}"
