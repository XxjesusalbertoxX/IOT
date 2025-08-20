// FeederStepperMotor.h
#ifndef FEEDER_STEPPER_MOTOR_H
#define FEEDER_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class FeederStepperMotor {
private:
    static const int DIR_PIN = 11;   // Dirección
    static const int EN_PIN = 14;    // Enable (activo LOW)
    static const int PULL_PIN = 12;  // Pulsos (Step)
    
    static const unsigned long STEP_DELAY_US = 1000; // 1ms entre pulsos
    static const int STEPS_PER_REVOLUTION = 200;     // Pasos por vuelta completa
    
    const char* actuatorId;
    const char* deviceId;
    bool motorEnabled;
    bool motorReady;
    bool motorRunning;              // Indica si el motor está en movimiento continuo
    int currentSpeed;               // Velocidad actual (0-255)
    int currentPosition;
    bool direction;                 // true = clockwise, false = counterclockwise
    unsigned long lastStepTime;     // Para control continuo
    
public:
    FeederStepperMotor(const char* id = ACTUATOR_FEEDER_MOTOR_ID_1, const char* devId = DEVICE_ID_FEEDER);
    bool initialize();
    void enable();
    void disable();
    void setDirection(bool clockwise);
    void setSpeed(int speed);       // Establece velocidad (0-255)
    int getSpeed() const { return currentSpeed; }
    void step(int steps);
    void rotate(float degrees);
    void feedPortion(int portions = 1); // Alimentar porciones
    void startContinuous();         // Inicia movimiento continuo
    void stopContinuous();          // Detiene movimiento continuo
    void update();                  // Actualizar motor en modo continuo
    bool isEnabled();
    bool isReady();
    bool isRunning() { return motorRunning; }
    const char* getActuatorId();
    const char* getDeviceId();
    String getStatus();
    int getCurrentPosition();
};

#endif