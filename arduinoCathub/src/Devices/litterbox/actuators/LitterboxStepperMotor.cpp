#include "LitterboxStepperMotor.h"

const int LitterboxStepperMotor::DIR_PIN = 15;
const int LitterboxStepperMotor::EN_PIN = 16;
const int LitterboxStepperMotor::PULL_PIN = 17;
const unsigned long LitterboxStepperMotor::STEP_DELAY_US = 1000;
const int LitterboxStepperMotor::STEPS_PER_REVOLUTION = 200;

LitterboxStepperMotor::LitterboxStepperMotor(const char* id, const char* devId) :
    actuatorId(id), deviceId(devId), motorEnabled(false), motorReady(false), torqueActive(false),
    currentPosition(0), direction(true), currentState(EMPTY),
    homePosition(0), readyPosition(0) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);

    // Iniciar todo desactivado
    digitalWrite(EN_PIN, HIGH);  // Motor desactivado
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PULL_PIN, LOW);

    // Configurar posiciones
    homePosition = 0;                    // Posición inicial
    readyPosition = degreesToSteps(-40); // 40 grados a la derecha

    // Empezar en estado EMPTY
    currentState = EMPTY;
    currentPosition = 0;
    motorReady = true;

    Serial.println("{\"device\":\"LITTERBOX\",\"motor\":\"INITIALIZED\",\"state\":0}");
    return true;
}

// ===== COMANDO PRINCIPAL: CAMBIAR ESTADO =====
bool LitterboxStepperMotor::setState(int newState) {
    LitterboxState targetState = static_cast<LitterboxState>(newState);
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"CHANGING_STATE\",\"from\":" + 
                  String(static_cast<int>(currentState)) + ",\"to\":" + String(newState) + "}");

    switch (targetState) {
        case EMPTY:
            // Estado SIN ARENA: Ir a home y desactivar motor
            if (moveToPosition(homePosition)) {
                if (disableTorque()) {
                    currentState = EMPTY;
                    Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"EMPTY\",\"position\":\"HOME\",\"motor\":\"DISABLED\"}");
                    return true;
                }
            }
            break;

        case READY:
            // Estado LISTO: Moverse a posición ready y activar motor
            if (moveToPosition(readyPosition)) {
                if (enableTorque()) {
                    currentState = READY;
                    Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"READY\",\"position\":\"READY\",\"motor\":\"ENABLED\"}");
                    return true;
                }
            }
            break;

        case BLOCKED:
            // Estado BLOQUEADO: Desactivar motor inmediatamente
            disableTorque();
            currentState = BLOCKED;
            Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"BLOCKED\",\"motor\":\"DISABLED\"}");
            return true;

        default:
            Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"INVALID_STATE\",\"requested\":" + String(newState) + "}");
            return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"STATE_CHANGE_FAILED\"}");
    return false;
}

// ===== LIMPIEZA NORMAL (CADA X MINUTOS) =====
bool LitterboxStepperMotor::executeNormalCleaning() {
    if (currentState != READY) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_NORMAL_CLEANING\",\"current_state\":" + String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_START\"}");
    
    // Recordar posición inicial
    int startPosition = currentPosition;
    
    // Girar 270 grados a la izquierda
    if (rotateLeft(270)) {
        delay(1000); // Pausa para que termine el movimiento
        
        // Regresar a la posición inicial
        if (moveToPosition(startPosition)) {
            Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_COMPLETE\"}");
            return true;
        }
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NORMAL_CLEANING_FAILED\"}");
    return false;
}

// ===== LIMPIEZA COMPLETA (MANUAL) =====
bool LitterboxStepperMotor::executeCompleteCleaning() {
    if (currentState != READY) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_COMPLETE_CLEANING\",\"current_state\":" + String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"COMPLETE_CLEANING_START\"}");
    
    // Girar 50 grados a la izquierda
    if (rotateLeft(50)) {
        delay(1000); // Pausa para que termine el movimiento
        
        // Ir a HOME y desactivar (cambiar a estado EMPTY)
        if (moveToPosition(homePosition)) {
            if (disableTorque()) {
                currentState = EMPTY;
                Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"COMPLETE_CLEANING_FINISHED\",\"state\":0}");
                return true;
            }
        }
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"COMPLETE_CLEANING_FAILED\"}");
    return false;
}

// ===== MÉTODOS PRIVADOS DE CONTROL =====
bool LitterboxStepperMotor::enableTorque() {
    if (!motorReady) return false;
    digitalWrite(EN_PIN, LOW);  // Activar motor (LOW = enabled)
    motorEnabled = true;
    torqueActive = true;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"ENABLED\"}");
    return true;
}

bool LitterboxStepperMotor::disableTorque() {
    digitalWrite(EN_PIN, HIGH); // Desactivar motor (HIGH = disabled)
    motorEnabled = false;
    torqueActive = false;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"DISABLED\"}");
    return true;
}

bool LitterboxStepperMotor::moveToPosition(int targetPosition) {
    if (!motorEnabled && !enableTorque()) return false;
    
    int deltaSteps = targetPosition - currentPosition;
    if (deltaSteps == 0) return true;
    
    setDirection(deltaSteps > 0);
    step(abs(deltaSteps));
    return true;
}

bool LitterboxStepperMotor::rotateRight(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(true);  // Clockwise
    step(steps);
    return true;
}

bool LitterboxStepperMotor::rotateLeft(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(false); // Counter-clockwise
    step(steps);
    return true;
}

void LitterboxStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void LitterboxStepperMotor::step(int steps) {
    if (!motorReady) return;
    
    for (int i = 0; i < steps; ++i) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        
        // Actualizar posición actual
        currentPosition += (direction ? 1 : -1);
    }
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
    return currentState == READY;
}

bool LitterboxStepperMotor::isEmpty() const {
    return currentState == EMPTY;
}

bool LitterboxStepperMotor::isTorqueActive() const {
    return torqueActive;
}

int LitterboxStepperMotor::getCurrentPosition() const {
    return currentPosition;
}

String LitterboxStepperMotor::getStateString() const {
    switch (currentState) {
        case EMPTY:   return "EMPTY";
        case READY:   return "READY";
        case BLOCKED: return "BLOCKED";
        default:      return "UNKNOWN";
    }
}

String LitterboxStepperMotor::getStatus() {
    return "{\"state\":" + String(static_cast<int>(currentState)) +
           ",\"position\":" + String(currentPosition) +
           ",\"torque\":" + String(torqueActive) +
           ",\"enabled\":" + String(motorEnabled) + "}";
}

void LitterboxStepperMotor::emergencyStop() {
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    torqueActive = false;
    currentState = BLOCKED;
    Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\"}");
}

const char* LitterboxStepperMotor::getActuatorId() {
    return actuatorId;
}

const char* LitterboxStepperMotor::getDeviceId() {
    return deviceId;
}