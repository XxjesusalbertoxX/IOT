import threading
import time
from datetime import datetime, timedelta
from repository.mongo import Mongo

class AreneroService:
    def __init__(self, limpiador, sensor_humedad, sensor_presencia):
        self.limpiador = limpiador
        self.sensor_humedad = sensor_humedad
        self.sensor_presencia = sensor_presencia
        self.activo = False
        self.hilo_monitoreo = None
        
        # Configuración de limpieza
        self.limpieza_automatica = True
        self.umbral_humedad_limpieza = 70  # % de humedad para limpiar
        self.limpieza_programada_horas = [6, 14, 22]  # Horas del día para limpieza
        self.tiempo_espera_despues_uso = 10  # Minutos a esperar después de detectar uso
        self.ultimo_uso_detectado = None
        
        # Estadísticas
        self.total_limpiezas_hoy = 0
        self.total_usos_detectados_hoy = 0
        self.tiempo_total_uso_hoy = 0  # minutos
        
    def iniciar(self):
        """Iniciar servicio del arenero"""
        if self.activo:
            print("⚠️ Servicio de arenero ya está activo")
            return
            
        self.activo = True
        self.hilo_monitoreo = threading.Thread(target=self._monitoreo_continuo, daemon=True)
        self.hilo_monitoreo.start()
        print("🏠 Servicio de arenero iniciado")
    
    def detener(self):
        """Detener servicio del arenero"""
        self.activo = False
        if self.hilo_monitoreo:
            self.hilo_monitoreo.join(timeout=5)
        print("🛑 Servicio de arenero detenido")
    
    def _monitoreo_continuo(self):
        """Monitoreo continuo del arenero"""
        while self.activo:
            try:
                # Leer sensores
                humedad = self.sensor_humedad.leer_valor()
                presencia = self.sensor_presencia.leer_valor()
                
                # Detectar uso del arenero
                self._detectar_uso_arenero()
                
                # Verificar necesidad de limpieza automática
                if self.limpieza_automatica:
                    self._verificar_limpieza_por_humedad()
                    self._verificar_limpieza_programada()
                    self._verificar_limpieza_post_uso()
                
                # Verificar alertas
                self._verificar_alertas()
                
                time.sleep(20)  # Verificar cada 20 segundos
                
            except Exception as e:
                print(f"❌ Error en monitoreo del arenero: {e}")
                time.sleep(30)
    
    def _detectar_uso_arenero(self):
        """Detectar cuando el gato está usando el arenero"""
        if self.sensor_presencia.hay_presencia_actual():
            if not hasattr(self, '_uso_en_progreso') or not self._uso_en_progreso:
                self._uso_en_progreso = True
                self._inicio_uso_actual = datetime.now()
                self.total_usos_detectados_hoy += 1
                print(f"🐱 Uso del arenero detectado - Total hoy: {self.total_usos_detectados_hoy}")
                
        else:
            if hasattr(self, '_uso_en_progreso') and self._uso_en_progreso:
                self._uso_en_progreso = False
                self.ultimo_uso_detectado = datetime.now()
                
                if hasattr(self, '_inicio_uso_actual'):
                    duracion_uso = (datetime.now() - self._inicio_uso_actual).total_seconds() / 60
                    self.tiempo_total_uso_hoy += duracion_uso
                    print(f"✅ Uso completado - Duración: {duracion_uso:.1f} minutos")
                    
                    # Registrar en base de datos
                    try:
                        Mongo.insert_litter_record(self.ultimo_uso_detectado, "uso_detectado")
                    except Exception as e:
                        print(f"❌ Error guardando uso del arenero: {e}")
    
    def _verificar_limpieza_por_humedad(self):
        """Verificar limpieza basada en nivel de humedad"""
        if (self.sensor_humedad.valor_actual is not None and 
            self.sensor_humedad.valor_actual >= self.umbral_humedad_limpieza):
            
            if not self.limpiador.motor_activo and self._puede_limpiar():
                print(f"🧹 Limpieza automática por humedad - {self.sensor_humedad.valor_actual}%")
                self.limpiar_automatico()
    
    def _verificar_limpieza_programada(self):
        """Verificar limpieza en horarios programados"""
        hora_actual = datetime.now().hour
        
        if hora_actual in self.limpieza_programada_horas:
            if not self._ya_se_limpio_en_esta_hora() and self._puede_limpiar():
                print(f"⏰ Limpieza programada - {hora_actual}:00")
                self.limpiar_automatico()
    
    def _verificar_limpieza_post_uso(self):
        """Verificar limpieza después de uso"""
        if not self.ultimo_uso_detectado:
            return
            
        tiempo_desde_uso = datetime.now() - self.ultimo_uso_detectado
        minutos_desde_uso = tiempo_desde_uso.total_seconds() / 60
        
        if (minutos_desde_uso >= self.tiempo_espera_despues_uso and 
            not self._ya_se_limpio_despues_de_uso() and 
            self._puede_limpiar()):
            
            print(f"🧹 Limpieza post-uso - {minutos_desde_uso:.1f} minutos después")
            self.limpiar_automatico()
    
    def _puede_limpiar(self):
        """Verificar si se puede realizar limpieza"""
        # No limpiar si hay presencia actual
        if self.sensor_presencia.hay_presencia_actual():
            print("⚠️ No se puede limpiar: gato presente en el arenero")
            return False
        
        # No limpiar si el motor ya está activo
        if self.limpiador.motor_activo:
            print("⚠️ Limpiador ya está activo")
            return False
        
        # Verificar intervalo mínimo del limpiador
        return self.limpiador._puede_limpiar()
    
    def _ya_se_limpio_en_esta_hora(self):
        """Verificar si ya se limpió en esta hora"""
        if not self.limpiador.ultima_limpieza:
            return False
            
        hora_actual = datetime.now().hour
        hora_ultima_limpieza = self.limpiador.ultima_limpieza.hour
        
        return hora_actual == hora_ultima_limpieza
    
    def _ya_se_limpio_despues_de_uso(self):
        """Verificar si ya se limpió después del último uso"""
        if not self.limpiador.ultima_limpieza or not self.ultimo_uso_detectado:
            return False
            
        return self.limpiador.ultima_limpieza > self.ultimo_uso_detectado
    
    def _verificar_alertas(self):
        """Verificar y enviar alertas necesarias"""
        # Alerta de humedad crítica
        if self.sensor_humedad.necesita_limpieza():
            print(f"🔔 ALERTA: Arenero necesita limpieza - Humedad: {self.sensor_humedad.valor_actual}%")
            self._enviar_alerta_limpieza()
        
        # Alerta de uso prolongado sin limpieza
        if self.ultimo_uso_detectado:
            tiempo_sin_limpieza = datetime.now() - (self.limpiador.ultima_limpieza or datetime.min)
            horas_sin_limpieza = tiempo_sin_limpieza.total_seconds() / 3600
            
            if horas_sin_limpieza > 8:  # Más de 8 horas sin limpieza
                print(f"🚨 ALERTA: {horas_sin_limpieza:.1f} horas sin limpieza del arenero")
                self._enviar_alerta_limpieza_urgente()
    
    def limpiar_automatico(self):
        """Realizar limpieza automática"""
        try:
            print("🧹 Iniciando limpieza automática...")
            
            success = self.limpiador.limpiar()
            
            if success:
                self.total_limpiezas_hoy += 1
                
                # Registrar en base de datos
                try:
                    Mongo.insert_litter_record(datetime.now(), "limpieza_automatica")
                except Exception as e:
                    print(f"❌ Error guardando limpieza: {e}")
                
                print(f"✅ Limpieza automática completada - Total hoy: {self.total_limpiezas_hoy}")
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error en limpieza automática: {e}")
            return False
    
    def limpiar_manual(self, tipo_limpieza="normal"):
        """Realizar limpieza manual"""
        try:
            if not self._puede_limpiar():
                return False
            
            print(f"🧹 Iniciando limpieza manual ({tipo_limpieza})...")
            
            if tipo_limpieza == "rapida":
                success = self.limpiador.limpieza_rapida()
            elif tipo_limpieza == "profunda":
                success = self.limpiador.limpieza_profunda()
            else:
                success = self.limpiador.limpiar()
            
            if success:
                self.total_limpiezas_hoy += 1
                
                # Registrar en base de datos
                try:
                    Mongo.insert_litter_record(datetime.now(), f"limpieza_manual_{tipo_limpieza}")
                except Exception as e:
                    print(f"❌ Error guardando limpieza: {e}")
                
                print(f"✅ Limpieza manual completada - Total hoy: {self.total_limpiezas_hoy}")
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error en limpieza manual: {e}")
            return False
    
    def configurar_limpieza_automatica(self, activar, umbral_humedad=None, horas_programadas=None):
        """Configurar parámetros de limpieza automática"""
        self.limpieza_automatica = activar
        
        if umbral_humedad and 50 <= umbral_humedad <= 90:
            self.umbral_humedad_limpieza = umbral_humedad
        
        if horas_programadas and isinstance(horas_programadas, list):
            self.limpieza_programada_horas = horas_programadas
        
        estado = "activada" if activar else "desactivada"
        print(f"⚙️ Limpieza automática {estado} - Umbral: {self.umbral_humedad_limpieza}% - Horas: {self.limpieza_programada_horas}")
        return True
    
    def configurar_tiempo_espera_post_uso(self, minutos):
        """Configurar tiempo de espera después de uso antes de limpiar"""
        if 5 <= minutos <= 60:
            self.tiempo_espera_despues_uso = minutos
            print(f"⚙️ Tiempo de espera post-uso configurado: {minutos} minutos")
            return True
        else:
            print(f"⚠️ Tiempo inválido: {minutos} minutos (rango: 5-60)")
            return False
    
    def _enviar_alerta_limpieza(self):
        """Enviar alerta de limpieza necesaria"""
        print("📧 Enviando notificación: Arenero necesita limpieza")
        # Aquí se integraría con sistema de notificaciones
    
    def _enviar_alerta_limpieza_urgente(self):
        """Enviar alerta urgente de limpieza"""
        print("🚨 Enviando alerta urgente: Arenero necesita limpieza inmediata")
        # Aquí se integraría con sistema de notificaciones de emergencia
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día"""
        promedio_duracion_uso = 0
        if self.total_usos_detectados_hoy > 0:
            promedio_duracion_uso = self.tiempo_total_uso_hoy / self.total_usos_detectados_hoy
        
        return {
            'total_limpiezas_hoy': self.total_limpiezas_hoy,
            'total_usos_detectados_hoy': self.total_usos_detectados_hoy,
            'tiempo_total_uso_hoy': round(self.tiempo_total_uso_hoy, 1),
            'promedio_duracion_uso': round(promedio_duracion_uso, 1),
            'humedad_actual': self.sensor_humedad.valor_actual,
            'presencia_actual': self.sensor_presencia.hay_presencia_actual(),
            'ultimo_uso': self.ultimo_uso_detectado.isoformat() if self.ultimo_uso_detectado else None,
            'limpieza_automatica': self.limpieza_automatica
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del servicio"""
        return {
            'activo': self.activo,
            'limpiador_estado': self.limpiador.estado,
            'sensor_humedad': self.sensor_humedad.obtener_info_completa(),
            'sensor_presencia': self.sensor_presencia.obtener_info_completa(),
            'estadisticas_diarias': self.obtener_estadisticas_diarias(),
            'configuracion': {
                'umbral_humedad_limpieza': self.umbral_humedad_limpieza,
                'limpieza_programada_horas': self.limpieza_programada_horas,
                'tiempo_espera_despues_uso': self.tiempo_espera_despues_uso
            }
        }
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        return f"Arenero Service: {estado} - Limpiezas hoy: {self.total_limpiezas_hoy} - Usos: {self.total_usos_detectados_hoy}"
