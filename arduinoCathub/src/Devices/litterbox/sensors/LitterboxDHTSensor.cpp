#include "LitterboxDHTSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

LitterboxDHTSensor::LitterboxDHTSensor() : dht(DATA_PIN, DHT_TYPE), lastTemperature(0), lastHumidity(0), lastReadTime(0), sensorReady(false) {}

bool LitterboxDHTSensor::initialize() {
    dht.begin();
    
    // Esperar un momento y hacer una lectura de prueba
    delay(2000);
    float testTemp = dht.readTemperature();
    float testHum = dht.readHumidity();
    
    if (!isnan(testTemp) && !isnan(testHum)) {
        sensorReady = true;
        lastTemperature = testTemp;
        lastHumidity = testHum;
        return true;
    }
    
    sensorReady = false;
    return false;
}

void LitterboxDHTSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();
        
        // Solo actualizar si las lecturas son válidas
        if (!isnan(temp) && !isnan(hum)) {
            lastTemperature = temp;
            lastHumidity = hum;
        }
        
        lastReadTime = now;
    }
}

float LitterboxDHTSensor::getTemperature() {
    return lastTemperature;
}

float LitterboxDHTSensor::getHumidity() {
    return lastHumidity;
}

bool LitterboxDHTSensor::isReady() {
    return sensorReady;
}

String LitterboxDHTSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    
    // Verificar si las últimas lecturas son válidas
    if (isnan(lastTemperature) || isnan(lastHumidity)) {
        return "READ_ERROR";
    }
    
    return "READY";
}

const char* LitterboxMQ2Sensor::getSensorId() {
    return sensorId;
}

const char* LitterboxMQ2Sensor::getDeviceId() {
    return deviceId;
}
