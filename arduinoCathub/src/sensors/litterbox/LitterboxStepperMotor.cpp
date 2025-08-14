#include "LitterboxStepperMotor.h"

const int LitterboxStepperMotor::DIR_PIN = 15;
const int LitterboxStepperMotor::EN_PIN = 16;
const int LitterboxStepperMotor::PULL_PIN = 17;

const unsigned long LitterboxStepperMotor::STEP_DELAY_US = 1000;
const int LitterboxStepperMotor::STEPS_PER_REVOLUTION = 200;

LitterboxStepperMotor::LitterboxStepperMotor() :
    motorEnabled(false), motorReady(false), torqueActive(false),
    currentPosition(0), direction(true), currentState(EMPTY),
    homePosition(0), readyPosition(0) {}
    // homePosition(0), readyPosition(-40) {}

bool LitterboxStepperMotor::initialize() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(PULL_PIN, OUTPUT);

    digitalWrite(EN_PIN, HIGH);
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PULL_PIN, LOW);

    currentState = EMPTY;
    currentPosition = 0;
    homePosition = 0;
    readyPosition = degreesToSteps(-40);

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
    if (rotateRight(270)) {
        delay(1000);
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
    if (rotateLeft(80)) {
        delay(1000);
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
    disableTorque();
    Serial.println("{\"device\":\"LITTERBOX\",\"status\":\"BLOCKED\",\"state\":-1}");
    return true;
}

bool LitterboxStepperMotor::unblockMotor() {
    if (currentState == BLOCKED) {
        if (abs(currentPosition - readyPosition) < 10) {
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
    digitalWrite(EN_PIN, LOW);
    motorEnabled = true;
    torqueActive = true;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"ENABLED\"}");
    return true;
}

bool LitterboxStepperMotor::disableTorque() {
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    torqueActive = false;
    Serial.println("{\"device\":\"LITTERBOX\",\"torque\":\"DISABLED\"}");
    return true;
}

bool LitterboxStepperMotor::moveToHome() { return moveToPosition(homePosition); }
bool LitterboxStepperMotor::moveToReady() { return moveToPosition(readyPosition); }

bool LitterboxStepperMotor::rotateRight(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(true);
    step(steps);
    return true;
}

bool LitterboxStepperMotor::rotateLeft(int degrees) {
    int steps = degreesToSteps(degrees);
    setDirection(false);
    step(steps);
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
        currentPosition += (direction ? 1 : -1);
    }
}

int LitterboxStepperMotor::degreesToSteps(int degrees) {
    return (degrees * STEPS_PER_REVOLUTION) / 360;
}

int LitterboxStepperMotor::getState() const { return static_cast<int>(currentState); }
bool LitterboxStepperMotor::isBlocked() const { return currentState == BLOCKED; }
bool LitterboxStepperMotor::isReady() const { return currentState == READY; }
bool LitterboxStepperMotor::isEmpty() const { return currentState == EMPTY; }
bool LitterboxStepperMotor::isTorqueActive() const { return torqueActive; }
int LitterboxStepperMotor::getCurrentPosition() const { return currentPosition; }

String LitterboxStepperMotor::getStateString() const {
    switch (currentState) {
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
    digitalWrite(EN_PIN, HIGH);
    motorEnabled = false;
    torqueActive = false;
    currentState = BLOCKED;
    Serial.println("{\"device\":\"LITTERBOX\",\"emergency\":\"STOPPED\"}");
}
// ...existing code...