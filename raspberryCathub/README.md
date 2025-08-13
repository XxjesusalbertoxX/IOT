# 🐱 CatHub IoT - Sistema de Entorno Inteligente para Gatos

**CatHub IoT** es un sistema integral de monitoreo y automatización para el cuidado de gatos, diseñado para ejecutarse en Raspberry Pi. Controla comederos, bebederos y areneros de forma inteligente, mientras monitorea la actividad y salud del gato.

## 🏗️ Arquitectura del Sistema

### Estructura del Proyecto
```
raspberryCathub/
├── main.py                     # Punto de entrada principal
├── requirements.txt            # Dependencias del proyecto
├── README.md                   # Este archivo
│
├── sensors/                    # Módulos de sensores
│   ├── sensor.py              # Clase base para sensores
│   ├── nivel_comida_sensor.py  # Sensor de nivel de comida
│   ├── nivel_agua_sensor.py    # Sensor de nivel de agua
│   ├── presencia_sensor.py     # Sensor de presencia (PIR)
│   ├── peso_sensor.py          # Sensor de peso (celda de carga)
│   ├── humedad_arenero_sensor.py # Sensor de humedad del arenero
│   ├── temperatura_sensor.py   # Sensor de temperatura
│   └── camara_sensor.py        # Cámara con detección
│
├── activadores/               # Módulos de actuadores
│   ├── dispensador_comida.py  # Control del dispensador de comida
│   ├── dispensador_agua.py    # Control del dispensador de agua
│   └── limpiador_arenero.py   # Control del limpiador de arenero
│
├── services/                  # Servicios de lógica de negocio
│   ├── hub_sensors.py         # Hub central de sensores
│   ├── hub_actuadores.py      # Hub central de actuadores
│   ├── comedero_service.py    # Servicio del comedero
│   ├── bebedero_service.py    # Servicio del bebedero
│   ├── arenero_service.py     # Servicio del arenero
│   └── gato_monitoring.py     # Monitoreo integral del gato
│
├── repository/                # Acceso a datos
│   ├── consultator.py         # Comunicación serial con hardware
│   ├── mongo.py               # Conexión y operaciones MongoDB
│   └── postgres.py            # Conexión y operaciones PostgreSQL
│
├── scripts/                   # Scripts utilitarios
│   └── serial_parser.py       # Parser de mensajes seriales
│
└── sensors_data/             # Datos de sensores (modo offline)
    ├── NC01_data.json         # Datos del sensor de comida
    ├── NA01_data.json         # Datos del sensor de agua
    ├── HA01_data.json         # Datos del sensor de humedad
    └── PG01_data.json         # Datos del sensor de presencia
```

## 🎯 Funcionalidades Principales

### 🍽️ Gestión del Comedero
- **Dispensado automático** por horarios programables
- **Control de porciones** (pequeña, normal, grande)
- **Detección de consumo** mediante sensor de peso
- **Monitoreo de nivel** con alerta de rellenado
- **Registro de alimentación** en base de datos

### 💧 Gestión del Bebedero  
- **Rellenado automático** basado en nivel
- **Control de calidad del agua** (temperatura)
- **Circulación periódica** para mantener frescura
- **Alertas de nivel bajo** y temperatura
- **Registro de consumo** de agua

### 🏠 Gestión del Arenero
- **Limpieza automática** por horarios y uso
- **Detección de uso** con sensor de presencia
- **Monitoreo de humedad** para limpieza inteligente
- **Diferentes tipos de limpieza** (rápida, normal, profunda)
- **Registro de actividad** del arenero

### 🐱 Monitoreo del Gato
- **Detección de actividad** en tiempo real
- **Captura de fotos** automática y manual
- **Análisis de patrones** de comportamiento
- **Alertas de salud** basadas en actividad
- **Detección de inactividad** prolongada

## 🛠️ Instalación

### Requisitos de Hardware
- **Raspberry Pi 4** (recomendado)
- **Sensores:**
  - Ultrasónicos HC-SR04 (nivel de comida y agua)
  - Celda de carga HX711 (peso del plato)
  - PIR HC-SR501 (detección de presencia)
  - DHT22 (temperatura y humedad)
  - Cámara Pi o USB
- **Actuadores:**
  - Servomotores o motores paso a paso
  - Bombas de agua 12V
  - Relés para control de motores
- **Arduino/ESP32** para interfaz de sensores

### Instalación del Software

1. **Clonar el repositorio:**
   ```bash
   git clone [url-del-repositorio]
   cd raspberryCathub
   ```

