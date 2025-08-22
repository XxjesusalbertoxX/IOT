#include "LitterboxDHTSensor.h"
#include <math.h>

LitterboxDHTSensor::LitterboxDHTSensor(const char* id, const char* deviceId)
    : sensorId(id),
      deviceId(deviceId),
      dht(DATA_PIN, DHT_TYPE),
      lastTemperature(NAN),
      lastHumidity(NAN),
      lastReadTime(0),
      sensorReady(false),
      lastReadValid(false) {
}

bool LitterboxDHTSensor::initialize() {
    dht.begin();

    // Intentar varias lecturas para estabilizar
    const int ATTEMPTS = 5;
    for (int i = 0; i < ATTEMPTS; ++i) {
        delay(2000); // DHT requiere ~2s entre lecturas
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        if (!isnan(t) && !isnan(h)) {
            lastTemperature = t;
            lastHumidity = h;
            lastReadTime = millis();
            sensorReady = true;
            lastReadValid = true;
            // Serial.println("{\"dht\":\"INITIALIZED\",\"temp\":" + String(t,2) + ",\"hum\":" + String(h,2) + "}");
            return true;
        }
        // si falla, seguir intentando
    }

    sensorReady = false;
    lastReadValid = false;
    // Serial.println("{\"dht\":\"INITIALIZE_FAILED\"}");
    return false;
}

void LitterboxDHTSensor::update() {
    if (!sensorReady) return;

    unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) return;

    // Intentar hasta N lecturas rápidas para evitar NAN transitorio
    const int RETRIES = 3;
    float t = NAN, h = NAN;
    for (int i = 0; i < RETRIES; ++i) {
        t = dht.readTemperature();
        h = dht.readHumidity();
        if (!isnan(t) && !isnan(h)) break;
        delay(200); // pequeño retardo entre reintentos
    }

    if (!isnan(t) && !isnan(h)) {
        lastTemperature = t;
        lastHumidity = h;
        lastReadValid = true;
        // log opcional mínimo:
        // Serial.println("{\"dht\":\"READ\",\"temp\":" + String(t,2) + ",\"hum\":" + String(h,2) + "}");
    } else {
        // Mantener últimos valores válidos (si existen); marcar lectura inválida
        lastReadValid = false;
        // Serial.println("{\"dht\":\"READ_ERROR\"}");
    }

    lastReadTime = now;
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
    if (!lastReadValid) return "READ_ERROR";
    return "READY";
}

const char* LitterboxDHTSensor::getSensorId() {
    return sensorId ? sensorId : "UNCONFIGURED";
}

const char* LitterboxDHTSensor::getDeviceId() {
    return deviceId ? deviceId : "UNCONFIGURED";
}
