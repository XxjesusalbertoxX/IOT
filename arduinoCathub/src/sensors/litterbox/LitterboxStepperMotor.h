#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>

class LitterboxStepperMotor {
private:
    static const int DIR_PIN = 15;   // Dirección del arenero
    static const int EN_PIN = 16;    // Enable del arenero
    static const int PULL_PIN = 17;  // Pulsos del arenero
    
    static const unsigned long STEP_DELAY_US = 2000; // 2ms más lento para limpieza
    static const int STEPS_PER_REVOLUTION = 200;
    
    bool motorEnabled;
    bool motorReady;
    int currentPosition;
    bool direction;
    bool isCleaningCycle;
    
public:
    LitterboxStepperMotor();
    bool initialize();
    void enable();
    void disable();
    void setDirection(bool clockwise);
    void step(int steps);
    void rotate(float degrees);
    void startCleaningCycle();
    void stopCleaningCycle();
    void siftLitter(); // Función específica para tamizar arena
    bool isEnabled();
    bool isReady();
    bool isCleaning();
    String getStatus();
    int getCurrentPosition();
};

#endif