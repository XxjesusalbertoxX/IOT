#include "LitterboxUltrasonicSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

LitterboxUltrasonicSensor::LitterboxUltrasonicSensor() : lastDistance(0), lastReadTime(0), sensorReady(false) {}

bool LitterboxUltrasonicSensor::initialize() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    // Hacer una lectura de prueba
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
    
    if (duration > 0) {
        sensorReady = true;
        lastDistance = (duration * 0.034) / 2; // Convertir a cm
        return true;
    }
    
    sensorReady = false;
    return false;
}

void LitterboxUltrasonicSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        // Enviar pulso
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        
        // Leer respuesta
        long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
        
        if (duration > 0) {
            lastDistance = (duration * 0.034) / 2; // Convertir a cm
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

bool LitterboxUltrasonicSensor::isCatInside() {
    // Si la distancia es menor al umbral, hay algo (gato) dentro
    return sensorReady && lastDistance > 0 && lastDistance < CAT_INSIDE_THRESHOLD_CM;
}


const char* LitterboxUltrasonicSensor::getSensorId() {
    return sensorId;
}

const char* LitterboxUltrasonicSensor::getDeviceId() {
    return deviceId;
}
