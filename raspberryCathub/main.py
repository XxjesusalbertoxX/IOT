import logging
import time
import signal
import sys
from core.DeviceManager import DeviceManager

# Configurar logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

# Variable global para el device manager
device_manager = None

def signal_handler(sig, frame):
    """Manejador para cerrar limpiamente"""
    print('\n🛑 Cerrando aplicación...')
    if device_manager:
        device_manager.stop()
    sys.exit(0)

def main():
    global device_manager
    
    # Configurar manejador de señales
    signal.signal(signal.SIGINT, signal_handler)
    
    # Código del dispositivo (en producción podrías leerlo de un archivo o variable de entorno)
    DEVICE_CODE = "I9J0K1L2"  # Ejemplo: arenero
    
    # Crear y iniciar device manager
    device_manager = DeviceManager(DEVICE_CODE, "/dev/ttyACM0")
    
    if device_manager.start():
        print(f"✅ Dispositivo {DEVICE_CODE} iniciado correctamente")
        print("⏳ Esperando identifier o eventos...")
        
        # Mantener la aplicación corriendo
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print('\n� Cerrando aplicación...')
            device_manager.stop()
    else:
        print(f"❌ No se pudo iniciar dispositivo {DEVICE_CODE}")

if __name__ == "__main__":
    main()