#include "FeederUltrasonicSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// ===== IMPLEMENTACIÓN DE FeederUltrasonicSensor1 =====
FeederUltrasonicSensor1::FeederUltrasonicSensor1(const char* id, const char* deviceId) 
    : sensorId(id), deviceId(deviceId), lastDistance(0.0), lastReadTime(0), sensorReady(false) {}

bool FeederUltrasonicSensor1::initialize() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
    
    if (duration > 0) {
        sensorReady = true;
        lastDistance = (duration * 0.034) / 2;
        return true;
    } else {
        sensorReady = false;
        return false;
    }
}

void FeederUltrasonicSensor1::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        
        long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
        
        if (duration > 0) {
            lastDistance = (duration * 0.034) / 2;
        }
        
        lastReadTime = now;
    }
}

float FeederUltrasonicSensor1::getDistance() {
    return lastDistance;
}

bool FeederUltrasonicSensor1::isReady() {
    return sensorReady;
}

String FeederUltrasonicSensor1::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}
const char* FeederUltrasonicSensor1::getSensorId() {
    if (sensorId == nullptr) {
        return "UNCONFIGURED";
    }
    return sensorId;
}

const char* FeederUltrasonicSensor1::getDeviceId() {
    if (deviceId == nullptr) {
        return "UNCONFIGURED";
    }
    return deviceId;
}

// ===== IMPLEMENTACIÓN DE FeederUltrasonicSensor2 =====
FeederUltrasonicSensor2::FeederUltrasonicSensor2(const char* id, const char* deviceId) 
    : sensorId(id), deviceId(deviceId), lastDistance(0.0), lastReadTime(0), sensorReady(false) {}

bool FeederUltrasonicSensor2::initialize() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
    
    if (duration > 0) {
        sensorReady = true;
        lastDistance = (duration * 0.034) / 2;
        return true;
    } else {
        sensorReady = false;
        return false;
    }
}

void FeederUltrasonicSensor2::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        
        long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
        
        if (duration > 0) {
            lastDistance = (duration * 0.034) / 2;
        }
        
        lastReadTime = now;
    }
}

float FeederUltrasonicSensor2::getDistance() {
    return lastDistance;
}

bool FeederUltrasonicSensor2::isReady() {
    return sensorReady;
}

String FeederUltrasonicSensor2::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}

const char* FeederUltrasonicSensor2::getSensorId() {
    if (sensorId == nullptr) {
        return "UNCONFIGURED";
    }
    return sensorId;
}

const char* FeederUltrasonicSensor2::getDeviceId() {
    if (deviceId == nullptr) {
        return "UNCONFIGURED";
    }
    return deviceId;
}