# CatHub IoT Weight Sensor System

Sistema completo de sensor de peso para el proyecto CatHub IoT, incluyendo código Arduino, procesamiento en Raspberry Pi y almacenamiento en MongoDB con soporte para replica set y almacenamiento offline.

## Arquitectura del Sistema

```
┌─────────────────┐    Serial     ┌──────────────────┐    Network    ┌─────────────────┐
│   Arduino Mega │ ── USB/TTL ──→ │  Raspberry Pi    │ ────────────→ │   MongoDB       │
│                 │                │                  │               │   Replica Set   │
│ - WeightSensor  │                │ - Serial Manager │               │                 │
│ - HX711         │                │ - Data Processor │               │ - Primary       │
│ - Load Cell     │                │ - Offline Store  │               │ - Secondary     │
└─────────────────┘                └──────────────────┘               │ - Arbiter       │
                                                                       └─────────────────┘
```

## Componentes del Sistema

### 1. Arduino (Firmware)
- **WeightSensor.h/cpp**: Clase para manejo del sensor de peso con HX711
- **SensorManager**: Coordinador de sensores con polling automático
- **CommandProcessor**: Procesador de comandos desde Raspberry Pi
- **BaseSensor**: Clase base para todos los sensores

### 2. Raspberry Pi (Edge Computing)
- **SerialDataManager**: Comunicación bidireccional con Arduino
- **WeightSensorHandler**: Procesamiento especializado de datos de peso
- **MongoRepository**: Conexión a MongoDB con soporte offline
- **WeightSensorService**: Servicio principal orquestador

### 3. MongoDB (Database)
- **Replica Set**: Configuración para alta disponibilidad
- **TTL Collections**: Gestión automática de datos antiguos
- **Offline Storage**: SQLite local para sincronización posterior

## Instalación

### Requisitos del Sistema

#### Arduino
- Arduino Mega 2560
- Sensor HX711
- Celda de carga (load cell)
- PlatformIO IDE

#### Raspberry Pi
- Raspberry Pi 4 (recomendado)
- Raspbian OS
- Python 3.8+
- Puerto USB disponible para Arduino

#### Base de Datos
- MongoDB 4.4+ con soporte para replica set
- 3 instancias MongoDB (primary, secondary, arbiter)

### 1. Configuración Arduino

```bash
# Clonar el repositorio
cd /path/to/project/IOT/arduinoCathub

# Compilar y subir con PlatformIO
pio run --target upload

# Monitor serial para debug
pio device monitor
```

#### Conexiones Hardware
- **HX711 DOUT** → Pin 2 (Arduino)
- **HX711 SCK** → Pin 3 (Arduino)
- **HX711 VDD** → 5V
- **HX711 VSS** → GND
- **Load Cell** → HX711 (E+, E-, A+, A-)

### 2. Configuración Raspberry Pi

```bash
# Navegar al directorio del proyecto
cd /path/to/project/IOT/raspberryCathub

# Hacer el script de instalación ejecutable
chmod +x install_weight_sensor_service.sh

# Ejecutar instalación (requiere sudo)
sudo ./install_weight_sensor_service.sh
```

#### Configuración Manual de Dependencias

```bash
# Instalar dependencias del sistema
sudo apt update
sudo apt install python3-pip python3-venv mongodb sqlite3

# Crear entorno virtual
python3 -m venv venv
source venv/bin/activate

# Instalar dependencias Python
pip install pymongo==4.5.0 pyserial==3.5 pyyaml==6.0.1
```

### 3. Configuración MongoDB Replica Set

#### Instalar MongoDB en 3 Servidores

```bash
# En cada servidor MongoDB
sudo apt update
sudo apt install -y mongodb-org

# Configurar /etc/mongod.conf
replication:
  replSetName: "rs0"
net:
  bindIp: 0.0.0.0
```

#### Inicializar Replica Set

```javascript
// En el servidor primary
mongo
rs.initiate({
  _id: "rs0",
  members: [
    { _id: 0, host: "mongodb-primary:27017" },
    { _id: 1, host: "mongodb-secondary:27017" },
    { _id: 2, host: "mongodb-arbiter:27017", arbiterOnly: true }
  ]
})
```

#### Crear Usuario de Base de Datos