2. **Crear entorno virtual:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # En Windows: venv\Scripts\activate
   ```

3. **Instalar dependencias:**
   ```bash
   pip install -r requirements.txt
   ```

4. **Configurar base de datos:**
   ```bash
   # MongoDB (opcional)
   sudo apt update
   sudo apt install mongodb
   
   # PostgreSQL (opcional)
   sudo apt install postgresql postgresql-contrib
   ```

## ⚙️ Configuración

### Configuración de Hardware
1. **Conectar sensores** según el esquema de pines
2. **Configurar puerto serial** en `repository/consultator.py`:
   ```python
   PORT = '/dev/ttyUSB0'  # Cambiar según tu configuración
   ```

3. **Configurar base de datos** en `repository/mongo.py`:
   ```python
   CONNECTION_STRING = "mongodb://usuario:password@host:puerto/database"
   ```

### Configuración de Sensores
Modificar IDs y parámetros en `main.py`:
```python
sensor_nivel_comida = SensorNivelComida("NC01", 0, 100)
sensor_nivel_agua = SensorNivelAgua("NA01", 0, 100)
# ... otros sensores
```

## 🚀 Uso

### Ejecución Principal
```bash
python main.py
```

### Comandos Principales
El sistema se ejecuta de forma autónoma, pero puedes interactuar programáticamente:

```python
# Dispensar comida manualmente
comedero_service.dispensar_porcion_manual(30)  # 30 gramos

# Forzar rellenado de agua
bebedero_service.rellenar_manual(200)  # 200 ml

# Limpiar arenero manualmente
arenero_service.limpiar_manual("profunda")

# Capturar foto
gato_monitoring.forzar_captura_foto()
```

## 📊 Monitoreo y Estadísticas

### Datos Disponibles
- **Consumo de comida y agua** diario
- **Frecuencia de uso** del arenero  
- **Patrones de actividad** del gato
- **Historial de fotos** con timestamps
- **Estadísticas de salud** basadas en comportamiento

### Acceso a Estadísticas
```python
# Estadísticas del comedero
stats_comedero = comedero_service.obtener_estadisticas_diarias()

# Resumen de actividad del gato  
actividad = gato_monitoring.obtener_resumen_actividad_diaria()

# Estado de todos los sensores
estado_sensores = hub_sensores.obtener_resumen_estado()
```

## 🔧 Configuración Avanzada

### Horarios de Alimentación
```python
nuevos_horarios = [
    {"hora": "07:00", "porcion": "normal"},
    {"hora": "13:00", "porcion": "pequena"}, 
    {"hora": "19:00", "porcion": "normal"}
]
comedero_service.configurar_horarios_alimentacion(nuevos_horarios)
```

### Limpieza Automática del Arenero
```python
arenero_service.configurar_limpieza_automatica(
    activar=True,
    umbral_humedad=70,
    horas_programadas=[6, 14, 22]
)
```

### Patrones de Actividad Normal
```python
nuevos_patrones = {
    'comedero_visitas_dia': (2, 6),
    'arenero_visitas_dia': (1, 3),
    'horas_activo_dia': (10, 14)
}
gato_monitoring.configurar_patrones_normales(nuevos_patrones)
```

## 🚨 Sistema de Alertas

El sistema envía alertas por:
- **Nivel bajo** de comida o agua
- **Necesidad de limpieza** del arenero
- **Inactividad prolongada** del gato
- **Patrones anormales** de comportamiento
- **Errores de hardware** o comunicación

## 🛡️ Modo Offline

Si no hay conexión a internet o base de datos:
- Los datos se guardan **localmente** en archivos JSON
- El sistema **continúa funcionando** normalmente
- Los datos se **sincronizan** cuando se restablece la conexión

## 📝 Logging y Debug

- Los logs se muestran en **consola** con emojis descriptivos
- **Timestamps** en todas las operaciones
- **Códigos de error** específicos para cada componente
- **Modo verbose** disponible para debugging

## 🤝 Contribuciones

1. Fork el proyecto
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está licenciado bajo la Licencia MIT - ver el archivo `LICENSE` para detalles.

## 👥 Autores

- **Tu Nombre** - *Desarrollo inicial* - [tu-github](https://github.com/tu-usuario)

## 🙏 Agradecimientos

- Inspirado en el proyecto de **estacionamiento inteligente** del equipo ParKings
- **Raspberry Pi Foundation** por la excelente documentación
- **Comunidad IoT** por los recursos y ejemplos

---

**🐱 ¡Hecho con ❤️ para nuestros amigos felinos!**
