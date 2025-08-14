"""
Actuator Controller - Controla motores, bombas y actuadores
"""

import logging
from typing import Dict, Any, List

class ActuatorController:
    """
    Controlador de actuadores - Ejecuta acciones físicas
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.arduino_serial = None
        
        # ✅ ESTADO DE ACTUADORES
        self.actuator_states = {
            "litterbox_motor": False,    # Motor de limpieza arenero
            "water_pump": False,         # Bomba de agua
            "feeder_motor": False,       # Motor dispensador comida
            "blocked_operations": []     # Operaciones bloqueadas
        }

    def initialize(self, arduino_serial):
        """Inicializa controlador con referencia al Arduino"""
        self.arduino_serial = arduino_serial
        self.logger.info("✅ ActuatorController inicializado")

    def execute_actions(self, actions: List[Dict[str, Any]]) -> int:
        """
        Ejecuta lista de acciones
        
        Args:
            actions: Lista de acciones a ejecutar
            
        Returns:
            Número de acciones ejecutadas exitosamente
        """
        executed_count = 0
        
        for action in actions:
            try:
                if self._execute_single_action(action):
                    executed_count += 1
            except Exception as e:
                self.logger.error(f"❌ Error ejecutando acción {action.get('type')}: {e}")
        
        return executed_count

    def _execute_single_action(self, action: Dict[str, Any]) -> bool:
        """Ejecuta una acción individual"""
        action_type = action.get("type")
        priority = action.get("priority", "info")
        message = action.get("message", "")
        
        try:
            # 🐱 ACCIONES DE PRESENCIA
            if action_type == "cat_in_litterbox":
                self._handle_cat_in_litterbox(action)
                return True
                
            elif action_type == "cat_drinking":
                self._handle_cat_drinking(action)
                return True
            
            # 🍽️ ACCIONES DE COMIDA
            elif action_type == "alert_empty_feeder":
                self._handle_empty_feeder(action)
                return True
                
            elif action_type == "alert_low_food":
                self._handle_low_food(action)
                return True
            
            # 🌡️ ACCIONES DE AMBIENTE
            elif action_type == "high_temperature":
                self._handle_high_temperature(action)
                return True
                
            elif action_type == "dangerous_gas":
                self._handle_dangerous_gas(action)
                return True
            
            # 💧 ACCIONES DE AGUA
            elif action_type == "critical_water_level":
                self._handle_critical_water(action)
                return True
            
            else:
                self.logger.warning(f"⚠️ Acción desconocida: {action_type}")
                return False
                
        except Exception as e:
            self.logger.error(f"❌ Error en acción {action_type}: {e}")
            return False

    def _handle_cat_in_litterbox(self, action: Dict[str, Any]):
        """Maneja gato en el arenero"""
        distance = action.get("data", {}).get("distance_cm", 0)
        
        # Bloquear operaciones de limpieza
        if "litterbox_cleaning" not in self.actuator_states["blocked_operations"]:
            self.actuator_states["blocked_operations"].append("litterbox_cleaning")
            self.logger.info(f"🚫 Limpieza del arenero BLOQUEADA (gato a {distance}cm)")
        
        # Si el motor estaba funcionando, detenerlo
        if self.actuator_states["litterbox_motor"]:
            self._send_arduino_command({
                "type": "STOP_ACTUATOR",
                "actuator": "litterbox_motor"
            })
            self.actuator_states["litterbox_motor"] = False
            self.logger.info("⏹️ Motor del arenero DETENIDO por presencia de gato")

    def _handle_cat_drinking(self, action: Dict[str, Any]):
        """Maneja gato bebiendo agua"""
        # Podrías activar bomba para mantener nivel
        self.logger.info("💧 Gato bebiendo - monitorear nivel de agua")

    def _handle_empty_feeder(self, action: Dict[str, Any]):
        """Maneja comedero vacío"""
        self.logger.warning("🚨 COMEDERO VACÍO - Dispensar comida automáticamente")
        
        # Activar motor dispensador
        self._send_arduino_command({
            "type": "ACTIVATE_ACTUATOR",
            "actuator": "feeder_motor",
            "duration_seconds": 3
        })
        
        self.actuator_states["feeder_motor"] = True

    def _handle_low_food(self, action: Dict[str, Any]):
        """Maneja comida baja"""
        self.logger.info("⚠️ Comida baja - notificar usuario")
        # Aquí podrías enviar notificación push, email, etc.

    def _handle_high_temperature(self, action: Dict[str, Any]):
        """Maneja temperatura alta"""
        self.logger.warning("🌡️ Temperatura alta - verificar ventilación")

    def _handle_dangerous_gas(self, action: Dict[str, Any]):
        """Maneja gas peligroso"""
        self.logger.error("☢️ GAS PELIGROSO - BLOQUEAR arenero")
        
        # Bloquear TODAS las operaciones del arenero
        if "litterbox_all" not in self.actuator_states["blocked_operations"]:
            self.actuator_states["blocked_operations"].append("litterbox_all")
        
        # Detener motor si está funcionando
        if self.actuator_states["litterbox_motor"]:
            self._send_arduino_command({
                "type": "EMERGENCY_STOP",
                "actuator": "litterbox_motor"
            })
            self.actuator_states["litterbox_motor"] = False

    def _handle_critical_water(self, action: Dict[str, Any]):
        """Maneja agua crítica"""
        self.logger.error("💧 AGUA CRÍTICA - Activar bomba")
        
        # Activar bomba de agua
        self._send_arduino_command({
            "type": "ACTIVATE_ACTUATOR", 
            "actuator": "water_pump",
            "duration_seconds": 10
        })
        
        self.actuator_states["water_pump"] = True

    def _send_arduino_command(self, command: Dict[str, Any]):
        """Envía comando al Arduino"""
        try:
            if self.arduino_serial:
                success = self.arduino_serial.send_actuator_command(command)
                if success:
                    self.logger.debug(f"✅ Comando enviado: {command['type']}")
                else:
                    self.logger.error(f"❌ Falló comando: {command['type']}")
            else:
                self.logger.error("❌ Arduino no disponible para comando")
        except Exception as e:
            self.logger.error(f"❌ Error enviando comando: {e}")

    def get_status(self) -> Dict[str, Any]:
        """Estado de actuadores"""
        return {
            "actuator_states": self.actuator_states.copy(),
            "arduino_available": self.arduino_serial is not None
        }

    def shutdown(self):
        """Apagado seguro - detener todos los actuadores"""
        try:
            self.logger.info("🛑 Deteniendo todos los actuadores...")
            
            # Detener todos los actuadores activos
            for actuator, is_active in self.actuator_states.items():
                if isinstance(is_active, bool) and is_active:
                    self._send_arduino_command({
                        "type": "STOP_ACTUATOR",
                        "actuator": actuator
                    })
            
            # Limpiar estados
            for key in self.actuator_states:
                if isinstance(self.actuator_states[key], bool):
                    self.actuator_states[key] = False
            
            self.actuator_states["blocked_operations"] = []
            
            self.logger.info("✅ Actuadores apagados")
            
        except Exception as e:
            self.logger.error(f"❌ Error en shutdown de actuadores: {e}")