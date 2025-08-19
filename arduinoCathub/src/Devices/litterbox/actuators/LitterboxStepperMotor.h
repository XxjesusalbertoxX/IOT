#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxStepperMotor {
public:
    // Estados del sistema
    enum LitterboxState { 
        EMPTY = 0,      // Sin arena, motor desactivado
        READY = 1,      // Con arena, motor activo, listo para usar
        BLOCKED = -1    // Bloqueado por sensores
    };

private:
    const char* actuatorId;
    const char* deviceId;

public:
    LitterboxStepperMotor(const char* id = ACTUATOR_LITTERBOX_MOTOR_ID_1, const char* devId = DEVICE_ID_LITTER);
    bool initialize();

    // ===== COMANDOS PRINCIPALES DESDE RASPBERRY =====
    bool setState(int newState);                    // Cambiar estado (0, 1, -1)
    bool executeNormalCleaning();                   // Limpieza por intervalo (270° izq)
    bool executeCompleteCleaning();                 // Limpieza manual completa (50° izq + EMPTY)

    // ===== GETTERS DE ESTADO =====
    int getState() const;
    bool isBlocked() const;
    bool isReady() const;
    bool isEmpty() const;
    bool isTorqueActive() const;
    int getCurrentPosition() const;
    String getStateString() const;
    const char* getActuatorId();
    const char* getDeviceId();
    String getStatus();

    // ===== EMERGENCIA =====
    void emergencyStop();

private:
    static const int DIR_PIN;
    static const int EN_PIN;
    static const int PULL_PIN;
    static const unsigned long STEP_DELAY_US;
    static const int STEPS_PER_REVOLUTION;

    // Variables de estado
    bool motorEnabled;
    bool motorReady;
    bool torqueActive;
    int currentPosition;
    bool direction;
    LitterboxState currentState;

    // Posiciones clave
    int homePosition;        // Posición inicial (0°)
    int readyPosition;       // Posición lista (-40°)

    // ===== MÉTODOS PRIVADOS DE CONTROL =====
    bool enableTorque();
    bool disableTorque();
    void setDirection(bool clockwise);
    void step(int steps);
    int degreesToSteps(int degrees);
    bool moveToPosition(int targetPosition);
    bool rotateRight(int degrees);
    bool rotateLeft(int degrees);
};

#endif