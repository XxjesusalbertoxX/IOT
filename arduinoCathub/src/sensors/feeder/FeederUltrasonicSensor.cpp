#include "FeederUltrasonicSensor.h"

FeederUltrasonicSensor::FeederUltrasonicSensor(int triggerPin, int echoPin, String name) 
    : trigPin(triggerPin), echoPin(echoPin), sensorName(name), lastDistance(0), lastReadTime(0), sensorReady(false) {}

bool FeederUltrasonicSensor::initialize() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    // Hacer una lectura de prueba
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, TIMEOUT_US);
    
    if (duration > 0) {
        sensorReady = true;
        lastDistance = (duration * 0.034) / 2; // Convertir a cm
        return true;
    }
    
    sensorReady = false;
    return false;
}

void FeederUltrasonicSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        // Enviar pulso
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        // Leer respuesta
        long duration = pulseIn(echoPin, HIGH, TIMEOUT_US);
        
        if (duration > 0) {
            lastDistance = (duration * 0.034) / 2; // Convertir a cm
        }
        // Si duration == 0, mantener el último valor válido
        
        lastReadTime = now;
    }
}

float FeederUltrasonicSensor::getDistance() {
    return lastDistance;
}

bool FeederUltrasonicSensor::isReady() {
    return sensorReady;
}

String FeederUltrasonicSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}

String FeederUltrasonicSensor::getName() {
    return sensorName;
}
