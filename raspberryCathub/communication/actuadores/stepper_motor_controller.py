"""
Controlador Inteligente del Arenero
Maneja toda la lógica compleja del motor basada en sensores
"""

import time
import logging
from datetime import datetime, timedelta
from enum import IntEnum
from typing import Dict, Any, Optional, Tuple
import threading

from communication.arduino_serial import ArduinoSerial
from sensors.litterbox.presence_sensor import PresenceSensor
from sensors.litterbox.humidity_sensor import HumiditySensor  
from sensors.litterbox.gas_sensor import GasSensor

class LitterboxState(IntEnum):
    EMPTY = 0      # Sin arena, sin torque
    READY = 1      # Con arena, con torque, listo
    BLOCKED = -1   # Bloqueado por condiciones inseguras

class CleaningInterval(IntEnum):
    EVERY_2_HOURS = 2
    EVERY_5_HOURS = 5
    EVERY_8_HOURS = 8

class LitterboxController:
    """
    Controlador principal del arenero con toda la lógica de estados,
    validaciones de seguridad y programación automática
    """
    
    def __init__(self, arduino_serial: ArduinoSerial):
        self.logger = logging.getLogger(__name__)
        self.arduino = arduino_serial
        
        # ✅ SENSORES REQUERIDOS
        self.presence_sensor = PresenceSensor(self.arduino)
        self.humidity_sensor = HumiditySensor(self.arduino)
        self.gas_sensor = GasSensor(self.arduino)
        
        # ✅ ESTADO ACTUAL
        self.current_state = LitterboxState.EMPTY
        self.motor_blocked = False
        self.last_sensor_check = 0
        self.sensor_check_interval = 10  # Verificar sensores cada 10s
        
        # ✅ CONFIGURACIÓN DE LIMPIEZA AUTOMÁTICA
        self.auto_cleaning_enabled = True
        self.cleaning_interval = CleaningInterval.EVERY_5_HOURS
        self.last_auto_cleaning = None
        
        # ✅ LÍMITES DE SEGURIDAD
        self.max_humidity = 75.0      # % máximo de humedad
        self.max_gas_level = 300      # ppm máximo de gas
        self.safety_check_retries = 3
        
        # ✅ THREAD DE MONITOREO
        self.monitoring_active = False
        self.monitoring_thread = None
        self.lock = threading.Lock()
        
        # ✅ ESTADÍSTICAS
        self.stats = {
            "normal_cleanings": 0,
            "complete_cleanings": 0, 
            "safety_blocks": 0,
            "fill_operations": 0,
            "last_fill": None,
            "last_cleaning": None
        }

    def start_monitoring(self):
        """Iniciar monitoreo continuo del arenero"""
        if self.monitoring_active:
            self.logger.warning("⚠️ Monitoreo ya está activo")
            return False
            
        self.monitoring_active = True
        self.monitoring_thread = threading.Thread(target=self._monitoring_loop, daemon=True)
        self.monitoring_thread.start()
        
        self.logger.info("🔍 Monitoreo del arenero iniciado")
        return True

    def stop_monitoring(self):
        """Detener monitoreo"""
        self.monitoring_active = False
        if self.monitoring_thread:
            self.monitoring_thread.join(timeout=5)
        
        self.logger.info("🛑 Monitoreo del arenero detenido")

    def _monitoring_loop(self):
        """Bucle principal de monitoreo"""
        while self.monitoring_active:
            try:
                # Verificar condiciones de seguridad
                self._check_safety_conditions()
                
                # Verificar limpieza automática
                if self.auto_cleaning_enabled and self.current_state == LitterboxState.READY:
                    self._check_auto_cleaning()
                
                # Sincronizar estado con Arduino
                self._sync_state_with_arduino()
                
                time.sleep(self.sensor_check_interval)
                
            except Exception as e:
                self.logger.error(f"❌ Error en monitoreo: {e}")
                time.sleep(30)  # Pausa más larga en caso de error

    def fill_with_litter(self) -> Tuple[bool, str]:
        """
        Llenar arenero con arena (Estado 0 -> 1)
        
        Returns:
            (success, message)
        """
        with self.lock:
            try:
                # ✅ VALIDAR ESTADO ACTUAL
                if self.current_state != LitterboxState.EMPTY:
                    if self.current_state == LitterboxState.READY:
                        return False, "Arenero ya tiene arena"
                    elif self.current_state == LitterboxState.BLOCKED:
                        return False, "Arenero bloqueado por condiciones inseguras"
                
                # ✅ VERIFICAR CONDICIONES DE SEGURIDAD
                is_safe, safety_msg = self._verify_safety_conditions()
                if not is_safe:
                    return False, f"Condiciones inseguras: {safety_msg}"
                
                self.logger.info("🪣 Iniciando llenado de arena...")
                
                # ✅ ENVIAR COMANDO AL ARDUINO
                command = {
                    "type": "LITTERBOX_COMMAND",
                    "action": "fill_litter"
                }
                
                response = self.arduino.request_sensor_data(command, timeout=15)
                
                if response and response.get("status") == "success":
                    self.current_state = LitterboxState.READY
                    self.stats["fill_operations"] += 1
                    self.stats["last_fill"] = datetime.now()
                    
                    self.logger.info("✅ Arenero llenado exitosamente - Estado: READY")
                    return True, "Arenero llenado y listo para usar"
                else:
                    error_msg = response.get("error", "Error desconocido") if response else "Sin respuesta"
                    return False, f"Error del Arduino: {error_msg}"
                    
            except Exception as e:
                self.logger.error(f"❌ Error llenando arenero: {e}")
                return False, f"Error interno: {str(e)}"

    def execute_normal_cleaning(self, force: bool = False) -> Tuple[bool, str]:
        """
        Ejecutar limpieza normal (270° derecha + regreso)
        
        Args:
            force: Saltar verificaciones de seguridad
            
        Returns:
            (success, message)
        """
        with self.lock:
            try:
                # ✅ VALIDAR ESTADO
                if not force and self.current_state != LitterboxState.READY:
                    return False, f"Estado incorrecto: {self.current_state.name}"
                
                # ✅ VERIFICAR SEGURIDAD
                if not force:
                    is_safe, safety_msg = self._verify_safety_conditions()
                    if not is_safe:
                        return False, f"Condiciones inseguras: {safety_msg}"
                
                self.logger.info("🧹 Iniciando limpieza normal...")
                
                # ✅ COMANDO AL ARDUINO
                command = {
                    "type": "LITTERBOX_COMMAND", 
                    "action": "normal_cleaning"
                }
                
                response = self.arduino.request_sensor_data(command, timeout=30)
                
                if response and response.get("status") == "success":
                    self.stats["normal_cleanings"] += 1
                    self.stats["last_cleaning"] = datetime.now()
                    self.last_auto_cleaning = datetime.now()
                    
                    self.logger.info("✅ Limpieza normal completada")
                    return True, "Limpieza normal ejecutada exitosamente"
                else:
                    error_msg = response.get("error", "Error desconocido") if response else "Sin respuesta"
                    return False, f"Error del Arduino: {error_msg}"
                    
            except Exception as e:
                self.logger.error(f"❌ Error en limpieza normal: {e}")
                return False, f"Error interno: {str(e)}"

    def execute_complete_cleaning(self, force: bool = False) -> Tuple[bool, str]:
        """
        Ejecutar limpieza completa (80° izquierda + regreso + quitar torque)
        Estado READY -> EMPTY
        
        Args:
            force: Saltar verificaciones de seguridad
            
        Returns:
            (success, message)
        """
        with self.lock:
            try:
                # ✅ VALIDAR ESTADO
                if not force and self.current_state != LitterboxState.READY:
                    return False, f"Estado incorrecto: {self.current_state.name}"
                
                # ✅ VERIFICAR SEGURIDAD
                if not force:
                    is_safe, safety_msg = self._verify_safety_conditions()
                    if not is_safe:
                        return False, f"Condiciones inseguras: {safety_msg}"
                
                self.logger.info("🧽 Iniciando limpieza completa...")
                
                # ✅ COMANDO AL ARDUINO
                command = {
                    "type": "LITTERBOX_COMMAND",
                    "action": "complete_cleaning"
                }
                
                response = self.arduino.request_sensor_data(command, timeout=30)
                
                if response and response.get("status") == "success":
                    self.current_state = LitterboxState.EMPTY  # Cambio de estado
                    self.stats["complete_cleanings"] += 1
                    self.stats["last_cleaning"] = datetime.now()
                    
                    self.logger.info("✅ Limpieza completa finalizada - Estado: EMPTY")
                    return True, "Limpieza completa ejecutada - Listo para nueva arena"
                else:
                    error_msg = response.get("error", "Error desconocido") if response else "Sin respuesta"
                    return False, f"Error del Arduino: {error_msg}"
                    
            except Exception as e:
                self.logger.error(f"❌ Error en limpieza completa: {e}")
                return False, f"Error interno: {str(e)}"

    def _verify_safety_conditions(self) -> Tuple[bool, str]:
        """
        Verificar todas las condiciones de seguridad antes de mover motor
        
        Returns:
            (is_safe, message)
        """
        try:
            # ✅ VERIFICAR PRESENCIA DEL GATO
            presence_data = self.presence_sensor.read()
            if presence_data and presence_data.get("detected", False):
                return False, "Gato detectado en el arenero"
            
            # ✅ VERIFICAR NIVEL DE HUMEDAD
            humidity_data = self.humidity_sensor.read()
            if humidity_data:
                humidity = humidity_data.get("humidity", 0)
                if humidity > self.max_humidity:
                    return False, f"Humedad muy alta: {humidity}% (máx: {self.max_humidity}%)"
            
            # ✅ VERIFICAR NIVEL DE GAS
            gas_data = self.gas_sensor.read()
            if gas_data:
                gas_level = gas_data.get("gas_ppm", 0)
                if gas_level > self.max_gas_level:
                    return False, f"Gas muy alto: {gas_level}ppm (máx: {self.max_gas_level}ppm)"
            
            return True, "Condiciones seguras"
            
        except Exception as e:
            self.logger.error(f"❌ Error verificando seguridad: {e}")
            return False, f"Error en sensores: {str(e)}"

    def _check_safety_conditions(self):
        """Verificar condiciones y bloquear/desbloquear motor si es necesario"""
        try:
            is_safe, reason = self._verify_safety_conditions()
            
            if not is_safe and not self.motor_blocked:
                # Bloquear motor
                self.logger.warning(f"🚫 Bloqueando motor: {reason}")
                self._block_motor()
                self.stats["safety_blocks"] += 1
                
            elif is_safe and self.motor_blocked:
                # Desbloquear motor
                self.logger.info("✅ Condiciones seguras - Desbloqueando motor")
                self._unblock_motor()
                
        except Exception as e:
            self.logger.error(f"❌ Error verificando condiciones: {e}")

    def _block_motor(self):
        """Bloquear motor por condiciones inseguras"""
        command = {
            "type": "LITTERBOX_COMMAND",
            "action": "block"
        }
        
        self.arduino.send_actuator_command(command)
        self.motor_blocked = True
        self.current_state = LitterboxState.BLOCKED

    def _unblock_motor(self):
        """Desbloquear motor cuando sea seguro"""
        command = {
            "type": "LITTERBOX_COMMAND", 
            "action": "unblock"
        }
        
        response = self.arduino.request_sensor_data(command, timeout=5)
        if response and response.get("status") == "success":
            self.motor_blocked = False
            # El Arduino nos dirá el estado correcto
            new_state = response.get("state", 0)
            self.current_state = LitterboxState(new_state)

    def _check_auto_cleaning(self):
        """Verificar si es momento de limpieza automática"""
        try:
            if not self.last_auto_cleaning:
                self.last_auto_cleaning = datetime.now()
                return
            
            time_since_cleaning = datetime.now() - self.last_auto_cleaning
            hours_passed = time_since_cleaning.total_seconds() / 3600
            
            if hours_passed >= self.cleaning_interval:
                self.logger.info(f"⏰ Ejecutando limpieza automática (cada {self.cleaning_interval}h)")
                success, msg = self.execute_normal_cleaning()
                
                if success:
                    self.logger.info(f"✅ Limpieza automática exitosa")
                else:
                    self.logger.warning(f"⚠️ Limpieza automática falló: {msg}")
                    
        except Exception as e:
            self.logger.error(f"❌ Error en limpieza automática: {e}")

    def _sync_state_with_arduino(self):
        """Sincronizar estado con Arduino"""
        try:
            command = {
                "type": "REQUEST_SENSOR_DATA",
                "sensor": "litterbox_status"
            }
            
            response = self.arduino.request_sensor_data(command, timeout=3)
            if response:
                arduino_state = response.get("state", 0)
                if arduino_state != self.current_state:
                    self.logger.info(f"🔄 Sincronizando estado: {self.current_state} -> {arduino_state}")
                    self.current_state = LitterboxState(arduino_state)
                    
        except Exception as e:
            self.logger.debug(f"Sync error (normal): {e}")

    def set_cleaning_interval(self, hours: int) -> bool:
        """
        Configurar intervalo de limpieza automática
        
        Args:
            hours: 2, 5, u 8 horas
        """
        if hours in [2, 5, 8]:
            self.cleaning_interval = CleaningInterval(hours)
            self.logger.info(f"⚙️ Intervalo de limpieza configurado: cada {hours} horas")
            return True
        else:
            self.logger.error(f"❌ Intervalo inválido: {hours} (válidos: 2, 5, 8)")
            return False

    def get_status(self) -> Dict[str, Any]:
        """Obtener estado completo del controlador"""
        return {
            "current_state": self.current_state.name,
            "motor_blocked": self.motor_blocked,
            "auto_cleaning_enabled": self.auto_cleaning_enabled,
            "cleaning_interval_hours": self.cleaning_interval.value,
            "last_auto_cleaning": self.last_auto_cleaning.isoformat() if self.last_auto_cleaning else None,
            "monitoring_active": self.monitoring_active,
            "stats": self.stats.copy()
        }

    def emergency_stop(self) -> bool:
        """Parada de emergencia"""
        try:
            command = {
                "type": "EMERGENCY_STOP"
            }
            
            success = self.arduino.send_actuator_command(command)
            if success:
                self.current_state = LitterboxState.BLOCKED
                self.motor_blocked = True
                self.logger.warning("🚨 PARADA DE EMERGENCIA ejecutada")
            
            return success
            
        except Exception as e:
            self.logger.error(f"❌ Error en parada de emergencia: {e}")
            return False