```javascript
// En el primary de MongoDB
use cathub_iot
db.createUser({
  user: "cathub_user",
  pwd: "cathub_password",
  roles: [
    { role: "readWrite", db: "cathub_iot" }
  ]
})
```

## Configuración

### Archivo de Configuración Principal

Editar `/etc/cathub/weight_sensor_config.yaml`:

```yaml
serial:
  port: '/dev/ttyACM0'  # Puerto del Arduino
  baudrate: 115200
  timeout: 1.0

sensors:
  weight:
    weight_threshold: 5.0      # Threshold de cambio (gramos)
    stability_timeout: 10      # Timeout de estabilidad (segundos)
    min_weight_alert: 10.0     # Alerta peso mínimo
    max_weight_alert: 5000.0   # Alerta peso máximo
    debug: false
    auto_tare: true           # Tara automática al iniciar

mongodb:
  hosts:
    - 'mongodb-primary:27017'
    - 'mongodb-secondary:27017'
    - 'mongodb-arbiter:27017'
  database: 'cathub_iot'
  collection: 'weight_sensor_data'
  replica_set: 'rs0'
  username: 'cathub_user'
  password: 'cathub_password'
  auth_source: 'cathub_iot'
  enable_offline: true
  offline_db_path: '/tmp/cathub_weight_offline.db'
  ttl_hours: 24  # Retención de datos

device:
  device_id: 'cathub_weight_sensor_01'
  location: 'food_bowl'
```

### Calibración del Sensor

El sensor requiere calibración inicial:

```bash
# Calibración interactiva
sudo cathub-weight-sensor calibrate

# Tara (establecer cero)
sudo cathub-weight-sensor tare
```

## Uso del Sistema

### Comandos de Gestión

```bash
# Iniciar el servicio
sudo cathub-weight-sensor start

# Ver estado del servicio
sudo cathub-weight-sensor status

# Ver logs en tiempo real
sudo cathub-weight-sensor logs

# Ver estado detallado del sensor
sudo cathub-weight-sensor service-status

# Detener el servicio
sudo cathub-weight-sensor stop

# Reiniciar el servicio
sudo cathub-weight-sensor restart

# Editar configuración
sudo cathub-weight-sensor config
```

### Monitoreo

#### Logs del Servicio

```bash
# Logs en tiempo real
sudo journalctl -u cathub-weight-sensor -f

# Logs del archivo
tail -f /var/log/cathub_weight_sensor.log
```

#### Estado del Sistema

```bash
# Estado completo en JSON
sudo cathub-weight-sensor service-status

# Ejemplo de salida:
{
  "service": {
    "is_running": true,
    "current_weight_grams": 245.67
  },
  "weight_sensor": {
    "sensor_type": "weight",
    "is_stable": true,
    "total_readings": 1250,
    "error_rate_percent": 0.8
  },
  "mongodb": {
    "is_connected": true,
    "documents_inserted": 1200,
    "offline_documents_pending": 0
  }
}
```

## Estructura de Datos

### Formato de Datos del Arduino

```json
{
  "sensor_type": "weight",
  "timestamp": 1234567890,
  "weight_grams": 245.67,
  "is_stable": true,
  "calibrated": true
}
```

### Formato Almacenado en MongoDB

```json
{
  "_id": ObjectId("..."),
  "sensor_type": "weight",
  "device_id": "cathub_weight_sensor_01",
  "location": "food_bowl",
  "timestamp": "2024-01-15T10:30:00.000Z",
  "arduino_timestamp": 1234567890,
  "data": {
    "weight_grams": 245.67,
    "weight_stable": true,
    "calibrated": true,
    "weight_change": 2.3,
    "last_stable_weight": 243.37,
    "time_since_stable": 5.2
  },
  "alerts": [
    {
      "type": "low_weight",
      "severity": "low",
      "message": "Peso bajo detectado: 8.5g",
      "timestamp": "2024-01-15T10:30:00.000Z",
      "threshold": 10.0
    }
  ],
  "metadata": {
    "processor": "raspberry_pi",
    "processing_time": "2024-01-15T10:30:00.100Z",
    "data_quality": "high"
  },
  "inserted_at": "2024-01-15T10:30:00.150Z",
  "_repository_metadata": {
    "inserted_by": "raspberry_pi",
    "version": "1.0",
    "source": "arduino_serial"
  }
}
```

