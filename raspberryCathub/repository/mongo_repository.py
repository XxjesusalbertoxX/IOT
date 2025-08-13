"""
MongoDB Repository with Replica Set Support
Maneja la conexión y operaciones con MongoDB incluyendo soporte para replica set,
reconexión automática y almacenamiento offline
"""

import pymongo
from pymongo import MongoClient
from pymongo.errors import ConnectionFailure, ServerSelectionTimeoutError, AutoReconnect
import json
import time
import logging
import sqlite3
import os
from typing import Dict, Any, List, Optional
from datetime import datetime, timedelta
from threading import Lock

class MongoRepository:
    """
    Repository para MongoDB con soporte para replica set,
    almacenamiento offline y reconexión automática
    """
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.client = None
        self.database = None
        self.collection = None
        self.is_connected = False
        
        # Configuración de conexión
        self.connection_string = self._build_connection_string()
        self.database_name = config.get('database', 'cathub_iot')
        self.collection_name = config.get('collection', 'sensor_data')
        
        # Configuración de reconexión
        self.max_retries = config.get('max_retries', 5)
        self.retry_delay = config.get('retry_delay', 5.0)
        self.connection_timeout = config.get('connection_timeout', 10.0)
        
        # Almacenamiento offline
        self.offline_db_path = config.get('offline_db_path', '/tmp/cathub_offline.db')
        self.enable_offline = config.get('enable_offline', True)
        self.offline_lock = Lock()
        
        # Estadísticas
        self.documents_inserted = 0
        self.offline_documents = 0
        self.connection_errors = 0
        self.last_successful_connection = None
        
        if self.enable_offline:
            self._init_offline_storage()
        
        logging.info(f"[MongoRepository] Inicializado para: {self.database_name}.{self.collection_name}")

    def _build_connection_string(self) -> str:
        """
        Construye la cadena de conexión para MongoDB
        
        Returns:
            Cadena de conexión MongoDB
        """
        hosts = self.config.get('hosts', ['localhost:27017'])
        username = self.config.get('username')
        password = self.config.get('password')
        replica_set = self.config.get('replica_set')
        auth_source = self.config.get('auth_source', 'admin')
        
        # Construir URI
        if username and password:
            connection_string = f"mongodb://{username}:{password}@"
        else:
            connection_string = "mongodb://"
        
        connection_string += ",".join(hosts)
        
        # Parámetros adicionales
        params = []
        if replica_set:
            params.append(f"replicaSet={replica_set}")
        if username and password:
            params.append(f"authSource={auth_source}")
        
        # Parámetros de configuración adicionales
        params.extend([
            "retryWrites=true",
            "w=majority",
            "readPreference=primary",
            f"serverSelectionTimeoutMS={int(self.connection_timeout * 1000)}",
            "socketTimeoutMS=30000",
            "connectTimeoutMS=10000"
        ])
        
        if params:
            connection_string += "/?" + "&".join(params)
        
        logging.info(f"[MongoRepository] Connection string construido: {connection_string.split('@')[0]}@****")
        return connection_string

    def connect(self) -> bool:
        """
        Establece conexión con MongoDB
        
        Returns:
            True si la conexión es exitosa, False caso contrario
        """
        try:
            logging.info("[MongoRepository] Intentando conectar a MongoDB...")
            
            self.client = MongoClient(
                self.connection_string,
                serverSelectionTimeoutMS=int(self.connection_timeout * 1000)
            )
            
            # Probar la conexión
            self.client.admin.command('ping')
            
            # Configurar database y collection
            self.database = self.client[self.database_name]
            self.collection = self.database[self.collection_name]
            
            # Crear índices si no existen
            self._ensure_indexes()
            
            self.is_connected = True
            self.last_successful_connection = datetime.utcnow()
            
            logging.info(f"[MongoRepository] Conectado exitosamente a MongoDB")
            
            # Intentar sincronizar datos offline si existen
            if self.enable_offline:
                self._sync_offline_data()
            
            return True
            
        except (ConnectionFailure, ServerSelectionTimeoutError) as e:
            self.connection_errors += 1
            logging.error(f"[MongoRepository] Error de conexión MongoDB: {e}")
            self.is_connected = False
            return False
        except Exception as e:
            self.connection_errors += 1
            logging.error(f"[MongoRepository] Error inesperado conectando: {e}")
            self.is_connected = False
            return False

    def disconnect(self):
        """
        Cierra la conexión con MongoDB
        """
        if self.client:
            self.client.close()
            self.is_connected = False
            logging.info("[MongoRepository] Desconectado de MongoDB")

    def insert_sensor_data(self, data: Dict[str, Any]) -> bool:
        """
        Inserta datos de sensor en MongoDB
        
        Args:
            data: Datos del sensor a insertar
            
        Returns:
            True si la inserción fue exitosa, False caso contrario
        """
        if not self._ensure_connection():
            # Si no hay conexión, guardar offline si está habilitado
            if self.enable_offline:
                return self._store_offline(data)
            return False
        
        try:
            # Agregar metadata adicional
            enhanced_data = self._enhance_document(data)
            
            result = self.collection.insert_one(enhanced_data)
            
            if result.inserted_id:
                self.documents_inserted += 1
                logging.debug(f"[MongoRepository] Documento insertado: {result.inserted_id}")
                return True
            else:
                logging.error("[MongoRepository] No se pudo insertar el documento")
                return False
                
        except AutoReconnect:
            logging.warning("[MongoRepository] Reconexión automática en progreso")
            if self.enable_offline:
                return self._store_offline(data)
            return False
        except Exception as e:
            logging.error(f"[MongoRepository] Error insertando datos: {e}")
            if self.enable_offline:
                return self._store_offline(data)
            return False

    def insert_multiple_sensor_data(self, data_list: List[Dict[str, Any]]) -> int:
        """
        Inserta múltiples documentos de datos de sensores
        
        Args:
            data_list: Lista de datos de sensores
            
        Returns:
            Número de documentos insertados exitosamente
        """
        if not self._ensure_connection():
            if self.enable_offline:
                count = 0
                for data in data_list:
                    if self._store_offline(data):
                        count += 1
                return count
            return 0
        
        try:
            enhanced_docs = [self._enhance_document(data) for data in data_list]
            result = self.collection.insert_many(enhanced_docs, ordered=False)
            
            inserted_count = len(result.inserted_ids)
            self.documents_inserted += inserted_count
            
            logging.info(f"[MongoRepository] Insertados {inserted_count} documentos")
            return inserted_count
            
        except Exception as e:
            logging.error(f"[MongoRepository] Error insertando múltiples datos: {e}")
            # Intentar insertar uno por uno en offline si falla la operación masiva
            if self.enable_offline:
                count = 0
                for data in data_list:
                    if self._store_offline(data):
                        count += 1
                return count
            return 0

    def _ensure_connection(self) -> bool:
        """
        Asegura que hay conexión activa con MongoDB
        
        Returns:
            True si hay conexión, False caso contrario
        """
        if not self.is_connected:
            return self.connect()
        
        try:
            # Probar la conexión
            self.client.admin.command('ping')
            return True
        except Exception:
            self.is_connected = False
            return self.connect()

    def _enhance_document(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Mejora el documento con metadata adicional
        
        Args:
            data: Datos originales del sensor
            
        Returns:
            Documento mejorado con metadata adicional
        """
        enhanced = data.copy()
        
        # Agregar timestamp de inserción si no existe
        if 'inserted_at' not in enhanced:
            enhanced['inserted_at'] = datetime.utcnow()
        
        # Agregar TTL si está configurado (para datos temporales)
        ttl_hours = self.config.get('ttl_hours')
        if ttl_hours:
            enhanced['expires_at'] = datetime.utcnow() + timedelta(hours=ttl_hours)
        
        # Agregar metadata del repositorio
        enhanced['_repository_metadata'] = {
            'inserted_by': 'raspberry_pi',
            'version': '1.0',
            'source': 'arduino_serial'
        }
        
        return enhanced

    def _ensure_indexes(self):
        """
        Crea índices necesarios en la colección
        """
        try:
            # Índice por timestamp para consultas temporales
            self.collection.create_index([
                ("timestamp", pymongo.ASCENDING)
            ])
            
            # Índice por tipo de sensor y timestamp
            self.collection.create_index([
                ("sensor_type", pymongo.ASCENDING),
                ("timestamp", pymongo.ASCENDING)
            ])
            
            # Índice por device_id
            self.collection.create_index([
                ("device_id", pymongo.ASCENDING)
            ])
            
            # Índice TTL si está configurado
            ttl_hours = self.config.get('ttl_hours')
            if ttl_hours:
                self.collection.create_index([
                    ("expires_at", pymongo.ASCENDING)
                ], expireAfterSeconds=0)
            
            logging.info("[MongoRepository] Índices creados/verificados")
            
        except Exception as e:
            logging.warning(f"[MongoRepository] Error creando índices: {e}")

    def _init_offline_storage(self):
        """
        Inicializa el almacenamiento offline SQLite
        """
        try:
            # Crear directorio si no existe
            os.makedirs(os.path.dirname(self.offline_db_path), exist_ok=True)
            
            with sqlite3.connect(self.offline_db_path) as conn:
                conn.execute('''
                    CREATE TABLE IF NOT EXISTS offline_sensor_data (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        data TEXT NOT NULL,
                        created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                        synced BOOLEAN DEFAULT FALSE
                    )
                ''')
                conn.commit()
            
            logging.info(f"[MongoRepository] Almacenamiento offline inicializado: {self.offline_db_path}")
            
        except Exception as e:
            logging.error(f"[MongoRepository] Error inicializando almacenamiento offline: {e}")
            self.enable_offline = False

    def _store_offline(self, data: Dict[str, Any]) -> bool:
        """
        Almacena datos en SQLite para sincronización posterior
        
        Args:
            data: Datos a almacenar offline
            
        Returns:
            True si se almacenó exitosamente, False caso contrario
        """
        if not self.enable_offline:
            return False
        
        try:
            with self.offline_lock:
                with sqlite3.connect(self.offline_db_path) as conn:
                    json_data = json.dumps(data, default=str)
                    conn.execute(
                        'INSERT INTO offline_sensor_data (data) VALUES (?)',
                        (json_data,)
                    )
                    conn.commit()
                    
                self.offline_documents += 1
                logging.debug("[MongoRepository] Datos almacenados offline")
                return True
                
        except Exception as e:
            logging.error(f"[MongoRepository] Error almacenando offline: {e}")
            return False

    def _sync_offline_data(self):
        """
        Sincroniza datos almacenados offline con MongoDB
        """
        if not self.enable_offline or not self.is_connected:
            return
        
        try:
            with self.offline_lock:
                with sqlite3.connect(self.offline_db_path) as conn:
                    cursor = conn.execute(
                        'SELECT id, data FROM offline_sensor_data WHERE synced = FALSE LIMIT 100'
                    )
                    
                    rows = cursor.fetchall()
                    if not rows:
                        return
                    
                    # Preparar documentos para inserción
                    documents = []
                    row_ids = []
                    
                    for row_id, json_data in rows:
                        try:
                            data = json.loads(json_data)
                            enhanced_data = self._enhance_document(data)
                            documents.append(enhanced_data)
                            row_ids.append(row_id)
                        except Exception as e:
                            logging.error(f"[MongoRepository] Error deserializando dato offline: {e}")
                            continue
                    
                    if documents:
                        # Insertar en MongoDB
                        inserted_count = self.insert_multiple_sensor_data(documents)
                        
                        if inserted_count > 0:
                            # Marcar como sincronizado
                            placeholders = ','.join(['?' for _ in row_ids])
                            conn.execute(
                                f'UPDATE offline_sensor_data SET synced = TRUE WHERE id IN ({placeholders})',
                                row_ids
                            )
                            conn.commit()
                            
                            self.offline_documents = max(0, self.offline_documents - inserted_count)
                            logging.info(f"[MongoRepository] Sincronizados {inserted_count} documentos offline")
                    
        except Exception as e:
            logging.error(f"[MongoRepository] Error sincronizando datos offline: {e}")

    def get_repository_status(self) -> Dict[str, Any]:
        """
        Retorna el estado del repositorio MongoDB
        
        Returns:
            Diccionario con información de estado
        """
        return {
            "is_connected": self.is_connected,
            "database_name": self.database_name,
            "collection_name": self.collection_name,
            "documents_inserted": self.documents_inserted,
            "offline_documents_pending": self.offline_documents,
            "connection_errors": self.connection_errors,
            "last_successful_connection": self.last_successful_connection.isoformat() if self.last_successful_connection else None,
            "offline_storage_enabled": self.enable_offline,
            "offline_db_path": self.offline_db_path if self.enable_offline else None
        }
