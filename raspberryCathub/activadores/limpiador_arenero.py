from repository.consultator import Consultation
from datetime import datetime
import time
import threading

class LimpiadorArenero:
    def __init__(self, id):
        self.id = id
        self.estado = "inactivo"
        self.ultima_limpieza = None
        self.total_limpiezas = 0
        self.motor_activo = False
        self.tiempo_limpieza_estandar = 30  # segundos
        self.intervalo_minimo_minutos = 60  # Mínimo 1 hora entre limpiezas
        
    def limpiar(self, duracion_segundos=None):
        """Ejecutar ciclo completo de limpieza"""
        try:
            if self.motor_activo:
                print(f"⚠️ Limpiador {self.id} ya está activo")
                return False
            
            # Verificar intervalo mínimo
            if not self._puede_limpiar():
                return False
            
            duracion = duracion_segundos or self.tiempo_limpieza_estandar
            
            if duracion > 300:  # Máximo 5 minutos
                print(f"⚠️ Duración demasiado larga: {duracion}s (máximo 300s)")
                return False
            
            print(f"🧹 Iniciando limpieza del arenero (duración: {duracion}s)...")
            self.estado = "limpiando"
            self.motor_activo = True
            
            # Fases de limpieza
            success = self._ejecutar_fases_limpieza(duracion)
            
            if success:
                self.total_limpiezas += 1
                self.ultima_limpieza = datetime.now()
                self.estado = "limpieza_completada"
                print(f"✅ Limpieza completada exitosamente")
                return True
            else:
                self.estado = "error_limpieza"
                print(f"❌ Error durante la limpieza")
                return False
                
        except Exception as e:
            print(f"❌ Error en limpiador {self.id}: {e}")
            self.estado = "error"
            return False
        finally:
            self.motor_activo = False
    
    def _puede_limpiar(self):
        """Verificar si puede ejecutar limpieza (intervalo mínimo)"""
        if self.ultima_limpieza is None:
            return True
        
        tiempo_transcurrido = datetime.now() - self.ultima_limpieza
        minutos_transcurridos = tiempo_transcurrido.total_seconds() / 60
        
        if minutos_transcurridos < self.intervalo_minimo_minutos:
            minutos_restantes = int(self.intervalo_minimo_minutos - minutos_transcurridos)
            print(f"⏰ Debe esperar {minutos_restantes} minutos antes de la próxima limpieza")
            return False
        
        return True
    
    def _ejecutar_fases_limpieza(self, duracion_total):
        """Ejecutar las diferentes fases de limpieza"""
        try:
            # Fase 1: Preparación y posicionamiento (10% del tiempo)
            fase1_tiempo = int(duracion_total * 0.1)
            print(f"🔧 Fase 1: Preparación ({fase1_tiempo}s)")
            self.estado = "preparando"
            success = Consultation.activar_actuador(self.id, f"PREPARE:{fase1_tiempo}")
            if not success:
                return False
            time.sleep(fase1_tiempo)
            
            # Fase 2: Tamizado y separación (70% del tiempo)
            fase2_tiempo = int(duracion_total * 0.7)
            print(f"🧹 Fase 2: Tamizado ({fase2_tiempo}s)")
            self.estado = "tamizando"
            success = Consultation.activar_actuador(self.id, f"SIFT:{fase2_tiempo}")
            if not success:
                return False
            time.sleep(fase2_tiempo)
            
            # Fase 3: Disposición de desechos (15% del tiempo)
            fase3_tiempo = int(duracion_total * 0.15)
            print(f"🗑️ Fase 3: Disposición ({fase3_tiempo}s)")
            self.estado = "disponiendo"
            success = Consultation.activar_actuador(self.id, f"DISPOSE:{fase3_tiempo}")
            if not success:
                return False
            time.sleep(fase3_tiempo)
            
            # Fase 4: Finalización y reset (5% del tiempo)
            fase4_tiempo = duracion_total - fase1_tiempo - fase2_tiempo - fase3_tiempo
            print(f"🏁 Fase 4: Finalizando ({fase4_tiempo}s)")
            self.estado = "finalizando"
            success = Consultation.activar_actuador(self.id, f"FINISH:{fase4_tiempo}")
            if not success:
                return False
            time.sleep(fase4_tiempo)
            
            return True
            
        except Exception as e:
            print(f"❌ Error en fases de limpieza: {e}")
            return False
    
    def limpieza_rapida(self):
        """Ejecutar limpieza rápida (15 segundos)"""
        return self.limpiar(15)
    
    def limpieza_profunda(self):
        """Ejecutar limpieza profunda (60 segundos)"""
        return self.limpiar(60)
    
    def detener_emergencia(self):
        """Detener limpieza en caso de emergencia"""
        try:
            Consultation.activar_actuador(self.id, "EMERGENCY_STOP")
            self.motor_activo = False
            self.estado = "detenido_emergencia"
            print(f"🛑 Limpiador {self.id} detenido por emergencia")
            return True
        except Exception as e:
            print(f"❌ Error deteniendo limpiador: {e}")
            return False
    
    def calibrar_tiempo_limpieza(self, segundos):
        """Calibrar tiempo estándar de limpieza"""
        if 10 <= segundos <= 300:
            self.tiempo_limpieza_estandar = segundos
            print(f"⚙️ Tiempo de limpieza calibrado: {segundos}s")
            return True
        else:
            print(f"⚠️ Tiempo inválido: {segundos}s (rango: 10-300)")
            return False
    
    def configurar_intervalo_minimo(self, minutos):
        """Configurar intervalo mínimo entre limpiezas"""
        if 30 <= minutos <= 480:  # Entre 30 minutos y 8 horas
            self.intervalo_minimo_minutos = minutos
            print(f"⚙️ Intervalo mínimo configurado: {minutos} minutos")
            return True
        else:
            print(f"⚠️ Intervalo inválido: {minutos} minutos (rango: 30-480)")
            return False
    
    def test_motor(self):
        """Probar funcionamiento del motor brevemente"""
        try:
            print(f"🧪 Probando motor del limpiador {self.id}...")
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
    
    def programar_limpieza_automatica(self, horas_intervalo=4):
        """Programar limpieza automática cada X horas"""
        # Esta funcionalidad sería implementada en el servicio principal
        print(f"📅 Limpieza automática programada cada {horas_intervalo} horas")
        return True
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día actual"""
        return {
            'total_limpiezas_hoy': self.total_limpiezas,
            'ultima_limpieza': self.ultima_limpieza.isoformat() if self.ultima_limpieza else None,
            'estado_actual': self.estado,
            'motor_activo': self.motor_activo,
            'tiempo_limpieza_estandar': self.tiempo_limpieza_estandar,
            'intervalo_minimo_minutos': self.intervalo_minimo_minutos
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del limpiador"""
        minutos_desde_limpieza = None
        if self.ultima_limpieza:
            tiempo_transcurrido = datetime.now() - self.ultima_limpieza
            minutos_desde_limpieza = int(tiempo_transcurrido.total_seconds() / 60)
        
        return {
            'id': self.id,
            'estado': self.estado,
            'motor_activo': self.motor_activo,
            'total_limpiezas': self.total_limpiezas,
            'ultima_limpieza': self.ultima_limpieza.isoformat() if self.ultima_limpieza else None,
            'minutos_desde_limpieza': minutos_desde_limpieza,
            'puede_limpiar': self._puede_limpiar(),
            'tiempo_limpieza_estandar': self.tiempo_limpieza_estandar,
            'intervalo_minimo_minutos': self.intervalo_minimo_minutos
        }
    
    def __str__(self):
        return f"Limpiador arenero {self.id}: {self.estado} - Total limpiezas: {self.total_limpiezas}"
