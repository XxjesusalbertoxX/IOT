"""
Serial Data Manager - Raspberry Pi
Maneja la comunicación serial con el Arduino y procesa los datos de sensores
"""

import serial
import json
import time
import threading
import logging
from typing import Dict, Any, Optional, Callable
from datetime import datetime
import queue

class SerialDataManager:
    """
    Maneja la comunicación serial bidireccional con el Arduino
    Procesa datos de sensores y envía comandos
    """
    
    def __init__(self, port: str = '/dev/ttyACM0', baudrate: int = 115200, timeout: float = 1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_connection = None
        self.is_connected = False
        self.is_running = False
        
        # Threading
        self.read_thread = None
        self.data_queue = queue.Queue()
        self.command_queue = queue.Queue()
        
        # Callbacks para procesamiento de datos
        self.data_callbacks = {}
        
        # Estadísticas
        self.bytes_received = 0
        self.bytes_sent = 0
        self.messages_received = 0
        self.messages_sent = 0
        self.connection_attempts = 0
        self.last_heartbeat = None
        
        # Configuración de reconexión
        self.reconnect_delay = 5.0
        self.max_reconnect_attempts = 10
        
        logging.info(f"[SerialDataManager] Inicializado para puerto: {port}")

    def connect(self) -> bool:
        """
        Establece conexión serial con el Arduino
        
        Returns:
            True si la conexión es exitosa, False caso contrario
        """
        try:
            self.connection_attempts += 1
            logging.info(f"[SerialDataManager] Intentando conectar a {self.port} (intento {self.connection_attempts})")
            
            self.serial_connection = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                write_timeout=self.timeout
            )
            
            # Esperar a que Arduino se inicialice
            time.sleep(2)
            
            # Verificar que recibimos datos
            if self.serial_connection.is_open:
                self.is_connected = True
                logging.info(f"[SerialDataManager] Conectado exitosamente a {self.port}")
                return True
            else:
                logging.error(f"[SerialDataManager] No se pudo abrir el puerto {self.port}")
                return False
                
        except serial.SerialException as e:
            logging.error(f"[SerialDataManager] Error de conexión serial: {e}")
            return False
        except Exception as e:
            logging.error(f"[SerialDataManager] Error inesperado conectando: {e}")
            return False

    def disconnect(self):
        """
        Cierra la conexión serial
        """
        self.is_connected = False
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
            logging.info("[SerialDataManager] Conexión serial cerrada")

    def start_reading(self):
        """
        Inicia el hilo de lectura de datos seriales
        """
        if not self.is_connected:
            logging.error("[SerialDataManager] No se puede iniciar lectura sin conexión")
            return False
        
        self.is_running = True
        self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
        self.read_thread.start()
        logging.info("[SerialDataManager] Iniciado hilo de lectura serial")
        return True

    def stop_reading(self):
        """
        Detiene el hilo de lectura de datos seriales
        """
        self.is_running = False
        if self.read_thread and self.read_thread.is_alive():
            self.read_thread.join(timeout=5.0)
        logging.info("[SerialDataManager] Detenido hilo de lectura serial")

    def register_data_callback(self, sensor_type: str, callback: Callable[[Dict[str, Any]], None]):
        """
        Registra un callback para procesar datos de un tipo específico de sensor
        
        Args:
            sensor_type: Tipo de sensor (ej: 'weight', 'gas', 'temperature')
            callback: Función a llamar cuando se reciban datos de este sensor
        """
        self.data_callbacks[sensor_type] = callback
        logging.info(f"[SerialDataManager] Callback registrado para sensor: {sensor_type}")

    def send_command(self, command: Dict[str, Any]) -> bool:
        """
        Envía un comando al Arduino
        
        Args:
            command: Comando en formato JSON
            
        Returns:
            True si se envió exitosamente, False caso contrario
        """
        try:
            if not self.is_connected or not self.serial_connection:
                logging.error("[SerialDataManager] No hay conexión para enviar comando")
                return False
            
            json_command = json.dumps(command) + '\n'
            self.serial_connection.write(json_command.encode('utf-8'))
            self.bytes_sent += len(json_command)
            self.messages_sent += 1
            
            logging.debug(f"[SerialDataManager] Comando enviado: {json_command.strip()}")
            return True
            
        except Exception as e:
            logging.error(f"[SerialDataManager] Error enviando comando: {e}")
            return False

    def _read_loop(self):
        """
        Bucle principal de lectura de datos seriales
        Ejecuta en hilo separado
        """
        logging.info("[SerialDataManager] Iniciado bucle de lectura serial")
        
        while self.is_running:
            try:
                if not self.is_connected or not self.serial_connection:
                    time.sleep(1.0)
                    continue
                
                # Leer línea del serial
                if self.serial_connection.in_waiting > 0:
                    line = self.serial_connection.readline().decode('utf-8').strip()
                    
                    if line:
                        self.bytes_received += len(line)
                        self.messages_received += 1
                        self._process_serial_line(line)
                
                time.sleep(0.01)  # Pequeña pausa para evitar uso excesivo de CPU
                
            except serial.SerialException as e:
                logging.error(f"[SerialDataManager] Error de lectura serial: {e}")
                self._handle_connection_error()
            except Exception as e:
                logging.error(f"[SerialDataManager] Error inesperado en lectura: {e}")
                time.sleep(1.0)

    def _process_serial_line(self, line: str):
        """
        Procesa una línea recibida del serial
        
        Args:
            line: Línea de texto recibida del Arduino
        """
        try:
            # Intentar parsear como JSON
            data = json.loads(line)
            
            # Procesar diferentes tipos de mensajes
            if "event" in data:
                self._handle_event_message(data)
            elif "sensor_type" in data:
                self._handle_sensor_data(data)
            else:
                logging.debug(f"[SerialDataManager] Mensaje desconocido: {line}")
            
        except json.JSONDecodeError:
            logging.warning(f"[SerialDataManager] Línea no es JSON válido: {line}")
        except Exception as e:
            logging.error(f"[SerialDataManager] Error procesando línea: {e}")

    def _handle_event_message(self, data: Dict[str, Any]):
        """
        Maneja mensajes de eventos del Arduino
        
        Args:
            data: Datos del evento
        """
        event_type = data.get("event", "unknown")
        
        if event_type == "BOOT":
            version = data.get("version", "unknown")
            logging.info(f"[SerialDataManager] Arduino reiniciado, versión: {version}")
        elif event_type == "HEARTBEAT":
            self.last_heartbeat = datetime.utcnow()
            logging.debug("[SerialDataManager] Heartbeat recibido")
        else:
            logging.debug(f"[SerialDataManager] Evento: {event_type}")

    def _handle_sensor_data(self, data: Dict[str, Any]):
        """
        Maneja datos de sensores del Arduino
        
        Args:
            data: Datos del sensor
        """
        sensor_type = data.get("sensor_type", "unknown")
        
        # Agregar timestamp de recepción
        data["received_timestamp"] = datetime.utcnow().isoformat()
        
        # Llamar callback específico del sensor si existe
        if sensor_type in self.data_callbacks:
            try:
                self.data_callbacks[sensor_type](data)
            except Exception as e:
                logging.error(f"[SerialDataManager] Error en callback de {sensor_type}: {e}")
        else:
            logging.debug(f"[SerialDataManager] Sin callback para sensor: {sensor_type}")

    def _handle_connection_error(self):
        """
        Maneja errores de conexión e intenta reconectar
        """
        logging.warning("[SerialDataManager] Perdida conexión, intentando reconectar...")
        self.disconnect()
        
        for attempt in range(self.max_reconnect_attempts):
            time.sleep(self.reconnect_delay)
            if self.connect():
                logging.info(f"[SerialDataManager] Reconectado exitosamente en intento {attempt + 1}")
                return
            else:
                logging.warning(f"[SerialDataManager] Fallo reconexión, intento {attempt + 1}/{self.max_reconnect_attempts}")
        
        logging.error("[SerialDataManager] No se pudo reconectar después de múltiples intentos")
        self.is_running = False

    def get_connection_status(self) -> Dict[str, Any]:
        """
        Retorna el estado de la conexión serial
        
        Returns:
            Diccionario con información de estado
        """
        return {
            "is_connected": self.is_connected,
            "port": self.port,
            "baudrate": self.baudrate,
            "is_reading": self.is_running,
            "connection_attempts": self.connection_attempts,
            "bytes_received": self.bytes_received,
            "bytes_sent": self.bytes_sent,
            "messages_received": self.messages_received,
            "messages_sent": self.messages_sent,
            "last_heartbeat": self.last_heartbeat.isoformat() if self.last_heartbeat else None,
            "registered_callbacks": list(self.data_callbacks.keys())
        }

    def calibrate_weight_sensor(self) -> bool:
        """
        Envía comando de calibración al sensor de peso
        
        Returns:
            True si el comando se envió exitosamente
        """
        command = "CALIBRATE_WEIGHT\n"
        try:
            if not self.is_connected or not self.serial_connection:
                logging.error("[SerialDataManager] No hay conexión para enviar comando")
                return False
            
            self.serial_connection.write(command.encode('utf-8'))
            self.bytes_sent += len(command)
            self.messages_sent += 1
            
            logging.info("[SerialDataManager] Comando de calibración enviado")
            return True
            
        except Exception as e:
            logging.error(f"[SerialDataManager] Error enviando comando de calibración: {e}")
            return False

    def tare_weight_sensor(self) -> bool:
        """
        Envía comando de tara al sensor de peso
        
        Returns:
            True si el comando se envió exitosamente
        """
        command = "TARE_WEIGHT\n"
        try:
            if not self.is_connected or not self.serial_connection:
                logging.error("[SerialDataManager] No hay conexión para enviar comando")
                return False
            
            self.serial_connection.write(command.encode('utf-8'))
            self.bytes_sent += len(command)
            self.messages_sent += 1
            
            logging.info("[SerialDataManager] Comando de tara enviado")
            return True
            
        except Exception as e:
            logging.error(f"[SerialDataManager] Error enviando comando de tara: {e}")
            return False

    def get_weight_status(self) -> bool:
        """
        Solicita el estado actual del sensor de peso
        
        Returns:
            True si el comando se envió exitosamente
        """
        command = "GET_WEIGHT_STATUS\n"
        try:
            if not self.is_connected or not self.serial_connection:
                logging.error("[SerialDataManager] No hay conexión para enviar comando")
                return False
            
            self.serial_connection.write(command.encode('utf-8'))
            self.bytes_sent += len(command)
            self.messages_sent += 1
            
            logging.debug("[SerialDataManager] Comando de estado enviado")
            return True
            
        except Exception as e:
            logging.error(f"[SerialDataManager] Error enviando comando de estado: {e}")
            return False
