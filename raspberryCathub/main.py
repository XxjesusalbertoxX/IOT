"""
Sistema CatHub - Loop Principal
NUNCA debe crashear - maneja todas las excepciones
"""

import time
import logging
import sys
from core.sensor_manager import SensorManager

# Configurar logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('cathub.log'),
        logging.StreamHandler(sys.stdout)
    ]
)

def main():
    """Loop principal - NUNCA debe fallar"""
    logger = logging.getLogger(__name__)
    logger.info("🐱 INICIANDO SISTEMA CATHUB...")
    
    # Crear manager principal
    sensor_manager = None
    
    while True:  # ♾️ LOOP INFINITO
        try:
            # Inicializar manager si no existe
            if sensor_manager is None:
                logger.info("🔄 Inicializando SensorManager...")
                sensor_manager = SensorManager()
                sensor_manager.initialize()
                logger.info("✅ SensorManager inicializado")
            
            # ⚡ CICLO PRINCIPAL
            sensor_manager.run_cycle()
            
            # Pausa corta para no saturar CPU
            time.sleep(0.1)
            
        except KeyboardInterrupt:
            logger.info("� Deteniendo sistema por usuario...")
            break
            
        except Exception as e:
            logger.error(f"❌ ERROR EN LOOP PRINCIPAL: {e}")
            
            # Si el error es crítico, reiniciar manager
            if "critical" in str(e).lower() or sensor_manager is None:
                logger.warning("🔧 Reiniciando SensorManager...")
                sensor_manager = None
                time.sleep(5)  # Pausa antes de reiniciar
            else:
                time.sleep(1)  # Pausa corta para errores menores
    
    logger.info("👋 Sistema CatHub detenido")

if __name__ == "__main__":
    main()