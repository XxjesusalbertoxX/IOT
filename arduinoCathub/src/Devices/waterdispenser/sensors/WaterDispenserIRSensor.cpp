#include "WaterDispenserIRSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

WaterDispenserIRSensor::WaterDispenserIRSensor(const char* id, const char* deviceId) : 
    sensorId(id), deviceId(deviceId), objectDetected(false), lastState(false), lastReadTime(0), 
    detectionStartTime(0), sensorReady(false) {}

bool WaterDispenserIRSensor::initialize() {
    pinMode(IR_PIN, INPUT);
    
    // Leer estado inicial
    delay(100);
    bool initialReading = digitalRead(IR_PIN);
    
    // El sensor MH-B generalmente es LOW cuando detecta objeto
    lastState = initialReading;
    objectDetected = !initialReading; // Invertir lógica
    
    sensorReady = true;
    return true;
}

void WaterDispenserIRSensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        bool currentReading = digitalRead(IR_PIN);
        bool currentDetection = !currentReading; // Invertir: LOW = detectado
        
        // Debounce para evitar falsos positivos
        if (currentDetection != objectDetected) {
            if (now - lastReadTime >= DEBOUNCE_TIME) {
                lastState = objectDetected;
                objectDetected = currentDetection;
                
                // Marcar tiempo de inicio de detección
                if (objectDetected && !lastState) {
                    detectionStartTime = now;
                }
            }
        }
        
        lastReadTime = now;
    }
}

bool WaterDispenserIRSensor::isObjectDetected() {
    return objectDetected;
}

bool WaterDispenserIRSensor::hasStateChanged() {
    return (objectDetected != lastState);
}

unsigned long WaterDispenserIRSensor::getDetectionDuration() {
    if (objectDetected && detectionStartTime > 0) {
        return millis() - detectionStartTime;
    }
    return 0;
}

bool WaterDispenserIRSensor::isReady() {
    return sensorReady;
}

String WaterDispenserIRSensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    if (objectDetected) return "OBJECT_DETECTED";
    return "CLEAR";
}

const char* WaterDispenserIRSensor::getSensorId() {
    return sensorId;
}

const char* WaterDispenserIRSensor::getDeviceId() {
    return deviceId;
}
