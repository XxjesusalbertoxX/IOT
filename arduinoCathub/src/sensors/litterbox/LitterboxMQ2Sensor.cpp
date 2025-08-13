#include "LitterboxMQ2Sensor.h"

LitterboxMQ2Sensor::LitterboxMQ2Sensor() : lastValue(0), lastReadTime(0), sensorReady(false) {}

bool LitterboxMQ2Sensor::initialize() {
    pinMode(ANALOG_PIN, INPUT);
    sensorReady = true;
    return true;
}

void LitterboxMQ2Sensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        lastValue = analogRead(ANALOG_PIN);
        lastReadTime = now;
    }
}

float LitterboxMQ2Sensor::getValue() {
    return lastValue;
}

bool LitterboxMQ2Sensor::isReady() {
    return sensorReady;
}

String LitterboxMQ2Sensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}
