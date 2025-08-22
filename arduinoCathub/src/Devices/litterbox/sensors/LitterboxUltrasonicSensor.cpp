#include "LitterboxUltrasonicSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

LitterboxUltrasonicSensor::LitterboxUltrasonicSensor(const char* id, const char* deviceId)
    : sensorId(id),
      deviceId(deviceId),
      lastDistance(-1.0f),   // -1 indica "sin lectura válida aún"
      lastReadTime(0),
      sensorReady(false) {
}

bool LitterboxUltrasonicSensor::initialize() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Test de pulso
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
    if (duration > 0) {
        sensorReady = true;
        lastDistance = (duration * 0.034) / 2.0;
        lastReadTime = millis();
        // Serial.println("{\"ultrasonic\":\"INITIALIZED\",\"distance_cm\":" + String(lastDistance) + "}");
        return true;
    }

    sensorReady = false;
    // Serial.println("{\"ultrasonic\":\"INITIALIZE_FAILED\"}");
    return false;
}

void LitterboxUltrasonicSensor::update() {
    if (!sensorReady) return;

    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        // Trigger pulse
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
        if (duration > 0) {
            lastDistance = (duration * 0.034) / 2.0;
        } else {
            // No eco: mantenemos la última lectura válida (puedes elegir setear -1.0 si prefieres)
            // lastDistance = -1.0f;
        }

        lastReadTime = now;
    }
}

float LitterboxUltrasonicSensor::getDistance() {
    return lastDistance;
}

bool LitterboxUltrasonicSensor::isReady() {
    return sensorReady;
}

String LitterboxUltrasonicSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}

bool LitterboxUltrasonicSensor::isObjectDetected() {
    return sensorReady && lastDistance > 0.0f && lastDistance <= DETECTION_THRESHOLD_CM;
}

bool LitterboxUltrasonicSensor::isCatBlocking() {
    return sensorReady && lastDistance > 0.0f && lastDistance <= BLOCK_THRESHOLD_CM;
}

const char* LitterboxUltrasonicSensor::getSensorId() {
    return sensorId ? sensorId : "UNCONFIGURED";
}

const char* LitterboxUltrasonicSensor::getDeviceId() {
    return deviceId ? deviceId : "UNCONFIGURED";
}
