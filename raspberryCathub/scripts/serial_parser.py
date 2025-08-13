import json
import re
from datetime import datetime

def procesar_mensajes(mensaje_serial):
    """
    Procesar mensajes recibidos por puerto serial desde Arduino/ESP32
    Formato esperado: SENSOR_ID:VALOR:TIMESTAMP o COMMAND:RESPONSE
    """
    try:
        if not mensaje_serial or mensaje_serial.strip() == "":
            return None
        
        mensaje = mensaje_serial.strip()
        
        # Patrones de mensajes
        patron_sensor = r'^([A-Z0-9]+):([0-9.-]+):?([0-9]*)'
        patron_comando = r'^([A-Z_]+):([A-Z_0-9]+)'
        patron_estado = r'^STATUS:([A-Z0-9]+):([A-Z_]+)'
        
        # Procesar mensaje de sensor
        match_sensor = re.match(patron_sensor, mensaje)
        if match_sensor:
            sensor_id = match_sensor.group(1)
            valor = float(match_sensor.group(2))
            timestamp = match_sensor.group(3) if match_sensor.group(3) else None
            
            return {
                'tipo': 'sensor_data',
                'sensor_id': sensor_id,
                'valor': valor,
                'timestamp': timestamp or datetime.now().isoformat(),
                'raw_message': mensaje
            }
        
        # Procesar respuesta de comando
        match_comando = re.match(patron_comando, mensaje)
        if match_comando:
            comando = match_comando.group(1)
            respuesta = match_comando.group(2)
            
            return {
                'tipo': 'command_response',
                'comando': comando,
                'respuesta': respuesta,
                'timestamp': datetime.now().isoformat(),
                'raw_message': mensaje
            }
        
        # Procesar estado de actuador
        match_estado = re.match(patron_estado, mensaje)
        if match_estado:
            actuador_id = match_estado.group(1)
            estado = match_estado.group(2)
            
            return {
                'tipo': 'actuator_status',
                'actuador_id': actuador_id,
                'estado': estado,
                'timestamp': datetime.now().isoformat(),
                'raw_message': mensaje
            }
        
        # Mensaje no reconocido
        return {
            'tipo': 'unknown',
            'mensaje': mensaje,
            'timestamp': datetime.now().isoformat(),
            'raw_message': mensaje
        }
        
    except Exception as e:
        return {
            'tipo': 'error',
            'error': str(e),
            'mensaje_original': mensaje_serial,
            'timestamp': datetime.now().isoformat()
        }

