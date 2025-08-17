#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// Ultrasónico 1 del comedero
class FeederUltrasonicSensor1 {
private:
    static const int TRIG_PIN = 4;
    static const int ECHO_PIN = 5;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    const char* sensorId;
    const char* deviceId;

    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor1(const char* id, const char* deviceId);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getDeviceId();
    const char* getSensorId();
};

// Ultrasónico 2 del comedero
class FeederUltrasonicSensor2 {
private:
    static const int TRIG_PIN = 6;
    static const int ECHO_PIN = 7;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    const char* sensorId;
    const char* deviceId;

    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor2(const char* id, const char* deviceId);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getDeviceId();
    const char* getSensorId();
};

#endif