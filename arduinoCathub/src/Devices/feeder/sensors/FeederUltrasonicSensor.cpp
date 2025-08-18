#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>

// Sensor 1: Detección de gato
class FeederUltrasonicSensor1 {
private:
    static const int TRIG_PIN = 9;
    static const int ECHO_PIN = 10;
    static const unsigned long READ_INTERVAL = 100;
    static const unsigned long TIMEOUT_US = 25000;
    
    const char* sensorId;
    const char* deviceId;
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor1(const char* id = nullptr, const char* deviceId = nullptr);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

// Sensor 2: Nivel de comida
class FeederUltrasonicSensor2 {
private:
    static const int TRIG_PIN = 11;
    static const int ECHO_PIN = 12;
    static const unsigned long READ_INTERVAL = 100;
    static const unsigned long TIMEOUT_US = 25000;
    
    const char* sensorId;
    const char* deviceId;
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor2(const char* id = nullptr, const char* deviceId = nullptr);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif