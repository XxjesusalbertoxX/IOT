#pragma once
#include <Arduino.h>

// Comedero
#include "feeder/FeederWeightSensor.h"
#include "feeder/FeederUltrasonicSensor.h"
#include "feeder/FeederStepperMotor.h"

// Bebedero
#include "waterdispenser/WaterDispenserSensor.h"
#include "waterdispenser/WaterDispenserIRSensor.h"
#include "waterdispenser/WaterDispenserPump.h"

// Arenero
#include "litterbox/LitterboxDHTSensor.h"
#include "litterbox/LitterboxUltrasonicSensor.h"
#include "litterbox/LitterboxMQ2Sensor.h"
#include "litterbox/LitterboxStepperMotor.h"

class SensorManager {
public:
    explicit SensorManager();
    void begin();
    void poll();
    
    // Comedero
    FeederWeightSensor* getFeederWeight();
    FeederUltrasonicSensor1* getFeederUltrasonic1();
    FeederUltrasonicSensor2* getFeederUltrasonic2();
    FeederStepperMotor* getFeederMotor();
    
    // Bebedero
    WaterDispenserSensor* getWaterDispenser();
    WaterDispenserIRSensor* getWaterDispenserIR();
    WaterDispenserPump* getWaterDispenserPump();
    
    // Arenero
    LitterboxDHTSensor* getLitterboxDHT();
    LitterboxUltrasonicSensor* getLitterboxUltrasonic();
    LitterboxMQ2Sensor* getLitterboxMQ2();
    LitterboxStepperMotor* getLitterboxMotor();

private:
    // Comedero
    FeederWeightSensor* feederWeight_;
    FeederUltrasonicSensor1* feederUltrasonic1_;
    FeederUltrasonicSensor2* feederUltrasonic2_;
    FeederStepperMotor* feederMotor_;
    
    // Bebedero
    WaterDispenserSensor* waterDispenser_;
    WaterDispenserIRSensor* waterDispenserIR_;
    WaterDispenserPump* waterDispenserPump_;
    
    // Arenero
    LitterboxDHTSensor* litterboxDHT_;
    LitterboxUltrasonicSensor* litterboxUltrasonic_;
    LitterboxMQ2Sensor* litterboxMQ2_;
    LitterboxStepperMotor* litterboxMotor_;
};