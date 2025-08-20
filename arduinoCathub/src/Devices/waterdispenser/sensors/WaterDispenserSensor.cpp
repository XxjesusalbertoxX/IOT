// WaterDispenserSensor.cpp
#include "WaterDispenserSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

WaterDispenserSensor::WaterDispenserSensor(const char* id, const char* deviceId) : sensorId(id), deviceId(deviceId), lastAnalogValue(0), lastReadTime(0), sensorReady(false) {}

bool WaterDispenserSensor::initialize() {
    pinMode(ANALOG_PIN, INPUT);
    
    // Hacer una lectura inicial para verificar que el sensor está conectado
    delay(100);
    float testReading = analogRead(ANALOG_PIN);
    
    // El sensor debería dar alguna lectura válida (0-1023)
    if (testReading >= 0 && testReading <= 1023) {
        sensorReady = true;
        lastAnalogValue = testReading;
        return true;
    }
    
    sensorReady = false;
    return false;
}

void WaterDispenserSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        lastAnalogValue = analogRead(ANALOG_PIN);
        lastReadTime = now;
    }
}

float WaterDispenserSensor::getAnalogValue() {
    return lastAnalogValue;
}

bool WaterDispenserSensor::isWaterDetected() {
    return lastAnalogValue > DRY_THRESHOLD;
}

String WaterDispenserSensor::getWaterLevel() {
    if (lastAnalogValue < DRY_THRESHOLD) {
        return "DRY";           // Sin agua - BOMBA ON
    } else if (lastAnalogValue < WET_THRESHOLD) {
        return "LOW";           // Poco agua - BOMBA ON
    } else if (lastAnalogValue < FLOOD_THRESHOLD) {
        return "WET";           // Agua suficiente - BOMBA ON aún
    } else {
        return "FLOOD";         // Lleno al máximo - BOMBA OFF
    }
}

bool WaterDispenserSensor::isReady() {
    return sensorReady;
}

String WaterDispenserSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}

const char* WaterDispenserSensor::getSensorId() {
    return sensorId;
}

const char* WaterDispenserSensor::getDeviceId() {
    return deviceId;
}
