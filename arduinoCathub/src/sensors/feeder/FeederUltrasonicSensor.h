#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>

class FeederUltrasonicSensor {
private:
    int trigPin;
    int echoPin;
    String sensorName;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor(int triggerPin, int echoPin, String name);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    String getName();
};

#endif
