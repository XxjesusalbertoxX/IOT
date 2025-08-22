#include "LitterboxStepperMotor.h"

LitterboxStepperMotor::LitterboxStepperMotor(const char* id, const char* devId) :
    actuatorId(id),
    deviceId(devId),
    motorEnabled(false),
    motorReady(false),
    currentPosition(0),
    currentState(INACTIVE) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);

    // EN = HIGH -> disabled (driver típico TB6600 activo en LOW)
    digitalWrite(EN_PIN, HIGH);
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PULL_PIN, LOW);

    motorReady = true;
    motorEnabled = false;
    currentPosition = 0;
    currentState = INACTIVE;

    // Serial.println("{\"device\":\"LITTERBOX\",\"motor\":\"INITIALIZED\",\"state\":1}");
    return true;
}

bool LitterboxStepperMotor::enableTorque() {
    if (!motorReady) return false;
    digitalWrite(EN_PIN, LOW); // LOW = enabled
    motorEnabled = true;
    delay(5);
    // Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"ENABLED\"}");
    return true;
}

bool LitterboxStepperMotor::disableTorque() {
    digitalWrite(EN_PIN, HIGH); // HIGH = disabled
    motorEnabled = false;
    delay(5);
    // Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"DISABLED\"}");
    return true;
}

void LitterboxStepperMotor::setDirection(bool clockwise) {
    // clockwise=true -> RIGHT, clockwise=false -> LEFT
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(2);
}

void LitterboxStepperMotor::stepSigned(int signedSteps) {
    if (!motorReady) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"MOTOR_NOT_READY\"}");
        return;
    }
    if (signedSteps == 0) return;

    // Requerimos torque activo para mover. Si no está activo, fallo (caller debe activar).
    if (!motorEnabled) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"TORQUE_DISABLED_CANNOT_MOVE\"}");
        return;
    }

    bool dirRight = (signedSteps > 0);
    int steps = abs(signedSteps);

    setDirection(dirRight);
    // Inicio movimiento
    for (int i = 0; i < steps; ++i) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US / 2);
        currentPosition += (dirRight ? 1 : -1);
        // NO imprimir dentro del bucle para no afectar timing
    }
    // Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"STEP_DONE\",\"dir\":" + String(dirRight ? "RIGHT":"LEFT") + ",\"steps\":" + String(steps) + ",\"pos\":" + String(currentPosition) + "}");
}

bool LitterboxStepperMotor::setReady() {
    if (currentState == ACTIVE) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"ALREADY_ACTIVE\"}");
        return true;
    }

    // 1) activar torque
    if (!enableTorque()) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"ENABLE_FAILED\"}");
        return false;
    }
    delay(20);

    // 2) mover READY_STEPS hacia la izquierda (negativo)
    stepSigned(-READY_STEPS);

    // 3) actualizar estado
    currentState = ACTIVE;
    // Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"ACTIVE\",\"position\":" + String(currentPosition) + "}");
    return true;
}

bool LitterboxStepperMotor::executeNormalCleaning() {
    if (currentState != ACTIVE) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_ACTIVE_CANNOT_CLEAN\"}");
        return false;
    }

    // MOVIMIENTO: RIGHT NORMAL_CLEAN_STEPS y luego regresar la misma cantidad (LEFT)
    stepSigned(NORMAL_CLEAN_STEPS);
    delay(150);
    stepSigned(-NORMAL_CLEAN_STEPS);

    // Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEAN_COMPLETE\",\"position\":" + String(currentPosition) + "}");
    return true;
}

bool LitterboxStepperMotor::executeDeepCleaning() {
    if (currentState != ACTIVE) {
        // Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_ACTIVE_CANNOT_DEEP_CLEAN\"}");
        return false;
    }

    // 1) LEFT DEEP_CLEAN_STEPS (vaciar)
    stepSigned(-DEEP_CLEAN_STEPS);
    delay(150);

    // 2) RIGHT DEEP_CLEAN_STEPS (volver al punto anterior)
    stepSigned(DEEP_CLEAN_STEPS);
    delay(150);

    // 3) Desactivar torque y pasar a INACTIVE (estado 1)
    disableTorque();
    currentState = INACTIVE;

    // Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEAN_COMPLETE\",\"state\":1,\"position\":" + String(currentPosition) + "}");
    return true;
}

int LitterboxStepperMotor::getState() const {
    return static_cast<int>(currentState);
}

bool LitterboxStepperMotor::isReady() const {
    return motorReady && (currentState == ACTIVE);
}

bool LitterboxStepperMotor::isTorqueActive() const {
    return motorEnabled;
}

long LitterboxStepperMotor::getCurrentPosition() const {
    return currentPosition;
}

String LitterboxStepperMotor::getStateString() const {
    return (currentState == ACTIVE) ? "ACTIVE" : "INACTIVE";
}

String LitterboxStepperMotor::getStatus() const {
    String s = "{";
    s += "\"device\":\"LITTERBOX\",";
    s += "\"state\":\"" + getStateString() + "\",";
    s += "\"torque\":" + String(isTorqueActive() ? "true" : "false") + ",";
    s += "\"position\":" + String(getCurrentPosition());
    s += "}";
    return s;
}

void LitterboxStepperMotor::emergencyStop() {
    disableTorque();
    currentState = INACTIVE;
    // Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\",\"state\":1}");
}

void LitterboxStepperMotor::forceDisableTorque() {
    // Sólo desactiva torque. NO cambia posición en el contador.
    disableTorque();
    // NO tocar currentState aquí si no lo quieres; lo dejamos en INACTIVE para seguridad explícita.
    currentState = INACTIVE;
}
