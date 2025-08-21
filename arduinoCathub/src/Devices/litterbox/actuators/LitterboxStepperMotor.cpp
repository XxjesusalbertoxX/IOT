#include "LitterboxStepperMotor.h"
#include <math.h> // fabs(), round()

LitterboxStepperMotor::LitterboxStepperMotor(const char* id, const char* devId) :
    actuatorId(id),
    deviceId(devId),
    motorEnabled(false),
    motorReady(false),
    currentPosition(0),
    direction(true),
    currentState(INACTIVE),
    cleaningIntervalMinutes(60),
    lastCleaningTime(0) {
}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);

    // EN = HIGH -> disabled (driver típico activo en LOW)
    digitalWrite(EN_PIN, HIGH);
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PULL_PIN, LOW);

    currentState = INACTIVE;
    currentPosition = 0;
    motorReady = true;
    motorEnabled = false;
    lastCleaningTime = millis();

    Serial.println("{\"device\":\"LITTERBOX\",\"motor\":\"INITIALIZED\",\"state\":" +
                   String(static_cast<int>(currentState)) + "}");
    return true;
}

bool LitterboxStepperMotor::setReady() {
    // Si ya está listo, nothing to do
    if (currentState == ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"ALREADY_READY\"}");
        return true;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"ACTIVATING_MOTOR\",\"debug\":\"starting_activation\"}");

    // 1) Garantizar torque deshabilitado antes de cambiar dirección (seguridad)
    disableTorque();
    delay(100);

    // 2) Dirección: LEFT (false)
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"SETTING_DIRECTION\",\"direction\":\"LEFT\"}");
    setDirection(false);
    delay(50);

    // 3) Activar torque (EN low)
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"ENABLING_TORQUE\"}");
    if (!enableTorque()) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"CANNOT_ENABLE_TORQUE\"}");
        return false;
    }

    // Dar tiempo al driver para estabilizarse
    delay(200);

    // 4) Calcular pasos para READY_DEGREES y mover **hacia la izquierda**
    int stepsToMove = degreesToSteps(READY_DEGREES);
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"MOVING_TO_READY\",\"degrees\":" +
                   String(READY_DEGREES) + ",\"steps\":" + String(stepsToMove) + "}");

    // Usamos step(...) con signedSteps negativo para mover LEFT (implementación step actual respeta signo)
    step(-stepsToMove);

    // 5) Mantener torque activado y actualizar estado
    currentState = ACTIVE;
    Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"READY\",\"state_code\":2,\"position\":" +
                   String(currentPosition) + "}");
    return true;
}

bool LitterboxStepperMotor::executeNormalCleaning() {
    if (currentState != ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_CLEANING\",\"current_state\":" +
                       String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_START\",\"state_code\":\"2.1\"}");

    // Girar 270 grados a la derecha (filtrado)
    setDirection(true); // Derecha
    step(degreesToSteps(270));
    delay(500);

    // Regresar a la posición inicial (izquierda)
    setDirection(false);
    step(degreesToSteps(270));

    updateLastCleaningTime();
    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEANING_COMPLETE\",\"returning_to_state\":2}");
    return true;
}

bool LitterboxStepperMotor::executeDeepCleaning() {
    if (currentState != ACTIVE) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"NOT_READY_FOR_DEEP_CLEANING\",\"current_state\":" +
                       String(static_cast<int>(currentState)) + "}");
        return false;
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEANING_START\",\"state_code\":\"2.2\"}");

    // Girar DEEP_CLEAN_DEGREES a la izquierda
    setDirection(false); // Izquierda
    step(degreesToSteps(DEEP_CLEAN_DEGREES));
    delay(500);

    // Regresar la misma cantidad a la derecha
    setDirection(true);
    step(degreesToSteps(DEEP_CLEAN_DEGREES));
    delay(500);

    // Desactivar torque y volver a estado INACTIVE
    disableTorque();
    currentState = INACTIVE;
    updateLastCleaningTime();

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEANING_COMPLETE\",\"new_state\":1}");
    return true;
}

void LitterboxStepperMotor::setCleaningInterval(int minutes) {
    cleaningIntervalMinutes = minutes;
    Serial.println("{\"device\":\"LITTERBOX\",\"config\":\"CLEANING_INTERVAL\",\"minutes\":" + String(minutes) + "}");
}

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

void LitterboxStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void LitterboxStepperMotor::step(int signedSteps) {
    if (!motorReady) {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"MOTOR_NOT_READY_FOR_STEP\"}");
        return;
    }

    if (signedSteps == 0) return;

    // Asegurar torque activo antes de intentar moverse
    if (!motorEnabled) {
        Serial.println("{\"device\":\"LITTERBOX\",\"warning\":\"TORQUE_WAS_DISABLED_ENABLING_FOR_STEP\"}");
        if (!enableTorque()) {
            Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"CANNOT_ENABLE_TORQUE_FOR_STEP\"}");
            return;
        }
        delay(50); // pequeña pausa para driver
    }

    bool stepDir = (signedSteps > 0); // true = RIGHT, false = LEFT
    setDirection(stepDir);
    int steps = abs(signedSteps);

    Serial.println("{\"device\":\"LITTERBOX\",\"action\":\"STEPPING\",\"steps\":" + String(steps) +
                   ",\"direction\":" + String(stepDir ? "\"RIGHT\"" : "\"LEFT\"") + "}");

    // tiempo entre pulsos en microsegundos (usa constante para controlar velocidad)
    const unsigned long stepDelayUs = STEP_DELAY_US;
    for (int i = 0; i < steps; ++i) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(stepDelayUs / 2);
        digitalWrite(PULL_PIN, LOW);
        delayMicroseconds(stepDelayUs / 2);

        // actualizar posición: RIGHT suma, LEFT resta
        currentPosition += (stepDir ? 1 : -1);

        if ((i % 10) == 0) {
            Serial.println("{\"device\":\"LITTERBOX\",\"step_progress\":" + String(i) +
                           ",\"position\":" + String(currentPosition) + "}");
        }
    }

    Serial.println("{\"device\":\"LITTERBOX\",\"steps_completed\":" + String(steps) +
                   ",\"final_position\":" + String(currentPosition) + "}");
}

bool LitterboxStepperMotor::rotateSteps(int steps) {
    step(steps);
    return true;
}

bool LitterboxStepperMotor::rotateDegreesSigned(int degreesSigned) {
    int s = degreesToSteps(degreesSigned);
    if (degreesSigned < 0) s = -s;
    step(s);
    return true;
}

int LitterboxStepperMotor::degreesToSteps(int degrees) {
    float stepsF = (fabs((float)degrees) * (float)STEPS_PER_REVOLUTION) / 360.0f;
    return (int) round(stepsF);
}

void LitterboxStepperMotor::emergencyStop() {
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    currentState = BLOCKED;
    Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\"}");
}

bool LitterboxStepperMotor::isReady() const {
    return motorReady && currentState == ACTIVE;
}

int LitterboxStepperMotor::getState() const {
    return static_cast<int>(currentState);
}

bool LitterboxStepperMotor::setBlocked() {
    if (currentState != BLOCKED) {
        currentState = BLOCKED;
        disableTorque();
        Serial.println("{\"device\":\"LITTERBOX\",\"state\":\"BLOCKED\",\"state_code\":-1}");
        return true;
    }
    return false;
}

void LitterboxStepperMotor::updateLastCleaningTime() {
    lastCleaningTime = millis();
}

bool LitterboxStepperMotor::shouldPerformCleaning() {
    if (cleaningIntervalMinutes <= 0) return false;
    unsigned long currentTime = millis();
    unsigned long intervalMs = cleaningIntervalMinutes * 60000UL;
    return (currentState == ACTIVE) && (currentTime - lastCleaningTime >= intervalMs);
}

bool LitterboxStepperMotor::isBlocked() const {
    return currentState == BLOCKED;
}

void LitterboxStepperMotor::setState(int state) {
    if (state == BLOCKED) {
        currentState = BLOCKED;
        disableTorque();
    } else if (state == INACTIVE) {
        currentState = INACTIVE;
    } else if (state == ACTIVE) {
        currentState = ACTIVE;
    } else {
        Serial.println("{\"device\":\"LITTERBOX\",\"warning\":\"INVALID_STATE_REQUEST\",\"requested\":" + String(state) + "}");
    }
}

int LitterboxStepperMotor::getCleaningInterval() const {
    return cleaningIntervalMinutes;
}

unsigned long LitterboxStepperMotor::getLastCleaningTime() const {
    return lastCleaningTime;
}

bool LitterboxStepperMotor::isTorqueActive() const {
    return motorEnabled;
}

int LitterboxStepperMotor::getCurrentPosition() const {
    return currentPosition;
}

String LitterboxStepperMotor::getStateString() const {
    switch (currentState) {
        case BLOCKED: return "BLOCKED";
        case INACTIVE: return "INACTIVE";
        case ACTIVE: return "ACTIVE";
        default: return "UNKNOWN";
    }
}

const char* LitterboxStepperMotor::getActuatorId() {
    return actuatorId ? actuatorId : "UNCONFIGURED";
}

const char* LitterboxStepperMotor::getDeviceId() {
    return deviceId ? deviceId : "UNCONFIGURED";
}

String LitterboxStepperMotor::getStatus() {
    String s = "{";
    s += "\"device\":\"LITTERBOX\",";
    s += "\"actuator\":\"" + String(getActuatorId()) + "\",";
    s += "\"state\":\"" + getStateString() + "\",";
    s += "\"torque\":" + String(isTorqueActive()) + ",";
    s += "\"position\":" + String(getCurrentPosition());
    s += "}";
    return s;
}
