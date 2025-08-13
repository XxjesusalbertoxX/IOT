from pymongo import MongoClient

def get_database():
    # Conexión a MongoDB para el proyecto CatHub
    CONNECTION_STRING = "mongodb://admin:admin123@cathub.local:27017/cathub_db?authSource=admin"
    
    # Crear conexión con MongoClient
    client = MongoClient(CONNECTION_STRING)
    
    # Retornar la base de datos para CatHub
    return client['cathub_db']


class Mongo:

    @staticmethod
    def get_collection(collection_name: str):
        dbname = get_database()
        return dbname[collection_name]
    
    @staticmethod
    def check_connection():
        try:
            client = MongoClient("mongodb://admin:admin123@cathub.local:27017/cathub_db?authSource=admin", serverSelectionTimeoutMS=5000)
            client.admin.command('ping')
        except Exception as e:
            print("Error al conectar a MongoDB")
            return False
        return True

    @staticmethod
    def insert_sensor_data(sensor_id: str, data: dict):
        """Insertar datos de sensores en MongoDB"""
        try:
            collection = Mongo.get_collection("sensor_data")
            data_with_id = {
                "sensor_id": sensor_id,
                "timestamp": data.get("timestamp"),
                "value": data.get("value"),
                "estado": data.get("estado"),
                "metadata": data.get("metadata", {})
            }
            result = collection.insert_one(data_with_id)
            return result.inserted_id
        except Exception as e:
            print(f"Error insertando datos del sensor {sensor_id}: {e}")
            return None

    @staticmethod
    def get_latest_sensor_data(sensor_id: str):
        """Obtener los datos más recientes de un sensor"""
        try:
            collection = Mongo.get_collection("sensor_data")
            result = collection.find_one(
                {"sensor_id": sensor_id},
                sort=[("timestamp", -1)]
            )
            return result
        except Exception as e:
            print(f"Error obteniendo datos del sensor {sensor_id}: {e}")
            return None

    @staticmethod
    def insert_feeding_record(gato_id: str, cantidad: float, timestamp):
        """Registrar evento de alimentación"""
        try:
            collection = Mongo.get_collection("feeding_records")
            record = {
                "gato_id": gato_id,
                "cantidad_gramos": cantidad,
                "timestamp": timestamp,
                "tipo": "alimentacion"
            }
            result = collection.insert_one(record)
            return result.inserted_id
        except Exception as e:
            print(f"Error registrando alimentación: {e}")
            return None

    @staticmethod
    def insert_water_record(gato_id: str, cantidad: float, timestamp):
        """Registrar evento de hidratación"""
        try:
            collection = Mongo.get_collection("water_records")
            record = {
                "gato_id": gato_id,
                "cantidad_ml": cantidad,
                "timestamp": timestamp,
                "tipo": "hidratacion"
            }
            result = collection.insert_one(record)
            return result.inserted_id
        except Exception as e:
            print(f"Error registrando hidratación: {e}")
            return None

    @staticmethod
    def insert_litter_record(timestamp, estado_limpieza: str):
        """Registrar evento de limpieza del arenero"""
        try:
            collection = Mongo.get_collection("litter_records")
            record = {
                "timestamp": timestamp,
                "estado_limpieza": estado_limpieza,
                "tipo": "limpieza_arenero"
            }
            result = collection.insert_one(record)
            return result.inserted_id
        except Exception as e:
            print(f"Error registrando limpieza del arenero: {e}")
            return None
