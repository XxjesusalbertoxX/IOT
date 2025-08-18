#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
// Sensores del arenero
#include "litterbox/sensors/LitterboxUltrasonicSensor.h"
#include "litterbox/sensors/LitterboxDHTSensor.h"
#include "litterbox/sensors/LitterboxMQ2Sensor.h"
#include "litterbox/actuators/LitterboxStepperMotor.h"
// Sensores del comedero
#include "feeder/sensors/FeederWeightSensor.h"
#include "feeder/sensors/FeederUltrasonicSensor.h"
#include "feeder/actuators/FeederStepperMotor.h"
// Sensores del bebedero
#include "waterdispenser/sensors/WaterDispenserSensor.h"
#include "waterdispenser/actuators/WaterDispenserPump.h"
#include "waterdispenser/sensors/WaterDispenserIRSensor.h"


class SensorManager {
private:
    // ===== SENSORES DEL ARENERO =====
    LitterboxUltrasonicSensor* ultrasonicSensor;
    LitterboxDHTSensor* dhtSensor;
    LitterboxMQ2Sensor* mq2Sensor;
    LitterboxStepperMotor* litterboxMotor; // <-- AGREGA ESTA LÍNEA
    
    // ===== SENSORES DEL COMEDERO =====
    FeederWeightSensor* weightSensor;
    FeederUltrasonicSensor1* feederUltrasonic1; // Detector de gato
    FeederUltrasonicSensor2* feederUltrasonic2; // Medidor de comida
    FeederStepperMotor* feederMotor;
    
    // ===== SENSORES DEL BEBEDERO =====
    WaterDispenserSensor* waterSensor;
    WaterDispenserPump* waterPump;
    WaterDispenserIRSensor* waterIRSensor;
    
    bool initialized;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 100; // Actualizar cada 100ms

public:
    void printAllSensorReadings();
    SensorManager();
    ~SensorManager();
    
    bool begin();
    void poll(); // Actualizar todos los sensores
    
    // ===== MÉTODOS PARA OBTENER DATOS DEL ARENERO =====
    float getLitterboxDistance();
    float getLitterboxTemperature();
    float getLitterboxHumidity();
    float getLitterboxGasPPM();
    LitterboxStepperMotor* getLitterboxMotor(); // <-- AGREGA EL GETTER
    
    // ===== MÉTODOS PARA OBTENER DATOS DEL COMEDERO =====
    float getFeederWeight();           // Peso actual en gramos
    float getFeederCatDistance();      // Distancia del gato (sensor 1)
    float getFeederFoodDistance();     // Distancia de la comida (sensor 2)
    FeederStepperMotor* getFeederMotor();

    // ===== MÉTODOS PARA OBTENER DATOS DEL BEBEDERO =====
    String getWaterLevel();             // Valor analógico del sensor de agua
    bool isWaterDetected();            // ¿Hay agua?
    String getWaterLevelString();      // "DRY", "DAMP", "WET", "FLOOD"
    bool isCatDrinking();              // ¿Gato tomando agua?
    WaterDispenserPump* getWaterPump();
    bool isWaterSensorReady();
    bool isWaterPumpReady();
    bool isWaterIRReady();
    
    // ===== MÉTODOS DE ESTADO DE SENSORES =====
    bool isLitterboxUltrasonicReady();
    bool isLitterboxDHTReady();
    bool isLitterboxMQ2Ready();
    bool isFeederWeightReady();
    bool isFeederUltrasonic1Ready();
    bool isFeederUltrasonic2Ready();
    bool isFeederMotorReady();
    bool areAllSensorsReady();


    
    // ===== MÉTODOS DE DIAGNÓSTICO =====
    String getSensorStatus();
    String getAllReadings();
};

#endif