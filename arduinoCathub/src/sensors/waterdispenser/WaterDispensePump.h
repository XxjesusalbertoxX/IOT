#ifndef WATER_DISPENSER_PUMP_H
#define WATER_DISPENSER_PUMP_H

#include <Arduino.h>

class WaterDispenserPump {
private:
    static const int PUMP_PIN = 18;  // Pin PWM para controlar el MOSFET
    static const int PUMP_POWER = 255; // Potencia máxima (0-255)
    static const unsigned long MAX_PUMP_TIME = 10000; // Máximo 10 segundos continuo
    
    bool pumpEnabled;
    bool pumpRunning;
    bool pumpReady;
    unsigned long pumpStartTime;
    unsigned long pumpDuration;
    int currentPower;
    
public:
    WaterDispenserPump();
    bool initialize();
    void turnOn(unsigned long duration = 3000);  // 3 segundos por defecto
    void turnOff();
    void setPower(int power); // 0-255
    bool isPumpRunning();
    bool isReady();
    unsigned long getRemainingTime();
    void update(); // Para auto-apagar después del tiempo
    String getStatus();
    void emergencyStop();
};

#endif