"""
Local Storage - Dos carpetas: sensor_data (permanente) y offline (temporal)
"""

import json
import sqlite3
import logging
from typing import Dict, Any, List
from datetime import datetime
import os

class LocalStorage:
    """
    Almacenamiento local con dos propósitos:
    1. sensor_data/ - Copia permanente local de TODOS los readings
    2. offline/ - Temporal para readings pendientes de subir a MongoDB
    """
    
    def __init__(self, base_path: str = "."):
        self.base_path = base_path
        self.logger = logging.getLogger(__name__)
        
        # ✅ Dos carpetas separadas
        self.sensor_data_path = os.path.join(base_path, "sensor_data")
        self.offline_path = os.path.join(base_path, "offline")
        
        # ✅ Dos bases de datos SQLite
        self.sensor_data_db = os.path.join(self.sensor_data_path, "sensor_readings.db")
        self.offline_db = os.path.join(self.offline_path, "offline_readings.db")
        
        # Conexiones
        self.sensor_data_conn = None
        self.offline_conn = None

    def initialize(self):
        """Inicializa carpetas y bases de datos"""
        try:
            # ✅ Crear carpetas si no existen
            os.makedirs(self.sensor_data_path, exist_ok=True)
            os.makedirs(self.offline_path, exist_ok=True)
            
            # ✅ Inicializar base de datos PERMANENTE (sensor_data)
            self._init_sensor_data_db()
            
            # ✅ Inicializar base de datos TEMPORAL (offline)
            self._init_offline_db()
            
            self.logger.info(f"✅ Local storage inicializado:")
            self.logger.info(f"   📁 Sensor data: {self.sensor_data_path}")
            self.logger.info(f"   📁 Offline: {self.offline_path}")
            
        except Exception as e:
            self.logger.error(f"❌ Error inicializando local storage: {e}")

    def _init_sensor_data_db(self):
        """Inicializa base de datos permanente"""
        self.sensor_data_conn = sqlite3.connect(self.sensor_data_db, check_same_thread=False)
        self.sensor_data_conn.row_factory = sqlite3.Row
        
        # ✅ Tabla para TODOS los readings (copia permanente)
        self.sensor_data_conn.execute('''
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sensor_name TEXT NOT NULL,
                identifier TEXT NOT NULL,
                value REAL NOT NULL,
                timestamp TEXT NOT NULL,
                device_id INTEGER,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Índices para consultas rápidas
        self.sensor_data_conn.execute('''
            CREATE INDEX IF NOT EXISTS idx_sensor_data_sensor 
            ON readings(sensor_name, timestamp)
        ''')
        
        self.sensor_data_conn.execute('''
            CREATE INDEX IF NOT EXISTS idx_sensor_data_identifier 
            ON readings(identifier, timestamp)
        ''')
        
        self.sensor_data_conn.commit()

    def _init_offline_db(self):
        """Inicializa base de datos temporal offline"""
        self.offline_conn = sqlite3.connect(self.offline_db, check_same_thread=False)
        self.offline_conn.row_factory = sqlite3.Row
        
        # ✅ Tabla TEMPORAL para readings pendientes de subir
        self.offline_conn.execute('''
            CREATE TABLE IF NOT EXISTS pending_readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sensor_name TEXT NOT NULL,
                identifier TEXT NOT NULL,
                value REAL NOT NULL,
                timestamp TEXT NOT NULL,
                device_id INTEGER,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Índice por timestamp para procesamiento FIFO
        self.offline_conn.execute('''
            CREATE INDEX IF NOT EXISTS idx_offline_timestamp 
            ON pending_readings(timestamp)
        ''')
        
        self.offline_conn.commit()

    def save_reading_permanent(self, document: Dict[str, Any]):
        """
        Guarda reading en copia PERMANENTE (sensor_data/)
        Se ejecuta SIEMPRE, independiente de si MongoDB funciona
        
        Args:
            document: Reading completo
        """
        try:
            self.sensor_data_conn.execute('''
                INSERT INTO readings 
                (sensor_name, identifier, value, timestamp, device_id)
                VALUES (?, ?, ?, ?, ?)
            ''', (
                document['sensor_name'],
                document['identifier'], 
                document['value'],
                document['timestamp'].isoformat() if isinstance(document['timestamp'], datetime) else document['timestamp'],
                document['device_id']
            ))
            self.sensor_data_conn.commit()
            
            self.logger.debug(f"💾 Reading permanente guardado: {document['sensor_name']}({document['identifier']})")
            
        except Exception as e:
            self.logger.error(f"❌ Error guardando reading permanente: {e}")

    def save_offline(self, document: Dict[str, Any]):
        """
        Guarda reading OFFLINE para subir después a MongoDB
        Solo cuando MongoDB NO está disponible
        
        Args:
            document: Reading pendiente de sincronizar
        """
        try:
            self.offline_conn.execute('''
                INSERT INTO pending_readings 
                (sensor_name, identifier, value, timestamp, device_id)
                VALUES (?, ?, ?, ?, ?)
            ''', (
                document['sensor_name'],
                document['identifier'],
                document['value'],
                document['timestamp'].isoformat() if isinstance(document['timestamp'], datetime) else document['timestamp'],
                document['device_id']
            ))
            self.offline_conn.commit()
            
            self.logger.info(f"📱 Reading offline guardado: {document['sensor_name']}({document['identifier']})")
            
        except Exception as e:
            self.logger.error(f"❌ Error guardando reading offline: {e}")

    def sync_offline_data(self, mongo_handler) -> int:
        """
        Sincroniza readings offline con MongoDB
        Los readings se ELIMINAN después de sincronizar exitosamente
        
        Args:
            mongo_handler: Handler de MongoDB
            
        Returns:
            Número de readings sincronizados
        """
        try:
            # Obtener readings pendientes (FIFO)
            cursor = self.offline_conn.execute('''
                SELECT * FROM pending_readings 
                ORDER BY timestamp 
                LIMIT 100
            ''')
            
            pending_readings = cursor.fetchall()
            synced_count = 0
            synced_ids = []
            
            for row in pending_readings:
                # Reconstruir documento para MongoDB
                document = {
                    'sensor_name': row['sensor_name'],
                    'identifier': row['identifier'],
                    'value': row['value'],
                    'timestamp': datetime.fromisoformat(row['timestamp']),
                    'device_id': row['device_id']
                }
                
                # Intentar subir a MongoDB
                if mongo_handler.save_sensor_reading(document):
                    synced_ids.append(row['id'])
                    synced_count += 1
                else:
                    break  # Si falla uno, parar para evitar desorden
            
            # ✅ ELIMINAR readings sincronizados exitosamente
            if synced_ids:
                placeholders = ','.join(['?'] * len(synced_ids))
                self.offline_conn.execute(
                    f'DELETE FROM pending_readings WHERE id IN ({placeholders})',
                    synced_ids
                )
                self.offline_conn.commit()
                
                self.logger.info(f"🔄 Sincronizados y eliminados {synced_count} readings offline")
            
            return synced_count
            
        except Exception as e:
            self.logger.error(f"❌ Error sincronizando readings offline: {e}")
            return 0

    def get_recent_local_readings(self, sensor_name: str = None, identifier: str = None, 
                                limit: int = 10) -> List[Dict[str, Any]]:
        """
        Obtiene readings recientes del storage PERMANENTE
        
        Args:
            sensor_name: Filtrar por sensor (opcional)
            identifier: Filtrar por identifier (opcional)
            limit: Número máximo de resultados
            
        Returns:
            Lista de readings locales permanentes
        """
        try:
            # Construir query
            where_clause = "WHERE 1=1"
            params = []
            
            if sensor_name:
                where_clause += " AND sensor_name = ?"
                params.append(sensor_name)
                
            if identifier:
                where_clause += " AND identifier = ?"
                params.append(identifier)
            
            query = f'''
                SELECT * FROM readings 
                {where_clause}
                ORDER BY timestamp DESC 
                LIMIT ?
            '''
            params.append(limit)
            
            cursor = self.sensor_data_conn.execute(query, params)
            
            readings = []
            for row in cursor.fetchall():
                reading = dict(row)
                # Convertir timestamp string a datetime para consistencia
                reading['timestamp'] = datetime.fromisoformat(reading['timestamp'])
                readings.append(reading)
            
            return readings
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo readings locales: {e}")
            return []

    def get_offline_count(self) -> int:
        """
        Obtiene número de readings pendientes de sincronizar
        
        Returns:
            Número de readings en cola offline
        """
        try:
            cursor = self.offline_conn.execute('SELECT COUNT(*) FROM pending_readings')
            count = cursor.fetchone()[0]
            return count
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo conteo offline: {e}")
            return 0

    def cleanup_old_permanent_data(self, days_old: int = 90):
        """
        Limpia readings permanentes muy antiguos para ahorrar espacio
        (Solo los MUY antiguos - 90 días por defecto)
        
        Args:
            days_old: Días de antigüedad para limpiar
        """
        try:
            self.sensor_data_conn.execute('''
                DELETE FROM readings 
                WHERE created_at < datetime('now', '-{} days')
            '''.format(days_old))
            
            deleted = self.sensor_data_conn.total_changes
            self.sensor_data_conn.commit()
            
            if deleted > 0:
                self.logger.info(f"🧹 Limpiados {deleted} readings permanentes antiguos (>{days_old} días)")
                
        except Exception as e:
            self.logger.error(f"❌ Error limpiando readings permanentes: {e}")

    def get_storage_stats(self) -> Dict[str, Any]:
        """
        Obtiene estadísticas del storage completo
        
        Returns:
            Diccionario con estadísticas de ambas bases
        """
        try:
            stats = {}
            
            # Estadísticas de sensor_data (permanente)
            cursor = self.sensor_data_conn.execute('SELECT COUNT(*) FROM readings')
            stats['permanent_readings'] = cursor.fetchone()[0]
            
            # Estadísticas de offline (temporal)
            cursor = self.offline_conn.execute('SELECT COUNT(*) FROM pending_readings')
            stats['offline_readings'] = cursor.fetchone()[0]
            
            # Tamaños de archivos
            if os.path.exists(self.sensor_data_db):
                stats['sensor_data_file_mb'] = round(os.path.getsize(self.sensor_data_db) / (1024*1024), 2)
            else:
                stats['sensor_data_file_mb'] = 0
                
            if os.path.exists(self.offline_db):
                stats['offline_file_mb'] = round(os.path.getsize(self.offline_db) / (1024*1024), 2)
            else:
                stats['offline_file_mb'] = 0
            
            stats['total_file_mb'] = stats['sensor_data_file_mb'] + stats['offline_file_mb']
                
            return stats
            
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo estadísticas: {e}")
            return {}

    def close_connections(self):
        """Cerrar conexiones de base de datos"""
        if self.sensor_data_conn:
            self.sensor_data_conn.close()
        if self.offline_conn:
            self.offline_conn.close()
        self.logger.info("👋 Conexiones locales cerradas")