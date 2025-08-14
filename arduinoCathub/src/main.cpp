#include <Arduino.h>
#include "sensors/SensorManager.h"

SensorManager sensorManager;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 3000; // 3 segundos para acomodar el DHT

// Declaraciones de funciones
void printAllSensorData();
void printFeederData();
void printWaterDispenserData();
void printLitterboxData();

void setup() {
    Serial.begin(115200);
    while(!Serial) { delay(10); }
    
    // Arreglar el problema con F() macro
    Serial.print(F("{\"event\":\"CATHUB_STARTING\",\"timestamp\":"));
    Serial.print(millis());
    Serial.println(F("}"));
    
    // Inicializar todos los sensores y actuadores
    sensorManager.begin();
    
    Serial.println(F("{\"event\":\"CATHUB_READY\",\"message\":\"All devices initialized\"}"));
    
    delay(2000); // Dar tiempo para que los sensores se estabilicen
}

void loop() {
    // Actualizar todos los sensores
    sensorManager.poll();

    // Enviar datos cada 3 segundos
    unsigned long now = millis();
    if (now - lastPrintTime >= PRINT_INTERVAL) {
        printAllSensorData();
        lastPrintTime = now;
    }
    
    delay(10); // Pequeña pausa para no saturar
}

void printAllSensorData() {
    Serial.println("{");
    Serial.print("  \"timestamp\": ");
    Serial.print(millis());
    Serial.println(",");
    
    // === COMEDERO ===
    printFeederData();
    Serial.println(",");
    
    // === BEBEDERO ===
    printWaterDispenserData();
    Serial.println(",");
    
    // === ARENERO ===
    printLitterboxData();
    
    Serial.println("}");
}

void printFeederData() {
    Serial.println("  \"feeder\": {");
    
    // Sensor de peso
    FeederWeightSensor* fw = sensorManager.getFeederWeight();
    if (fw && fw->isReady()) {
        float weight = fw->getCurrentWeight();
        Serial.print("    \"weight_grams\": ");
        Serial.print(weight, 2);
        Serial.println(",");
        Serial.print("    \"weight_status\": \"");
        Serial.print(fw->getStatus());
        Serial.println("\",");
    } else {
        Serial.println("    \"weight_grams\": null,");
        Serial.println("    \"weight_status\": \"ERROR\",");
    }
    
    // Sensores ultrasónicos
    FeederUltrasonicSensor1* fu1 = sensorManager.getFeederUltrasonic1();
    FeederUltrasonicSensor2* fu2 = sensorManager.getFeederUltrasonic2();
    
    if (fu1 && fu1->isReady()) {
        Serial.print("    \"level_sensor_1_cm\": ");
        Serial.print(fu1->getDistance(), 1);
        Serial.println(",");
    } else {
        Serial.println("    \"level_sensor_1_cm\": null,");
    }
    
    if (fu2 && fu2->isReady()) {
        Serial.print("    \"level_sensor_2_cm\": ");
        Serial.print(fu2->getDistance(), 1);
        Serial.println(",");
    } else {
        Serial.println("    \"level_sensor_2_cm\": null,");
    }
    
    // Motor del comedero
    FeederStepperMotor* fm = sensorManager.getFeederMotor();
    if (fm && fm->isReady()) {
        Serial.print("    \"motor_status\": \"");
        Serial.print(fm->getStatus());
        Serial.println("\",");
        Serial.print("    \"motor_enabled\": ");
        Serial.print(fm->isEnabled() ? "true" : "false");
        Serial.println(",");
        Serial.print("    \"motor_position\": ");
        Serial.print(fm->getCurrentPosition());
        Serial.println();
    } else {
        Serial.println("    \"motor_status\": \"ERROR\"");
    }
    
    Serial.print("  }");
}

