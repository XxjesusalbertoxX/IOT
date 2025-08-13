#include "FeederStepperMotor.h"

FeederStepperMotor::FeederStepperMotor() : 
    motorEnabled(false), motorReady(false), currentPosition(0), direction(true) {}

bool FeederStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);
    
    // Inicializar en estado seguro
    digitalWrite(EN_PIN, HIGH);   // Deshabilitado (activo LOW)
    digitalWrite(DIR_PIN, HIGH);  // Dirección por defecto
    digitalWrite(PULL_PIN, LOW);  // Pulso en bajo
    
    motorReady = true;
    return true;
}

void FeederStepperMotor::enable() {
    if (motorReady) {
        digitalWrite(EN_PIN, LOW); // Activo LOW
        motorEnabled = true;
        delay(10); // Tiempo para estabilizar
    }
}

void FeederStepperMotor::disable() {
    digitalWrite(EN_PIN, HIGH); // Desactivar
    motorEnabled = false;
}

void FeederStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5); // Tiempo de setup para TB6600
}

void FeederStepperMotor::step(int steps) {
    if (!motorEnabled || !motorReady) return;
    
    for (int i = 0; i < abs(steps); i++) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        
        // Actualizar posición
        currentPosition += (direction ? 1 : -1);
    }
}

void FeederStepperMotor::rotate(float degrees) {
    int steps = (int)((degrees / 360.0) * STEPS_PER_REVOLUTION);
    setDirection(degrees > 0);
    step(abs(steps));
}

void FeederStepperMotor::feedPortion(int portions) {
    if (!motorEnabled || !motorReady) return;
    
    // Una porción = 1/8 de vuelta (45 grados)
    float degreesPerPortion = 45.0;
    float totalDegrees = degreesPerPortion * portions;
    
    setDirection(true); // Siempre hacia adelante para alimentar
    rotate(totalDegrees);
    
    Serial.println("{\"device\":\"FEEDER\",\"action\":\"FEED\",\"portions\":" + String(portions) + ",\"degrees\":" + String(totalDegrees) + "}");
}

bool FeederStepperMotor::isEnabled() {
    return motorEnabled;
}

bool FeederStepperMotor::isReady() {
    return motorReady;
}

String FeederStepperMotor::getStatus() {
    if (!motorReady) return "NOT_INITIALIZED";
    if (motorEnabled) return "ENABLED";
    return "DISABLED";
}

int FeederStepperMotor::getCurrentPosition() {
    return currentPosition;
}