## Características Avanzadas

### 1. Almacenamiento Offline

El sistema puede funcionar sin conexión a MongoDB:
- Los datos se almacenan en SQLite local
- Sincronización automática cuando MongoDB está disponible
- Garantía de no pérdida de datos

### 2. Reconexión Automática

- Reconexión automática a Arduino si se desconecta
- Reconexión automática a MongoDB con retry exponencial
- Manejo de errores de red y timeouts

### 3. Sistema de Alertas

Tipos de alertas soportadas:
- **Calibration Error**: Sensor no calibrado
- **Low Weight**: Peso por debajo del threshold
- **Weight Overload**: Peso excesivo detectado
- **Instability**: Peso inestable por tiempo prolongado

### 4. Alta Disponibilidad MongoDB

- Failover automático en replica set
- Lectura desde secondary si primary falla
- Escritura con acknowledgment de mayoría

## Troubleshooting

### Problemas Comunes

#### Arduino no se conecta

```bash
# Verificar puerto serie
ls -la /dev/ttyACM*
ls -la /dev/ttyUSB*

# Verificar permisos
sudo usermod -a -G dialout cathub

# Reiniciar servicio
sudo cathub-weight-sensor restart
```

#### MongoDB desconectado

```bash
# Verificar estado del replica set
mongo --host mongodb-primary:27017
rs.status()

# Verificar conectividad
ping mongodb-primary
telnet mongodb-primary 27017
```

#### Datos no se almacenan

```bash
# Verificar logs detallados
sudo cathub-weight-sensor logs | grep ERROR

# Verificar almacenamiento offline
ls -la /tmp/cathub_weight_offline.db

# Verificar configuración
sudo cathub-weight-sensor config
```

#### Sensor no calibrado

```bash
# Recalibrar sensor
sudo cathub-weight-sensor stop
sudo cathub-weight-sensor calibrate
sudo cathub-weight-sensor start
```

### Logs de Debug

Para habilitar logging detallado:

```bash
# Editar configuración
sudo nano /etc/cathub/weight_sensor_config.yaml

# Cambiar:
sensors:
  weight:
    debug: true

# Reiniciar servicio
sudo cathub-weight-sensor restart
```

## Desarrollo y Personalización

### Estructura del Código

```
arduinoCathub/src/
├── sensors/
│   ├── BaseSensor.h          # Clase base para sensores
│   ├── WeightSensor.h        # Header del sensor de peso
│   └── WeightSensor.cpp      # Implementación del sensor
├── protocol/
│   └── CommandProcessor.cpp  # Procesador de comandos
└── main.cpp                  # Programa principal

raspberryCathub/
├── sensors/
│   ├── sensor_base.py           # Clase base para handlers
│   └── weight_sensor_handler.py # Handler del sensor de peso
├── scripts/
│   └── serial_data_manager.py   # Gestor de comunicación serial
├── repository/
│   └── mongo_repository.py      # Repositorio MongoDB
└── weight_sensor_service.py     # Servicio principal
```

### Agregar Nuevos Sensores

Para agregar un nuevo sensor:

1. **Arduino**: Crear clase heredada de `BaseSensor`
2. **Raspberry Pi**: Crear handler heredado de `SensorHandler`
3. **Registrar**: Agregar al `SensorManager` y `SerialDataManager`

### Personalizar Alertas

Editar `WeightSensorHandler._check_alerts()` para agregar nuevos tipos de alertas.

## Performance y Escalabilidad

### Métricas del Sistema

- **Throughput**: ~10-20 lecturas/segundo por sensor
- **Latencia**: <100ms desde Arduino a MongoDB
- **Storage**: ~1KB por documento de sensor
- **Retención**: Configurable con TTL (default 24h)

### Optimizaciones

- Uso de índices MongoDB para consultas eficientes
- Batch inserts para reducir overhead de red
- Almacenamiento offline para evitar pérdida de datos
- Pooling de conexiones MongoDB

## Licencia

Este proyecto es parte del sistema CatHub IoT. Consultar licencia del proyecto principal.

## Soporte

Para reportar issues o solicitar features:
1. Crear issue en el repositorio del proyecto
2. Incluir logs relevantes
3. Especificar configuración del sistema
4. Describir pasos para reproducir el problema
