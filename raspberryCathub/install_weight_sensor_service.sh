#!/bin/bash

# CatHub Weight Sensor Service Installation Script
# Este script instala y configura el servicio de sensor de peso

set -e  # Salir si cualquier comando falla

echo "=== CatHub Weight Sensor Service Installation ==="

# Verificar que se ejecuta como root
if [ "$EUID" -ne 0 ]; then 
    echo "Error: Este script debe ejecutarse como root (sudo)"
    exit 1
fi

# Variables de configuración
SERVICE_NAME="cathub-weight-sensor"
SERVICE_USER="cathub"
INSTALL_DIR="/opt/cathub"
LOG_DIR="/var/log"
CONFIG_DIR="/etc/cathub"

echo "1. Creando usuario del servicio..."
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /bin/false -d "$INSTALL_DIR" "$SERVICE_USER"
    echo "Usuario $SERVICE_USER creado"
else
    echo "Usuario $SERVICE_USER ya existe"
fi

echo "2. Creando directorios..."
mkdir -p "$INSTALL_DIR"
mkdir -p "$CONFIG_DIR"
mkdir -p "$LOG_DIR"

# Dar permisos al usuario
chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
chown -R "$SERVICE_USER:$SERVICE_USER" "$LOG_DIR"

echo "3. Instalando dependencias del sistema..."
apt-get update
apt-get install -y python3 python3-pip python3-venv mongodb sqlite3

echo "4. Creando entorno virtual Python..."
cd "$INSTALL_DIR"
python3 -m venv venv
source venv/bin/activate

echo "5. Instalando dependencias Python..."
pip install --upgrade pip
cat > requirements.txt << EOF
pymongo==4.5.0
pyserial==3.5
pyyaml==6.0.1
sqlite3
EOF

pip install -r requirements.txt

echo "6. Copiando archivos del servicio..."
# Copiar archivos desde el directorio actual
cp weight_sensor_service.py "$INSTALL_DIR/"
cp weight_sensor_config.yaml "$CONFIG_DIR/"
cp -r scripts/ "$INSTALL_DIR/" 2>/dev/null || echo "Directorio scripts no encontrado"
cp -r sensors/ "$INSTALL_DIR/" 2>/dev/null || echo "Directorio sensors no encontrado"  
cp -r repository/ "$INSTALL_DIR/" 2>/dev/null || echo "Directorio repository no encontrado"

# Ajustar permisos
chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
chown -R "$SERVICE_USER:$SERVICE_USER" "$CONFIG_DIR"
chmod +x "$INSTALL_DIR/weight_sensor_service.py"

echo "7. Creando servicio systemd..."
cat > "/etc/systemd/system/$SERVICE_NAME.service" << EOF
[Unit]
Description=CatHub Weight Sensor IoT Service
After=network.target mongodb.service
Wants=mongodb.service

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=$INSTALL_DIR
Environment=PATH=$INSTALL_DIR/venv/bin
ExecStart=$INSTALL_DIR/venv/bin/python $INSTALL_DIR/weight_sensor_service.py --config $CONFIG_DIR/weight_sensor_config.yaml
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=cathub-weight-sensor

[Install]
WantedBy=multi-user.target
EOF

echo "8. Configurando permisos de dispositivo serial..."
# Agregar usuario al grupo dialout para acceso a puerto serie
usermod -a -G dialout "$SERVICE_USER"

echo "9. Habilitando e iniciando el servicio..."
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"

echo "10. Configurando logrotate..."
cat > "/etc/logrotate.d/$SERVICE_NAME" << EOF
$LOG_DIR/cathub_weight_sensor.log {
    daily
    missingok
    rotate 7
    compress
    delaycompress
    notifempty
    create 644 $SERVICE_USER $SERVICE_USER
    postrotate
        systemctl reload-or-restart $SERVICE_NAME
    endscript
}
EOF

echo "11. Creando script de utilidades..."
cat > "/usr/local/bin/cathub-weight-sensor" << EOF
#!/bin/bash
# CatHub Weight Sensor Management Script

INSTALL_DIR="$INSTALL_DIR"
CONFIG_DIR="$CONFIG_DIR"
SERVICE_NAME="$SERVICE_NAME"

case "\$1" in
    start)
        systemctl start "\$SERVICE_NAME"
        ;;
    stop)
        systemctl stop "\$SERVICE_NAME"
        ;;
    restart)
        systemctl restart "\$SERVICE_NAME"
        ;;
    status)
        systemctl status "\$SERVICE_NAME"
        ;;
    logs)
        journalctl -u "\$SERVICE_NAME" -f
        ;;
    calibrate)
        sudo -u $SERVICE_USER \$INSTALL_DIR/venv/bin/python \$INSTALL_DIR/weight_sensor_service.py --config \$CONFIG_DIR/weight_sensor_config.yaml --calibrate
        ;;
    tare)
        sudo -u $SERVICE_USER \$INSTALL_DIR/venv/bin/python \$INSTALL_DIR/weight_sensor_service.py --config \$CONFIG_DIR/weight_sensor_config.yaml --tare
        ;;
    service-status)
        sudo -u $SERVICE_USER \$INSTALL_DIR/venv/bin/python \$INSTALL_DIR/weight_sensor_service.py --config \$CONFIG_DIR/weight_sensor_config.yaml --status
        ;;
    config)
        nano "\$CONFIG_DIR/weight_sensor_config.yaml"
        ;;
    *)
        echo "Uso: \$0 {start|stop|restart|status|logs|calibrate|tare|service-status|config}"
        echo ""
        echo "Comandos:"
        echo "  start         - Iniciar el servicio"
        echo "  stop          - Detener el servicio" 
        echo "  restart       - Reiniciar el servicio"
        echo "  status        - Ver estado del servicio systemd"
        echo "  logs          - Ver logs en tiempo real"
        echo "  calibrate     - Calibrar sensor de peso"
        echo "  tare          - Realizar tara del sensor"
        echo "  service-status - Ver estado detallado del servicio"
        echo "  config        - Editar configuración"
        exit 1
        ;;
esac
EOF

chmod +x "/usr/local/bin/cathub-weight-sensor"

echo ""
echo "=== INSTALACIÓN COMPLETADA ==="
echo ""
echo "El servicio CatHub Weight Sensor ha sido instalado exitosamente."
echo ""
echo "Archivos instalados:"
echo "  - Servicio: $INSTALL_DIR/"
echo "  - Configuración: $CONFIG_DIR/weight_sensor_config.yaml"
echo "  - Logs: $LOG_DIR/cathub_weight_sensor.log"
echo "  - Systemd service: /etc/systemd/system/$SERVICE_NAME.service"
echo ""
echo "Comandos útiles:"
echo "  sudo cathub-weight-sensor start     # Iniciar servicio"
echo "  sudo cathub-weight-sensor status    # Ver estado"
echo "  sudo cathub-weight-sensor logs      # Ver logs"
echo "  sudo cathub-weight-sensor calibrate # Calibrar sensor"
echo "  sudo cathub-weight-sensor tare      # Realizar tara"
echo ""
echo "IMPORTANTE:"
echo "1. Edita la configuración: sudo nano $CONFIG_DIR/weight_sensor_config.yaml"
echo "2. Ajusta el puerto serial y configuración de MongoDB"
echo "3. Conecta el Arduino al puerto serie configurado"
echo "4. Inicia el servicio: sudo cathub-weight-sensor start"
echo ""
echo "Para verificar el estado: sudo cathub-weight-sensor service-status"
