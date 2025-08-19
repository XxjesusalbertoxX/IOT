#ifndef WATER_DISPENSER_SENSOR_H
#define WATER_DISPENSER_SENSOR_H
#define DRY_THRESHOLD 100
#define WET_THRESHOLD 500
#define FLOOD_THRESHOLD 900

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class WaterDispenserSensor {
private:
    static const int ANALOG_PIN = A1;
    static const unsigned long READ_INTERVAL = 300;
    const char* sensorId;
    const char* deviceId;
    
    float lastAnalogValue;
    unsigned long lastReadTime;
    bool sensorReady;
public:
    WaterDispenserSensor(const char* id = SENSOR_ID_WATER_LEVEL, const char* deviceId = DEVICE_ID_WATER);
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