// WaterDispenserPump.h
#ifndef WATER_DISPENSER_PUMP_H
#define WATER_DISPENSER_PUMP_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class WaterDispenserPump {
private:
    static const int PUMP_PIN = 18;  // Pin digital (NO PWM)
    static const int PUMP_POWER = 1;  // 🔥 Cambiar a 1 (solo HIGH/LOW)
    static const unsigned long MAX_PUMP_TIME = 10000;
    
    const char* actuatorId;
    const char* deviceId;
    bool pumpEnabled;
    bool pumpRunning;
    bool pumpReady;
    unsigned long pumpStartTime;
    unsigned long pumpDuration;
    int currentPower;  // Solo para compatibilidad (0 = LOW, >0 = HIGH)
    
public:
    WaterDispenserPump(const char* id = ACTUATOR_WATERDISPENSER_PUMP_ID_1, const char* devId = DEVICE_ID_WATER);
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
    const char* getActuatorId();
    const char* getDeviceId();
};

#endif