import threading
import time
from datetime import datetime

class HubSensors:
    def __init__(self, sensores):
        self.sensores = sensores
        self.activo = False
        self.hilo_monitoreo = None
        self.intervalo_lectura = 5  # segundos entre lecturas
        self.callbacks = {}  # Callbacks para eventos de sensores
        
    def iniciar_monitoreo(self):
        """Iniciar monitoreo continuo de todos los sensores"""
        if self.activo:
            print("⚠️ Hub de sensores ya está activo")
            return
            
        self.activo = True
        self.hilo_monitoreo = threading.Thread(target=self._monitoreo_continuo, daemon=True)
        self.hilo_monitoreo.start()
        print("🚀 Hub de sensores iniciado")
    
    def detener(self):
        """Detener monitoreo de sensores"""
        self.activo = False
        if self.hilo_monitoreo:
            self.hilo_monitoreo.join(timeout=5)
        print("🛑 Hub de sensores detenido")
    
    def _monitoreo_continuo(self):
        """Función principal de monitoreo continuo"""
        while self.activo:
            try:
                for sensor in self.sensores:
                    if self.activo:  # Verificar que siga activo
                        valor = sensor.leer_valor()
                        self._procesar_evento_sensor(sensor, valor)
                        
                time.sleep(self.intervalo_lectura)
                
            except Exception as e:
                print(f"❌ Error en monitoreo continuo: {e}")
                time.sleep(self.intervalo_lectura)
    
    def _procesar_evento_sensor(self, sensor, valor):
        """Procesar eventos específicos de cada sensor"""
        try:
            # Verificar callbacks registrados
            if sensor.id in self.callbacks:
                callback = self.callbacks[sensor.id]
                callback(sensor, valor)
            
            # Procesar eventos especiales
            if hasattr(sensor, 'necesita_rellenado') and sensor.necesita_rellenado():
                print(f"🔔 ALERTA: {sensor.id} necesita rellenado")
                self._trigger_evento('nivel_critico', sensor)
            
            if hasattr(sensor, 'necesita_limpieza') and sensor.necesita_limpieza():
                print(f"🔔 ALERTA: {sensor.id} necesita limpieza")
                self._trigger_evento('limpieza_requerida', sensor)
            
            if hasattr(sensor, 'hay_presencia_actual') and sensor.hay_presencia_actual():
                print(f"🐱 Presencia detectada en {sensor.zona if hasattr(sensor, 'zona') else sensor.id}")
                self._trigger_evento('presencia_detectada', sensor)
                
        except Exception as e:
            print(f"❌ Error procesando evento de sensor {sensor.id}: {e}")
    
    def _trigger_evento(self, tipo_evento, sensor):
        """Disparar evento para otros servicios"""
        # Aquí se pueden integrar con otros servicios
        evento = {
            'tipo': tipo_evento,
            'sensor_id': sensor.id,
            'timestamp': datetime.now().isoformat(),
            'valor': sensor.valor_actual,
            'estado': sensor.estado
        }
        
        # En implementación real, esto podría publicar a un message broker
        print(f"📡 Evento: {evento}")
    
    def registrar_callback(self, sensor_id, callback_function):
        """Registrar callback para eventos de sensor específico"""
        self.callbacks[sensor_id] = callback_function
        print(f"📋 Callback registrado para sensor {sensor_id}")
    
    def obtener_sensor_por_id(self, sensor_id):
        """Obtener sensor por ID"""
        for sensor in self.sensores:
            if sensor.id == sensor_id:
                return sensor
        return None
    
    def obtener_sensores_por_tipo(self, tipo_clase):
        """Obtener sensores de un tipo específico"""
        return [sensor for sensor in self.sensores if isinstance(sensor, tipo_clase)]
    
    def leer_todos_sensores(self):
        """Leer todos los sensores una vez"""
        lecturas = {}
        for sensor in self.sensores:
            try:
                valor = sensor.leer_valor()
                lecturas[sensor.id] = {
                    'valor': valor,
                    'estado': sensor.estado,
                    'timestamp': datetime.now().isoformat()
                }
            except Exception as e:
                lecturas[sensor.id] = {
                    'error': str(e),
                    'timestamp': datetime.now().isoformat()
                }
        return lecturas
    
    def obtener_resumen_estado(self):
        """Obtener resumen del estado de todos los sensores"""
        resumen = {
            'total_sensores': len(self.sensores),
            'sensores_activos': 0,
            'sensores_con_error': 0,
            'sensores_criticos': 0,
            'ultima_actualizacion': datetime.now().isoformat()
        }
        
        for sensor in self.sensores:
            if sensor.estado == "error_lectura":
                resumen['sensores_con_error'] += 1
            elif sensor.estado in ["critico", "critico_necesita_limpieza", "vacio"]:
                resumen['sensores_criticos'] += 1
            else:
                resumen['sensores_activos'] += 1
        
        return resumen
    
    def configurar_intervalo_lectura(self, segundos):
        """Configurar intervalo entre lecturas"""
        if 1 <= segundos <= 60:
            self.intervalo_lectura = segundos
            print(f"⚙️ Intervalo de lectura configurado: {segundos}s")
            return True
        else:
            print(f"⚠️ Intervalo inválido: {segundos}s (rango: 1-60)")
            return False
    
    def obtener_estadisticas_hub(self):
        """Obtener estadísticas del hub"""
        estadisticas = []
        
        for sensor in self.sensores:
            try:
                stats = sensor.obtener_estadisticas(10)  # Últimas 10 lecturas
                if stats:
                    estadisticas.append({
                        'sensor_id': sensor.id,
                        'tipo': sensor.__class__.__name__,
                        'estadisticas': stats,
                        'estado_actual': sensor.estado
                    })
            except Exception as e:
                print(f"❌ Error obteniendo estadísticas de {sensor.id}: {e}")
        
        return estadisticas
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        return f"Hub Sensores: {estado} - {len(self.sensores)} sensores - Intervalo: {self.intervalo_lectura}s"
