#include <Arduino.h>
#include "Devices/SensorManager.h"
#include "protocol/CommandProcessor.h"
#include "Devices/litterbox/actuators/LitterboxStepperMotor.h"
#include "Devices/feeder/actuators/FeederStepperMotor.h"
#include "Devices/waterdispenser/actuators/WaterDispenserPump.h"

// 🔥 CREAR TODAS LAS INSTANCIAS UNA SOLA VEZ EN MAIN
// LITTERBOX
LitterboxUltrasonicSensor litterboxUltrasonic;
LitterboxDHTSensor litterboxDHT;
LitterboxMQ2Sensor litterboxMQ2;
LitterboxStepperMotor litterboxMotor;

// FEEDER  
FeederWeightSensor feederWeight;
FeederUltrasonicSensor1 feederUltrasonicCat;
FeederUltrasonicSensor2 feederUltrasonicFood;
FeederStepperMotor feederMotor;

// WATER DISPENSER
WaterDispenserSensor waterSensor;
WaterDispenserIRSensor waterIRSensor;
WaterDispenserPump waterPump;

// 🔥 SENSORMANAGER RECIBE TODAS LAS INSTANCIAS COMO PUNTEROS
SensorManager sensorManager(&litterboxUltrasonic, &litterboxDHT, &litterboxMQ2, &litterboxMotor,
                            &feederWeight, &feederUltrasonicCat, &feederUltrasonicFood, &feederMotor,
                            &waterSensor, &waterPump, &waterIRSensor);

// 🔥 COMMANDPROCESSOR RECIBE LAS MISMAS INSTANCIAS
CommandProcessor commandProcessor(&sensorManager, &litterboxMotor, &feederMotor, &waterPump);

void setup() {
    Serial.begin(115200);
    while(!Serial) { delay(10); }
    
    Serial.println(F("{\"event\":\"CATHUB_STARTING\"}"));
    
    // 🔥 INICIALIZAR SISTEMAS (CADA OBJETO EXISTE UNA SOLA VEZ)
    sensorManager.begin();
    commandProcessor.initialize();
    
    Serial.println(F("{\"event\":\"CATHUB_READY\",\"message\":\"Esperando comandos de la Ras\"}"));
    
    delay(2000);
}

void loop() {
    // Leer comandos del Serial
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        commandProcessor.processCommand(command);
    }
    
    // Actualizar sensores
    sensorManager.poll();
    
    // Actualizar sistema automático
    commandProcessor.update();

    // Actualizar motor del comedero (genera los pasos cuando está en modo continuous)
    feederMotor.update();
    
    // Actualizar bomba de agua directamente
    waterPump.update();
    
    delay(50);
}