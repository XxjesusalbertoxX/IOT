import psycopg2
from psycopg2.extras import RealDictCursor
import json
from datetime import datetime

class PostgresDB:
    
    def __init__(self):
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
            print(f"Error conectando a PostgreSQL: {e}")
            return None
    
    def check_connection(self):
        """Verificar conexión a PostgreSQL"""
        try:
            conn = self.get_connection()
            if conn:
                conn.close()
                return True
            return False
        except Exception as e:
            print(f"Error verificando conexión: {e}")
            return False
    
    def create_tables(self):
        """Crear tablas necesarias para CatHub"""
        create_queries = [
            """
            CREATE TABLE IF NOT EXISTS gatos (
                id SERIAL PRIMARY KEY,
                nombre VARCHAR(50) NOT NULL,
                edad INTEGER,
                peso DECIMAL(5,2),
                raza VARCHAR(50),
                fecha_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
            """,
            """
            CREATE TABLE IF NOT EXISTS sensor_data (
                id SERIAL PRIMARY KEY,
                sensor_id VARCHAR(20) NOT NULL,
                valor DECIMAL(10,2),
                estado VARCHAR(20),
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                metadata JSONB
            );
            """,
            """
            CREATE TABLE IF NOT EXISTS feeding_events (
                id SERIAL PRIMARY KEY,
                gato_id INTEGER REFERENCES gatos(id),
                cantidad_gramos DECIMAL(6,2),
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                sensor_trigger VARCHAR(20)
            );
            """,
            """
            CREATE TABLE IF NOT EXISTS water_events (
                id SERIAL PRIMARY KEY,
                gato_id INTEGER REFERENCES gatos(id),
                cantidad_ml DECIMAL(6,2),
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                sensor_trigger VARCHAR(20)
            );
            """,
            """
            CREATE TABLE IF NOT EXISTS litter_events (
                id SERIAL PRIMARY KEY,
                tipo_evento VARCHAR(30),
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                duracion_minutos INTEGER,
                sensor_trigger VARCHAR(20)
            );
            """
        ]
        
        try:
            conn = self.get_connection()
            if not conn:
                return False
            
            cursor = conn.cursor()
            for query in create_queries:
                cursor.execute(query)
            
            conn.commit()
            cursor.close()
            conn.close()
            return True
        except psycopg2.Error as e:
            print(f"Error creando tablas: {e}")
            return False
    
    def insert_sensor_data(self, sensor_id: str, valor: float, estado: str, metadata=None):
        """Insertar datos de sensor en PostgreSQL"""
        try:
            conn = self.get_connection()
            if not conn:
                return None
            
            cursor = conn.cursor()
            query = """
                INSERT INTO sensor_data (sensor_id, valor, estado, metadata)
                VALUES (%s, %s, %s, %s)
                RETURNING id;
            """
            cursor.execute(query, (sensor_id, valor, estado, json.dumps(metadata) if metadata else None))
            
            result_id = cursor.fetchone()[0]
            conn.commit()
            cursor.close()
            conn.close()
            return result_id
        except psycopg2.Error as e:
            print(f"Error insertando datos del sensor: {e}")
            return None
    
    def get_latest_sensor_data(self, sensor_id: str):
        """Obtener los datos más recientes de un sensor"""
        try:
            conn = self.get_connection()
            if not conn:
                return None
            
            cursor = conn.cursor(cursor_factory=RealDictCursor)
            query = """
                SELECT * FROM sensor_data 
                WHERE sensor_id = %s 
                ORDER BY timestamp DESC 
                LIMIT 1;
            """
            cursor.execute(query, (sensor_id,))
            result = cursor.fetchone()
            
            cursor.close()
            conn.close()
            return dict(result) if result else None
        except psycopg2.Error as e:
            print(f"Error obteniendo datos del sensor: {e}")
            return None
