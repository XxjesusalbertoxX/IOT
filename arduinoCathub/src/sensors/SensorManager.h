#pragma once
#include <Arduino.h>

// Comedero
#include "feeder/FeederWeightSensor.h"
#include "feeder/FeederUltrasonicSensor.h"

// Bebedero
#include "waterdispenser/WaterDispenserSensor.h"

// Arenero
#include "litterbox/LitterboxDHTSensor.h"
#include "litterbox/LitterboxUltrasonicSensor.h"
#include "litterbox/LitterboxMQ2Sensor.h"

class SensorManager {
public:
    explicit SensorManager();
    void begin();
    void poll();
    
    // Comedero
    FeederWeightSensor* getFeederWeight();
    FeederUltrasonicSensor* getFeederUltrasonic1();
    FeederUltrasonicSensor* getFeederUltrasonic2();
    
    // Bebedero
    WaterDispenserSensor* getWaterDispenser();
    
    // Arenero
    LitterboxDHTSensor* getLitterboxDHT();
    LitterboxUltrasonicSensor* getLitterboxUltrasonic();
    LitterboxMQ2Sensor* getLitterboxMQ2();

private:
    // Comedero
    FeederWeightSensor* feederWeight_;
    FeederUltrasonicSensor* feederUltrasonic1_;
    FeederUltrasonicSensor* feederUltrasonic2_;
    
    // Bebedero
    WaterDispenserSensor* waterDispenser_;
    
    // Arenero
    LitterboxDHTSensor* litterboxDHT_;
    LitterboxUltrasonicSensor* litterboxUltrasonic_;
    LitterboxMQ2Sensor* litterboxMQ2_;
};