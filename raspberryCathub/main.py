from sensors.nivel_comida_sensor import SensorNivelComida
from sensors.nivel_agua_sensor import SensorNivelAgua
from sensors.presencia_sensor import SensorPresencia
from sensors.peso_sensor import SensorPeso
from sensors.humedad_arenero_sensor import SensorHumedadArenero
from sensors.temperatura_sensor import SensorTemperatura
from sensors.camara_sensor import SensorCamara

from activadores.dispensador_comida import DispensadorComida
from activadores.dispensador_agua import DispensadorAgua
from activadores.limpiador_arenero import LimpiadorArenero

from services.hub_sensors import HubSensors
from services.hub_actuadores import HubActuadores
from services.comedero_service import ComederoService
from services.bebedero_service import BebederoService
from services.arenero_service import AreneroService
from services.gato_monitoring import GatoMonitoring

from repository.consultator import Consultation
from repository.mongo import Mongo
from scripts.serial_parser import procesar_mensajes

import time
import threading

# Sensores del comedero
sensor_nivel_comida = SensorNivelComida("NC01", 0, 100)
sensor_peso_plato = SensorPeso("PP01", 0, 1000)

# Sensores del bebedero
sensor_nivel_agua = SensorNivelAgua("NA01", 0, 100)
sensor_calidad_agua = SensorTemperatura("CA01", 15, 35)

# Sensores del arenero
sensor_humedad_arenero = SensorHumedadArenero("HA01", 0, 100)
sensor_presencia_arenero = SensorPresencia("PA01", 0, 1)

# Sensores generales
sensor_temperatura_ambiente = SensorTemperatura("TA01", 15, 35)
sensor_presencia_general = SensorPresencia("PG01", 0, 1)
camara_gato = SensorCamara("CAM01")

# Actuadores
dispensador_comida = DispensadorComida("DC01")
dispensador_agua = DispensadorAgua("DA01") 
limpiador_arenero = LimpiadorArenero("LA01")

# Listas de componentes
sensores = [
    sensor_nivel_comida,
    sensor_peso_plato,
    sensor_nivel_agua,
    sensor_calidad_agua,
    sensor_humedad_arenero,
    sensor_presencia_arenero,
    sensor_temperatura_ambiente,
    sensor_presencia_general,
    camara_gato
]

actuadores = [
    dispensador_comida,
    dispensador_agua,
    limpiador_arenero
]

# Hubs de control
hub_sensores = HubSensors(sensores)
hub_actuadores = HubActuadores(actuadores)

# Servicios especializados
comedero_service = ComederoService(
    dispensador=dispensador_comida,
    sensor_nivel=sensor_nivel_comida,
    sensor_peso=sensor_peso_plato
)

bebedero_service = BebederoService(
    dispensador=dispensador_agua,
    sensor_nivel=sensor_nivel_agua,
    sensor_calidad=sensor_calidad_agua
)

arenero_service = AreneroService(
    limpiador=limpiador_arenero,
    sensor_humedad=sensor_humedad_arenero,
    sensor_presencia=sensor_presencia_arenero
)

gato_monitoring = GatoMonitoring(
    camara=camara_gato,
    sensor_presencia=sensor_presencia_general,
    comedero=comedero_service,
    bebedero=bebedero_service,
    arenero=arenero_service
)

def main():
    print("🐱 Iniciando Sistema de Entorno Inteligente para Gatos...")
    
    # Inicializar base de datos
    if Mongo.check_connection():
        print("✅ Conexión a MongoDB establecida")
    else:
        print("⚠️  Modo offline - datos guardados localmente")
    
    # Iniciar monitoreo de sensores
    hub_sensores.iniciar_monitoreo()
    
    # Iniciar servicios
    comedero_service.iniciar()
    bebedero_service.iniciar()
    arenero_service.iniciar()
    gato_monitoring.iniciar()
    
    try:
        while True:
            time.sleep(10)
            print("🔄 Sistema funcionando correctamente...")
    except KeyboardInterrupt:
        print("\n🛑 Deteniendo sistema...")
        hub_sensores.detener()
        comedero_service.detener()
        bebedero_service.detener()
        arenero_service.detener()
        gato_monitoring.detener()

if __name__ == "__main__":
    main()
