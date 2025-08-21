#include "LitterboxDHTSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// IMPORTANTE: inicializar sensorId y deviceId en la lista de inicialización
LitterboxDHTSensor::LitterboxDHTSensor(const char* id, const char* deviceId)
    : sensorId(id),
      deviceId(deviceId),
      dht(DATA_PIN, DHT_TYPE),
      lastTemperature(0.0f),
      lastHumidity(0.0f),
      lastReadTime(0),
      sensorReady(false) {
}

bool LitterboxDHTSensor::initialize() {
    dht.begin();

    // Esperar un momento y hacer una lectura de prueba
    delay(2000);
    float testTemp = dht.readTemperature();
    float testHum  = dht.readHumidity();

    if (!isnan(testTemp) && !isnan(testHum)) {
        sensorReady = true;
        lastTemperature = testTemp;
        lastHumidity = testHum;
        Serial.println("{\"dht\":\"INITIALIZED\",\"temp\":" + String(testTemp) + ",\"hum\":" + String(testHum) + "}");
        return true;
    }

    sensorReady = false;
    Serial.println("{\"dht\":\"INITIALIZE_FAILED\"}");
    return false;
}

void LitterboxDHTSensor::update() {
    if (!sensorReady) return;

    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        float temp = dht.readTemperature();
        float hum  = dht.readHumidity();

        // Solo actualizar si las lecturas son válidas
        if (!isnan(temp) && !isnan(hum)) {
            lastTemperature = temp;
            lastHumidity = hum;
        } else {
            // Mantener últimos valores válidos, y reportar en serial
            Serial.println("{\"dht\":\"READ_ERROR\",\"temp\":" + String(temp) + ",\"hum\":" + String(hum) + "}");
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

    if (isnan(lastTemperature) || isnan(lastHumidity)) {
        return "READ_ERROR";
    }

    return "READY";
}

const char* LitterboxDHTSensor::getSensorId() {
    return sensorId ? sensorId : "UNCONFIGURED";
}

const char* LitterboxDHTSensor::getDeviceId() {
    return deviceId ? deviceId : "UNCONFIGURED";
}
