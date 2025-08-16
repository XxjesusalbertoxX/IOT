#ifndef LITTERBOX_ULTRASONIC_SENSOR_H
#define LITTERBOX_ULTRASONIC_SENSOR_H
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

#include <Arduino.h>

class LitterboxUltrasonicSensor {
private:
    static const int TRIG_PIN = 10;
    static const int ECHO_PIN = 11;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;

    const char* sensorId;
    const char* deviceId;

    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

    static constexpr float CAT_INSIDE_THRESHOLD_CM = 3.0; // Umbral para detectar gato dentro del arenero
    
public:
    LitterboxUltrasonicSensor();
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    bool isCatInside(); // Método para verificar si el gato está dentro
    const char* getSensorId();
    const char* getDeviceId();
};

#endif
