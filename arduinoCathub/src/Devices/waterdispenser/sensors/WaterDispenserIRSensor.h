#ifndef WATER_DISPENSER_IR_SENSOR_H
#define WATER_DISPENSER_IR_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class WaterDispenserIRSensor {
private:
    static const int IR_PIN = 9;  // Pin digital para el sensor infrarrojo
    static const unsigned long READ_INTERVAL = 100; // Lectura rápida para detección

    const char* sensorId;
    const char* deviceId;

    bool objectDetected;
    bool lastState;
    unsigned long lastReadTime;
    unsigned long detectionStartTime;
    bool sensorReady;
    
    
    // Para evitar falsos positivos
    static const unsigned long DEBOUNCE_TIME = 50;
    
public:
    WaterDispenserIRSensor(const char* id = SENSOR_ID_WATER_IR, const char* deviceId = DEVICE_ID_WATER);
    bool initialize();
    void update();
    bool isObjectDetected();
    bool hasStateChanged();
    unsigned long getDetectionDuration();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif