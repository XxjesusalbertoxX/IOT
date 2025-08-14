"""
Ejemplo de uso del controlador del arenero
"""

from communication.arduino_serial import ArduinoSerial
from communication.actuadores.stepper_motor_controller import LitterboxController, CleaningInterval
import time

def main():
    # Inicializar comunicación
    arduino = ArduinoSerial(port='/dev/ttyACM*', baudrate=115200)
    
    if not arduino.connect():
        print("❌ No se pudo conectar al Arduino")
        return
    
    # Crear controlador
    litter_controller = LitterboxController(arduino)
    
    # Iniciar monitoreo
    litter_controller.start_monitoring()
    
    print("🏠 Controlador del arenero iniciado")
    print(f"📊 Estado inicial: {litter_controller.get_status()}")
    
    try:
        while True:
            print("\n" + "="*50)
            print("🐱 CONTROL DEL ARENERO")
            print("="*50)
            print("1. Llenar con arena (EMPTY -> READY)")
            print("2. Limpieza normal (270° derecha)")
            print("3. Limpieza completa (80° izquierda + READY -> EMPTY)")
            print("4. Ver estado")
            print("5. Configurar intervalo (2/5/8 horas)")
            print("6. Parada de emergencia")
            print("0. Salir")
            
            opcion = input("\nSelecciona opción: ").strip()
            
            if opcion == "1":
                print("\n🪣 Llenando arenero...")
                success, msg = litter_controller.fill_with_litter()
                print(f"{'✅' if success else '❌'} {msg}")
                
            elif opcion == "2":
                print("\n🧹 Ejecutando limpieza normal...")
                success, msg = litter_controller.execute_normal_cleaning()
                print(f"{'✅' if success else '❌'} {msg}")
                
            elif opcion == "3":
                print("\n🧽 Ejecutando limpieza completa...")
                success, msg = litter_controller.execute_complete_cleaning()
                print(f"{'✅' if success else '❌'} {msg}")
                
            elif opcion == "4":
                status = litter_controller.get_status()
                print(f"\n📊 ESTADO ACTUAL:")
                print(f"   Estado: {status['current_state']}")
                print(f"   Bloqueado: {status['motor_blocked']}")
                print(f"   Intervalo: cada {status['cleaning_interval_hours']}h")
                print(f"   Monitoreo: {status['monitoring_active']}")
                print(f"   Llenados: {status['stats']['fill_operations']}")
                print(f"   Limpiezas normales: {status['stats']['normal_cleanings']}")
                print(f"   Limpiezas completas: {status['stats']['complete_cleanings']}")
                
            elif opcion == "5":
                print("\nIntervalos disponibles:")
                print("2 = Cada 2 horas")
                print("5 = Cada 5 horas")  
                print("8 = Cada 8 horas")
                
                try:
                    hours = int(input("Selecciona intervalo: "))
                    if litter_controller.set_cleaning_interval(hours):
                        print(f"✅ Intervalo configurado: cada {hours} horas")
                    else:
                        print("❌ Intervalo inválido")
                except ValueError:
                    print("❌ Número inválido")
                    
            elif opcion == "6":
                print("\n🚨 EJECUTANDO PARADA DE EMERGENCIA...")
                if litter_controller.emergency_stop():
                    print("✅ Parada de emergencia ejecutada")
                else:
                    print("❌ Error en parada de emergencia")
                    
            elif opcion == "0":
                break
                
            else:
                print("❌ Opción inválida")
            
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n👋 Cerrando aplicación...")
    
    finally:
        litter_controller.stop_monitoring()
        arduino.disconnect()
        print("🛑 Sistema cerrado correctamente")

if __name__ == "__main__":
    main()