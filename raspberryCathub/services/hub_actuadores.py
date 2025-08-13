import threading
import time
from datetime import datetime, timedelta

class HubActuadores:
    def __init__(self, actuadores):
        self.actuadores = actuadores
        self.activo = False
        self.cola_comandos = []
        self.hilo_procesamiento = None
        self.lock_cola = threading.Lock()
        self.historial_comandos = []
        
    def iniciar(self):
        """Iniciar hub de actuadores"""
        if self.activo:
            print("⚠️ Hub de actuadores ya está activo")
            return
            
        self.activo = True
        self.hilo_procesamiento = threading.Thread(target=self._procesar_comandos, daemon=True)
        self.hilo_procesamiento.start()
        print("🚀 Hub de actuadores iniciado")
    
    def detener(self):
        """Detener hub de actuadores"""
        self.activo = False
        if self.hilo_procesamiento:
            self.hilo_procesamiento.join(timeout=5)
        print("🛑 Hub de actuadores detenido")
    
    def _procesar_comandos(self):
        """Procesar cola de comandos de forma segura"""
        while self.activo:
            try:
                with self.lock_cola:
                    if self.cola_comandos:
                        comando = self.cola_comandos.pop(0)
                    else:
                        comando = None
                
                if comando:
                    self._ejecutar_comando(comando)
                else:
                    time.sleep(0.5)  # Esperar si no hay comandos
                    
            except Exception as e:
                print(f"❌ Error procesando comandos: {e}")
                time.sleep(1)
    
    def _ejecutar_comando(self, comando):
        """Ejecutar comando de actuador"""
        try:
            actuador_id = comando.get('actuador_id')
            accion = comando.get('accion')
            parametros = comando.get('parametros', {})
            
            actuador = self.obtener_actuador_por_id(actuador_id)
            
            if not actuador:
                print(f"❌ Actuador {actuador_id} no encontrado")
                return False
            
            print(f"🔧 Ejecutando: {accion} en {actuador_id}")
            
            # Ejecutar acción según tipo
            resultado = self._dispatch_accion(actuador, accion, parametros)
            
            # Registrar en historial
            self.historial_comandos.append({
                'timestamp': datetime.now(),
                'actuador_id': actuador_id,
                'accion': accion,
                'parametros': parametros,
                'resultado': resultado
            })
            
            # Mantener historial de últimos 100 comandos
            if len(self.historial_comandos) > 100:
                self.historial_comandos = self.historial_comandos[-100:]
            
            return resultado
            
        except Exception as e:
            print(f"❌ Error ejecutando comando: {e}")
            return False
    
    def _dispatch_accion(self, actuador, accion, parametros):
        """Despachar acción específica según tipo de actuador"""
        try:
            # Dispensador de comida
            if hasattr(actuador, 'dispensar') and 'dispensar' in accion:
                cantidad = parametros.get('cantidad', 0)
                if accion == 'dispensar_pequena':
                    return actuador.dispensar_porcion_pequena()
                elif accion == 'dispensar_normal':
                    return actuador.dispensar_porcion_normal()
                elif accion == 'dispensar_grande':
                    return actuador.dispensar_porcion_grande()
                elif accion == 'dispensar_custom':
                    return actuador.dispensar(cantidad)
            
            # Limpiador de arenero
            elif hasattr(actuador, 'limpiar') and 'limpiar' in accion:
                if accion == 'limpiar_rapida':
                    return actuador.limpieza_rapida()
                elif accion == 'limpiar_profunda':
                    return actuador.limpieza_profunda()
                elif accion == 'limpiar_custom':
                    duracion = parametros.get('duracion', 30)
                    return actuador.limpiar(duracion)
            
            # Dispensador de agua
            elif hasattr(actuador, 'llenar_bebedero') and 'agua' in accion:
                if accion == 'llenar_bebedero':
                    nivel = parametros.get('nivel', 80)
                    return actuador.llenar_bebedero(nivel)
                elif accion == 'dispensar_agua_pequena':
                    return actuador.dispensar_porcion_pequena()
                elif accion == 'dispensar_agua_normal':
                    return actuador.dispensar_porcion_normal()
                elif accion == 'dispensar_agua_grande':
                    return actuador.dispensar_porcion_grande()
                elif accion == 'circular_agua':
                    duracion = parametros.get('duracion', 10)
                    return actuador.activar_circulacion(duracion)
            
            # Acciones generales
            elif accion == 'test':
                return actuador.test_motor() if hasattr(actuador, 'test_motor') else actuador.test_bomba()
            elif accion == 'stop':
                return actuador.detener_emergencia()
            
            print(f"⚠️ Acción no reconocida: {accion}")
            return False
            
        except Exception as e:
            print(f"❌ Error en dispatch de acción: {e}")
            return False
    
    def agregar_comando(self, actuador_id, accion, parametros=None, prioridad=False):
        """Agregar comando a la cola"""
        comando = {
            'actuador_id': actuador_id,
            'accion': accion,
            'parametros': parametros or {},
            'timestamp_creado': datetime.now()
        }
        
        with self.lock_cola:
            if prioridad:
                self.cola_comandos.insert(0, comando)  # Agregar al inicio
            else:
                self.cola_comandos.append(comando)     # Agregar al final
        
        print(f"📋 Comando agregado: {accion} para {actuador_id}")
        return True
    
    def obtener_actuador_por_id(self, actuador_id):
        """Obtener actuador por ID"""
        for actuador in self.actuadores:
            if actuador.id == actuador_id:
                return actuador
        return None
    
    def detener_todos_emergencia(self):
        """Detener todos los actuadores inmediatamente"""
        print("🚨 PARADA DE EMERGENCIA - Deteniendo todos los actuadores")
        
        for actuador in self.actuadores:
            try:
                actuador.detener_emergencia()
            except Exception as e:
                print(f"❌ Error deteniendo {actuador.id}: {e}")
        
        # Limpiar cola de comandos
        with self.lock_cola:
            self.cola_comandos.clear()
        
        return True
    
    def obtener_estado_todos(self):
        """Obtener estado de todos los actuadores"""
        estados = {}
        for actuador in self.actuadores:
            try:
                estados[actuador.id] = {
                    'estado': actuador.estado,
                    'activo': getattr(actuador, 'motor_activo', False) or getattr(actuador, 'bomba_activa', False),
                    'tipo': actuador.__class__.__name__
                }
            except Exception as e:
                estados[actuador.id] = {
                    'error': str(e)
                }
        return estados
    
    def programar_comando(self, actuador_id, accion, parametros=None, delay_segundos=0):
        """Programar comando para ejecutar después de un delay"""
        def ejecutar_despues():
            time.sleep(delay_segundos)
            if self.activo:
                self.agregar_comando(actuador_id, accion, parametros, prioridad=False)
        
        hilo = threading.Thread(target=ejecutar_despues, daemon=True)
        hilo.start()
        
        print(f"⏰ Comando programado: {accion} para {actuador_id} en {delay_segundos}s")
        return True
    
    def obtener_historial_comandos(self, ultimos_n=20):
        """Obtener historial de comandos ejecutados"""
        return self.historial_comandos[-ultimos_n:] if self.historial_comandos else []
    
    def obtener_cola_pendiente(self):
        """Obtener comandos pendientes en cola"""
        with self.lock_cola:
            return list(self.cola_comandos)
    
    def limpiar_cola(self):
        """Limpiar cola de comandos"""
        with self.lock_cola:
            comandos_removidos = len(self.cola_comandos)
            self.cola_comandos.clear()
        
        print(f"🧹 Cola limpiada: {comandos_removidos} comandos removidos")
        return comandos_removidos
    
    def obtener_estadisticas_hub(self):
        """Obtener estadísticas del hub"""
        return {
            'activo': self.activo,
            'total_actuadores': len(self.actuadores),
            'comandos_en_cola': len(self.cola_comandos),
            'total_comandos_ejecutados': len(self.historial_comandos),
            'actuadores_activos': sum(1 for actuador in self.actuadores 
                                    if getattr(actuador, 'motor_activo', False) or 
                                       getattr(actuador, 'bomba_activa', False))
        }
    
    def __str__(self):
        estado = "Activo" if self.activo else "Inactivo"
        return f"Hub Actuadores: {estado} - {len(self.actuadores)} actuadores - {len(self.cola_comandos)} comandos en cola"
