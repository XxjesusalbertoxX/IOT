#include <Arduino.h>
#include "Devices/SensorManager.h"
#include "protocol/CommandProcessor.h"
#include "Devices/litterbox/actuators/LitterboxStepperMotor.h"
#include "Devices/feeder/actuators/FeederStepperMotor.h"
#include "Devices/waterdispenser/actuators/WaterDispenserPump.h"

SensorManager sensorManager;
LitterboxStepperMotor litterboxMotor;
FeederStepperMotor feederMotor;
WaterDispenserPump waterPump;
CommandProcessor commandProcessor(&sensorManager, &litterboxMotor, &feederMotor, &waterPump);

void setup() {
    Serial.begin(115200);
    while(!Serial) { delay(10); }
    
    Serial.println(F("{\"event\":\"CATHUB_STARTING\"}"));
    
    // Inicializar todos los sensores y actuadores
    sensorManager.begin();
    commandProcessor.initialize();
    
    Serial.println(F("{\"event\":\"CATHUB_READY\",\"message\":\"Esperando comandos de la Ras\"}"));
    
    delay(2000); // Dar tiempo para que los sensores se estabilicen
}


void loop() {
    // Leer comandos del Serial
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        commandProcessor.processCommand(command);
    }
    
    // Actualizar sensores
    sensorManager.poll();
    
    // 🔥 ACTUALIZAR SISTEMA AUTOMÁTICO (MUY IMPORTANTE)
    commandProcessor.update();
    
    // 🔥 ACTUALIZAR BOMBA DE AGUA DIRECTAMENTE TAMBIÉN
    waterPump.update();  // <-- Usa punto, no flecha
    
    delay(50); // Pequeña pausa para no saturar
}