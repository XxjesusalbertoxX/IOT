// SensorManager.h
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
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
    // Objetos de sensores (NO los posee, solo los referencia)
    LitterboxUltrasonicSensor* ultrasonicSensor;  // ✅ ADD THIS
    LitterboxDHTSensor* dhtSensor;               // ✅ ADD THIS  
    LitterboxMQ2Sensor* mq2Sensor;              // ✅ ADD THIS
    LitterboxStepperMotor* litterboxMotor;
    FeederWeightSensor* weightSensor;            // ✅ ADD THIS
    FeederUltrasonicSensor1* feederUltrasonic1;
    FeederUltrasonicSensor2* feederUltrasonic2;
    FeederStepperMotor* feederMotor;
    WaterDispenserSensor* waterSensor;
    WaterDispenserPump* waterPump;
    WaterDispenserIRSensor* waterIRSensor;
    
    bool initialized;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 500; // ✅ ADD THIS

public:
    SensorManager(LitterboxUltrasonicSensor* litterboxUltrasonic,
                  LitterboxDHTSensor* litterboxDHT,
                  LitterboxMQ2Sensor* litterboxMQ2,
                  LitterboxStepperMotor* litterboxMotorPtr,
                  FeederWeightSensor* feederWeight,
                  FeederUltrasonicSensor1* feederUltrasonic1Ptr,
                  FeederUltrasonicSensor2* feederUltrasonic2Ptr,
                  FeederStepperMotor* feederMotorPtr,
                  WaterDispenserSensor* waterSensorPtr,
                  WaterDispenserPump* waterPumpPtr,
                  WaterDispenserIRSensor* waterIRSensorPtr);
    
    // 🔥 AGREGAR DECLARACIÓN DEL DESTRUCTOR:
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
    String getStorageFoodStatus();
    String getPlateFoodStatus();
    
    // ===== MÉTODOS DEL BEBEDERO =====
    String getWaterLevel();
    bool isWaterDetected();
    bool isCatDrinking();
    WaterDispenserPump* getWaterPump();
    WaterDispenserSensor* getWaterSensor();
    
    // ===== ESTADO DE SENSORES =====
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
    String getSensorStatus();
    String getAllReadings();
    void printAllSensorReadings();
};

#endif