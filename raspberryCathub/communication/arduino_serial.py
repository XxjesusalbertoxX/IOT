"""
Arduino Serial Communication - Maneja toda la comunicación con Arduino
"""

import serial
import json
import time
import logging
import threading
from typing import Dict, Any, Optional, Union
from queue import Queue, Empty

class ArduinoSerial:
    """
    Manejador de comunicación serial con Arduino
    
    Responsabilidades:
    1. Conectar/desconectar del puerto serial
    2. Enviar comandos JSON al Arduino
    3. Recibir y parsear respuestas JSON
    4. Manejar timeouts y reconexión
    5. Thread-safe para múltiples sensores
    """
    
    def __init__(self, port: str = '/dev/ttyACM0', baudrate: int = 9600):
        self.logger = logging.getLogger(__name__)
        
        # ✅ CONFIGURACIÓN SERIAL
        self.port = port
        self.baudrate = baudrate
        self.timeout = 5  # 5 segundos timeout
        
        # ✅ ESTADO DE CONEXIÓN
        self.serial_connection = None
        self.connected = False
        self.last_connection_attempt = 0
        self.reconnect_interval = 10  # Intentar reconectar cada 10 segundos
        
        # ✅ THREAD SAFETY
        self.serial_lock = threading.Lock()
        self.command_queue = Queue()
        
        # ✅ ESTADÍSTICAS
        self.stats = {
            "commands_sent": 0,
            "responses_received": 0,
            "timeouts": 0,
            "errors": 0,
            "last_communication": None
        }

    def connect(self) -> bool:
        """
        Conectar al Arduino
        
        Returns:
            True si conexión exitosa
        """
        try:
            if self.connected:
                return True
            
            self.logger.info(f"🔗 Conectando a Arduino en {self.port}...")
            
            # Crear conexión serial
            self.serial_connection = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                write_timeout=3
            )
            
            # Esperar que Arduino se inicialice
            time.sleep(2)
            
            # Limpiar buffer
            self.serial_connection.flushInput()
            self.serial_connection.flushOutput()
            
            # Verificar comunicación con ping
            if self._test_connection():
                self.connected = True
                self.last_connection_attempt = time.time()
                self.logger.info("✅ Arduino conectado exitosamente")
                return True
            else:
                self.logger.error("❌ Arduino no responde al ping")
                self._disconnect()
                return False
                
        except serial.SerialException as e:
            self.logger.error(f"❌ Error de puerto serial: {e}")
            self.connected = False
            return False
            
        except Exception as e:
            self.logger.error(f"❌ Error conectando Arduino: {e}")
            self.connected = False
            return False

    def _test_connection(self) -> bool:
        """Prueba la conexión con un ping simple"""
        try:
            ping_command = {"type": "PING"}
            
            # ✅ ENVIAR COMANDO
            if not self._send_command_raw(ping_command):
                self.logger.error("❌ No se pudo enviar PING")
                return False
            
            # ✅ LEER CUALQUIER RESPUESTA (no solo PONG)
            time.sleep(1)  # Dar tiempo al Arduino
            
            for attempt in range(5):  # Intentar leer varias líneas
                response = self._read_response()
                if response:
                    self.logger.info(f"📥 Arduino respondió: {response}")
                    # ✅ ACEPTAR CUALQUIER RESPUESTA VÁLIDA
                    return True
                time.sleep(0.2)
            
            self.logger.error("❌ Arduino no envió ninguna respuesta")
            return False
            
        except Exception as e:
            self.logger.error(f"❌ Error en test de conexión: {e}")
            return False
    def disconnect(self):
        """Desconectar del Arduino"""
        try:
            with self.serial_lock:
                self._disconnect()
        except Exception as e:
            self.logger.error(f"❌ Error desconectando: {e}")

    def _disconnect(self):
        """Desconexión interna (sin lock)"""
        if self.serial_connection:
            try:
                self.serial_connection.close()
            except:
                pass
            self.serial_connection = None
        
        self.connected = False
        self.logger.info("👋 Arduino desconectado")

    def request_sensor_data(self, command: Dict[str, Any], timeout: int = 5) -> Optional[Dict[str, Any]]:
        """
        Solicita datos de un sensor específico
        
        Args:
            command: Comando JSON para el sensor
            timeout: Timeout en segundos
            
        Returns:
            Respuesta del Arduino o None
        """
        try:
            if not self.is_connected():
                self.logger.warning("⚠️ Arduino no conectado - intentando reconectar...")
                if not self._attempt_reconnect():
                    return None
            
            # Enviar comando y esperar respuesta
            response = self._send_and_wait(command, timeout)
            
            if response:
                self.stats["responses_received"] += 1
                self.stats["last_communication"] = time.time()
                
                self.logger.debug(f"✅ Respuesta recibida: {response.get('sensor', 'unknown')}")
                return response
            else:
                self.stats["timeouts"] += 1
                self.logger.warning(f"⏰ Timeout esperando respuesta de {command.get('sensor', 'unknown')}")
                return None
                
        except Exception as e:
            self.stats["errors"] += 1
            self.logger.error(f"❌ Error solicitando datos: {e}")
            return None

    def send_actuator_command(self, command: Dict[str, Any]) -> bool:
        """
        Envía comando a actuador (motor, bomba, etc.)
        
        Args:
            command: Comando JSON para actuador
            
        Returns:
            True si comando enviado exitosamente
        """
        try:
            if not self.is_connected():
                if not self._attempt_reconnect():
                    return False
            
            # Los comandos de actuadores pueden no necesitar respuesta
            success = self._send_command(command)
            
            if success:
                self.logger.info(f"✅ Comando actuador enviado: {command.get('actuator', 'unknown')}")
                return True
            else:
                self.logger.error(f"❌ Falló comando actuador: {command.get('actuator', 'unknown')}")
                return False
                
        except Exception as e:
            self.stats["errors"] += 1
            self.logger.error(f"❌ Error enviando comando actuador: {e}")
            return False

    def _send_and_wait(self, command: Dict[str, Any], timeout: int = 5) -> Optional[Dict[str, Any]]:
        """
        Envía comando y espera respuesta específica
        
        Args:
            command: Comando a enviar
            timeout: Timeout para respuesta
            
        Returns:
            Respuesta parseada o None
        """
        with self.serial_lock:
            try:
                # Enviar comando
                if not self._send_command_raw(command):
                    return None
                
                # Esperar respuesta
                start_time = time.time()
                while (time.time() - start_time) < timeout:
                    response = self._read_response()
                    if response:
                        return response
                    
                    time.sleep(0.1)  # Pequeña pausa para no saturar CPU
                
                return None  # Timeout
                
            except Exception as e:
                self.logger.error(f"❌ Error en send_and_wait: {e}")
                return None

    def _send_command(self, command: Dict[str, Any]) -> bool:
        """Envía comando sin esperar respuesta"""
        with self.serial_lock:
            return self._send_command_raw(command)

    def _send_command_raw(self, command: Dict[str, Any]) -> bool:
        """
        Envía comando raw (sin lock, para uso interno)
        
        Args:
            command: Diccionario comando
            
        Returns:
            True si envío exitoso
        """
        try:
            if not self.serial_connection or not self.connected:
                return False
            
            # Convertir a JSON y agregar terminador
            command_json = json.dumps(command)
            command_bytes = (command_json + '\n').encode('utf-8')
            
            # Enviar comando
            self.serial_connection.write(command_bytes)
            self.serial_connection.flush()
            
            self.stats["commands_sent"] += 1
            self.logger.debug(f"📤 Comando enviado: {command_json}")
            
            return True
            
        except serial.SerialTimeoutError:
            self.logger.error("❌ Timeout enviando comando")
            return False
            
        except serial.SerialException as e:
            self.logger.error(f"❌ Error serial enviando: {e}")
            self.connected = False
            return False
            
        except Exception as e:
            self.logger.error(f"❌ Error enviando comando: {e}")
            return False

    def _read_response(self) -> Optional[Dict[str, Any]]:
        """
        Lee respuesta del Arduino
        
        Returns:
            Diccionario parseado o None
        """
        try:
            if not self.serial_connection or not self.connected:
                return None
            
            # Leer línea completa
            line = self.serial_connection.readline().decode('utf-8').strip()
            
            if not line:
                return None
            
            # Parsear JSON
            response = json.loads(line)
            
            self.logger.debug(f"📥 Respuesta recibida: {line}")
            return response
            
        except json.JSONDecodeError as e:
            self.logger.error(f"❌ Error JSON en respuesta: {e}")
            return None
            
        except serial.SerialException as e:
            self.logger.error(f"❌ Error serial leyendo: {e}")
            self.connected = False
            return None
            
        except Exception as e:
            self.logger.error(f"❌ Error leyendo respuesta: {e}")
            return None

    def _attempt_reconnect(self) -> bool:
        """
        Intenta reconectar al Arduino
        
        Returns:
            True si reconexión exitosa
        """
        current_time = time.time()
        
        # No intentar reconnectar muy seguido
        if (current_time - self.last_connection_attempt) < self.reconnect_interval:
            return False
        
        self.logger.info("🔄 Intentando reconectar a Arduino...")
        
        # Cerrar conexión anterior si existe
        self._disconnect()
        
        # Intentar nueva conexión
        self.last_connection_attempt = current_time
        return self.connect()

    def is_connected(self) -> bool:
        """
        Verifica si está conectado
        
        Returns:
            True si conexión activa
        """
        return self.connected and self.serial_connection is not None

    def get_status(self) -> Dict[str, Any]:
        """
        Obtiene estado de la comunicación
        
        Returns:
            Diccionario con estado y estadísticas
        """
        return {
            "connected": self.connected,
            "port": self.port,
            "baudrate": self.baudrate,
            "stats": self.stats.copy(),
            "last_connection_attempt": self.last_connection_attempt
        }

    def flush_buffers(self):
        """Limpia buffers de entrada y salida"""
        try:
            with self.serial_lock:
                if self.serial_connection and self.connected:
                    self.serial_connection.flushInput()
                    self.serial_connection.flushOutput()
                    self.logger.debug("🧹 Buffers limpiados")
        except Exception as e:
            self.logger.error(f"❌ Error limpiando buffers: {e}")

    def send_emergency_stop(self) -> bool:
        """
        Envía comando de parada de emergencia
        
        Returns:
            True si comando enviado
        """
        emergency_command = {
            "type": "EMERGENCY_STOP",
            "timestamp": time.time()
        }
        
        try:
            # Comando de emergencia - intentar múltiples veces
            for attempt in range(3):
                if self._send_command(emergency_command):
                    self.logger.info("🚨 Comando EMERGENCY_STOP enviado")
                    return True
                
                time.sleep(0.5)
            
            self.logger.error("❌ Falló envío de EMERGENCY_STOP")
            return False
            
        except Exception as e:
            self.logger.error(f"❌ Error en emergency stop: {e}")
            return False

    def __del__(self):
        """Destructor - cerrar conexión automáticamente"""
        try:
            self.disconnect()
        except:
            pass