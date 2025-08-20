#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// Sensor para detectar presencia del gato
class FeederUltrasonicSensor1 {
private:
    static const int TRIG_PIN = 4;   // Pin trigger para sensor 1
    static const int ECHO_PIN = 5;  // Pin echo para sensor 1
    static const unsigned long READ_INTERVAL = 100;
    static const unsigned long TIMEOUT_US = 25000; // 25ms = ~4m máximo
    
    const char* sensorId;
    const char* deviceId;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor1(const char* id = SENSOR_ID_FEEDER_SONIC1, const char* deviceId = DEVICE_ID_FEEDER);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

// Sensor para medir nivel de comida en dispensador
class FeederUltrasonicSensor2 {
private:
    static const int TRIG_PIN = 6;  // Pin trigger para sensor 2
    static const int ECHO_PIN = 7;  // Pin echo para sensor 2
    static const unsigned long READ_INTERVAL = 100;
    static const unsigned long TIMEOUT_US = 25000; // 25ms = ~4m máximo
    
    const char* sensorId;
    const char* deviceId;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor2(const char* id = SENSOR_ID_FEEDER_SONIC2, const char* deviceId = DEVICE_ID_FEEDER);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif