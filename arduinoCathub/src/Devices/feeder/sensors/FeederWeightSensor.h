#ifndef FEEDER_WEIGHT_SENSOR_H
#define FEEDER_WEIGHT_SENSOR_H

#include <Arduino.h>
#include "HX711.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class FeederWeightSensor {
private:
    static const int DOUT_PIN = 3;
    static const int SCK_PIN = 2;
    static const float CALIBRATION_FACTOR;
    static const unsigned long READ_INTERVAL = 500;
    const char* sensorId;
    const char* deviceId;
    
    HX711 scale;
    float currentWeight;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederWeightSensor(const char* id = FEEDER_WEIGHT_SENSOR_ID, const char* deviceId = DEVICE_ID_FEEDER);
    bool initialize();
    void update();
    float getCurrentWeight();
    bool isReady();
    void tare();
    void calibrate(float knownWeight);
    const char* getSensorId();
    const char* getDeviceId();
    String getStatus();
};

#endif
