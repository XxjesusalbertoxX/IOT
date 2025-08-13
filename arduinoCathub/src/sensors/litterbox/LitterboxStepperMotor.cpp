#include "LitterboxStepperMotor.h"

LitterboxStepperMotor::LitterboxStepperMotor() : 
    motorEnabled(false), motorReady(false), currentPosition(0), 
    direction(true), isCleaningCycle(false) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);
    
    // Inicializar en estado seguro
    digitalWrite(EN_PIN, HIGH);   // Deshabilitado
    digitalWrite(DIR_PIN, HIGH);  // Dirección por defecto
    digitalWrite(PULL_PIN, LOW);  // Pulso en bajo
    
    motorReady = true;
    return true;
}

void LitterboxStepperMotor::enable() {
    if (motorReady) {
        digitalWrite(EN_PIN, LOW); // Activo LOW
        motorEnabled = true;
        delay(10);
    }
}

void LitterboxStepperMotor::disable() {
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    isCleaningCycle = false;
}

void LitterboxStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void LitterboxStepperMotor::step(int steps) {
    if (!motorEnabled || !motorReady) return;
    
    for (int i = 0; i < abs(steps); i++) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        
        currentPosition += (direction ? 1 : -1);
    }
}

void LitterboxStepperMotor::rotate(float degrees) {
    int steps = (int)((degrees / 360.0) * STEPS_PER_REVOLUTION);
    setDirection(degrees > 0);
    step(abs(steps));
}

void LitterboxStepperMotor::startCleaningCycle() {
    if (!motorEnabled || !motorReady) return;
    
    isCleaningCycle = true;
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"CLEANING_START\"}");
}

void LitterboxStepperMotor::stopCleaningCycle() {
    isCleaningCycle = false;
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"CLEANING_STOP\"}");
}

void LitterboxStepperMotor::siftLitter() {
    if (!motorEnabled || !motorReady) return;
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"SIFTING_START\"}");
    
    // Ciclo de tamizado: 3 rotaciones lentas en cada dirección
    for (int cycle = 0; cycle < 3; cycle++) {
        // Hacia adelante
        setDirection(true);
        rotate(180); // Media vuelta
        delay(500);
        
        // Hacia atrás
        setDirection(false);
        rotate(180); // Media vuelta de regreso
        delay(500);
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"SIFTING_COMPLETE\"}");
}

bool LitterboxStepperMotor::isEnabled() {
    return motorEnabled;
}

bool LitterboxStepperMotor::isReady() {
    return motorReady;
}

bool LitterboxStepperMotor::isCleaning() {
    return isCleaningCycle;
}

String LitterboxStepperMotor::getStatus() {
    if (!motorReady) return "NOT_INITIALIZED";
    if (isCleaningCycle) return "CLEANING";
    if (motorEnabled) return "ENABLED";
    return "DISABLED";
}

int LitterboxStepperMotor::getCurrentPosition() {
    return currentPosition;
}