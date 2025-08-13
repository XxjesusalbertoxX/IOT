from repository.consultator import Consultation
from datetime import datetime
import time
import threading

class DispensadorComida:
    def __init__(self, id, capacidad_maxima=2000):
        self.id = id
        self.capacidad_maxima = capacidad_maxima  # gramos
        self.estado = "inactivo"
        self.ultimo_dispensado = None
        self.cantidad_total_dispensada = 0
        self.motor_activo = False
        self.gramos_por_segundo = 10  # Velocidad de dispensado
        
    def dispensar(self, cantidad_gramos, duracion_maxima=30):
        """Dispensar cantidad específica de comida"""
        try:
            if self.motor_activo:
                print(f"⚠️ Dispensador {self.id} ya está activo")
                return False
            
            if cantidad_gramos <= 0:
                print(f"⚠️ Cantidad inválida: {cantidad_gramos}g")
                return False
            
            if cantidad_gramos > 200:  # Límite de seguridad
                print(f"⚠️ Cantidad demasiado grande: {cantidad_gramos}g (máximo 200g)")
                return False
            
            print(f"🍽️ Dispensando {cantidad_gramos}g de comida...")
            self.estado = "dispensando"
            self.motor_activo = True
            
            # Calcular tiempo necesario para dispensar
            tiempo_dispensado = min(cantidad_gramos / self.gramos_por_segundo, duracion_maxima)
            
            # Enviar comando al hardware
            success = Consultation.dispensar_comida(cantidad_gramos)
            
            if success:
                # Simular tiempo de dispensado
                time.sleep(tiempo_dispensado)
                
                self.cantidad_total_dispensada += cantidad_gramos
                self.ultimo_dispensado = datetime.now()
                self.estado = "completado"
                
                print(f"✅ Dispensado exitoso: {cantidad_gramos}g")
                return True
            else:
                self.estado = "error"
                print(f"❌ Error dispensando comida")
                return False
                
        except Exception as e:
            print(f"❌ Error en dispensador {self.id}: {e}")
            self.estado = "error"
            return False
        finally:
            self.motor_activo = False
    
    def dispensar_porcion_pequena(self):
        """Dispensar porción pequeña (15-20g)"""
        cantidad = 17  # gramos
        return self.dispensar(cantidad)
    
    def dispensar_porcion_normal(self):
        """Dispensar porción normal (30-40g)"""
        cantidad = 35  # gramos
        return self.dispensar(cantidad)
    
    def dispensar_porcion_grande(self):
        """Dispensar porción grande (50-60g)"""
        cantidad = 55  # gramos
        return self.dispensar(cantidad)
    
    def detener_emergencia(self):
        """Detener dispensador en caso de emergencia"""
        try:
            # Enviar comando de parada
            Consultation.activar_actuador(self.id, "STOP")
            self.motor_activo = False
            self.estado = "detenido_emergencia"
            print(f"🛑 Dispensador {self.id} detenido por emergencia")
            return True
        except Exception as e:
            print(f"❌ Error deteniendo dispensador: {e}")
            return False
    
    def calibrar_velocidad(self, gramos_por_segundo):
        """Calibrar velocidad de dispensado"""
        if 5 <= gramos_por_segundo <= 50:
            self.gramos_por_segundo = gramos_por_segundo
            print(f"⚙️ Velocidad calibrada: {gramos_por_segundo}g/s")
            return True
        else:
            print(f"⚠️ Velocidad inválida: {gramos_por_segundo}g/s (rango: 5-50)")
            return False
    
    def test_motor(self):
        """Probar funcionamiento del motor brevemente"""
        try:
            print(f"🧪 Probando motor del dispensador {self.id}...")
            self.estado = "probando"
            
            success = Consultation.activar_actuador(self.id, "TEST")
            
            if success:
                self.estado = "funcionando"
                print(f"✅ Motor funcionando correctamente")
                return True
            else:
                self.estado = "error_motor"
                print(f"❌ Motor no responde")
                return False
                
        except Exception as e:
            print(f"❌ Error probando motor: {e}")
            self.estado = "error_motor"
            return False
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día actual"""
        # En implementación real, consultar base de datos
        return {
            'total_dispensado_hoy': self.cantidad_total_dispensada,
            'ultimo_dispensado': self.ultimo_dispensado.isoformat() if self.ultimo_dispensado else None,
            'estado_actual': self.estado,
            'motor_activo': self.motor_activo,
            'velocidad_dispensado': f"{self.gramos_por_segundo}g/s"
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del dispensador"""
        return {
            'id': self.id,
            'capacidad_maxima': self.capacidad_maxima,
            'estado': self.estado,
            'motor_activo': self.motor_activo,
            'gramos_por_segundo': self.gramos_por_segundo,
            'cantidad_total_dispensada': self.cantidad_total_dispensada,
            'ultimo_dispensado': self.ultimo_dispensado.isoformat() if self.ultimo_dispensado else None
        }
    
    def __str__(self):
        return f"Dispensador comida {self.id}: {self.estado} - Total dispensado: {self.cantidad_total_dispensada}g"
