#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class FeederUltrasonicSensor {
private:
    static const int TRIGGER_PIN_1 = 9;   // Pin para instancia 1
    static const int ECHO_PIN_1 = 10;     // Pin para instancia 1
    static const int TRIGGER_PIN_2 = 11;  // Pin para instancia 2
    static const int ECHO_PIN_2 = 12;     // Pin para instancia 2
    static const unsigned long TIMEOUT_US = 25000; // 25ms = ~4m máximo
    static const unsigned long READ_INTERVAL = 100;
    
    const char* sensorId;
    const char* deviceId;
    int triggerPin;
    int echoPin;
    bool isFirstInstance;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    // Constructor modificado para soportar múltiples instancias
    FeederUltrasonicSensor(bool isFirstSensor = true, 
                           const char* id = nullptr,
                           const char* deviceId = nullptr);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    const char* getSensorId();
    const char* getDeviceId();
    String getStatus();
};

#endif