"""
CatHub IoT Weight Sensor Service - Raspberry Pi
Servicio específico para el sensor de peso que orquesta la comunicación con Arduino,
procesamiento de datos del sensor de peso y almacenamiento en MongoDB
"""

import json
import time
import signal
import logging
import yaml
import sys
import os
from threading import Event
from typing import Dict, Any

# Importar nuestras clases personalizadas
from scripts.serial_data_manager import SerialDataManager
from sensors.weight_sensor_handler import WeightSensorHandler
from repository.mongo_repository import MongoRepository

class CatHubWeightSensorService:
    """
    Servicio especializado para el sensor de peso que coordina todos los componentes
    """
    
    def __init__(self, config_path: str = 'weight_sensor_config.yaml'):
        self.config_path = config_path
        self.config = {}
        self.is_running = False
        self.shutdown_event = Event()
        
        # Componentes principales
        self.serial_manager = None
        self.weight_sensor_handler = None
        self.mongo_repository = None
        
        # Configurar logging
        self._setup_logging()
        
        # Cargar configuración
        self._load_config()
        
        # Configurar manejo de señales para shutdown graceful
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
        
        logging.info("[CatHubWeightSensorService] Servicio inicializado")

    def _setup_logging(self):
        """
        Configura el sistema de logging
        """
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('/var/log/cathub_weight_sensor.log'),
                logging.StreamHandler(sys.stdout)
            ]
        )

    def _load_config(self):
        """
        Carga la configuración desde archivo YAML
        """
        try:
            if os.path.exists(self.config_path):
                with open(self.config_path, 'r') as f:
                    self.config = yaml.safe_load(f)
                logging.info(f"[CatHubWeightSensorService] Configuración cargada desde: {self.config_path}")
            else:
                # Usar configuración por defecto si no existe el archivo
                self.config = self._get_default_config()
                self._save_config()
                logging.info("[CatHubWeightSensorService] Usando configuración por defecto")
                
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error cargando configuración: {e}")
            self.config = self._get_default_config()

    def _get_default_config(self) -> Dict[str, Any]:
        """
        Retorna la configuración por defecto específica para el sensor de peso
        
        Returns:
            Configuración por defecto
        """
        return {
            'serial': {
                'port': '/dev/ttyACM0',
                'baudrate': 115200,
                'timeout': 1.0
            },
            'sensors': {
                'weight': {
                    'weight_threshold': 5.0,
                    'stability_timeout': 10,
                    'min_weight_alert': 10.0,
                    'max_weight_alert': 5000.0,
                    'debug': False
                }
            },
            'mongodb': {
                'hosts': ['localhost:27017', 'mongodb-replica1:27017', 'mongodb-replica2:27017'],
                'database': 'cathub_iot',
                'collection': 'weight_sensor_data',
                'replica_set': 'rs0',
                'username': 'cathub_user',
                'password': 'cathub_password',
                'auth_source': 'cathub_iot',
                'max_retries': 5,
                'retry_delay': 5.0,
                'connection_timeout': 10.0,
                'enable_offline': True,
                'offline_db_path': '/tmp/cathub_weight_offline.db',
                'ttl_hours': 24
            },
            'device': {
                'device_id': 'cathub_weight_sensor_01',
                'location': 'food_bowl'
            },
            'logging': {
                'level': 'INFO',
                'log_file': '/var/log/cathub_weight_sensor.log'
            }
        }

    def _save_config(self):
        """
        Guarda la configuración actual en archivo
        """
        try:
            with open(self.config_path, 'w') as f:
                yaml.dump(self.config, f, default_flow_style=False, indent=2)
            logging.info(f"[CatHubWeightSensorService] Configuración guardada en: {self.config_path}")
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error guardando configuración: {e}")

    def _signal_handler(self, signum, frame):
        """
        Maneja señales del sistema para shutdown graceful
        
        Args:
            signum: Número de señal
            frame: Frame de ejecución
        """
        logging.info(f"[CatHubWeightSensorService] Recibida señal {signum}, iniciando shutdown...")
        self.shutdown_event.set()

    def initialize_components(self) -> bool:
        """
        Inicializa todos los componentes del servicio
        
        Returns:
            True si todos los componentes se inicializaron correctamente
        """
        try:
            logging.info("[CatHubWeightSensorService] Inicializando componentes...")
            
            # Inicializar MongoDB Repository
            mongo_config = self.config.get('mongodb', {})
            mongo_config['device_id'] = self.config.get('device', {}).get('device_id', 'cathub_weight_default')
            
            self.mongo_repository = MongoRepository(mongo_config)
            if not self.mongo_repository.connect():
                logging.warning("[CatHubWeightSensorService] MongoDB no disponible, continuando con almacenamiento offline")
            
            # Inicializar Weight Sensor Handler
            weight_config = self.config.get('sensors', {}).get('weight', {})
            weight_config.update(self.config.get('device', {}))  # Agregar device_id y location
            
            self.weight_sensor_handler = WeightSensorHandler(weight_config)
            
            # Inicializar Serial Manager
            serial_config = self.config.get('serial', {})
            self.serial_manager = SerialDataManager(
                port=serial_config.get('port', '/dev/ttyACM0'),
                baudrate=serial_config.get('baudrate', 115200),
                timeout=serial_config.get('timeout', 1.0)
            )
            
            # Registrar callback para datos del sensor de peso
            self.serial_manager.register_data_callback('weight', self._process_weight_data)
            
            # Conectar al Arduino
            if not self.serial_manager.connect():
                logging.error("[CatHubWeightSensorService] No se pudo conectar al Arduino")
                return False
            
            # Iniciar lectura de datos seriales
            if not self.serial_manager.start_reading():
                logging.error("[CatHubWeightSensorService] No se pudo iniciar lectura serial")
                return False
            
            logging.info("[CatHubWeightSensorService] Todos los componentes inicializados correctamente")
            return True
            
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error inicializando componentes: {e}")
            return False

    def _process_weight_data(self, arduino_data: Dict[str, Any]):
        """
        Procesa datos del sensor de peso recibidos del Arduino
        
        Args:
            arduino_data: Datos recibidos del Arduino
        """
        try:
            # Procesar datos a través del handler especializado
            processed_data = self.weight_sensor_handler.process_sensor_data(arduino_data)
            
            if processed_data:
                # Actualizar estadísticas del handler
                self.weight_sensor_handler.update_statistics(success=True)
                
                # Almacenar en MongoDB
                if self.mongo_repository.insert_sensor_data(processed_data):
                    logging.debug("[CatHubWeightSensorService] Datos de peso almacenados exitosamente")
                    
                    # Log detallado de datos importantes
                    weight = processed_data['data']['weight_grams']
                    is_stable = processed_data['data']['weight_stable']
                    alerts = processed_data.get('alerts', [])
                    
                    if is_stable:
                        logging.info(f"[CatHubWeightSensorService] Peso estable: {weight:.2f}g")
                    
                    if alerts:
                        for alert in alerts:
                            logging.warning(f"[CatHubWeightSensorService] ALERTA: {alert['message']}")
                            
                else:
                    logging.warning("[CatHubWeightSensorService] Error almacenando datos de peso")
            else:
                self.weight_sensor_handler.update_statistics(success=False)
                logging.warning("[CatHubWeightSensorService] Error procesando datos de peso")
                
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error en callback de peso: {e}")
            self.weight_sensor_handler.update_statistics(success=False)

    def run(self):
        """
        Ejecuta el bucle principal del servicio
        """
        if not self.initialize_components():
            logging.error("[CatHubWeightSensorService] Error inicializando componentes, abortando")
            return False
        
        self.is_running = True
        logging.info("[CatHubWeightSensorService] Servicio de sensor de peso iniciado, presiona Ctrl+C para detener")
        
        # Estadísticas para logging periódico
        last_stats_time = time.time()
        stats_interval = 60.0  # 1 minuto
        
        # Enviar comando de tara inicial si está configurado
        if self.config.get('sensors', {}).get('weight', {}).get('auto_tare', False):
            logging.info("[CatHubWeightSensorService] Enviando tara inicial...")
            self.tare_weight_sensor()
        
        try:
            while self.is_running and not self.shutdown_event.is_set():
                current_time = time.time()
                
                # Log de estadísticas periódicas
                if current_time - last_stats_time >= stats_interval:
                    self._log_service_statistics()
                    last_stats_time = current_time
                
                # Verificar estado de conexiones y intentar reconectar si es necesario
                self._check_connections()
                
                # Verificar si necesitamos sincronizar datos offline
                if self.mongo_repository and self.mongo_repository.is_connected:
                    self.mongo_repository._sync_offline_data()
                
                # Pequeña pausa para no consumir CPU innecesariamente
                time.sleep(1.0)
                
        except KeyboardInterrupt:
            logging.info("[CatHubWeightSensorService] Interrupción por teclado recibida")
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error en bucle principal: {e}")
        finally:
            self._cleanup()
        
        logging.info("[CatHubWeightSensorService] Servicio detenido")
        return True

    def _check_connections(self):
        """
        Verifica el estado de las conexiones y intenta reconectar si es necesario
        """
        # Verificar conexión MongoDB
        if self.mongo_repository and not self.mongo_repository.is_connected:
            logging.debug("[CatHubWeightSensorService] Intentando reconectar MongoDB...")
            self.mongo_repository.connect()
        
        # Verificar conexión Serial
        if self.serial_manager and not self.serial_manager.is_connected:
            logging.debug("[CatHubWeightSensorService] Intentando reconectar Arduino...")
            if self.serial_manager.connect():
                self.serial_manager.start_reading()

    def _log_service_statistics(self):
        """
        Log periódico de estadísticas del servicio
        """
        try:
            current_weight = self.weight_sensor_handler.get_current_weight()
            stable_weight = self.weight_sensor_handler.get_stable_weight()
            is_stable = self.weight_sensor_handler.is_weight_stable()
            
            weight_stats = self.weight_sensor_handler.get_sensor_status()
            serial_stats = self.serial_manager.get_connection_status() if self.serial_manager else {}
            mongo_stats = self.mongo_repository.get_repository_status() if self.mongo_repository else {}
            
            logging.info(f"[CatHubWeightSensorService] === ESTADÍSTICAS PERIÓDICAS ===")
            logging.info(f"  - Peso actual: {current_weight:.2f}g (estable: {'Sí' if is_stable else 'No'})")
            logging.info(f"  - Último peso estable: {stable_weight:.2f}g")
            logging.info(f"  - Total lecturas: {weight_stats.get('total_readings', 0)}")
            logging.info(f"  - Tasa de error: {weight_stats.get('error_rate_percent', 0):.1f}%")
            logging.info(f"  - Mensajes serie RX: {serial_stats.get('messages_received', 0)}")
            logging.info(f"  - Mensajes serie TX: {serial_stats.get('messages_sent', 0)}")
            logging.info(f"  - Docs MongoDB: {mongo_stats.get('documents_inserted', 0)}")
            logging.info(f"  - Docs offline pendientes: {mongo_stats.get('offline_documents_pending', 0)}")
            logging.info(f"  - Estado MongoDB: {'Conectado' if mongo_stats.get('is_connected', False) else 'Desconectado'}")
            logging.info(f"  - Estado Serial: {'Conectado' if serial_stats.get('is_connected', False) else 'Desconectado'}")
            
        except Exception as e:
            logging.error(f"[CatHubWeightSensorService] Error generando estadísticas: {e}")

    def _cleanup(self):
        """
        Limpia recursos y cierra conexiones
        """
        logging.info("[CatHubWeightSensorService] Iniciando limpieza de recursos...")
        
        self.is_running = False
        
        # Detener lectura serial
        if self.serial_manager:
            self.serial_manager.stop_reading()
            self.serial_manager.disconnect()
        
        # Cerrar conexión MongoDB
        if self.mongo_repository:
            # Intentar sincronizar datos offline antes de cerrar
            if self.mongo_repository.is_connected:
                self.mongo_repository._sync_offline_data()
            self.mongo_repository.disconnect()
        
        logging.info("[CatHubWeightSensorService] Limpieza completada")

    def calibrate_weight_sensor(self) -> bool:
        """
        Calibra el sensor de peso
        
        Returns:
            True si el comando de calibración se envió exitosamente
        """
        logging.info("[CatHubWeightSensorService] Enviando comando de calibración...")
        if self.serial_manager:
            return self.serial_manager.calibrate_weight_sensor()
        return False

    def tare_weight_sensor(self) -> bool:
        """
        Realiza tara del sensor de peso
        
        Returns:
            True si el comando de tara se envió exitosamente
        """
        logging.info("[CatHubWeightSensorService] Enviando comando de tara...")
        if self.serial_manager:
            return self.serial_manager.tare_weight_sensor()
        return False

    def get_current_weight(self) -> float:
        """
        Obtiene el peso actual del sensor
        
        Returns:
            Peso actual en gramos
        """
        if self.weight_sensor_handler:
            return self.weight_sensor_handler.get_current_weight()
        return 0.0

    def get_service_status(self) -> Dict[str, Any]:
        """
        Retorna el estado completo del servicio
        
        Returns:
            Diccionario con información de estado de todos los componentes
        """
        return {
            'service': {
                'is_running': self.is_running,
                'config_path': self.config_path,
                'current_weight_grams': self.get_current_weight()
            },
            'weight_sensor': self.weight_sensor_handler.get_sensor_status() if self.weight_sensor_handler else None,
            'serial_manager': self.serial_manager.get_connection_status() if self.serial_manager else None,
            'mongodb': self.mongo_repository.get_repository_status() if self.mongo_repository else None
        }


