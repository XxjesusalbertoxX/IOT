#ifndef LITTERBOX_ULTRASONIC_SENSOR_H
#define LITTERBOX_ULTRASONIC_SENSOR_H

#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"
#include <Arduino.h>

class LitterboxUltrasonicSensor {
private:
    static const int TRIG_PIN = 10;
    static const int ECHO_PIN = 11;
    static const unsigned long READ_INTERVAL = 100; // ms entre lecturas
    static const long TIMEOUT_US = 30000;           // timeout para pulseIn en microsegundos

    const char* sensorId;
    const char* deviceId;

    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

    // Umbrales
    static constexpr float DETECTION_THRESHOLD_CM = 15.0f; // presencia general
    static constexpr float BLOCK_THRESHOLD_CM     = 10.0f; // bloqueo (gato dentro)

public:
    LitterboxUltrasonicSensor(const char* id = SENSOR_ID_LITTER_ULTRA,
                              const char* deviceId = DEVICE_ID_LITTERBOX);
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();

    // Métodos útiles
    bool isObjectDetected();   // <= DETECTION_THRESHOLD_CM
    bool isCatBlocking();      // <= BLOCK_THRESHOLD_CM

    const char* getSensorId();
    const char* getDeviceId();
};

#endif // LITTERBOX_ULTRASONIC_SENSOR_H
