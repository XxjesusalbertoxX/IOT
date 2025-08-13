#ifndef WATER_DISPENSER_IR_SENSOR_H
#define WATER_DISPENSER_IR_SENSOR_H

#include <Arduino.h>

class WaterDispenserIRSensor {
private:
    static const int IR_PIN = 9;  // Pin digital para el sensor infrarrojo
    static const unsigned long READ_INTERVAL = 100; // Lectura rápida para detección
    
    bool objectDetected;
    bool lastState;
    unsigned long lastReadTime;
    unsigned long detectionStartTime;
    bool sensorReady;
    
    // Para evitar falsos positivos
    static const unsigned long DEBOUNCE_TIME = 50;
    
public:
    WaterDispenserIRSensor();
    bool initialize();
    void update();
    bool isObjectDetected();
    bool hasStateChanged();
    unsigned long getDetectionDuration();
    bool isReady();
    String getStatus();
};

#endif