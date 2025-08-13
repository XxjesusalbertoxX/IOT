#ifndef FEEDER_WEIGHT_SENSOR_H
#define FEEDER_WEIGHT_SENSOR_H

#include <Arduino.h>
#include "HX711.h"

class FeederWeightSensor {
private:
    static const int DOUT_PIN = 3;
    static const int SCK_PIN = 2;
    static const float CALIBRATION_FACTOR;
    static const unsigned long READ_INTERVAL = 500;
    
    HX711 scale;
    float currentWeight;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederWeightSensor();
    bool initialize();
    void update();
    float getCurrentWeight();
    bool isReady();
    void tare();
    void calibrate(float knownWeight);
    String getStatus();
};

#endif
