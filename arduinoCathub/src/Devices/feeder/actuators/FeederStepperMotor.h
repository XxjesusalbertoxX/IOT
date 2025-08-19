#ifndef FEEDER_STEPPER_MOTOR_H
#define FEEDER_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class FeederStepperMotor {
private:
    static const int DIR_PIN = 13;   // Dirección
    static const int EN_PIN = 14;    // Enable (activo LOW)
    static const int PULL_PIN = 12;  // Pulsos (Step)
    
    static const unsigned long STEP_DELAY_US = 1000; // 1ms entre pulsos
    static const int STEPS_PER_REVOLUTION = 200;     // Pasos por vuelta completa
    
    const char* actuatorId;
    const char* deviceId;
    bool motorEnabled;
    bool motorReady;
    int currentPosition;
    bool direction; // true = clockwise, false = counterclockwise
    
public:
    FeederStepperMotor(const char* id = ACTUATOR_FEEDER_MOTOR_ID_1, const char* devId = DEVICE_ID_FEEDER);
    bool initialize();
    void enable();
    void disable();
    void setDirection(bool clockwise);
    void step(int steps);
    void rotate(float degrees);
    void feedPortion(int portions = 1); // Alimentar porciones
    bool isEnabled();
    bool isReady();
    const char* getActuatorId();
    const char* getDeviceId();
    String getStatus();
    int getCurrentPosition();
};

#endif