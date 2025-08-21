#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../config/ActuatorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxStepperMotor {
public:
    enum LitterboxState {
        BLOCKED = -1,
        INACTIVE = 1,
        ACTIVE = 2
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
    static const int EN_PIN  = 16;
    static const int PULL_PIN = 17;

    // Tiempo entre pulsos en microsegundos. Aumentar para movimientos más lentos y fiables.
    static const unsigned long STEP_DELAY_US = 5000UL; // 5 ms por pulso (ajustable)

    static const int STEPS_PER_REVOLUTION = 200;

    // Grados que debe mover cuando se marca READY (LTR1:2)
    static const int READY_DEGREES = 70;

    // Profundidad de limpieza completa por defecto (en grados)
    static const int DEEP_CLEAN_DEGREES = 45; // ajustar si hace falta

    bool enableTorque();
    bool disableTorque();
    void setDirection(bool clockwise);
    void step(int steps);
    bool rotateSteps(int steps);
    int degreesToSteps(int degrees);

public:
    LitterboxStepperMotor(const char* id = ACTUATOR_LITTERBOX_MOTOR_ID_1,
                          const char* devId = DEVICE_ID_LITTERBOX);
    bool initialize();

    bool setReady();                    // PREPARAR: mover READY_DEGREES a la izquierda y mantener torque
    bool executeNormalCleaning();
    bool executeDeepCleaning();
    void setCleaningInterval(int minutes);

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

    // API helper para girar en grados (con signo)
    bool rotateDegreesSigned(int degreesSigned);
};

#endif // LITTERBOX_STEPPER_MOTOR_H
