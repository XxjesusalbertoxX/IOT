from sensors.sensor import Sensor
from repository.consultator import Consultation
import json
import random

class SensorPeso(Sensor):
    def __init__(self, id, valor_min=0, valor_max=1000, ubicacion="plato"):
        super().__init__(id, valor_min, valor_max, unidad="g")
        self.ubicacion = ubicacion  # plato, comedero_completo, etc.
        self.peso_tara = 0  # Peso del contenedor vacío
        self.peso_anterior = None
        self.diferencia_peso = 0
    
    def leer_valor(self):
        """Leer peso desde celda de carga"""
        try:
            response = Consultation.consultar_sensor_especifico(self.id)
            
            if response:
                data = json.loads(response)
                peso_bruto = data.get('peso_gramos', 0)
                peso_neto = peso_bruto - self.peso_tara
                
                if self.validar_rango(peso_neto):
                    self._calcular_diferencia(peso_neto)
                    self.guardar_lectura(peso_neto)
                    return peso_neto
            
            return self._simular_lectura()
            
        except Exception as e:
            print(f"Error leyendo sensor de peso {self.id}: {e}")
            return self._simular_lectura()
    
    def _simular_lectura(self):
        """Simular lectura de peso"""
        if hasattr(self, '_ultimo_peso_simulado'):
            # Simular variaciones pequeñas o consumo
            if random.random() < 0.1:  # 10% probabilidad de consumo
                consumo = random.uniform(5, 25)  # 5-25g consumidos
                self._ultimo_peso_simulado = max(0, self._ultimo_peso_simulado - consumo)
            else:
                # Pequeñas variaciones por vibración/viento
                variacion = random.uniform(-2, 2)
                self._ultimo_peso_simulado = max(0, self._ultimo_peso_simulado + variacion)
        else:
            self._ultimo_peso_simulado = random.uniform(50, 200)
        
        peso = round(self._ultimo_peso_simulado, 1)
        self._calcular_diferencia(peso)
        self.guardar_lectura(peso)
        return peso
    
    def _calcular_diferencia(self, peso_actual):
        """Calcular diferencia con peso anterior"""
        if self.peso_anterior is not None:
            self.diferencia_peso = peso_actual - self.peso_anterior
        self.peso_anterior = peso_actual
    
    def calibrar_tara(self, peso_tara_gramos):
        """Calibrar el peso tara (peso del contenedor vacío)"""
        self.peso_tara = peso_tara_gramos
        print(f"Tara calibrada en {peso_tara_gramos}g para sensor {self.id}")
    
    def se_consumio_comida(self, umbral_gramos=5):
        """Detectar si se consumió comida basado en diferencia de peso"""
        return self.diferencia_peso <= -umbral_gramos
    
    def se_agrego_comida(self, umbral_gramos=10):
        """Detectar si se agregó comida"""
        return self.diferencia_peso >= umbral_gramos
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para sensor de peso"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        elif valor <= 5:
            self.estado = "vacio"
        elif self.se_consumio_comida():
            self.estado = "consumiendo"
        elif self.se_agrego_comida():
            self.estado = "rellenando"
        else:
            self.estado = "estable"
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'ubicacion': self.ubicacion,
            'peso_actual': self.valor_actual,
            'peso_tara': self.peso_tara,
            'diferencia_peso': self.diferencia_peso,
            'estado': self.estado,
            'se_consumio_comida': self.se_consumio_comida(),
            'se_agrego_comida': self.se_agrego_comida(),
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def __str__(self):
        simbolo_cambio = ""
        if self.diferencia_peso > 0:
            simbolo_cambio = f" (+{self.diferencia_peso:.1f}g)"
        elif self.diferencia_peso < 0:
            simbolo_cambio = f" ({self.diferencia_peso:.1f}g)"
        
        return f"Peso {self.ubicacion} {self.id}: {self.valor_actual}g{simbolo_cambio} - Estado: {self.estado}"
