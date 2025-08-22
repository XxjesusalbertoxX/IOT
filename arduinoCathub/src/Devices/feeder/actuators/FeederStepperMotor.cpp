#include "FeederStepperMotor.h"

FeederStepperMotor::FeederStepperMotor(const char* id, const char* devId) : 
    actuatorId(id), deviceId(devId), motorEnabled(false), motorReady(false), 
    motorRunning(false), currentSpeed(50), currentPosition(0), direction(true),
    lastStepTime(0) {}

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
    motorRunning = false;
    digitalWrite(EN_PIN, HIGH); // Desactivar
    motorEnabled = false;
}

void FeederStepperMotor::setDirection(bool clockwise) {
    direction = clockwise;
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);
    delayMicroseconds(5); // Tiempo de setup para TB6600
}

void FeederStepperMotor::setSpeed(int speed) {
    currentSpeed = constrain(speed, 0, 255);
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
    
    // Serial.println("{\"device\":\"FEEDER\",\"action\":\"FEED\",\"portions\":" + String(portions) + ",\"degrees\":" + String(totalDegrees) + "}");
}

void FeederStepperMotor::startContinuous() {
    if (!motorEnabled || !motorReady) return;
    motorRunning = true;
    lastStepTime = micros();
}

void FeederStepperMotor::stopContinuous() {
    motorRunning = false;
}

void FeederStepperMotor::update() {
    if (!motorRunning || !motorEnabled || !motorReady) return;
    
    // Ajustar velocidad según currentSpeed (0-255)
    // Mayor velocidad = menor delay entre pasos
    unsigned long stepDelay = map(currentSpeed, 0, 255, 10000, 1000); // 10ms a 1ms
    
    unsigned long now = micros();
    if (now - lastStepTime >= stepDelay) {
        digitalWrite(PULL_PIN, HIGH);
        delayMicroseconds(50);
        digitalWrite(PULL_PIN, LOW);
        // Actualizar posición
        currentPosition += (direction ? 1 : -1);
        lastStepTime = now;
    }
}

bool FeederStepperMotor::isEnabled() {
    return motorEnabled;
}

bool FeederStepperMotor::isReady() {
    return motorReady;
}

String FeederStepperMotor::getStatus() {
    if (!motorReady) return "NOT_INITIALIZED";
    if (motorRunning) return "RUNNING";
    if (motorEnabled) return "ENABLED";
    return "DISABLED";
}

int FeederStepperMotor::getCurrentPosition() {
    return currentPosition;
}

const char* FeederStepperMotor::getActuatorId() {
    return actuatorId;
}

const char* FeederStepperMotor::getDeviceId() {
    return deviceId;
}

// Nota: controlFromSerial mantiene compatibilidad pero NO debe usarse
// directamente para arrancar sin validar sensores; use tryStart en su lugar.
void FeederStepperMotor::controlFromSerial(int command) {
    if (command == 1) {
        // Advertencia por serial: uso recomendado -> tryStart(storage, plate)
        // Serial.println("{\"feeder_motor\":\"REQUEST_START_RECEIVED\",\"note\":\"use tryStart(storage,plate) to validate sensors\"}");
        // Si quieres compatibilidad para arrancar sin sensores:
        // enable(); setDirection(false); setSpeed(120); startContinuous();
    } 
    else if (command == 0) {
        stopContinuous();
        disable();
        // Serial.println("{\"feeder_motor\":\"STOPPED_BY_SERIAL\"}");
    }
}

// canStart: revisar distancias con tolerancia y manejar lecturas desconocidas (-1)
bool FeederStepperMotor::canStart(float foodStorageDistance, float plateFoodDistance) {
    // Si no hay lectura del depósito o lectura indica vacío (>= 13.0 cm) -> no arrancar
    if (foodStorageDistance <= 0) return false;
    if (foodStorageDistance >= 13.0) return false;

    // Si hay lectura del plato y este está muy cerca (<= 2.0 cm) -> plato lleno -> no arrancar
    if (plateFoodDistance > 0 && plateFoodDistance <= 2.0) return false;

    // En los demás casos, permitir arranque
    return true;
}


// tryStart: mejor método a usar desde el parser de comandos.
// Devuelve true si el motor efectivamente arrancó.
bool FeederStepperMotor::tryStart(float foodStorageDistance, float plateFoodDistance) {
    if (!motorReady) {
        // Serial.println("{\"feeder_motor\":\"START_BLOCKED\",\"reason\":\"MOTOR_NOT_READY\"}");
        return false;
    }
    if (!canStart(foodStorageDistance, plateFoodDistance)) {
        // Serial.println("{\"feeder_motor\":\"START_BLOCKED\",\"reason\":\"SENSOR_CHECK_FAILED\",\"storage\":" + String(foodStorageDistance) + ",\"plate\":" + String(plateFoodDistance) + "}");
        return false;
    }
    // Inicio seguro
    setDirection(false);  // dirección de dispensado (izquierda en tu diseño)
    setSpeed(200);
    enable();
    startContinuous();
    // Serial.println("{\"feeder_motor\":\"STARTED\",\"direction\":\"LEFT\",\"speed\":120}");
    return true;
}

// monitorAndStop: llama periódicamente desde SensorManager::poll()
// Si motor está corriendo y ahora las condiciones no son seguras, lo detiene y devuelve true.
bool FeederStepperMotor::monitorAndStop(float foodStorageDistance, float plateFoodDistance) {
    if (!motorRunning) return false;
    if (!canStart(foodStorageDistance, plateFoodDistance)) {
        stopContinuous();
        disable();
        // Serial.println("{\"feeder_motor\":\"AUTO_STOP\",\"reason\":\"SENSOR_STATE_CHANGED\",\"storage\":" + String(foodStorageDistance) + ",\"plate\":" + String(plateFoodDistance) + "}");
        return true;
    }
    return false;
}

void FeederStepperMotor::emergencyStop() {
    stopContinuous();
    disable();
    // Serial.println("{\"feeder_motor\":\"EMERGENCY_STOP\"}");
}
