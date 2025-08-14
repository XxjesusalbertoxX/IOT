#include <Arduino.h>
#include "sensors/SensorManager.h"
#include "protocol/CommandProcessor.h"
#include "sensors/litterbox/LitterboxStepperMotor.h"

SensorManager sensorManager;
LitterboxStepperMotor litterboxMotor;
CommandProcessor commandProcessor(&sensorManager, &litterboxMotor);

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
    // ✅ SIEMPRE actualizar sensores para que tengan datos frescos
    sensorManager.poll();
    
    // ✅ PROCESAR comandos de la Ras cuando lleguen
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            commandProcessor.processCommand(command);
        }
    }
    
    // ✅ Actualizar actuadores
    commandProcessor.update();
    
    // ❌ YA NO enviamos datos automáticamente cada 3 segundos
    
    delay(10); // Pequeña pausa para no saturar
}