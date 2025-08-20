#ifndef WATER_DISPENSER_PUMP_H
#define WATER_DISPENSER_PUMP_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class WaterDispenserPump {
private:
    static const int PUMP_PIN = 18;  // Pin fijo para el MOSFET
    static const int PUMP_POWER = 255;
    static const unsigned long MAX_PUMP_TIME = 10000;
    
    const char* actuatorId;
    const char* deviceId;
    bool pumpEnabled;
    bool pumpRunning;
    bool pumpReady;
    unsigned long pumpStartTime;
    unsigned long pumpDuration;
    int currentPower;
    
public:
    WaterDispenserPump(const char* id = ACTUATOR_WATERDISPENSER_PUMP_ID_1, const char* devId = DEVICE_ID_WATER);
    bool initialize();
    void turnOn(unsigned long duration = 3000);  // Solo para auto-llenado
    void turnOff();
    void setPower(int power);    
    bool isPumpRunning();
    bool isReady();
    unsigned long getRemainingTime();
    void update();
    String getStatus();
    void emergencyStop();
    const char* getActuatorId();
    const char* getDeviceId();
};

#endif