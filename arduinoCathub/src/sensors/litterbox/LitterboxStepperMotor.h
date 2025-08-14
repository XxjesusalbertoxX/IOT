// ...existing code...
#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../../config/MotorConfigs.h"

class LitterboxStepperMotor {
public:
    LitterboxStepperMotor();
    bool initialize();

    // Control de estados principales (Arduino SOLO ejecuta comandos)
    bool fillWithLitter();
    bool executeNormalCleaning();
    bool executeCompleteCleaning();
    bool blockMotor();
    bool unblockMotor();

    // Torque / movimiento
    bool enableTorque();
    bool disableTorque();
    bool moveToHome();
    bool moveToReady();
    bool rotateRight(int degrees);
    bool rotateLeft(int degrees);

    // Estado
    int getState() const;
    bool isBlocked() const;
    bool isReady() const;
    bool isEmpty() const;
    bool isTorqueActive() const;
    int getCurrentPosition() const;
    String getStateString() const;
    String getStatus();

    void emergencyStop();

private:
    static const int DIR_PIN;
    static const int EN_PIN;
    static const int PULL_PIN;

    static const unsigned long STEP_DELAY_US;
    static const int STEPS_PER_REVOLUTION;

    enum LitterboxState { EMPTY = 0, READY = 1, BLOCKED = -1 };

    bool motorEnabled;
    bool motorReady;
    bool torqueActive;
    int currentPosition;
    bool direction;
    LitterboxState currentState;

    int homePosition;
    int readyPosition;

    void setDirection(bool clockwise);
    void step(int steps);
    int degreesToSteps(int degrees);
    bool moveToPosition(int targetPosition);
};

#endif