def main():
    """
    Función principal para ejecutar el servicio
    """
    import argparse
    
    parser = argparse.ArgumentParser(description='CatHub Weight Sensor IoT Service')
    parser.add_argument('--config', '-c', default='weight_sensor_config.yaml', help='Archivo de configuración')
    parser.add_argument('--calibrate', action='store_true', help='Calibrar sensor de peso al inicio')
    parser.add_argument('--tare', action='store_true', help='Realizar tara del sensor de peso al inicio')
    parser.add_argument('--status', action='store_true', help='Mostrar estado actual y salir')
    parser.add_argument('--debug', action='store_true', help='Habilitar modo debug')
    
    args = parser.parse_args()
    
    # Ajustar nivel de logging si se requiere debug
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Crear e inicializar servicio
    service = CatHubWeightSensorService(config_path=args.config)
    
    # Mostrar estado y salir
    if args.status:
        if service.initialize_components():
            status = service.get_service_status()
            print(json.dumps(status, indent=2, default=str))
        return
    
    # Comandos especiales
    if args.calibrate:
        logging.info("Modo calibración activado...")
        if service.initialize_components():
            service.calibrate_weight_sensor()
            time.sleep(10)  # Esperar a que complete
        return
    
    if args.tare:
        logging.info("Realizando tara...")
        if service.initialize_components():
            service.tare_weight_sensor()
            time.sleep(5)  # Esperar a que complete
        return
    
    # Ejecutar servicio normal
    success = service.run()
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
