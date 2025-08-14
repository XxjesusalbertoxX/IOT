#ifndef WATER_DISPENSER_PUMP_H
#define WATER_DISPENSER_PUMP_H

#include <Arduino.h>

class WaterDispenserPump {
private:
    static const int PUMP_PIN = 18;  // Pin fijo para el MOSFET
    static const int PUMP_POWER = 255;
    static const unsigned long MAX_PUMP_TIME = 10000;
    
    bool pumpEnabled;
    bool pumpRunning;
    bool pumpReady;
    unsigned long pumpStartTime;
    unsigned long pumpDuration;
    int currentPower;
    
public:
    WaterDispenserPump();
    bool initialize();
    void turnOn(unsigned long duration = 3000);
    void turnOff();
    void setPower(int power);
    bool isPumpRunning();
    bool isReady();
    unsigned long getRemainingTime();
    void update();
    String getStatus();
    void emergencyStop();
};

#endif