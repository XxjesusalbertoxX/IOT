#ifndef LITTERBOX_DHT_SENSOR_H
#define LITTERBOX_DHT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

class LitterboxDHTSensor {
private:
    static const int DATA_PIN = 21;        // Pin digital para el sensor DHT
    static const int DHT_TYPE = DHT22;    // Tipo de sensor (DHT22 para RQ-S003)
    static const unsigned long READ_INTERVAL = 2000; // DHT necesita al menos 2 segundos entre lecturas
    
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
};

#endif
