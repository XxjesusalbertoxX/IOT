#include "WaterDispenserPump.h"

WaterDispenserPump::WaterDispenserPump() : 
    pumpEnabled(true), pumpRunning(false), pumpReady(false),
    pumpStartTime(0), pumpDuration(0), currentPower(PUMP_POWER) {}

bool WaterDispenserPump::initialize() {
    pinMode(PUMP_PIN, OUTPUT);
    
    // Inicializar en estado seguro (apagado)
    analogWrite(PUMP_PIN, 0);
    
    pumpReady = true;
    pumpRunning = false;
    
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"component\":\"PUMP\",\"status\":\"INITIALIZED\",\"pin\":" + String(PUMP_PIN) + "}"));
    return true;
}

void WaterDispenserPump::turnOn(unsigned long duration) {
    if (!pumpReady || !pumpEnabled) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"ERROR\",\"reason\":\"NOT_READY_OR_DISABLED\"}"));
        return;
    }
    
    // Limitar tiempo máximo por seguridad
    if (duration > MAX_PUMP_TIME) {
        duration = MAX_PUMP_TIME;
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"WARNING\",\"reason\":\"DURATION_LIMITED\",\"max_time\":" + String(MAX_PUMP_TIME) + "}"));
    }
    
    pumpDuration = duration;
    pumpStartTime = millis();
    pumpRunning = true;
    
    // Encender bomba con PWM
    analogWrite(PUMP_PIN, currentPower);
    
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"ON\",\"duration\":" + String(duration) + ",\"power\":" + String(currentPower) + "}"));
}

void WaterDispenserPump::turnOff() {
    analogWrite(PUMP_PIN, 0);
    pumpRunning = false;
    pumpStartTime = 0;
    pumpDuration = 0;
    
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"OFF\"}"));
}

void WaterDispenserPump::setPower(int power) {
    // Limitar rango 0-255
    currentPower = constrain(power, 0, 255);
    
    // Si la bomba está funcionando, aplicar nuevo poder inmediatamente
    if (pumpRunning) {
        analogWrite(PUMP_PIN, currentPower);
    }
    
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"POWER_SET\",\"value\":" + String(currentPower) + "}"));
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
    
    unsigned long elapsed = millis() - pumpStartTime;
    
    // Auto-apagar después del tiempo programado
    if (elapsed >= pumpDuration) {
        turnOff();
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"AUTO_OFF\",\"reason\":\"DURATION_COMPLETE\"}"));
    }
}

String WaterDispenserPump::getStatus() {
    if (!pumpReady) return "NOT_INITIALIZED";
    if (!pumpEnabled) return "DISABLED";
    if (pumpRunning) return "RUNNING";
    return "READY";
}

void WaterDispenserPump::emergencyStop() {
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"EMERGENCY_STOP\"}"));
    analogWrite(PUMP_PIN, 0);
    pumpRunning = false;
    pumpEnabled = false; // Deshabilitar hasta reinicio
    pumpStartTime = 0;
    pumpDuration = 0;
}