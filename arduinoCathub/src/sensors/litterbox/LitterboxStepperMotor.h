#include "LitterboxStepperMotor.h"

LitterboxStepperMotor::LitterboxStepperMotor() : 
    motorEnabled(false), motorReady(false), torqueActive(false),
    currentPosition(0), direction(true), currentState(EMPTY),
    homePosition(0), readyPosition(-40) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);
    
    // Inicializar en estado seguro
    digitalWrite(EN_PIN, HIGH);   // Motor deshabilitado
    digitalWrite(DIR_PIN, HIGH);  // Dirección por defecto
    digitalWrite(PULL_PIN, LOW);  // Sin pulsos
    
    currentState = EMPTY;
    currentPosition = 0;  // Asumir que inicia en home
    homePosition = 0;
    readyPosition = degreesToSteps(-40);  // -40 grados en steps
    
    motorReady = true;
    Serial.println("{\"device\":\"LITTERBOX\",\"motor\":\"INITIALIZED\",\"state\":0}");
    return true;
}

bool LitterboxStepperMotor::fillWithLitter() {
    if (currentState != EMPTY) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_EMPTY\"}");
        return false;
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"FILLING_LITTER\"}");
    
    // Mover a posición ready (-40°) y activar torque
    if (moveToPosition(readyPosition)) {
        if (enableTorque()) {
            currentState = READY;
            Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"FILL_COMPLETE\",\"state\":1}");
            return true;
        }
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"FILL_FAILED\"}");
    return false;
}

bool LitterboxStepperMotor::executeNormalCleaning() {
    if (currentState != READY) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_CLEANING\"}");
        return false;
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_START\"}");
    
    int startPosition = currentPosition;
    
    // Girar 270° a la derecha
    if (rotateRight(270)) {
        delay(1000);  // Pausa para que termine el movimiento
        
        // Regresar a posición original
        if (moveToPosition(startPosition)) {
            Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_COMPLETE\"}");
            return true;
        }
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NORMAL_CLEANING_FAILED\"}");
    return false;
}

bool LitterboxStepperMotor::executeCompleteCleaning() {
    if (currentState != READY) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_COMPLETE_CLEANING\"}");
        return false;
    }
    
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"COMPLETE_CLEANING_START\"}");
    
    // Girar 80° a la izquierda desde posición actual
    if (rotateLeft(80)) {
        delay(1000);
        
        // Regresar a home y quitar torque
        if (moveToHome()) {
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

bool LitterboxStepperMotor::blockMotor() {
    currentState = BLOCKED;
    disableTorque();  // Quitar torque por seguridad
    Serial.println("{\"device\":\"LITTERBOX\",\"status\":\"BLOCKED\",\"state\":-1}");
    return true;
}

bool LitterboxStepperMotor::unblockMotor() {
    if (currentState == BLOCKED) {
        // Determinar estado apropiado basado en posición
        if (abs(currentPosition - readyPosition) < 10) {  // Cerca de posición ready
            currentState = READY;
            enableTorque();
        } else {
            currentState = EMPTY;
        }
        Serial.println("{\"device\":\"LITTERBOX\",\"status\":\"UNBLOCKED\",\"state\":" + String(static_cast<int>(currentState)) + "}");
        return true;
    }
    return false;
}

bool LitterboxStepperMotor::enableTorque() {
    if (!motorReady) return false;
    
    digitalWrite(EN_PIN, LOW);  // Activar motor (activo LOW)
    motorEnabled = true;
    torqueActive = true;
    
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"ENABLED\"}");
    return true;
}

bool LitterboxStepperMotor::disableTorque() {
    digitalWrite(EN_PIN, HIGH);  // Desactivar motor
    motorEnabled = false;
    torqueActive = false;
    
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"DISABLED\"}");
    return true;
}

bool LitterboxStepperMotor::moveToHome() {
    return moveToPosition(homePosition);
}

bool LitterboxStepperMotor::moveToReady() {
    return moveToPosition(readyPosition);
}

bool LitterboxStepperMotor::rotateRight(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(true);  // Clockwise
    step(steps);
    return true;
}

bool LitterboxStepperMotor::rotateLeft(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(false);  // Counterclockwise  
    step(steps);
    return true;
}

bool LitterboxStepperMotor::moveToPosition(int targetPosition) {
    if (!motorEnabled && !enableTorque()) {
        return false;
    }
    
    int deltaSteps = targetPosition - currentPosition;
    
    if (deltaSteps == 0) {
        return true;  // Ya está en posición
    }
    
    setDirection(deltaSteps > 0);
    step(abs(deltaSteps));
    
    return true;
}

void LitterboxStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void LitterboxStepperMotor::step(int steps) {
    if (!motorReady) return;
    
    for (int i = 0; i < steps; i++) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        
        currentPosition += (direction ? 1 : -1);
    }
}

int LitterboxStepperMotor::degreesToSteps(int degrees) {
    return (degrees * STEPS_PER_REVOLUTION) / 360;
}

String LitterboxStepperMotor::getStateString() const {
    switch(currentState) {
        case EMPTY: return "EMPTY";
        case READY: return "READY";
        case BLOCKED: return "BLOCKED";
        default: return "UNKNOWN";
    }
}

String LitterboxStepperMotor::getStatus() {
    return "{\"state\":" + String(static_cast<int>(currentState)) + 
           ",\"position\":" + String(currentPosition) + 
           ",\"torque\":" + String(torqueActive) + 
           ",\"enabled\":" + String(motorEnabled) + "}";
}

void LitterboxStepperMotor::emergencyStop() {
    digitalWrite(EN_PIN, HIGH);  // Desactivar inmediatamente
    motorEnabled = false;
    torqueActive = false;
    currentState = BLOCKED;
    Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\"}");
}