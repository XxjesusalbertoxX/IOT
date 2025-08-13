from sensors.sensor import Sensor
from repository.consultator import Consultation
import json
import random
from datetime import datetime, timedelta

class SensorPresencia(Sensor):
    def __init__(self, id, valor_min=0, valor_max=1, zona="general"):
        super().__init__(id, valor_min, valor_max, unidad="")
        self.zona = zona  # comedero, bebedero, arenero, general
        self.ultima_presencia_detectada = None
        self.tiempo_sin_presencia = None
        self.contador_detecciones = 0
    
    def leer_valor(self):
        """Leer sensor de presencia (PIR, ultrasónico, peso)"""
        try:
            response = Consultation.consultar_sensor_especifico(self.id)
            
            if response:
                data = json.loads(response)
                presencia = data.get('presencia', 0)  # 0 = no presencia, 1 = presencia
                
                if self.validar_rango(presencia):
                    self._procesar_deteccion(presencia)
                    self.guardar_lectura(presencia)
                    return presencia
            
            return self._simular_lectura()
            
        except Exception as e:
            print(f"Error leyendo sensor de presencia {self.id}: {e}")
            return self._simular_lectura()
    
    def _simular_lectura(self):
        """Simular detección de presencia"""
        # Simular patrones más realistas según la zona
        if self.zona == "comedero":
            # Gatos comen varias veces al día
            probabilidad_presencia = 0.15
        elif self.zona == "bebedero":
            # Beben con menos frecuencia
            probabilidad_presencia = 0.08
        elif self.zona == "arenero":
            # Usan arenero regularmente
            probabilidad_presencia = 0.12
        else:
            # Presencia general
            probabilidad_presencia = 0.20
        
        presencia = 1 if random.random() < probabilidad_presencia else 0
        self._procesar_deteccion(presencia)
        self.guardar_lectura(presencia)
        return presencia
    
    def _procesar_deteccion(self, presencia):
        """Procesar la detección de presencia"""
        if presencia == 1:
            self.ultima_presencia_detectada = datetime.now()
            self.contador_detecciones += 1
            self.tiempo_sin_presencia = None
            print(f"🐱 Presencia detectada en {self.zona} - Sensor {self.id}")
        else:
            if self.ultima_presencia_detectada:
                self.tiempo_sin_presencia = datetime.now() - self.ultima_presencia_detectada
    
    def hay_presencia_actual(self):
        """Verificar si hay presencia en este momento"""
        return self.valor_actual == 1
    
    def minutos_sin_presencia(self):
        """Calcular minutos sin presencia"""
        if self.tiempo_sin_presencia:
            return int(self.tiempo_sin_presencia.total_seconds() / 60)
        return 0
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para sensor de presencia"""
        if valor is None:
            self.estado = "error_lectura"
        elif not self.validar_rango(valor):
            self.estado = "fuera_de_rango"
        elif valor == 1:
            self.estado = "presencia_detectada"
        else:
            # Determinar estado basado en tiempo sin presencia
            minutos_sin_presencia = self.minutos_sin_presencia()
            if minutos_sin_presencia > 480:  # 8 horas
                self.estado = "inactivo_prolongado"
            elif minutos_sin_presencia > 60:  # 1 hora
                self.estado = "sin_actividad"
            else:
                self.estado = "normal"
    
    def obtener_estadisticas_actividad(self, horas=24):
        """Obtener estadísticas de actividad de las últimas X horas"""
        try:
            estadisticas = self.obtener_estadisticas(ultimas_n_lecturas=horas * 12)  # Cada 5 min
            if estadisticas:
                total_detecciones = estadisticas['total_lecturas']
                promedio_actividad = estadisticas['promedio'] * 100  # Como porcentaje
                
                return {
                    'total_detecciones': total_detecciones,
                    'porcentaje_actividad': round(promedio_actividad, 1),
                    'contador_detecciones': self.contador_detecciones,
                    'ultima_presencia': self.ultima_presencia_detectada.isoformat() if self.ultima_presencia_detectada else None,
                    'minutos_sin_presencia': self.minutos_sin_presencia()
                }
            return None
        except Exception as e:
            print(f"Error calculando estadísticas de actividad: {e}")
            return None
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'zona': self.zona,
            'presencia_actual': self.hay_presencia_actual(),
            'estado': self.estado,
            'contador_detecciones': self.contador_detecciones,
            'ultima_presencia': self.ultima_presencia_detectada.isoformat() if self.ultima_presencia_detectada else None,
            'minutos_sin_presencia': self.minutos_sin_presencia(),
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def __str__(self):
        estado_presencia = "Sí" if self.hay_presencia_actual() else "No"
        return f"Presencia {self.zona} {self.id}: {estado_presencia} - Estado: {self.estado}"
