#ifndef LITTERBOX_MQ2_SENSOR_H
#define LITTERBOX_MQ2_SENSOR_H

#include <Arduino.h>

class LitterboxMQ2Sensor {
private:
    static const int ANALOG_PIN = A0;  // Pin analógico para el sensor MQ2
    static const unsigned long READ_INTERVAL = 500;
    
    float lastValue;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    LitterboxMQ2Sensor();
    bool initialize();
    void update();
    float getValue();
    bool isReady();
    String getStatus();
};

#endif
