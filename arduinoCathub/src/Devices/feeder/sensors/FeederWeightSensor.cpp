// FeederWeightSensor.cpp
#include "FeederWeightSensor.h"

const float FeederWeightSensor::CALIBRATION_FACTOR = 422.0;

FeederWeightSensor::FeederWeightSensor(const char* id, const char* devId) 
    : sensorId(id), deviceId(devId), currentWeight(0), lastReadTime(0), sensorReady(false) {
}

bool FeederWeightSensor::initialize() {
    scale.begin(DOUT_PIN, SCK_PIN);
    
    if (scale.is_ready()) {
        scale.set_scale(CALIBRATION_FACTOR);
        scale.tare(); // Reset the scale to 0
        sensorReady = true;
        return true;
    }
    
    sensorReady = false;
    return false;
}

void FeederWeightSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL && scale.is_ready()) {
        currentWeight = scale.get_units(10); // Promedio de 10 lecturas
        lastReadTime = now;
    }
}

float FeederWeightSensor::getCurrentWeight() {
    if (scale.is_ready()) {
        return scale.get_units(5); // Lectura inmediata con 5 muestras
    }
    return currentWeight; // Devolver último valor conocido
}

bool FeederWeightSensor::isReady() {
    return sensorReady && scale.is_ready();
}

void FeederWeightSensor::tare() {
    if (scale.is_ready()) {
        scale.tare();
        currentWeight = 0.0;
    }
}

void FeederWeightSensor::calibrate(float knownWeight) {
    if (scale.is_ready() && knownWeight > 0) {
        float reading = scale.get_units(10);
        float newFactor = reading / knownWeight;
        scale.set_scale(newFactor);
    }
}

String FeederWeightSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    if (!scale.is_ready()) return "NOT_READY";
    return "READY";
}

const char* FeederWeightSensor::getSensorId() {
    return sensorId;
}

const char* FeederWeightSensor::getDeviceId() {
    return deviceId;
}