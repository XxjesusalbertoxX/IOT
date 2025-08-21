#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxStepperMotor {
public:
    enum LitterboxState {
        INACTIVE = 1,
        ACTIVE = 2
    };

private:
    const char* actuatorId;
    const char* deviceId;
    bool motorEnabled;     // true = EN low (holding)
    bool motorReady;
    long currentPosition;  // contador de pasos (puede ser negativo)
    LitterboxState currentState;

    static const int DIR_PIN = 15;
    static const int EN_PIN  = 16;
    static const int PULL_PIN = 17;

    static const unsigned long STEP_DELAY_US = 4000UL; // 1ms = velocidad moderada
    static const int STEPS_PER_REVOLUTION = 1600;      // 200 * 8 = 1600 pasos/vuelta (con microstepping 1/8)

    // PARÁMETROS CALIBRADOS PARA NEMA 21 + TB6600
    static const int READY_STEPS = 480; // 40° izquierda (40° ÷ 0.225° = 178 pasos)
    static const int NORMAL_CLEAN_STEPS = 1200; // 270° derecha (270° ÷ 0.225° = 1200 pasos)
    static const int DEEP_CLEAN_STEPS = 450; // pasos para limpieza completa (izquierda)
    // -------------------------------------

    bool enableTorque();
    bool disableTorque();
    void setDirection(bool clockwise);
    void stepSigned(int signedSteps); // acepta + (RIGHT) o - (LEFT)

public:
    LitterboxStepperMotor(const char* id = ACTUATOR_LITTERBOX_MOTOR_ID_1,
                         const char* devId = DEVICE_ID_LITTERBOX);
    bool initialize();

    // Operaciones (basadas en estados)
    bool setReady();               // LTR1:2
    bool executeNormalCleaning();  // LTR1:2.1
    bool executeDeepCleaning();    // LTR1:2.2

    // Getters
    int getState() const;
    bool isReady() const;
    bool isTorqueActive() const;
    long getCurrentPosition() const;
    String getStateString() const;
    String getStatus() const;

    // Emergencia / control externo
    void emergencyStop();          // desactiva torque y pone INACTIVE
    void forceDisableTorque();     // sólo desactiva torque (NO cambia la posición en hardware)
};

#endif // LITTERBOX_STEPPER_MOTOR_H