void printWaterDispenserData() {
    Serial.println("  \"water_dispenser\": {");
    
    // Sensor de nivel de agua
    WaterDispenserSensor* wd = sensorManager.getWaterDispenser();
    if (wd && wd->isReady()) {
        float waterLevel = wd->getAnalogValue();
        String waterStatus = wd->getWaterLevel();
        bool waterDetected = wd->isWaterDetected();
        
        Serial.print("    \"water_level_analog\": ");
        Serial.print(waterLevel, 0);
        Serial.println(",");
        Serial.print("    \"water_status\": \"");
        Serial.print(waterStatus);
        Serial.println("\",");
        Serial.print("    \"water_detected\": ");
        Serial.print(waterDetected ? "true" : "false");
        Serial.println(",");
    } else {
        Serial.println("    \"water_level_analog\": null,");
        Serial.println("    \"water_status\": \"ERROR\",");
        Serial.println("    \"water_detected\": false,");
    }
    
    // Sensor infrarrojo
    WaterDispenserIRSensor* wir = sensorManager.getWaterDispenserIR();
    if (wir && wir->isReady()) {
        bool objectDetected = wir->isObjectDetected();
        unsigned long detectionTime = wir->getDetectionDuration();
        
        Serial.print("    \"ir_object_detected\": ");
        Serial.print(objectDetected ? "true" : "false");
        Serial.println(",");
        Serial.print("    \"ir_detection_duration_ms\": ");
        Serial.print(detectionTime);
        Serial.println(",");
    } else {
        Serial.println("    \"ir_object_detected\": false,");
        Serial.println("    \"ir_detection_duration_ms\": 0,");
    }
    
    // Bomba de agua
    WaterDispenserPump* wp = sensorManager.getWaterDispenserPump();
    if (wp && wp->isReady()) {
        bool pumpRunning = wp->isPumpRunning();
        unsigned long remainingTime = wp->getRemainingTime();
        String pumpStatus = wp->getStatus();
        
        Serial.print("    \"pump_running\": ");
        Serial.print(pumpRunning ? "true" : "false");
        Serial.println(",");
        Serial.print("    \"pump_remaining_time_ms\": ");
        Serial.print(remainingTime);
        Serial.println(",");
        Serial.print("    \"pump_status\": \"");
        Serial.print(pumpStatus);
        Serial.println("\"");
    } else {
        Serial.println("    \"pump_running\": false,");
        Serial.println("    \"pump_remaining_time_ms\": 0,");
        Serial.println("    \"pump_status\": \"ERROR\"");
    }
    
    Serial.print("  }");
}

void printLitterboxData() {
    Serial.println("  \"litterbox\": {");
    
    // Sensor de temperatura y humedad
    LitterboxDHTSensor* ldht = sensorManager.getLitterboxDHT();
    if (ldht && ldht->isReady()) {
        float temperature = ldht->getTemperature();
        float humidity = ldht->getHumidity();
        
        Serial.print("    \"temperature_celsius\": ");
        Serial.print(temperature, 1);
        Serial.println(",");
        Serial.print("    \"humidity_percent\": ");
        Serial.print(humidity, 1);
        Serial.println(",");
    } else {
        Serial.println("    \"temperature_celsius\": null,");
        Serial.println("    \"humidity_percent\": null,");
    }
    
    // Sensor ultrasónico
    LitterboxUltrasonicSensor* lu = sensorManager.getLitterboxUltrasonic();
    if (lu && lu->isReady()) {
        float distance = lu->getDistance();
        Serial.print("    \"distance_cm\": ");
        Serial.print(distance, 1);
        Serial.println(",");
    } else {
        Serial.println("    \"distance_cm\": null,");
    }
    
    // Sensor de gas MQ2
    LitterboxMQ2Sensor* lmq2 = sensorManager.getLitterboxMQ2();
    if (lmq2 && lmq2->isReady()) {
        float gasLevel = lmq2->getValue();
        Serial.print("    \"gas_level\": ");
        Serial.print(gasLevel, 0);
        Serial.println(",");
    } else {
        Serial.println("    \"gas_level\": null,");
    }
    
    // Motor del arenero
    LitterboxStepperMotor* lm = sensorManager.getLitterboxMotor();
    if (lm && lm->isReady()) {
        String motorStatus = lm->getStatus();
        bool motorEnabled = lm->isEnabled();
        bool isCleaning = lm->isCleaning();
        int motorPosition = lm->getCurrentPosition();
        
        Serial.print("    \"motor_status\": \"");
        Serial.print(motorStatus);
        Serial.println("\",");
        Serial.print("    \"motor_enabled\": ");
        Serial.print(motorEnabled ? "true" : "false");
        Serial.println(",");
        Serial.print("    \"is_cleaning\": ");
        Serial.print(isCleaning ? "true" : "false");
        Serial.println(",");
        Serial.print("    \"motor_position\": ");
        Serial.print(motorPosition);
        Serial.println();
    } else {
        Serial.println("    \"motor_status\": \"ERROR\"");
    }
    
    Serial.print("  }");
}