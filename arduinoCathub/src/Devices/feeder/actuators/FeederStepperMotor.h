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

    static const unsigned long STEP_DELAY_US = 5000; // 10ms entre pulsos (valor base)
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
    void startContinuous();         // Inicia movimiento continuo (interno)
    void stopContinuous();          // Detiene movimiento continuo (interno)
    void update();                  // Actualizar motor en modo continuo (llamar desde loop/poll)
    bool isEnabled();
    bool isReady();
    bool isRunning() { return motorRunning; }
    
    // IDs y estado
    const char* getActuatorId();
    const char* getDeviceId();
    String getStatus();
    int getCurrentPosition();

    // Control por serial (mantengo compatibilidad, ver nota abajo)
    void controlFromSerial(int command);

    // MÉTODOS NUEVOS
    // Intenta arrancar consultando condiciones de sensores; devuelve true si inició.
    bool tryStart(float foodStorageDistance, float plateFoodDistance);
    // Monitor: si motor está corriendo y las condiciones dejan de ser seguras, lo detiene y devuelve true si se detuvo.
    bool monitorAndStop(float foodStorageDistance, float plateFoodDistance);
    // Paro inmediato de emergencia
    void emergencyStop();

    // Verificar si puede arrancar según distancias (no cambia estado)
    bool canStart(float foodStorageDistance, float plateFoodDistance);
};

#endif
