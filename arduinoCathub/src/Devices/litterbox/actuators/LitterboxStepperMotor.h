#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxStepperMotor {
public:
    // Estados del sistema
    enum LitterboxState { 
        BLOCKED = -1,  // Bloqueado por sensores (gato o gas)
        INACTIVE = 1,  // Estado 1: Motor sin fuerza (EN desactivado)
        ACTIVE = 2     // Estado 2: Motor con fuerza (EN activado)
    };

private:
    const char* actuatorId;
    const char* deviceId;
    bool motorEnabled;
    bool motorReady;
    int currentPosition;
    bool direction;
    LitterboxState currentState;
    int cleaningIntervalMinutes;
    unsigned long lastCleaningTime;
    
    static const int DIR_PIN = 15;
    static const int EN_PIN = 16;
    static const int PULL_PIN = 17;
    static const unsigned long STEP_DELAY_US = 1000;
    static const int STEPS_PER_REVOLUTION = 200;

    // ===== MÉTODOS PRIVADOS =====
    bool enableTorque();
    bool disableTorque();
    void setDirection(bool clockwise);
    void step(int steps);
    bool rotateSteps(int steps);
    int degreesToSteps(int degrees);

public:
    LitterboxStepperMotor(const char* id = ACTUATOR_LITTERBOX_MOTOR_ID_1, const char* devId = DEVICE_ID_LITTER);
    bool initialize();

    // ===== COMANDOS PRINCIPALES =====
    bool setReady();                      // Cambiar a estado 2 (ACTIVE)
    bool executeNormalCleaning();         // Limpieza normal (estado 2.1)
    bool executeDeepCleaning();           // Limpieza completa (estado 2.2)
    void setCleaningInterval(int minutes); // Configurar intervalo de limpieza

    // ===== GETTERS =====
    int getState() const;
    bool isBlocked() const;
    bool isReady() const;
    bool setBlocked();
    void setState(int state);
    int getCleaningInterval() const;
    unsigned long getLastCleaningTime() const;
    void updateLastCleaningTime();
    bool shouldPerformCleaning();
    
    bool isTorqueActive() const;
    int getCurrentPosition() const;
    String getStateString() const;
    const char* getActuatorId();
    const char* getDeviceId();
    String getStatus();
    void emergencyStop();
};

#endif