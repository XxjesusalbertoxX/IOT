import threading
import time
from datetime import datetime, timedelta
from repository.mongo import Mongo

class GatoMonitoring:
    def __init__(self, camara, sensor_presencia, comedero, bebedero, arenero):
        self.camara = camara
        self.sensor_presencia = sensor_presencia
        self.comedero = comedero
        self.bebedero = bebedero
        self.arenero = arenero
        self.activo = False
        self.hilo_monitoreo = None
        
        # Configuración de monitoreo
        self.intervalo_monitoreo = 30  # segundos
        self.deteccion_actividad_activa = True
        self.captura_fotos_automatica = True
        self.alertas_actividad_activas = True
        
        # Estadísticas del gato
        self.actividad_diaria = {
            'comedero': {'visitas': 0, 'tiempo_total': 0},
            'bebedero': {'visitas': 0, 'tiempo_total': 0},
            'arenero': {'visitas': 0, 'tiempo_total': 0},
            'general': {'actividad_total': 0, 'fotos_capturadas': 0}
        }
        
        # Patrones de actividad
        self.patron_actividad_normal = {
            'comedero_visitas_dia': (3, 8),      # Min y máx visitas normales
            'bebedero_visitas_dia': (2, 6),      # Min y máx visitas normales
            'arenero_visitas_dia': (1, 4),       # Min y máx visitas normales
            'horas_activo_dia': (8, 16)          # Horas activo normales
        }
        
        # Control de inactividad
        self.tiempo_maximo_sin_actividad = 6  # horas
        self.ultima_actividad_detectada = None
        
    def iniciar(self):
        """Iniciar servicio de monitoreo del gato"""
        if self.activo:
            print("⚠️ Servicio de monitoreo de gato ya está activo")
            return
            
        self.activo = True
        self.hilo_monitoreo = threading.Thread(target=self._monitoreo_continuo, daemon=True)
        self.hilo_monitoreo.start()
        print("🐱 Servicio de monitoreo de gato iniciado")
    
    def detener(self):
        """Detener servicio de monitoreo"""
        self.activo = False
        if self.hilo_monitoreo:
            self.hilo_monitoreo.join(timeout=5)
        print("🛑 Servicio de monitoreo de gato detenido")
    
    def _monitoreo_continuo(self):
        """Monitoreo continuo de la actividad del gato"""
        while self.activo:
            try:
                # Monitorear actividad general
                self._monitorear_actividad_general()
                
                # Monitorear zonas específicas
                self._monitorear_zonas_especificas()
                
                # Capturar fotos periódicamente
                if self.captura_fotos_automatica:
                    self._captura_periodica()
                
                # Verificar patrones de salud
                self._verificar_patrones_salud()
                
                # Verificar inactividad prolongada
                self._verificar_inactividad()
                
                time.sleep(self.intervalo_monitoreo)
                
            except Exception as e:
                print(f"❌ Error en monitoreo del gato: {e}")
                time.sleep(60)
    
    def _monitorear_actividad_general(self):
        """Monitorear actividad general del gato"""
        try:
            # Leer sensor de presencia general
            presencia_general = self.sensor_presencia.leer_valor()
            
            if self.sensor_presencia.hay_presencia_actual():
                self.ultima_actividad_detectada = datetime.now()
                self.actividad_diaria['general']['actividad_total'] += 1
                
                # Registrar actividad en base de datos
                self._registrar_actividad_general()
                
        except Exception as e:
            print(f"❌ Error monitoreando actividad general: {e}")
    
    def _monitorear_zonas_especificas(self):
        """Monitorear actividad en zonas específicas"""
        try:
            # Monitorear comedero
            if hasattr(self.comedero, 'sensor_presencia'):
                if self.comedero.sensor_presencia.hay_presencia_actual():
                    self._registrar_visita_zona('comedero')
            
            # Monitorear bebedero (basado en presencia en comedero como aproximación)
            # En implementación real tendría su propio sensor
            
            # Monitorear arenero
            if self.arenero.sensor_presencia.hay_presencia_actual():
                self._registrar_visita_zona('arenero')
                
        except Exception as e:
            print(f"❌ Error monitoreando zonas específicas: {e}")
    
    def _registrar_visita_zona(self, zona):
        """Registrar visita a zona específica"""
        self.actividad_diaria[zona]['visitas'] += 1
        self.ultima_actividad_detectada = datetime.now()
        
        print(f"🐱 Actividad en {zona} - Visita #{self.actividad_diaria[zona]['visitas']}")
        
        # Registrar en base de datos
        try:
            data = {
                'zona': zona,
                'timestamp': datetime.now(),
                'tipo_evento': 'visita_zona'
            }
            # Mongo.insert_activity_record(data)  # Implementar método específico
        except Exception as e:
            print(f"❌ Error registrando visita a {zona}: {e}")
    
    def _captura_periodica(self):
        """Capturar fotos periódicamente"""
        try:
            # Capturar foto cada 10 ciclos de monitoreo (5 minutos aprox)
            if not hasattr(self, '_contador_capturas'):
                self._contador_capturas = 0
            
            self._contador_capturas += 1
            
            if self._contador_capturas >= 10:
                self._contador_capturas = 0
                
                # Solo capturar si hay actividad reciente
                if self.ultima_actividad_detectada:
                    tiempo_desde_actividad = datetime.now() - self.ultima_actividad_detectada
                    if tiempo_desde_actividad.total_seconds() <= 300:  # 5 minutos
                        foto_capturada = self.camara.capturar_foto_manual()
                        if foto_capturada:
                            self.actividad_diaria['general']['fotos_capturadas'] += 1
                
        except Exception as e:
            print(f"❌ Error en captura periódica: {e}")
    
    def _verificar_patrones_salud(self):
        """Verificar patrones de actividad para detectar problemas de salud"""
        try:
            # Verificar si las visitas están dentro de rangos normales
            comedero_visitas = self.actividad_diaria['comedero']['visitas']
            arenero_visitas = self.actividad_diaria['arenero']['visitas']
            
            # Alerta por pocas visitas al comedero
            if comedero_visitas < self.patron_actividad_normal['comedero_visitas_dia'][0]:
                print(f"⚠️ ALERTA: Pocas visitas al comedero hoy ({comedero_visitas})")
                self._enviar_alerta_salud('comedero_poco_uso')
            
            # Alerta por muchas visitas al arenero (posible problema digestivo)
            if arenero_visitas > self.patron_actividad_normal['arenero_visitas_dia'][1]:
                print(f"⚠️ ALERTA: Muchas visitas al arenero hoy ({arenero_visitas})")
                self._enviar_alerta_salud('arenero_uso_excesivo')
            
            # Verificar inactividad general
            actividad_total = self.actividad_diaria['general']['actividad_total']
            if actividad_total < 10:  # Muy poca actividad en el día
                print(f"⚠️ ALERTA: Baja actividad general hoy ({actividad_total})")
                self._enviar_alerta_salud('baja_actividad')
                
        except Exception as e:
            print(f"❌ Error verificando patrones de salud: {e}")
    
    def _verificar_inactividad(self):
        """Verificar inactividad prolongada"""
        if not self.ultima_actividad_detectada:
            return
            
        tiempo_sin_actividad = datetime.now() - self.ultima_actividad_detectada
        horas_sin_actividad = tiempo_sin_actividad.total_seconds() / 3600
        
        if horas_sin_actividad >= self.tiempo_maximo_sin_actividad:
            print(f"🚨 ALERTA CRÍTICA: {horas_sin_actividad:.1f} horas sin actividad detectada")
            self._enviar_alerta_critica_inactividad(horas_sin_actividad)
    
    def _registrar_actividad_general(self):
        """Registrar actividad general en base de datos"""
        try:
            data = {
                'gato_id': 'gato_principal',
                'timestamp': datetime.now(),
                'tipo_actividad': 'presencia_general',
                'sensor_id': self.sensor_presencia.id
            }
            # Mongo.insert_activity_record(data)  # Implementar método específico
        except Exception as e:
            print(f"❌ Error registrando actividad general: {e}")
    
    def forzar_captura_foto(self):
        """Forzar captura de foto manualmente"""
        try:
            foto_capturada = self.camara.capturar_foto_manual()
            if foto_capturada:
                self.actividad_diaria['general']['fotos_capturadas'] += 1
                print("📸 Foto capturada manualmente")
                return True
            return False
        except Exception as e:
            print(f"❌ Error capturando foto manual: {e}")
            return False
    
    def obtener_resumen_actividad_diaria(self):
        """Obtener resumen de actividad del día"""
        return {
            'actividad_por_zona': self.actividad_diaria,
            'ultima_actividad': self.ultima_actividad_detectada.isoformat() if self.ultima_actividad_detectada else None,
            'tiempo_sin_actividad_horas': self._calcular_horas_sin_actividad(),
            'patrones_normales': self._evaluar_patrones_normales(),
            'alertas_activas': self._obtener_alertas_activas()
        }
    
    def _calcular_horas_sin_actividad(self):
        """Calcular horas sin actividad"""
        if not self.ultima_actividad_detectada:
            return None
        
        tiempo_sin_actividad = datetime.now() - self.ultima_actividad_detectada
        return round(tiempo_sin_actividad.total_seconds() / 3600, 1)
    
    def _evaluar_patrones_normales(self):
        """Evaluar si los patrones están dentro de lo normal"""
        comedero_normal = (self.patron_actividad_normal['comedero_visitas_dia'][0] <= 
                          self.actividad_diaria['comedero']['visitas'] <= 
                          self.patron_actividad_normal['comedero_visitas_dia'][1])
        
        arenero_normal = (self.patron_actividad_normal['arenero_visitas_dia'][0] <= 
                         self.actividad_diaria['arenero']['visitas'] <= 
                         self.patron_actividad_normal['arenero_visitas_dia'][1])
        
        return {
            'comedero_normal': comedero_normal,
            'arenero_normal': arenero_normal,
            'actividad_general_normal': self.actividad_diaria['general']['actividad_total'] >= 10
        }
    
    def _obtener_alertas_activas(self):
        """Obtener lista de alertas activas"""
        alertas = []
        
        # Verificar cada patrón
        patrones = self._evaluar_patrones_normales()
        
        if not patrones['comedero_normal']:
            alertas.append('patron_comedero_anormal')
        
        if not patrones['arenero_normal']:
            alertas.append('patron_arenero_anormal')
        
        if not patrones['actividad_general_normal']:
            alertas.append('baja_actividad_general')
        
        # Verificar inactividad prolongada
        horas_sin_actividad = self._calcular_horas_sin_actividad()
        if horas_sin_actividad and horas_sin_actividad >= self.tiempo_maximo_sin_actividad:
            alertas.append('inactividad_prolongada')
        
        return alertas
    
    def configurar_patrones_normales(self, nuevos_patrones):
        """Configurar nuevos patrones de actividad normal"""
        self.patron_actividad_normal.update(nuevos_patrones)
        print("⚙️ Patrones de actividad normal actualizados")
        return True
    
    def configurar_alertas(self, activar_alertas, tiempo_max_inactividad=None):
        """Configurar sistema de alertas"""
        self.alertas_actividad_activas = activar_alertas
        
        if tiempo_max_inactividad and 2 <= tiempo_max_inactividad <= 24:
            self.tiempo_maximo_sin_actividad = tiempo_max_inactividad
        
        estado = "activadas" if activar_alertas else "desactivadas"
        print(f"⚙️ Alertas de actividad {estado} - Máx inactividad: {self.tiempo_maximo_sin_actividad}h")
        return True
    
    def _enviar_alerta_salud(self, tipo_alerta):
        """Enviar alerta de salud"""
        if self.alertas_actividad_activas:
            print(f"📧 Enviando alerta de salud: {tipo_alerta}")
            # Aquí se integraría con sistema de notificaciones
    
    def _enviar_alerta_critica_inactividad(self, horas_sin_actividad):
        """Enviar alerta crítica por inactividad"""
        print(f"🚨 Enviando alerta crítica: {horas_sin_actividad:.1f} horas sin actividad")
        # Aquí se integraría con sistema de notificaciones de emergencia
    
    def obtener_info_completa(self):
        """Obtener información completa del servicio"""
        return {
            'activo': self.activo,
            'camara_estado': self.camara.obtener_info_completa(),
            'sensor_presencia': self.sensor_presencia.obtener_info_completa(),
            'resumen_actividad': self.obtener_resumen_actividad_diaria(),
            'configuracion': {
                'intervalo_monitoreo': self.intervalo_monitoreo,
                'deteccion_actividad_activa': self.deteccion_actividad_activa,
                'captura_fotos_automatica': self.captura_fotos_automatica,
                'alertas_actividad_activas': self.alertas_actividad_activas,
                'tiempo_maximo_sin_actividad': self.tiempo_maximo_sin_actividad
            }
        }
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        actividad_total = self.actividad_diaria['general']['actividad_total']
        return f"Gato Monitoring: {estado} - Actividad total hoy: {actividad_total}"
