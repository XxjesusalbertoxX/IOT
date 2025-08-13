import threading
import time
from datetime import datetime, timedelta
from repository.mongo import Mongo

class ComederoService:
    def __init__(self, dispensador, sensor_nivel, sensor_peso):
        self.dispensador = dispensador
        self.sensor_nivel = sensor_nivel
        self.sensor_peso = sensor_peso
        self.activo = False
        self.hilo_monitoreo = None
        
        # Configuración de alimentación
        self.horarios_alimentacion = [
            {"hora": "07:00", "porcion": "normal"},   # Desayuno
            {"hora": "14:00", "porcion": "normal"},   # Almuerzo
            {"hora": "20:00", "porcion": "normal"}    # Cena
        ]
        
        self.alimentacion_automatica = True
        self.porcion_por_deteccion = "pequena"  # pequeña, normal, grande
        self.tiempo_espera_entre_porciones = 30  # minutos
        self.ultima_porcion_dispensada = None
        
        # Estadísticas
        self.total_dispensado_hoy = 0
        self.numero_alimentaciones_hoy = 0
        
    def iniciar(self):
        """Iniciar servicio del comedero"""
        if self.activo:
            print("⚠️ Servicio de comedero ya está activo")
            return
            
        self.activo = True
        self.hilo_monitoreo = threading.Thread(target=self._monitoreo_continuo, daemon=True)
        self.hilo_monitoreo.start()
        print("🍽️ Servicio de comedero iniciado")
    
    def detener(self):
        """Detener servicio del comedero"""
        self.activo = False
        if self.hilo_monitoreo:
            self.hilo_monitoreo.join(timeout=5)
        print("🛑 Servicio de comedero detenido")
    
    def _monitoreo_continuo(self):
        """Monitoreo continuo del comedero"""
        while self.activo:
            try:
                # Leer sensores
                nivel = self.sensor_nivel.leer_valor()
                peso = self.sensor_peso.leer_valor()
                
                # Verificar si necesita rellenado
                self._verificar_necesidad_rellenado()
                
                # Verificar alimentación programada
                if self.alimentacion_automatica:
                    self._verificar_horarios_alimentacion()
                
                # Detectar si hay gato comiendo
                self._detectar_actividad_alimentacion()
                
                time.sleep(10)  # Verificar cada 10 segundos
                
            except Exception as e:
                print(f"❌ Error en monitoreo del comedero: {e}")
                time.sleep(30)
    
    def _verificar_necesidad_rellenado(self):
        """Verificar si el comedero necesita ser rellenado"""
        if self.sensor_nivel.necesita_rellenado():
            print(f"🔔 ALERTA: Comedero necesita rellenado - Nivel: {self.sensor_nivel.valor_actual}%")
            self._enviar_alerta_rellenado()
        
        if self.sensor_nivel.esta_vacio():
            print(f"🚨 CRÍTICO: Comedero vacío - Nivel: {self.sensor_nivel.valor_actual}%")
            self._enviar_alerta_critica()
    
    def _verificar_horarios_alimentacion(self):
        """Verificar si es hora de alimentación programada"""
        ahora = datetime.now()
        hora_actual = ahora.strftime("%H:%M")
        
        for horario in self.horarios_alimentacion:
            if hora_actual == horario["hora"]:
                if not self._ya_se_alimento_en_horario(horario["hora"]):
                    print(f"⏰ Hora de alimentación: {horario['hora']}")
                    self.dispensar_porcion_automatica(horario["porcion"])
    
    def _ya_se_alimento_en_horario(self, hora_horario):
        """Verificar si ya se alimentó en este horario"""
        if not self.ultima_porcion_dispensada:
            return False
        
        # Verificar si la última porción fue en los últimos 30 minutos del horario
        hora_horario_dt = datetime.strptime(hora_horario, "%H:%M").time()
        ultima_porcion_dt = self.ultima_porcion_dispensada
        
        # Lógica simplificada - en implementación real sería más robusta
        diferencia = datetime.now() - ultima_porcion_dt
        return diferencia.total_seconds() < 1800  # 30 minutos
    
    def _detectar_actividad_alimentacion(self):
        """Detectar si hay actividad de alimentación basada en peso"""
        if self.sensor_peso.se_consumio_comida():
            print(f"🍽️ Detección de consumo: {abs(self.sensor_peso.diferencia_peso):.1f}g")
            self._registrar_consumo_detectado(abs(self.sensor_peso.diferencia_peso))
    
    def dispensar_porcion_automatica(self, tipo_porcion="normal"):
        """Dispensar porción automáticamente"""
        try:
            if not self._puede_dispensar():
                return False
            
            print(f"🍽️ Dispensando porción {tipo_porcion}...")
            
            if tipo_porcion == "pequena":
                success = self.dispensador.dispensar_porcion_pequena()
            elif tipo_porcion == "grande":
                success = self.dispensador.dispensar_porcion_grande()
            else:
                success = self.dispensador.dispensar_porcion_normal()
            
            if success:
                self._registrar_dispensado_exitoso(tipo_porcion)
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error dispensando porción automática: {e}")
            return False
    
    def dispensar_porcion_manual(self, cantidad_gramos):
        """Dispensar porción manual"""
        try:
            if not self._puede_dispensar():
                return False
            
            success = self.dispensador.dispensar(cantidad_gramos)
            
            if success:
                self._registrar_dispensado_exitoso("manual", cantidad_gramos)
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error dispensando porción manual: {e}")
            return False
    
    def _puede_dispensar(self):
        """Verificar si se puede dispensar comida"""
        # Verificar si hay suficiente comida
        if self.sensor_nivel.esta_vacio():
            print("⚠️ No se puede dispensar: comedero vacío")
            return False
        
        # Verificar tiempo entre porciones
        if self.ultima_porcion_dispensada:
            tiempo_transcurrido = datetime.now() - self.ultima_porcion_dispensada
            minutos_transcurridos = tiempo_transcurrido.total_seconds() / 60
            
            if minutos_transcurridos < self.tiempo_espera_entre_porciones:
                minutos_restantes = int(self.tiempo_espera_entre_porciones - minutos_transcurridos)
                print(f"⏰ Debe esperar {minutos_restantes} minutos antes de dispensar")
                return False
        
        # Verificar que el dispensador no esté activo
        if self.dispensador.motor_activo:
            print("⚠️ Dispensador ya está activo")
            return False
        
        return True
    
    def _registrar_dispensado_exitoso(self, tipo_porcion, cantidad_custom=None):
        """Registrar dispensado exitoso"""
        self.ultima_porcion_dispensada = datetime.now()
        self.numero_alimentaciones_hoy += 1
        
        # Calcular cantidad dispensada
        if cantidad_custom:
            cantidad = cantidad_custom
        elif tipo_porcion == "pequena":
            cantidad = 17
        elif tipo_porcion == "grande":
            cantidad = 55
        else:
            cantidad = 35
        
        self.total_dispensado_hoy += cantidad
        
        # Guardar en base de datos
        try:
            Mongo.insert_feeding_record("gato_principal", cantidad, datetime.now())
        except Exception as e:
            print(f"❌ Error guardando registro de alimentación: {e}")
        
        print(f"✅ Porción dispensada: {cantidad}g ({tipo_porcion})")
    
    def _registrar_consumo_detectado(self, cantidad_consumida):
        """Registrar consumo detectado por sensor de peso"""
        try:
            # Registrar en base de datos
            Mongo.insert_feeding_record("gato_principal", -cantidad_consumida, datetime.now())
            print(f"📊 Consumo registrado: {cantidad_consumida}g")
        except Exception as e:
            print(f"❌ Error guardando consumo: {e}")
    
    def _enviar_alerta_rellenado(self):
        """Enviar alerta de rellenado necesario"""
        print("📧 Enviando notificación: Comedero necesita rellenado")
        # Aquí se integraría con sistema de notificaciones
    
    def _enviar_alerta_critica(self):
        """Enviar alerta crítica de comedero vacío"""
        print("🚨 Enviando alerta crítica: Comedero vacío")
        # Aquí se integraría con sistema de notificaciones de emergencia
    
    def configurar_horarios_alimentacion(self, nuevos_horarios):
        """Configurar nuevos horarios de alimentación"""
        self.horarios_alimentacion = nuevos_horarios
        print(f"⚙️ Horarios de alimentación actualizados: {len(nuevos_horarios)} horarios")
        return True
    
    def toggle_alimentacion_automatica(self):
        """Alternar alimentación automática"""
        self.alimentacion_automatica = not self.alimentacion_automatica
        estado = "activada" if self.alimentacion_automatica else "desactivada"
        print(f"⚙️ Alimentación automática {estado}")
        return self.alimentacion_automatica
    
    def obtener_estadisticas_diarias(self):
        """Obtener estadísticas del día"""
        return {
            'total_dispensado_hoy': self.total_dispensado_hoy,
            'numero_alimentaciones_hoy': self.numero_alimentaciones_hoy,
            'ultima_porcion': self.ultima_porcion_dispensada.isoformat() if self.ultima_porcion_dispensada else None,
            'nivel_comida_actual': self.sensor_nivel.valor_actual,
            'peso_plato_actual': self.sensor_peso.valor_actual,
            'alimentacion_automatica': self.alimentacion_automatica,
            'horarios_configurados': len(self.horarios_alimentacion)
        }
    
    def obtener_info_completa(self):
        """Obtener información completa del servicio"""
        return {
            'activo': self.activo,
            'dispensador_estado': self.dispensador.estado,
            'sensor_nivel': self.sensor_nivel.obtener_info_completa(),
            'sensor_peso': self.sensor_peso.obtener_info_completa(),
            'estadisticas_diarias': self.obtener_estadisticas_diarias(),
            'horarios_alimentacion': self.horarios_alimentacion
        }
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        return f"Comedero Service: {estado} - Dispensado hoy: {self.total_dispensado_hoy}g - Alimentaciones: {self.numero_alimentaciones_hoy}"
