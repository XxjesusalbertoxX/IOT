#ifndef WATER_DISPENSER_SENSOR_H
#define WATER_DISPENSER_SENSOR_H

#include <Arduino.h>

class WaterDispenserSensor {
private:
    static const int ANALOG_PIN = A1;
    static const unsigned long READ_INTERVAL = 300;
    
    float lastAnalogValue;
    unsigned long lastReadTime;
    bool sensorReady;
    
    // Umbrales para clasificar el nivel de agua
    static const int DRY_THRESHOLD = 100;
    static const int WET_THRESHOLD = 300;
    static const int FLOOD_THRESHOLD = 600;
    
public:
    WaterDispenserSensor();
    bool initialize();
    void update();
    float getAnalogValue();
    bool isWaterDetected();
    String getWaterLevel();
    bool isReady();
    String getStatus();
};

#endif
