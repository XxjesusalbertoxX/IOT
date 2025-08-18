import socketio
import logging
from database.postgres_handler import PostgresHandler

# Configura el logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Instancia de la base de datos
db = PostgresHandler()

# Cliente Socket.IO
sio = socketio.Client()

# Evento: cambio de estatus de dispositivo
@sio.on('device_status_changed')
def on_device_status_changed(data):
    identifier = data.get('identifier')
    logger.info(f"🔔 Cambio de estatus detectado para {identifier}")

    # Buscar status y el intervalo del motor en la base de datos
    status = db.get_status_device_environment(identifier)
    interval = db.get_interval_motor(identifier)
    logger.info(f"Status actual: {status}, Intervalo motor: {interval}")

    # Aquí podrías mandar el status al Arduino (lógica a implementar después)

# Evento: limpiar completamente desde el frontend
@sio.on('clean_litterbox')
def on_clean_litterbox(data):
    identifier = data.get('identifier')
    logger.info(f"🧹 Solicitud de limpieza completa para {identifier}")

    # Ejecutar la acción de limpieza (lógica a implementar)
    # Actualizar el status a 'empty' en la base de datos si es necesario
    # db.set_status_device_environment(identifier, 'empty') # Si tienes este método

    # Aquí podrías mandar el comando al Arduino (lógica a implementar después)

def main():
    # Conéctate al servidor de sockets (ajusta la URL)
    sio.connect('http://localhost:3000')  # Cambia la URL y puerto según tu backend
    logger.info("Conectado al servidor de sockets.")

    # Suscribirse a otros canales/topics si es necesario
    # Por ejemplo, sio.emit('join', {'room': 'litterbox_updates'})

    sio.wait()  # Mantiene el script corriendo

if __name__ == "__main__":
    main()