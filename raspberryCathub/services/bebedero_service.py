import threading
import time
from datetime import datetime, timedelta
from repository.mongo import Mongo

class BebederoService:
    def __init__(self, dispensador, sensor_nivel, sensor_calidad):
        self.dispensador = dispensador
        self.sensor_nivel = sensor_nivel
        self.sensor_calidad = sensor_calidad
        self.activo = False
        self.hilo_monitoreo = None
        
        # Configuración del bebedero
        self.rellenado_automatico = True
        self.nivel_objetivo = 80  # Porcentaje
        self.nivel_minimo_rellenado = 20  # Cuando rellenar automáticamente
        self.circulacion_automatica = True
        self.intervalo_circulacion_horas = 4  # Circular agua cada 4 horas
        self.ultima_circulacion = None
        
        # Estadísticas
        self.total_dispensado_hoy = 0
        self.numero_rellenados_hoy = 0
        self.total_circulaciones_hoy = 0
        
    def iniciar(self):
        """Iniciar servicio del bebedero"""
        if self.activo:
            print("⚠️ Servicio de bebedero ya está activo")
            return
            
        self.activo = True
        self.hilo_monitoreo = threading.Thread(target=self._monitoreo_continuo, daemon=True)
        self.hilo_monitoreo.start()
        print("💧 Servicio de bebedero iniciado")
    
    def detener(self):
        """Detener servicio del bebedero"""
        self.activo = False
        if self.hilo_monitoreo:
            self.hilo_monitoreo.join(timeout=5)
        print("🛑 Servicio de bebedero detenido")
    
    def _monitoreo_continuo(self):
        """Monitoreo continuo del bebedero"""
        while self.activo:
            try:
                # Leer sensores
                nivel = self.sensor_nivel.leer_valor()
                calidad = self.sensor_calidad.leer_valor()
                
                # Verificar necesidad de rellenado automático
                if self.rellenado_automatico:
                    self._verificar_rellenado_automatico()
                
                # Verificar necesidad de circulación
                if self.circulacion_automatica:
                    self._verificar_circulacion_automatica()
                
                # Verificar calidad del agua
                self._verificar_calidad_agua()
                
                # Verificar alertas
                self._verificar_alertas()
                
                time.sleep(15)  # Verificar cada 15 segundos
                
            except Exception as e:
                print(f"❌ Error en monitoreo del bebedero: {e}")
                time.sleep(30)
    
    def _verificar_rellenado_automatico(self):
        """Verificar si necesita rellenado automático"""
        if (self.sensor_nivel.valor_actual is not None and 
            self.sensor_nivel.valor_actual <= self.nivel_minimo_rellenado):
            
            if not self.dispensador.bomba_activa:
                print(f"🔄 Rellenado automático - Nivel: {self.sensor_nivel.valor_actual}%")
                self.rellenar_automatico()
    
    def _verificar_circulacion_automatica(self):
        """Verificar si necesita circulación automática"""
        if not self.ultima_circulacion:
            self.circular_agua_automatica()
            return
        
        tiempo_transcurrido = datetime.now() - self.ultima_circulacion
        horas_transcurridas = tiempo_transcurrido.total_seconds() / 3600
        
        if horas_transcurridas >= self.intervalo_circulacion_horas:
            print(f"🔄 Circulación automática - Hace {horas_transcurridas:.1f} horas")
            self.circular_agua_automatica()
    
    def _verificar_calidad_agua(self):
        """Verificar calidad del agua"""
        if self.sensor_calidad.valor_actual is not None:
            if self.sensor_calidad.estado in ["muy_caliente", "muy_frio"]:
                print(f"⚠️ Temperatura del agua no ideal: {self.sensor_calidad.valor_actual}°C")
                self._enviar_alerta_temperatura()
    
    def _verificar_alertas(self):
        """Verificar y enviar alertas necesarias"""
        # Alerta de nivel bajo
        if self.sensor_nivel.necesita_rellenado():
            print(f"🔔 ALERTA: Bebedero necesita rellenado - Nivel: {self.sensor_nivel.valor_actual}%")
            self._enviar_alerta_rellenado()
        
        # Alerta crítica de nivel vacío
        if self.sensor_nivel.esta_vacio():
            print(f"🚨 CRÍTICO: Bebedero vacío - Nivel: {self.sensor_nivel.valor_actual}%")
            self._enviar_alerta_critica()
    
    def rellenar_automatico(self):
        """Rellenar bebedero automáticamente hasta nivel objetivo"""
        try:
            if self.dispensador.bomba_activa:
                print("⚠️ Dispensador ya está activo")
                return False
            
            print(f"💧 Rellenando automáticamente hasta {self.nivel_objetivo}%...")
            
            success = self.dispensador.llenar_bebedero(self.nivel_objetivo)
            
            if success:
                self.numero_rellenados_hoy += 1
                cantidad_dispensada = self._calcular_cantidad_dispensada()
                self.total_dispensado_hoy += cantidad_dispensada
                
                # Registrar en base de datos
                try:
                    Mongo.insert_water_record("gato_principal", cantidad_dispensada, datetime.now())
                except Exception as e:
                    print(f"❌ Error guardando registro de agua: {e}")
                
                print(f"✅ Rellenado completado: ~{cantidad_dispensada}ml")
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error en rellenado automático: {e}")
            return False
    
    def rellenar_manual(self, cantidad_ml):
        """Rellenar bebedero manualmente"""
        try:
            if self.dispensador.bomba_activa:
                print("⚠️ Dispensador ya está activo")
                return False
            
            success = self.dispensador.dispensar(cantidad_ml)
            
            if success:
                self.total_dispensado_hoy += cantidad_ml
                
                # Registrar en base de datos
                try:
                    Mongo.insert_water_record("gato_principal", cantidad_ml, datetime.now())
                except Exception as e:
                    print(f"❌ Error guardando registro de agua: {e}")
                
                print(f"✅ Rellenado manual completado: {cantidad_ml}ml")
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error en rellenado manual: {e}")
            return False
    
    def circular_agua_automatica(self, duracion_segundos=10):
        """Circular agua automáticamente para mantenerla fresca"""
        try:
            print(f"🔄 Circulando agua por {duracion_segundos}s...")
            
            success = self.dispensador.activar_circulacion(duracion_segundos)
            
            if success:
                self.ultima_circulacion = datetime.now()
                self.total_circulaciones_hoy += 1
                print(f"✅ Circulación completada")
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error circulando agua: {e}")
            return False
    
    def _calcular_cantidad_dispensada(self):
        """Calcular cantidad aproximada dispensada en el último rellenado"""
        # Estimación basada en capacidad y nivel objetivo
        if self.sensor_nivel.valor_actual is not None:
            nivel_anterior = self.nivel_minimo_rellenado  # Aproximación
            diferencia_nivel = self.nivel_objetivo - nivel_anterior
            cantidad_estimada = (diferencia_nivel / 100) * self.sensor_nivel.capacidad_maxima_ml
            return round(cantidad_estimada, 1)
        return 0
    
    def configurar_rellenado_automatico(self, activar, nivel_objetivo=None, nivel_minimo=None):
        """Configurar parámetros de rellenado automático"""
        self.rellenado_automatico = activar
        
        if nivel_objetivo and 30 <= nivel_objetivo <= 100:
            self.nivel_objetivo = nivel_objetivo
        
        if nivel_minimo and 5 <= nivel_minimo <= 50:
            self.nivel_minimo_rellenado = nivel_minimo
        
        estado = "activado" if activar else "desactivado"
        print(f"⚙️ Rellenado automático {estado} - Objetivo: {self.nivel_objetivo}% - Mínimo: {self.nivel_minimo_rellenado}%")
        return True
    
    def configurar_circulacion_automatica(self, activar, intervalo_horas=None):
        """Configurar circulación automática"""
        self.circulacion_automatica = activar
        
        if intervalo_horas and 1 <= intervalo_horas <= 24:
            self.intervalo_circulacion_horas = intervalo_horas
        
        estado = "activada" if activar else "desactivada"
        print(f"⚙️ Circulación automática {estado} - Intervalo: {self.intervalo_circulacion_horas}h")
        return True
    
    def forzar_circulacion(self, duracion_segundos=15):
        """Forzar circulación manual"""
        return self.circular_agua_automatica(duracion_segundos)
    
    def verificar_calidad_agua_manual(self):
        """Verificar calidad del agua manualmente"""
        try:
            calidad = self.dispensador.verificar_calidad_agua()
            if calidad:
                print(f"🧪 Calidad del agua verificada: {calidad}")
                return calidad
            return None
        except Exception as e:
            print(f"❌ Error verificando calidad: {e}")
            return None
    
    def _enviar_alerta_rellenado(self):
        """Enviar alerta de rellenado necesario"""
        print("📧 Enviando notificación: Bebedero necesita rellenado")
        # Aquí se integraría con sistema de notificaciones
    
    def _enviar_alerta_critica(self):
        """Enviar alerta crítica de bebedero vacío"""
        print("🚨 Enviando alerta crítica: Bebedero vacío")
        # Aquí se integraría con sistema de notificaciones de emergencia
    
    def _enviar_alerta_temperatura(self):
        """Enviar alerta de temperatura del agua"""
        print("📧 Enviando notificación: Temperatura del agua no ideal")
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día"""
        return {
            'total_dispensado_hoy': self.total_dispensado_hoy,
            'numero_rellenados_hoy': self.numero_rellenados_hoy,
            'total_circulaciones_hoy': self.total_circulaciones_hoy,
            'nivel_agua_actual': self.sensor_nivel.valor_actual,
            'temperatura_agua_actual': self.sensor_calidad.valor_actual,
            'rellenado_automatico': self.rellenado_automatico,
            'circulacion_automatica': self.circulacion_automatica,
            'ultima_circulacion': self.ultima_circulacion.isoformat() if self.ultima_circulacion else None
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del servicio"""
        return {
            'activo': self.activo,
            'dispensador_estado': self.dispensador.estado,
            'sensor_nivel': self.sensor_nivel.obtener_info_completa(),
            'sensor_calidad': self.sensor_calidad.obtener_info_completa(),
            'estadisticas_diarias': self.obtener_estadisticas_diarias(),
            'configuracion': {
                'nivel_objetivo': self.nivel_objetivo,
                'nivel_minimo_rellenado': self.nivel_minimo_rellenado,
                'intervalo_circulacion_horas': self.intervalo_circulacion_horas
            }
        }
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        return f"Bebedero Service: {estado} - Dispensado hoy: {self.total_dispensado_hoy}ml - Rellenados: {self.numero_rellenados_hoy}"
