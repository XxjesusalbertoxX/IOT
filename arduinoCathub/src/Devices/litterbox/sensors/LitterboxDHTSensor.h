#ifndef LITTERBOX_DHT_SENSOR_H
#define LITTERBOX_DHT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxDHTSensor {
private:
    static const int DATA_PIN = 21;        // Pin digital para el sensor DHT
    static const int DHT_TYPE = DHT11;     // Cambia a DHT22 si usas ese
    static const unsigned long READ_INTERVAL = 2000; // 2s entre lecturas

    const char* sensorId;
    const char* deviceId;

    DHT dht;
    float lastTemperature;
    float lastHumidity;
    unsigned long lastReadTime;
    bool sensorReady;
    bool lastReadValid;

public:
    LitterboxDHTSensor(const char* id = SENSOR_ID_LITTER_DHT,
                       const char* deviceId = DEVICE_ID_LITTERBOX);
    bool initialize();
    void update();
    float getTemperature();   // puede devolver NAN si no hay lectura válida
    float getHumidity();      // puede devolver NAN si no hay lectura válida
    bool isReady();           // si se pudo inicializar (sensor detectado)
    String getStatus();       // READY | NOT_INITIALIZED | READ_ERROR
    const char* getSensorId();
    const char* getDeviceId();
};

#endif
