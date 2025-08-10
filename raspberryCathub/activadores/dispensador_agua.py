from repository.consultator import Consultation
from datetime import datetime
import time
import threading

class DispensadorAgua:
    def __init__(self, id, capacidad_maxima=1500):
        self.id = id
        self.capacidad_maxima = capacidad_maxima  # mililitros
        self.estado = "inactivo"
        self.ultimo_dispensado = None
        self.cantidad_total_dispensada = 0
        self.bomba_activa = False
        self.ml_por_segundo = 30  # Velocidad de dispensado
        self.temperatura_ideal = 22  # Grados Celsius
        
    def dispensar(self, cantidad_ml, duracion_maxima=20):
        """Dispensar cantidad específica de agua"""
        try:
            if self.bomba_activa:
                print(f"⚠️ Dispensador de agua {self.id} ya está activo")
                return False
            
            if cantidad_ml <= 0:
                print(f"⚠️ Cantidad inválida: {cantidad_ml}ml")
                return False
            
            if cantidad_ml > 500:  # Límite de seguridad
                print(f"⚠️ Cantidad demasiado grande: {cantidad_ml}ml (máximo 500ml)")
                return False
            
            print(f"💧 Dispensando {cantidad_ml}ml de agua...")
            self.estado = "dispensando"
            self.bomba_activa = True
            
            # Calcular tiempo necesario para dispensar
            tiempo_dispensado = min(cantidad_ml / self.ml_por_segundo, duracion_maxima)
            
            # Enviar comando al hardware
            success = Consultation.dispensar_agua(cantidad_ml)
            
            if success:
                # Simular tiempo de dispensado
                time.sleep(tiempo_dispensado)
                
                self.cantidad_total_dispensada += cantidad_ml
                self.ultimo_dispensado = datetime.now()
                self.estado = "completado"
                
                print(f"✅ Dispensado exitoso: {cantidad_ml}ml")
                return True
            else:
                self.estado = "error"
                print(f"❌ Error dispensando agua")
                return False
                
        except Exception as e:
            print(f"❌ Error en dispensador de agua {self.id}: {e}")
            self.estado = "error"
            return False
        finally:
            self.bomba_activa = False
    
    def llenar_bebedero(self, nivel_objetivo=80):
        """Llenar bebedero hasta nivel objetivo (%)"""
        # Calcular cantidad necesaria basada en capacidad y nivel objetivo
        cantidad_necesaria = (self.capacidad_maxima * nivel_objetivo / 100)
        return self.dispensar(cantidad_necesaria)
    
    def dispensar_porcion_pequena(self):
        """Dispensar porción pequeña (50ml)"""
        return self.dispensar(50)
    
    def dispensar_porcion_normal(self):
        """Dispensar porción normal (100ml)"""
        return self.dispensar(100)
    
    def dispensar_porcion_grande(self):
        """Dispensar porción grande (200ml)"""
        return self.dispensar(200)
    
    def activar_circulacion(self, duracion_segundos=10):
        """Activar circulación de agua para mantenerla fresca"""
        try:
            print(f"🔄 Activando circulación de agua por {duracion_segundos}s...")
            self.estado = "circulando"
            
            success = Consultation.activar_actuador(self.id, f"CIRCULATE:{duracion_segundos}")
            
            if success:
                time.sleep(duracion_segundos)
                self.estado = "circulacion_completada"
                print(f"✅ Circulación completada")
                return True
            else:
                self.estado = "error_circulacion"
                return False
                
        except Exception as e:
            print(f"❌ Error en circulación: {e}")
            self.estado = "error_circulacion"
            return False
    
    def detener_emergencia(self):
        """Detener dispensador en caso de emergencia"""
        try:
            Consultation.activar_actuador(self.id, "STOP")
            self.bomba_activa = False
            self.estado = "detenido_emergencia"
            print(f"🛑 Dispensador de agua {self.id} detenido por emergencia")
            return True
        except Exception as e:
            print(f"❌ Error deteniendo dispensador: {e}")
            return False
    
    def calibrar_velocidad(self, ml_por_segundo):
        """Calibrar velocidad de dispensado"""
        if 10 <= ml_por_segundo <= 100:
            self.ml_por_segundo = ml_por_segundo
            print(f"⚙️ Velocidad calibrada: {ml_por_segundo}ml/s")
            return True
        else:
            print(f"⚠️ Velocidad inválida: {ml_por_segundo}ml/s (rango: 10-100)")
            return False
    
    def verificar_calidad_agua(self):
        """Verificar calidad del agua (temperatura, pH, etc.)"""
        try:
            # En implementación real, consultar sensores de calidad
            print(f"🧪 Verificando calidad del agua...")
            self.estado = "verificando_calidad"
            
            # Simular verificación
            time.sleep(2)
            
            # Por ahora, siempre retornar buena calidad
            self.estado = "calidad_verificada"
            return {
                'temperatura': self.temperatura_ideal,
                'ph': 7.0,
                'calidad': 'buena',
                'apta_consumo': True
            }
            
        except Exception as e:
            print(f"❌ Error verificando calidad: {e}")
            return None
    
    def test_bomba(self):
        """Probar funcionamiento de la bomba brevemente"""
        try:
            print(f"🧪 Probando bomba del dispensador {self.id}...")
            self.estado = "probando"
            
            success = Consultation.activar_actuador(self.id, "TEST")
            
            if success:
                self.estado = "funcionando"
                print(f"✅ Bomba funcionando correctamente")
                return True
            else:
                self.estado = "error_bomba"
                print(f"❌ Bomba no responde")
                return False
                
        except Exception as e:
            print(f"❌ Error probando bomba: {e}")
            self.estado = "error_bomba"
            return False
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día actual"""
        return {
            'total_dispensado_hoy': self.cantidad_total_dispensada,
            'ultimo_dispensado': self.ultimo_dispensado.isoformat() if self.ultimo_dispensado else None,
            'estado_actual': self.estado,
            'bomba_activa': self.bomba_activa,
            'velocidad_dispensado': f"{self.ml_por_segundo}ml/s",
            'temperatura_ideal': self.temperatura_ideal
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del dispensador"""
        return {
            'id': self.id,
            'capacidad_maxima': self.capacidad_maxima,
            'estado': self.estado,
            'bomba_activa': self.bomba_activa,
            'ml_por_segundo': self.ml_por_segundo,
            'cantidad_total_dispensada': self.cantidad_total_dispensada,
            'ultimo_dispensado': self.ultimo_dispensado.isoformat() if self.ultimo_dispensado else None,
            'temperatura_ideal': self.temperatura_ideal
        }
    
    def __str__(self):
        return f"Dispensador agua {self.id}: {self.estado} - Total dispensado: {self.cantidad_total_dispensada}ml"
