from communication.arduino_serial import ArduinoSerial
import time

def main():
    arduino = ArduinoSerial(port='/dev/ttyACM0', baudrate=115200)
    
    if not arduino.connect():
        print("❌ No se pudo conectar al Arduino")
        return

    print("✅ Arduino conectado. Probando sensores...\n")

    try:
        while True:
            command = {"type": "SENSOR", "command": "READ_ALL"}
            arduino._send_command(command)

            print("Lecturas de sensores:")
            for _ in range(10):
                response = arduino._read_response()
                if response:
                    print(response)
                else:
                    break
            print("-" * 40)
            time.sleep(5)  

    except KeyboardInterrupt:
        print("\n👋 Prueba de sensores finalizada.")

    finally:
        arduino.disconnect()
        print("🛑 Desconectado del Arduino.")

if __name__ == "__main__":
    main()