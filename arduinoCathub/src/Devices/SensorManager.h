#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "config/DeviceIDs.h"
#include "litterbox/sensors/LitterboxUltrasonicSensor.h"
#include "litterbox/sensors/LitterboxDHTSensor.h"
#include "litterbox/sensors/LitterboxMQ2Sensor.h"
#include "litterbox/actuators/LitterboxStepperMotor.h"
#include "feeder/sensors/FeederWeightSensor.h"
#include "feeder/sensors/FeederUltrasonicSensor.h"
#include "feeder/actuators/FeederStepperMotor.h"
#include "waterdispenser/sensors/WaterDispenserSensor.h"
#include "waterdispenser/actuators/WaterDispenserPump.h"
#include "waterdispenser/sensors/WaterDispenserIRSensor.h"

class SensorManager {
private:
    LitterboxUltrasonicSensor* ultrasonicSensor;
    LitterboxDHTSensor* dhtSensor;
    LitterboxMQ2Sensor* mq2Sensor;
    LitterboxStepperMotor* litterboxMotor;
    
    FeederWeightSensor* weightSensor;
    FeederUltrasonicSensor1* feederUltrasonic1;
    FeederUltrasonicSensor2* feederUltrasonic2;
    FeederStepperMotor* feederMotor;
    
    WaterDispenserSensor* waterSensor;
    WaterDispenserPump* waterPump;
    WaterDispenserIRSensor* waterIRSensor;
    
    bool initialized;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 100;

public:
    SensorManager();
    ~SensorManager();
    
    bool begin();
    void poll();
    
    // ===== MÉTODOS DEL ARENERO =====
    float getLitterboxDistance();
    float getLitterboxTemperature();
    float getLitterboxHumidity();
    float getLitterboxGasPPM();
    LitterboxStepperMotor* getLitterboxMotor();
    
    // ===== MÉTODOS DEL COMEDERO =====
    float getFeederWeight();
    float getFeederCatDistance();
    float getFeederFoodDistance();
    FeederStepperMotor* getFeederMotor();

    // ===== MÉTODOS DEL BEBEDERO =====
    String getWaterLevel();
    bool isWaterDetected();
    bool isCatDrinking();
    WaterDispenserSensor* getWaterSensor() { return waterSensor; }
    WaterDispenserPump* getWaterPump();
    
    // ===== MÉTODOS DE ESTADO =====
    bool isLitterboxUltrasonicReady();
    bool isLitterboxDHTReady();
    bool isLitterboxMQ2Ready();
    bool isFeederWeightReady();
    bool isFeederCatUltrasonicReady();
    bool isFeederFoodUltrasonicReady();
    bool isFeederMotorReady();
    bool isWaterLevelReady();
    bool isWaterIRReady();
    bool areAllSensorsReady();
    
    // ===== MÉTODOS DE DIAGNÓSTICO =====
    String getSensorStatus();
    String getAllReadings();
    void printAllSensorReadings(); // ✅ IMPLEMENTADO EN .cpp
};

#endif