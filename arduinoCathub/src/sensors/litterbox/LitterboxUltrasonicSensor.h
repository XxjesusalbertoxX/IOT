#ifndef LITTERBOX_ULTRASONIC_SENSOR_H
#define LITTERBOX_ULTRASONIC_SENSOR_H

#include <Arduino.h>

class LitterboxUltrasonicSensor {
private:
    static const int TRIG_PIN = 10;
    static const int ECHO_PIN = 11;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    LitterboxUltrasonicSensor();
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
};

#endif
