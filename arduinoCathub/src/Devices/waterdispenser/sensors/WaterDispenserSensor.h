#ifndef WATER_DISPENSER_SENSOR_H
#define WATER_DISPENSER_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class WaterDispenserSensor {
private:
    static const int ANALOG_PIN = A1;
    static const unsigned long READ_INTERVAL = 300;

    const char* sensorId;
    const char* deviceId;

public:
    WaterDispenserSensor();
    bool initialize();
    void update();
    float getAnalogValue();
    bool isWaterDetected();
    String getWaterLevel();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif