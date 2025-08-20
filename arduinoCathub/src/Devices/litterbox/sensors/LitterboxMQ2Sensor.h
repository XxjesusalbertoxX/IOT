#ifndef LITTERBOX_MQ2_SENSOR_H
#define LITTERBOX_MQ2_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxMQ2Sensor {
private:
    static const int ANALOG_PIN = A0;
    static const unsigned long READ_INTERVAL = 500;

    const char* sensorId;
    const char* deviceId;

    float lastValue;        // Valor analógico crudo
    float lastPPM;          // Valor convertido a PPM
    unsigned long lastReadTime;
    bool sensorReady;
    
    // Función para convertir analógico a PPM
    float analogToPPM(int analogValue);


    
public:
    LitterboxMQ2Sensor(const char* id = SENSOR_ID_LITTER_MQ2, const char* deviceId = DEVICE_ID_LITTERBOX);
    bool initialize();
    void update();
    float getValue();       // Valor analógico crudo (0-1023)
    float getPPM();         // Valor convertido a PPM
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif