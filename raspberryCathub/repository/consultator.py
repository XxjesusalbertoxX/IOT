import random
import serial 
import time
import threading

PORT = '/dev/ttyUSB0'  # Puerto para Raspberry Pi
serial_lock = threading.Lock()

class SerialSingleton:
    _instance = None

    @staticmethod
    def get_instance(puerto=PORT, baudrate=9600, timeout=2):
        if SerialSingleton._instance is None:
            try:
                SerialSingleton._instance = serial.Serial(puerto, baudrate, timeout=timeout)
                time.sleep(2)
            except serial.SerialException as e:
                print(f"Error conectando al puerto serial {puerto}: {e}")
                SerialSingleton._instance = None
        return SerialSingleton._instance

class Consultation:

    @staticmethod
    def consultar_todos_los_datos(puerto=PORT, baudrate=9600, timeout=2):
        """Consultar todos los datos de sensores conectados por serial"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return None
                
                # Enviar comando para solicitar todos los datos
                ser.write(b'GET_ALL_DATA\n')
                time.sleep(0.5)
                
                response = ser.readline().decode('utf-8').strip()
                return response
                
        except Exception as e:
            print(f"Error consultando datos seriales: {e}")
            return None
    
    @staticmethod
    def consultar_sensor_especifico(sensor_id: str, puerto=PORT, baudrate=9600, timeout=2):
        """Consultar datos de un sensor específico"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return None
                
                # Enviar comando para sensor específico
                command = f'GET_SENSOR:{sensor_id}\n'
                ser.write(command.encode('utf-8'))
                time.sleep(0.5)
                
                response = ser.readline().decode('utf-8').strip()
                return response
                
        except Exception as e:
            print(f"Error consultando sensor {sensor_id}: {e}")
            return None
    
    @staticmethod
    def activar_actuador(actuador_id: str, accion: str, puerto=PORT, baudrate=9600, timeout=2):
        """Enviar comando para activar un actuador"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return False
                
                # Enviar comando para activar actuador
                command = f'SET_ACTUATOR:{actuador_id}:{accion}\n'
                ser.write(command.encode('utf-8'))
                time.sleep(0.5)
                
                response = ser.readline().decode('utf-8').strip()
                return response == 'OK'
                
        except Exception as e:
            print(f"Error activando actuador {actuador_id}: {e}")
            return False
    
    @staticmethod
    def dispensar_comida(cantidad_gramos: float, puerto=PORT, baudrate=9600, timeout=2):
        """Comando específico para dispensar comida"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return False
                
                command = f'DISPENSE_FOOD:{cantidad_gramos}\n'
                ser.write(command.encode('utf-8'))
                time.sleep(1)  # Tiempo extra para dispensar
                
                response = ser.readline().decode('utf-8').strip()
                return response == 'DISPENSED'
                
        except Exception as e:
            print(f"Error dispensando comida: {e}")
            return False
    
    @staticmethod
    def dispensar_agua(cantidad_ml: float, puerto=PORT, baudrate=9600, timeout=2):
        """Comando específico para dispensar agua"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return False
                
                command = f'DISPENSE_WATER:{cantidad_ml}\n'
                ser.write(command.encode('utf-8'))
                time.sleep(1)
                
                response = ser.readline().decode('utf-8').strip()
                return response == 'DISPENSED'
                
        except Exception as e:
            print(f"Error dispensando agua: {e}")
            return False
    
    @staticmethod
    def limpiar_arenero(tiempo_segundos: int = 30, puerto=PORT, baudrate=9600, timeout=2):
        """Comando específico para limpiar arenero"""
        try:
            with serial_lock:
                ser = SerialSingleton.get_instance(puerto, baudrate, timeout)
                if not ser:
                    return False
                
                command = f'CLEAN_LITTER:{tiempo_segundos}\n'
                ser.write(command.encode('utf-8'))
                time.sleep(tiempo_segundos + 1)  # Esperar a que termine la limpieza
                
                response = ser.readline().decode('utf-8').strip()
                return response == 'CLEANED'
                
        except Exception as e:
            print(f"Error limpiando arenero: {e}")
            return False
