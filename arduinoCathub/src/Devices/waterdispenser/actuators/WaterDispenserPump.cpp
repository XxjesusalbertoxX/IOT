#include "WaterDispenserPump.h"

// 🔥 SOLO UN CONSTRUCTOR (eliminar el duplicado)
WaterDispenserPump::WaterDispenserPump(const char* id, const char* devId) : 
    actuatorId(id), deviceId(devId), pumpEnabled(true), pumpRunning(false), pumpReady(false),
    pumpStartTime(0), pumpDuration(0), currentPower(PUMP_POWER) {}

bool WaterDispenserPump::initialize() {
    pinMode(PUMP_PIN, OUTPUT);
    analogWrite(PUMP_PIN, 0);
    pumpReady = true;
    pumpRunning = false;
    return true;
}

void WaterDispenserPump::turnOn(unsigned long duration) {
    if (!pumpReady || !pumpEnabled) return;
    
    if (duration > MAX_PUMP_TIME) {
        duration = MAX_PUMP_TIME;
    }
    
    pumpDuration = duration;
    pumpStartTime = millis();
    pumpRunning = true;
    analogWrite(PUMP_PIN, currentPower);
}

void WaterDispenserPump::turnOff() {
    analogWrite(PUMP_PIN, 0);
    pumpRunning = false;
    pumpStartTime = 0;
    pumpDuration = 0;
}

void WaterDispenserPump::setPower(int power) {
    currentPower = constrain(power, 0, 255);
    if (pumpRunning) {
        analogWrite(PUMP_PIN, currentPower);
    }
}

bool WaterDispenserPump::isPumpRunning() {
    return pumpRunning;
}

bool WaterDispenserPump::isReady() {
    return pumpReady;
}

unsigned long WaterDispenserPump::getRemainingTime() {
    if (!pumpRunning || pumpStartTime == 0) return 0;
    unsigned long elapsed = millis() - pumpStartTime;
    if (elapsed >= pumpDuration) return 0;
    return pumpDuration - elapsed;
}

void WaterDispenserPump::update() {
    if (!pumpRunning || pumpStartTime == 0) return;
    
    // Solo verificar duración normal (auto-llenado)
    if (pumpDuration > 0) {
        unsigned long elapsed = millis() - pumpStartTime;
        if (elapsed >= pumpDuration) {
            turnOff();
        }
    }
}

String WaterDispenserPump::getStatus() {
    if (!pumpReady) return "NOT_INITIALIZED";
    if (!pumpEnabled) return "DISABLED";
    if (pumpRunning) return "RUNNING";
    return "READY";
}

void WaterDispenserPump::emergencyStop() {
    analogWrite(PUMP_PIN, 0);
    pumpRunning = false;
    pumpEnabled = false;
    pumpStartTime = 0;
    pumpDuration = 0;
}

const char* WaterDispenserPump::getActuatorId() {
    return actuatorId;
}

const char* WaterDispenserPump::getDeviceId() {
    return deviceId;
}