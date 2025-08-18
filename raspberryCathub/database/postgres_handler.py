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
            'host': 'atenasoficial.com',
            'database': 'db_cathub', 
            'user': 'admin',
            'password': 'admin123',
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

    def get_device_identifier(self, code: str) -> Optional[str]:
        """
        Obtiene el identifier de un dispositivo a partir del código que se ve en la carcasa.

        Args:
            code: Código del dispositivo visible al usuario (ej: ARENERO-001)

        Returns:
            identifier o None si no se encuentra
        """
        try:
            conn = self.get_connection()
            if not conn:
                return None

            cursor = conn.cursor()

            query = """
                SELECT identifier
                FROM device_codes c
                WHERE c.code = %s
                LIMIT 1
            """

            cursor.execute(query, (code,))
            result = cursor.fetchone()

            cursor.close()
            conn.close()

            if result:
                identifier = result[0]
                self.logger.debug(f"✅ Identifier para code {code}: {identifier}")
                return identifier
            else:
                self.logger.warning(f"⚠️ No se encontró identifier para code {code}")
                return None

        except Exception as e:
            self.logger.error(f"❌ Error consultando identifier: {e}")
            return None

    def get_interval_motor(self, identifier: str) -> Optional[int]:
        """
        Obtiene el intervalo de un motor a partir del identificador.

        Args:
            identifier: Identificador del motor (ej: ARENERO-001)

        Returns:
            intervalo o None si no se encuentra
        """

        try:
            conn = self.get_connection()
            if not conn:
                return None

            cursor = conn.cursor()

            query = """
                SELECT ads.interval
                FROM device_environment de
                JOIN actuator_device_environments ade
                  ON de.device_environment_id = ade.device_environment_id
                JOIN actuator_device_settings ads
                  ON ade.actuator_device_environment_id = ads.actuator_device_environment_id
                WHERE de.identifier = %s
                  AND de.active = true
                  AND ade.active = true
                  AND ads.active = true
                LIMIT 1
            """
            cursor.execute(query, (identifier,))
            result = cursor.fetchone()

            cursor.close()
            conn.close()

            if result:
                interval = result[0]
                self.logger.debug(f"✅ Interval para code {identifier}: {interval}")
                return interval
            else:
                self.logger.warning(f"⚠️ No se encontró interval para code {identifier}")
                return None

        except Exception as e:
            self.logger.error(f"❌ Error consultando interval: {e}")
            return None
        
    def get_status_device_environment(self, identifier: str) -> Optional[bool]:
        """
        Obtiene el estado de un dispositivo a partir del identificador.

        Args:
            identifier: Identificador del dispositivo (ej: ARENERO-001)

        Returns:
            Estado del dispositivo (True/False) o None si no se encuentra
        """
        try:
            conn = self.get_connection()
            if not conn:
                return None

            cursor = conn.cursor()

            query = """
                SELECT de.status
                FROM device_environment de
                WHERE de.identifier = %s
                LIMIT 1
            """

            cursor.execute(query, (identifier,))
            result = cursor.fetchone()

            cursor.close()
            conn.close()

            if result is not None:
                status = result[0]
                self.logger.debug(f"✅ Estado para device {identifier}: {status}")
                return status
            else:
                self.logger.warning(f"⚠️ No se encontró estado para device {identifier}")
                return None

        except Exception as e:
            self.logger.error(f"❌ Error consultando estado: {e}")
            return None

    def get_device_environment_id(self, identifier: str) -> Optional[int]:
        """
        Obtiene el ID del entorno del dispositivo a partir del identificador.

        Args:
            identifier: Identificador del dispositivo (ej: ARENERO-001)

        Returns:
            ID del entorno del dispositivo o None si no se encuentra
        """
        try:
            conn = self.get_connection()
            if not conn:
                return None

            cursor = conn.cursor()

            query = """
                SELECT de.id
                FROM device_environment de
                WHERE de.identifier = %s
                LIMIT 1
            """

            cursor.execute(query, (identifier,))
            result = cursor.fetchone()

            cursor.close()
            conn.close()

            if result is not None:
                device_environment_id = result[0]
                self.logger.debug(f"✅ ID del entorno para device {identifier}: {device_environment_id}")
                return device_environment_id
            else:
                self.logger.warning(f"⚠️ No se encontró ID del entorno para device {identifier}")
                return None

        except Exception as e:
            self.logger.error(f"❌ Error consultando ID del entorno: {e}")
            return None

    def get_all_identified_sensors(self, environment_id: str) -> list:
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
            
            query = """
                SELECT sensor_identifier
                FROM environment_sensors
                WHERE environment_id = %s
                ORDER BY sensor_identifier
            """

            cursor.execute(query, (environment_id,))
            results = cursor.fetchall()
            
            cursor.close()
            conn.close()
            
            sensors = [dict(row) for row in results]
            self.logger.debug(f"✅ Obtenidos {len(sensors)} sensores activos")
            return sensors
            
        except psycopg2.Error as e:
            self.logger.error(f"❌ Error obteniendo sensores: {e}")
            return []

    def get_device_sensor_setting(self, sensor_id: str) -> Optional[Dict[str, Any]]:
        """
        Obtiene la configuración de un sensor específico en un entorno dado.

        Args:
            environment_id: ID del entorno
            sensor_id: ID del sensor

        Returns:
            Diccionario con la configuración del sensor o None si no se encuentra
        """
        try:
            conn = self.get_connection()
            if not conn:
                return None

            cursor = conn.cursor(cursor_factory=RealDictCursor)

            query = """
                SELECT dss.value
                FROM environment_sensors es
                JOIN device_sensors_settings dss
                  ON es.id = dss.environment_sensor_id
                WHERE es.id = %s
                LIMIT 1
            """
            cursor.execute(query, (sensor_id,))
            result = cursor.fetchone()

            cursor.close()
            conn.close()

            if result:
                self.logger.debug(f"✅ Configuración para sensor {sensor_id} en entorno {environment_id}: {result}")
                return result
            else:
                self.logger.warning(f"⚠️ No se encontró configuración para sensor {sensor_id} en entorno {environment_id}")
                return None

        except Exception as e:
            self.logger.error(f"❌ Error obteniendo configuración de sensor: {e}")
            return None
