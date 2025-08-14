#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <Arduino.h>
#include "../../config/MotorConfigs.h"

class LitterboxStepperMotor {
private:
    static const int DIR_PIN = 15;    // Dirección
    static const int EN_PIN = 16;     // Enable (activo LOW)  
    static const int PULL_PIN = 17;   // Pulsos (Step)
    
    static const unsigned long STEP_DELAY_US = 1000;
    static const int STEPS_PER_REVOLUTION = 200;
    
    // Estados del arenero
    enum LitterboxState {
        EMPTY = 0,           // Sin arena, sin torque
        READY = 1,           // Con arena, con torque, listo para usar
        BLOCKED = -1         // Bloqueado por condiciones inseguras
    };
    
    enum CleaningType {
        NORMAL_CLEAN,        // 270° derecha + regreso
        COMPLETE_CLEAN       // 80° izquierda + regreso + quitar torque
    };
    
    // Estado del motor
    bool motorEnabled;
    bool motorReady;
    bool torqueActive;
    int currentPosition;
    bool direction;
    LitterboxState currentState;
    
    // Posiciones de referencia
    int homePosition;        // Posición inicial (0°)
    int readyPosition;       // Posición con arena (-40°)
    
public:
    LitterboxStepperMotor();
    bool initialize();
    
    // Control de estados principales
    bool fillWithLitter();              // Estado 0 -> 1 (llenar arena)
    bool executeNormalCleaning();       // Limpieza normal (270° derecha)
    bool executeCompleteCleaning();     // Limpieza completa (80° izquierda)
    bool blockMotor();                  // Bloquear por condiciones inseguras
    bool unblockMotor();               // Desbloquear cuando sea seguro
    
    // Control de torque
    bool enableTorque();
    bool disableTorque();
    
    // Movimientos específicos
    bool moveToHome();                  // Ir a posición inicial (0°)
    bool moveToReady();                 // Ir a posición lista (-40°)
    bool rotateRight(int degrees);      // Girar derecha
    bool rotateLeft(int degrees);       // Girar izquierda
    
    // Getters de estado
    int getState() const { return static_cast<int>(currentState); }
    bool isBlocked() const { return currentState == BLOCKED; }
    bool isReady() const { return currentState == READY; }
    bool isEmpty() const { return currentState == EMPTY; }
    bool isTorqueActive() const { return torqueActive; }
    int getCurrentPosition() const { return currentPosition; }
    String getStateString() const;
    String getStatus();
    
    // Control de emergencia
    void emergencyStop();
    
private:
    void setDirection(bool clockwise);
    void step(int steps);
    int degreesToSteps(int degrees);
    bool moveToPosition(int targetPosition);
};

#endif