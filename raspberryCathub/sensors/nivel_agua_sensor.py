from sensors.sensor import Sensor
from repository.consultator import Consultation
import json
import random

class SensorNivelAgua(Sensor):
    def __init__(self, id, valor_min=0, valor_max=100):
        super().__init__(id, valor_min, valor_max, unidad="%")
        self.capacidad_maxima_ml = 1500  # 1.5L de agua máximo
        self.nivel_critico = 15  # 15% es nivel crítico
        self.nivel_minimo = 5    # 5% es nivel mínimo
    
    def leer_valor(self):
        """Leer nivel de agua desde el sensor"""
        try:
            # Intentar leer del puerto serial
            response = Consultation.consultar_sensor_especifico(self.id)
            
            if response:
                data = json.loads(response)
                nivel_porcentaje = data.get('nivel_porcentaje', 0)
                
                if self.validar_rango(nivel_porcentaje):
                    self.guardar_lectura(nivel_porcentaje)
                    return nivel_porcentaje
            
            return self._simular_lectura()
            
        except Exception as e:
            print(f"Error leyendo sensor de agua {self.id}: {e}")
            return self._simular_lectura()
    
    def _simular_lectura(self):
        """Simular lectura para desarrollo"""
        if hasattr(self, '_ultimo_nivel_simulado'):
            # El agua se consume más lento que la comida
            decremento = random.uniform(0.05, 0.5)
            self._ultimo_nivel_simulado = max(0, self._ultimo_nivel_simulado - decremento)
        else:
            self._ultimo_nivel_simulado = random.uniform(40, 100)
        
        nivel = round(self._ultimo_nivel_simulado, 1)
        self.guardar_lectura(nivel)
        return nivel
    
    def calcular_ml_restantes(self):
        """Calcular mililitros de agua restantes"""
        if self.valor_actual is not None:
            return round((self.valor_actual / 100) * self.capacidad_maxima_ml, 1)
        return 0
    
    def necesita_rellenado(self):
        """Verificar si necesita ser rellenado"""
        return self.valor_actual is not None and self.valor_actual <= self.nivel_critico
    
    def esta_vacio(self):
        """Verificar si está prácticamente vacío"""
        return self.valor_actual is not None and self.valor_actual <= self.nivel_minimo
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para nivel de agua"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        elif valor <= self.nivel_minimo:
            self.estado = "vacio"
        elif valor <= self.nivel_critico:
            self.estado = "critico"
        elif valor >= 95:
            self.estado = "lleno"
        else:
            self.estado = "normal"
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'nivel_porcentaje': self.valor_actual,
            'ml_restantes': self.calcular_ml_restantes(),
            'estado': self.estado,
            'necesita_rellenado': self.necesita_rellenado(),
            'esta_vacio': self.esta_vacio(),
            'capacidad_maxima': self.capacidad_maxima_ml,
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def __str__(self):
        ml = self.calcular_ml_restantes()
        return f"Bebedero {self.id}: {self.valor_actual}% ({ml}ml) - Estado: {self.estado}"
