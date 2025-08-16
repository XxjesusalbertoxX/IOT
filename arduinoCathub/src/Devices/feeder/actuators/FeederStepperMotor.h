#ifndef FEEDER_STEPPER_MOTOR_H
#define FEEDER_STEPPER_MOTOR_H

#include <Arduino.h>

class FeederStepperMotor {
private:
    static const int DIR_PIN = 12;   // Dirección
    static const int EN_PIN = 13;    // Enable (activo LOW)
    static const int PULL_PIN = 14;  // Pulsos (Step)
    
    static const unsigned long STEP_DELAY_US = 1000; // 1ms entre pulsos
    static const int STEPS_PER_REVOLUTION = 200;     // Pasos por vuelta completa
    
    bool motorEnabled;
    bool motorReady;
    int currentPosition;
    bool direction; // true = clockwise, false = counterclockwise
    
public:
    FeederStepperMotor();
    bool initialize();
    void enable();
    void disable();
    void setDirection(bool clockwise);
    void step(int steps);
    void rotate(float degrees);
    void feedPortion(int portions = 1); // Alimentar porciones
    bool isEnabled();
    bool isReady();
    String getStatus();
    int getCurrentPosition();
};

#endif