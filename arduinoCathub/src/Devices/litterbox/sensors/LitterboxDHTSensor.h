#ifndef LITTERBOX_DHT_SENSOR_H
#define LITTERBOX_DHT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxDHTSensor {
private:
    static const int DATA_PIN = 21;        // Pin digital para el sensor DHT
    static const int DHT_TYPE = DHT22;    // Tipo de sensor (DHT22 para RQ-S003)
    static const unsigned long READ_INTERVAL = 2000; // DHT necesita al menos 2 segundos entre lecturas

    const char* sensorId;
    const char* deviceId;

    DHT dht;
    float lastTemperature;
    float lastHumidity;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    LitterboxDHTSensor();
    bool initialize();
    void update();
    float getTemperature();
    float getHumidity();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif
