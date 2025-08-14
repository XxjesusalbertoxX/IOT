"""
MongoDB Handler - RESPONSABLE EXCLUSIVO de guardar readings
"""

import logging
from typing import Dict, Any, Optional, List
from pymongo import MongoClient
from pymongo.errors import ConnectionFailure, ServerSelectionTimeoutError, AutoReconnect
from datetime import datetime
from database.postgres_handler import PostgresHandler

class MongoHandler:
    """
    Handler para MongoDB - RESPONSABILIDAD EXCLUSIVA de persistencia
    
    Responsabilidades:
    1. Conexión y mantenimiento de MongoDB
    2. Guardar readings con fallbacks
    3. Consultar readings
    4. Manejo de errores de conexión
    5. Sync con local_storage cuando falle
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.client = None
        self.db = None
        self.readings_collection = None
        self.connected = False
        
        # Handler PostgreSQL para configuraciones
        self.postgres = PostgresHandler()
        
        # ✅ REFERENCIA A LOCAL_STORAGE (se asigna después)
        self.local_storage = None
        
        # Cache de device_ids para eficiencia
        self.device_id_cache = {}

    def set_local_storage(self, local_storage):
        """
        Asigna referencia al LocalStorage para fallbacks
        
        Args:
            local_storage: LocalStorage instance
        """
        self.local_storage = local_storage
        self.logger.info("✅ LocalStorage asignado para fallbacks")

    def connect(self) -> bool:
        """Conectar a MongoDB usando configuración de PostgreSQL"""
        try:
            # Obtener configuración desde PostgreSQL
            config = self.postgres.get_mongo_config()
            
            # Construir connection string
            connection_string = (
                f"mongodb://{config['username']}:{config['password']}@"
                f"{config['host']}:{config['port']}/{config['database']}"
                f"?authSource={config['auth_source']}"
            )
            
            self.logger.info("🔗 Conectando a MongoDB...")
            
            self.client = MongoClient(
                connection_string,
                serverSelectionTimeoutMS=5000  # 5 segundos timeout
            )
            
            # Probar conexión
            self.client.admin.command('ping')
            
            # Configurar database y colección
            self.db = self.client[config['database']]
            self.readings_collection = self.db['readings']  # ✅ Colección readings
            
            # Crear índices
            self._create_indexes()
            
            self.connected = True
            self.logger.info("✅ Conectado a MongoDB - Colección: readings")
            
            return True
            
        except Exception as e:
            self.logger.error(f"❌ Error conectando MongoDB: {e}")
            self.connected = False
            return False

    def _create_indexes(self):
        """Crear índices para la colección readings"""
        try:
            # Índice por timestamp (para consultas temporales)
            self.readings_collection.create_index("timestamp")
            
            # Índice por sensor_name
            self.readings_collection.create_index("sensor_name")
            
            # Índice por identifier
            self.readings_collection.create_index("identifier")
            
            # Índice por device_id
            self.readings_collection.create_index("device_id")
            
            # Índice compuesto para consultas eficientes
            self.readings_collection.create_index([
                ("sensor_name", 1),
                ("timestamp", -1)
            ])
            
            self.logger.debug("✅ Índices creados en colección readings")
            
        except Exception as e:
            self.logger.warning(f"⚠️ Error creando índices: {e}")

    def save_sensor_reading(self, document: Dict[str, Any]) -> bool:
        """
        ✅ RESPONSABILIDAD EXCLUSIVA - Guardar reading con fallback inteligente
        
        Args:
            document: {
                "sensor_name": "feeder_weight",
                "identifier": "WSR001", 
                "value": 150.5,
                "timestamp": datetime,
                "device_id": 1
            }
            
        Returns:
            True si se guardó (MongoDB o LocalStorage)
        """
        try:
            # ✅ VALIDAR ESTRUCTURA
            if not self._validate_document(document):
                return False
            
            # ✅ INTENTAR MONGODB PRIMERO
            if self.connected and self._save_to_mongo(document):
                # ✅ ÉXITO MONGODB - Guardar copia permanente local
                if self.local_storage:
                    self.local_storage.save_reading_permanent(document)
                
                self.logger.debug(
                    f"✅ Reading guardado: {document['sensor_name']}({document['identifier']}) = {document['value']}"
                )
                return True
            
            # ✅ FALLBACK A LOCAL_STORAGE
            if self.local_storage:
                self.logger.warning(f"⚠️ MongoDB falló - guardando offline: {document['sensor_name']}")
                self.local_storage.save_offline(document)
                self.local_storage.save_reading_permanent(document)
                return True
            
            # ✅ NO HAY FALLBACK
            self.logger.error("❌ No se pudo guardar reading - sin fallback")
            return False
                
        except Exception as e:
            self.logger.error(f"❌ Error en save_sensor_reading: {e}")
            
            # ✅ FALLBACK EN CASO DE EXCEPCIÓN
            if self.local_storage:
                try:
                    self.local_storage.save_offline(document)
                    self.local_storage.save_reading_permanent(document)
                    return True
                except:
                    pass
            
            return False

    def _validate_document(self, document: Dict[str, Any]) -> bool:
        """Valida estructura del documento"""
        required_fields = ["sensor_name", "identifier", "value", "timestamp", "device_id"]
        
        for field in required_fields:
            if field not in document:
                self.logger.error(f"❌ Campo requerido faltante: {field}")
                return False
        
        return True

    def _save_to_mongo(self, document: Dict[str, Any]) -> bool:
        """Intenta guardar en MongoDB"""
        try:
            # Verificar conexión
            if not self.is_connected():
                return False
            
            # Insertar
            result = self.readings_collection.insert_one(document)
            
            if result.inserted_id:
                return True
            else:
                self.logger.error("❌ MongoDB insertó pero sin ID")
                return False
                
        except AutoReconnect:
            self.logger.warning("🔄 MongoDB reconectando automáticamente...")
            self.connected = False
            return False
            
        except Exception as e:
            self.logger.error(f"❌ Error insertando en MongoDB: {e}")
            self.connected = False  # Marcar como desconectado
            return False

    def get_device_id(self, identifier: str) -> Optional[int]:
        """
        Obtiene device_id desde PostgreSQL (con cache)
        
        Args:
            identifier: Identificador del sensor (WSR001, PRS001, etc.)
            
        Returns:
            device_id o None
        """
        # Verificar cache primero
        if identifier in self.device_id_cache:
            return self.device_id_cache[identifier]
        
        # Obtener desde PostgreSQL
        device_id = self.postgres.get_device_id(identifier)
        
        # Guardar en cache si se encontró
        if device_id:
            self.device_id_cache[identifier] = device_id
            
        return device_id

    def is_connected(self) -> bool:
        """Verifica estado de conexión con ping"""
        if not self.connected or not self.client:
            return False
            
        try:
            self.client.admin.command('ping')
            return True
        except:
            self.connected = False
            return False

    def get_recent_readings(self, sensor_name: str = None, identifier: str = None, limit: int = 10) -> List[Dict[str, Any]]:
        """
        Obtiene readings recientes
        
        Args:
            sensor_name: Filtrar por nombre de sensor (opcional)
            identifier: Filtrar por identifier (opcional) 
            limit: Número máximo de resultados
            
        Returns:
            Lista de readings
        """
        try:
            if not self.is_connected():
                self.logger.warning("⚠️ MongoDB desconectado para consulta")
                return []
            
            # Construir filtro
            filter_query = {}
            if sensor_name:
                filter_query["sensor_name"] = sensor_name
            if identifier:
                filter_query["identifier"] = identifier
            
            # Consultar readings
            cursor = self.readings_collection.find(filter_query).sort("timestamp", -1).limit(limit)
            
            readings = []
            for doc in cursor:
                # Convertir ObjectId a string para serialización
                doc['_id'] = str(doc['_id'])
                readings.append(doc)
            
            self.logger.debug(f"✅ Obtenidos {len(readings)} readings")
            return readings
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo readings: {e}")
            return []

    def get_readings_by_timerange(self, start_time: datetime, end_time: datetime, 
                                 sensor_name: str = None) -> List[Dict[str, Any]]:
        """
        Obtiene readings por rango de tiempo
        
        Args:
            start_time: Fecha/hora inicio
            end_time: Fecha/hora fin
            sensor_name: Filtrar por sensor (opcional)
            
        Returns:
            Lista de readings en el rango
        """
        try:
            if not self.is_connected():
                return []
            
            # Construir filtro
            filter_query = {
                "timestamp": {
                    "$gte": start_time,
                    "$lte": end_time
                }
            }
            
            if sensor_name:
                filter_query["sensor_name"] = sensor_name
            
            # Consultar
            cursor = self.readings_collection.find(filter_query).sort("timestamp", 1)
            
            readings = []
            for doc in cursor:
                doc['_id'] = str(doc['_id'])
                readings.append(doc)
            
            self.logger.debug(f"✅ Obtenidos {len(readings)} readings en rango temporal")
            return readings
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo readings por tiempo: {e}")
            return []

    def get_latest_reading(self, identifier: str) -> Optional[Dict[str, Any]]:
        """
        Obtiene la última lectura de un sensor específico
        
        Args:
            identifier: Identificador del sensor
            
        Returns:
            Último reading o None
        """
        try:
            if not self.is_connected():
                return None
            
            doc = self.readings_collection.find_one(
                {"identifier": identifier},
                sort=[("timestamp", -1)]
            )
            
            if doc:
                doc['_id'] = str(doc['_id'])
                return doc
                
            return None
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo último reading: {e}")
            return None

    def sync_offline_data(self, local_storage) -> int:
        """
        Sincroniza datos offline desde LocalStorage
        
        Args:
            local_storage: LocalStorage instance
            
        Returns:
            Número de readings sincronizados
        """
        if not self.is_connected():
            self.logger.warning("⚠️ No se puede sincronizar - MongoDB desconectado")
            return 0
        
        try:
            synced_count = 0
            offline_data = local_storage.get_offline_data()
            
            for document in offline_data:
                if self._save_to_mongo(document):
                    local_storage.mark_as_synced(document)
                    synced_count += 1
                else:
                    break  # Si falla uno, probablemente fallarán todos
            
            if synced_count > 0:
                self.logger.info(f"🔄 {synced_count} readings offline sincronizados")
            
            return synced_count
            
        except Exception as e:
            self.logger.error(f"❌ Error sincronizando offline: {e}")
            return 0

    def disconnect(self):
        """Cerrar conexión"""
        if self.client:
            self.client.close()
            self.connected = False
            self.logger.info("👋 Desconectado de MongoDB")

    def get_status(self) -> Dict[str, Any]:
        """Estado del handler MongoDB"""
        return {
            "connected": self.connected,
            "database": self.db.name if self.db else None,
            "collection": "readings",
            "cached_device_ids": len(self.device_id_cache),
            "has_local_storage": self.local_storage is not None
        }