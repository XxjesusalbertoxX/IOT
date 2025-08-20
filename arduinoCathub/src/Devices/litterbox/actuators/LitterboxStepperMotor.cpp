#include "LitterboxStepperMotor.h"

LitterboxStepperMotor::LitterboxStepperMotor(const char* id, const char* devId) :
    actuatorId(id), deviceId(devId), motorEnabled(false), motorReady(false),
    currentPosition(0), direction(true), currentState(INACTIVE),
    cleaningIntervalMinutes(60), lastCleaningTime(0) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);

    // Iniciar en estado desactivado (Estado 1)
    digitalWrite(EN_PIN, HIGH);  // Motor sin fuerza (HIGH = disabled)
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PULL_PIN, LOW);

    // Establecer estado inicial
    currentState = INACTIVE;
    currentPosition = 0;
    motorReady = true;
    motorEnabled = false;
    lastCleaningTime = millis(); // Iniciar el contador de limpieza

    Serial.println("{\"device\":\"LITTERBOX\",\"motor\":\"INITIALIZED\",\"state\":" + String(static_cast<int>(currentState)) + "}");
    return true;
}

// ===== COMANDO: PONER ARENERO EN MODO LISTO (ESTADO 2) =====
bool LitterboxStepperMotor::setReady() {
    if (currentState == BLOCKED) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"BLOCKED_CANNOT_SET_READY\"}");
        return false;
    }

    if (currentState == ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"ALREADY_READY\"}");
        return true; // Ya está listo
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"ACTIVATING_MOTOR\"}");
    
    // 1. Activar torque para permitir movimiento
    if (enableTorque()) {
        // 2. Girar 30 pasos a la izquierda
        if (rotateSteps(-30)) {
            currentState = ACTIVE;
            Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"READY\",\"state_code\":2}");
            return true;
        }
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"ACTIVATION_FAILED\"}");
    return false;
}

// ===== LIMPIEZA NORMAL (ESTADO 2.1) =====
bool LitterboxStepperMotor::executeNormalCleaning() {
    if (currentState != ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_CLEANING\",\"current_state\":" + String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_START\",\"state_code\":\"2.1\"}");
    
    // Iniciar limpieza normal
    
    // Girar 270 grados a la derecha
    setDirection(true); // Derecha
    step(degreesToSteps(270));
    delay(1000); // Pausa para que termine el movimiento
    
    // Regresar a la posición inicial
    setDirection(false); // Izquierda
    step(degreesToSteps(270));
    
    // Actualizar tiempo de última limpieza
    updateLastCleaningTime();
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_COMPLETE\",\"returning_to_state\":2}");
    return true;
}

// ===== LIMPIEZA COMPLETA (ESTADO 2.2) =====
bool LitterboxStepperMotor::executeDeepCleaning() {
    if (currentState != ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_DEEP_CLEANING\",\"current_state\":" + String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEANING_START\",\"state_code\":\"2.2\"}");
    
    // Girar 40 grados a la izquierda
    setDirection(false); // Izquierda
    step(degreesToSteps(40));
    delay(1000); // Pausa
    
    // Regresar a posición anterior
    setDirection(true); // Derecha
    step(degreesToSteps(40));
    delay(500);
    
    // Desactivar motor y pasar a estado 1
    disableTorque();
    currentState = INACTIVE;
    
    updateLastCleaningTime();
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEANING_COMPLETE\",\"new_state\":1}");
    return true;
}

// ===== CONFIGURAR INTERVALO DE LIMPIEZA =====
void LitterboxStepperMotor::setCleaningInterval(int minutes) {
    cleaningIntervalMinutes = minutes;
    Serial.println("{\"device\":\"LITTERBOX\",\"config\":\"CLEANING_INTERVAL\",\"minutes\":" + String(minutes) + "}");
}

// ===== CONTROL DEL TORQUE =====
bool LitterboxStepperMotor::enableTorque() {
    if (!motorReady) return false;
    digitalWrite(EN_PIN, LOW);  // Activar motor (LOW = enabled)
    motorEnabled = true;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"ENABLED\"}");
    return true;
}

bool LitterboxStepperMotor::disableTorque() {
    digitalWrite(EN_PIN, HIGH); // Desactivar motor (HIGH = disabled)
    motorEnabled = false;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"DISABLED\"}");
    return true;
}

// ===== CONTROL DE DIRECCIÓN Y PASOS =====
void LitterboxStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void LitterboxStepperMotor::step(int steps) {
    if (!motorReady) return;
    
    for (int i = 0; i < abs(steps); ++i) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        
        // Actualizar posición actual
        currentPosition += (direction ? 1 : -1);
    }
}

bool LitterboxStepperMotor::rotateSteps(int steps) {
    setDirection(steps > 0);
    step(abs(steps));
    return true;
}

int LitterboxStepperMotor::degreesToSteps(int degrees) {
    return (degrees * STEPS_PER_REVOLUTION) / 360;
}

// ===== GETTERS =====
int LitterboxStepperMotor::getState() const {
    return static_cast<int>(currentState);
}

bool LitterboxStepperMotor::isBlocked() const {
    return currentState == BLOCKED;
}

bool LitterboxStepperMotor::isReady() const {
    return motorReady;
}

bool LitterboxStepperMotor::setBlocked() {
    if (motorEnabled) {
        disableTorque();
    }
    currentState = BLOCKED;
    return true;
}

void LitterboxStepperMotor::setState(int state) {
    switch (state) {
        case -1:
            disableTorque();
            currentState = BLOCKED;
            break;
        case 1:
            disableTorque();
            currentState = INACTIVE;
            break;
        case 2:
            if (enableTorque()) {
                rotateSteps(-30); // Girar 30 pasos a la izquierda
                currentState = ACTIVE;
            }
            break;
    }
}

int LitterboxStepperMotor::getCleaningInterval() const {
    return cleaningIntervalMinutes;
}

unsigned long LitterboxStepperMotor::getLastCleaningTime() const {
    return lastCleaningTime;
}

void LitterboxStepperMotor::updateLastCleaningTime() {
    lastCleaningTime = millis();
}

bool LitterboxStepperMotor::shouldPerformCleaning() {
    if (currentState != ACTIVE) return false;
    
    unsigned long now = millis();
    unsigned long interval = cleaningIntervalMinutes * 60 * 1000; // Minutos a milisegundos
    
    // Verificar si ha pasado el intervalo desde la última limpieza
    return (now - lastCleaningTime) >= interval;
}

bool LitterboxStepperMotor::isTorqueActive() const {
    return motorEnabled;
}

int LitterboxStepperMotor::getCurrentPosition() const {
    return currentPosition;
}

String LitterboxStepperMotor::getStateString() const {
    switch (currentState) {
        case INACTIVE: return "INACTIVE";
        case ACTIVE:   return "ACTIVE";
        case BLOCKED:  return "BLOCKED";
        default:       return "UNKNOWN";
    }
}

String LitterboxStepperMotor::getStatus() {
    return "{\"state\":" + String(static_cast<int>(currentState)) +
           ",\"position\":" + String(currentPosition) +
           ",\"torque\":" + String(motorEnabled) +
           ",\"cleaning_interval_min\":" + String(cleaningIntervalMinutes) + "}";
}

void LitterboxStepperMotor::emergencyStop() {
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    currentState = BLOCKED;
    Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\"}");
}

const char* LitterboxStepperMotor::getActuatorId() {
    return actuatorId;
}

const char* LitterboxStepperMotor::getDeviceId() {
    return deviceId;
}