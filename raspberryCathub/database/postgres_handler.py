"""
PostgreSQL Handler - SOLO CONSUMIR la base de datos existente
"""

import psycopg2
from psycopg2.extras import RealDictCursor
import logging
from typing import Dict, Any, Optional
import json

class PostgresHandler:
    """
    Handler para PostgreSQL - Solo consume datos existentes
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.connection_params = {
            'host': 'localhost',
            'database': 'cathub_db', 
            'user': 'cathub_user',
            'password': 'cathub_password',
            'port': 5432
        }

    def get_connection(self):
        """Obtener conexión a PostgreSQL"""
        try:
            conn = psycopg2.connect(**self.connection_params)
            return conn
        except psycopg2.Error as e:
            self.logger.error(f"❌ Error conectando a PostgreSQL: {e}")
            return None

    def check_connection(self) -> bool:
        """Verificar conexión a PostgreSQL"""
        try:
            conn = self.get_connection()
            if conn:
                conn.close()
                return True
            return False
        except Exception as e:
            self.logger.error(f"❌ Error verificando conexión PostgreSQL: {e}")
            return False

    def get_device_id(self, identifier: str) -> Optional[int]:
        """
        Obtiene device_id para un sensor por su identifier
        
        Args:
            identifier: Identificador del sensor (ej: WSR001, PRS001)
            
        Returns:
            device_id o None si no se encuentra
        """
        try:
            conn = self.get_connection()
            if not conn:
                return None
                
            cursor = conn.cursor()
            
            # ✅ SOLO CONSULTAR - NO CREAR
            query = """
                SELECT device_id FROM sensors 
                WHERE identifier = %s 
                AND active = true
                LIMIT 1
            """
            
            cursor.execute(query, (identifier,))
            result = cursor.fetchone()
            
            cursor.close()
            conn.close()
            
            if result:
                device_id = result[0]
                self.logger.debug(f"✅ Device ID para {identifier}: {device_id}")
                return device_id
            else:
                self.logger.warning(f"⚠️ No se encontró device_id para {identifier}")
                return None
                
        except psycopg2.Error as e:
            self.logger.error(f"❌ Error obteniendo device_id para {identifier}: {e}")
            return None

    def get_mongo_config(self) -> Dict[str, Any]:
        """
        Obtiene configuración de MongoDB desde PostgreSQL
        
        Returns:
            Diccionario con configuración de MongoDB
        """
        try:
            conn = self.get_connection()
            if not conn:
                return self._get_default_mongo_config()
                
            cursor = conn.cursor(cursor_factory=RealDictCursor)
            
            # ✅ SOLO CONSULTAR configuración existente
            query = """
                SELECT config_value FROM configurations 
                WHERE config_key = 'mongodb_connection'
                AND active = true
                LIMIT 1
            """
            
            cursor.execute(query)
            result = cursor.fetchone()
            
            cursor.close()
            conn.close()
            
            if result and result['config_value']:
                config = json.loads(result['config_value'])
                self.logger.info("✅ Configuración MongoDB obtenida desde PostgreSQL")
                return config
            else:
                self.logger.warning("⚠️ No hay configuración MongoDB en PostgreSQL, usando defaults")
                return self._get_default_mongo_config()
                
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo configuración MongoDB: {e}")
            return self._get_default_mongo_config()

    def _get_default_mongo_config(self) -> Dict[str, Any]:
        """Configuración por defecto de MongoDB"""
        return {
            'host': 'cathub.local',
            'port': 27017,
            'username': 'admin',
            'password': 'admin123',
            'database': 'cathub_db',
            'auth_source': 'admin'
        }

    def get_all_sensors(self) -> list:
        """
        Obtiene todos los sensores activos
        
        Returns:
            Lista de sensores con sus device_ids
        """
        try:
            conn = self.get_connection()
            if not conn:
                return []
                
            cursor = conn.cursor(cursor_factory=RealDictCursor)
            
            # ✅ SOLO CONSULTAR sensores existentes
            query = """
                SELECT device_id, identifier, sensor_name, description, location
                FROM sensors 
                WHERE active = true
                ORDER BY sensor_name, identifier
            """
            
            cursor.execute(query)
            results = cursor.fetchall()
            
            cursor.close()
            conn.close()
            
            sensors = [dict(row) for row in results]
            self.logger.debug(f"✅ Obtenidos {len(sensors)} sensores activos")
            return sensors
            
        except psycopg2.Error as e:
            self.logger.error(f"❌ Error obteniendo sensores: {e}")
            return []

    def get_sensor_intervals(self) -> Dict[str, int]:
        """
        Obtiene intervalos de sensores desde PostgreSQL (si están configurados)
        
        Returns:
            Diccionario con intervalos por sensor
        """
        try:
            conn = self.get_connection()
            if not conn:
                return {}
                
            cursor = conn.cursor(cursor_factory=RealDictCursor)
            
            # Consultar intervalos si están configurados en BD
            query = """
                SELECT config_value FROM configurations 
                WHERE config_key = 'sensor_intervals'
                AND active = true
                LIMIT 1
            """
            
            cursor.execute(query)
            result = cursor.fetchone()
            
            cursor.close()
            conn.close()
            
            if result and result['config_value']:
                intervals = json.loads(result['config_value'])
                self.logger.info("✅ Intervalos de sensores obtenidos desde PostgreSQL")
                return intervals
            else:
                self.logger.info("⚠️ No hay intervalos configurados, usando defaults")
                return {}
                
        except Exception as e:
            self.logger.error(f"❌ Error obteniendo intervalos: {e}")
            return {}