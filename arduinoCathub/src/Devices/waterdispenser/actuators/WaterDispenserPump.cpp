// WaterDispenserPump.cpp
#include "WaterDispenserPump.h"

WaterDispenserPump::WaterDispenserPump(const char* id, const char* devId) : 
    actuatorId(id), deviceId(devId), pumpEnabled(true), pumpRunning(false), pumpReady(false),
    pumpStartTime(0), pumpDuration(0), currentPower(PUMP_POWER) {}

bool WaterDispenserPump::initialize() {
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);  // 🔥 Cambiar analogWrite por digitalWrite
    pumpReady = true;
    pumpRunning = false;
    Serial.println("{\"pump_init\":\"SUCCESS\",\"pin\":" + String(PUMP_PIN) + ",\"mode\":\"DIGITAL\"}");
    return true;
}

void WaterDispenserPump::turnOn(unsigned long duration) {
    if (!pumpReady || !pumpEnabled) {
        Serial.println("{\"pump_error\":\"CANNOT_START\",\"ready\":" + String(pumpReady) + ",\"enabled\":" + String(pumpEnabled) + "}");
        return;
    }
    
    if (duration > MAX_PUMP_TIME) {
        duration = MAX_PUMP_TIME;
    }
    
    pumpDuration = duration;
    pumpStartTime = millis();
    pumpRunning = true;
    digitalWrite(PUMP_PIN, HIGH);  // 🔥 Cambiar analogWrite por digitalWrite HIGH
    
    Serial.println("{\"pump_action\":\"TURNED_ON\",\"pin\":" + String(PUMP_PIN) + 
                   ",\"duration_ms\":" + String(duration) + ",\"digital_state\":\"HIGH\"}");
}

void WaterDispenserPump::turnOff() {
    digitalWrite(PUMP_PIN, LOW);  // 🔥 Cambiar analogWrite por digitalWrite LOW
    pumpRunning = false;
    pumpStartTime = 0;
    pumpDuration = 0;
    
    Serial.println("{\"pump_action\":\"TURNED_OFF\",\"pin\":" + String(PUMP_PIN) + ",\"digital_state\":\"LOW\"}");
}

void WaterDispenserPump::setPower(int power) {
    // 🔥 Como ahora es digital, solo importa si power > 0
    currentPower = constrain(power, 0, 255);
    if (pumpRunning) {
        digitalWrite(PUMP_PIN, currentPower > 0 ? HIGH : LOW);  // 🔥 Digital: HIGH si power > 0
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
            Serial.println("{\"pump_auto\":\"TIMEOUT_REACHED\",\"elapsed_ms\":" + String(elapsed) + "}");
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
    digitalWrite(PUMP_PIN, LOW);  // 🔥 Cambiar analogWrite por digitalWrite LOW
    pumpRunning = false;
    pumpEnabled = false;
    pumpStartTime = 0;
    pumpDuration = 0;
    
    Serial.println("{\"pump_action\":\"EMERGENCY_STOP\",\"pin\":" + String(PUMP_PIN) + "}");
}

const char* WaterDispenserPump::getActuatorId() {
    return actuatorId;
}

const char* WaterDispenserPump::getDeviceId() {
    return deviceId;
}