def parsear_datos_sensor_comida(mensaje):
    """
    Parsear datos específicos del sensor de nivel de comida
    Formato: NC01:75.5:DISTANCE:12.3
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('NC'):
            return {
                'sensor_id': partes[0],
                'nivel_porcentaje': float(partes[1]),
                'tipo_medicion': partes[2],
                'distancia_cm': float(partes[3]) if partes[3] else None,
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def parsear_datos_sensor_agua(mensaje):
    """
    Parsear datos específicos del sensor de nivel de agua
    Formato: NA01:60.2:ULTRASONIC:8.5
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('NA'):
            return {
                'sensor_id': partes[0],
                'nivel_porcentaje': float(partes[1]),
                'tipo_sensor': partes[2],
                'distancia_cm': float(partes[3]) if partes[3] else None,
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def parsear_datos_sensor_peso(mensaje):
    """
    Parsear datos del sensor de peso
    Formato: PP01:245.7:GRAMS:STABLE
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('PP'):
            return {
                'sensor_id': partes[0],
                'peso_gramos': float(partes[1]),
                'unidad': partes[2],
                'estado': partes[3],  # STABLE, CHANGING, etc.
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def parsear_datos_sensor_presencia(mensaje):
    """
    Parsear datos del sensor de presencia
    Formato: PG01:1:PIR:MOTION
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('P'):
            return {
                'sensor_id': partes[0],
                'presencia': int(partes[1]),  # 0 o 1
                'tipo_sensor': partes[2],  # PIR, ULTRASONIC, etc.
                'estado': partes[3],  # MOTION, STILL, etc.
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def parsear_datos_sensor_humedad_arenero(mensaje):
    """
    Parsear datos del sensor de humedad del arenero
    Formato: HA01:65.3:HUMIDITY:DHT22
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('HA'):
            return {
                'sensor_id': partes[0],
                'humedad_porcentaje': float(partes[1]),
                'tipo_medicion': partes[2],
                'sensor_tipo': partes[3],
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def parsear_datos_sensor_temperatura(mensaje):
    """
    Parsear datos del sensor de temperatura
    Formato: TA01:23.5:CELSIUS:DHT22
    """
    try:
        partes = mensaje.split(':')
        if len(partes) >= 4 and partes[0].startswith('TA'):
            return {
                'sensor_id': partes[0],
                'temperatura_celsius': float(partes[1]),
                'unidad': partes[2],
                'sensor_tipo': partes[3],
                'timestamp': datetime.now().isoformat()
            }
    except:
        pass
    return None

def generar_comando_dispensar_comida(cantidad_gramos):
    """
    Generar comando para dispensar comida
    Formato: DISPENSE_FOOD:cantidad_gramos
    """
    if 1 <= cantidad_gramos <= 200:
        return f"DISPENSE_FOOD:{cantidad_gramos}"
    return None

def generar_comando_dispensar_agua(cantidad_ml):
    """
    Generar comando para dispensar agua
    Formato: DISPENSE_WATER:cantidad_ml
    """
    if 10 <= cantidad_ml <= 500:
        return f"DISPENSE_WATER:{cantidad_ml}"
    return None

def generar_comando_limpiar_arenero(duracion_segundos=30):
    """
    Generar comando para limpiar arenero
    Formato: CLEAN_LITTER:duracion_segundos
    """
    if 10 <= duracion_segundos <= 300:
        return f"CLEAN_LITTER:{duracion_segundos}"
    return None

def validar_respuesta_comando(comando_enviado, respuesta_recibida):
    """
    Validar que la respuesta corresponda al comando enviado
    """
    try:
        respuestas_esperadas = {
            'DISPENSE_FOOD': ['DISPENSING', 'DISPENSED', 'ERROR'],
            'DISPENSE_WATER': ['DISPENSING', 'DISPENSED', 'ERROR'],
            'CLEAN_LITTER': ['CLEANING', 'CLEANED', 'ERROR'],
            'GET_ALL_DATA': ['DATA_START', 'ERROR'],
            'TEST': ['TESTING', 'TEST_OK', 'TEST_FAIL']
        }
        
        comando_base = comando_enviado.split(':')[0]
        
        if comando_base in respuestas_esperadas:
            return respuesta_recibida in respuestas_esperadas[comando_base]
        
        return True  # Para comandos no definidos, asumir válido
        
    except:
        return False

def procesar_multiples_mensajes(mensajes_seriales):
    """
    Procesar múltiples mensajes seriales de una vez
    """
    resultados = []
    
    for mensaje in mensajes_seriales:
        if mensaje.strip():
            resultado = procesar_mensajes(mensaje)
            if resultado:
                resultados.append(resultado)
    
    return resultados

def generar_log_comunicacion(comando, respuesta, exito=True):
    """
    Generar log de comunicación para debugging
    """
    return {
        'timestamp': datetime.now().isoformat(),
        'comando_enviado': comando,
        'respuesta_recibida': respuesta,
        'comunicacion_exitosa': exito,
        'tipo_log': 'comunicacion_serial'
    }

# Funciones específicas para cada sensor (llamadas por los objetos sensor)
PARSERS_SENSORES = {
    'NC': parsear_datos_sensor_comida,      # Nivel Comida
    'NA': parsear_datos_sensor_agua,        # Nivel Agua  
    'PP': parsear_datos_sensor_peso,        # Peso Plato
    'PG': parsear_datos_sensor_presencia,   # Presencia General
    'PA': parsear_datos_sensor_presencia,   # Presencia Arenero
    'HA': parsear_datos_sensor_humedad_arenero,  # Humedad Arenero
    'TA': parsear_datos_sensor_temperatura, # Temperatura Ambiente
    'CA': parsear_datos_sensor_temperatura  # Calidad Agua (temperatura)
}

def obtener_parser_por_sensor_id(sensor_id):
    """
    Obtener el parser apropiado basado en el ID del sensor
    """
    prefijo = sensor_id[:2]
    return PARSERS_SENSORES.get(prefijo, None)
