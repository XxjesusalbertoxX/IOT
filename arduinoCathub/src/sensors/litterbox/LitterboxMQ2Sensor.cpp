#include "LitterboxMQ2Sensor.h"

LitterboxMQ2Sensor::LitterboxMQ2Sensor() : lastValue(0), lastPPM(0), lastReadTime(0), sensorReady(false) {}

bool LitterboxMQ2Sensor::initialize() {
    pinMode(ANALOG_PIN, INPUT);
    
    // Hacer lectura inicial
    int initialValue = analogRead(ANALOG_PIN);
    lastValue = initialValue;
    lastPPM = analogToPPM(initialValue);
    
    sensorReady = true;
    return true;
}

void LitterboxMQ2Sensor::update() {
    if (!sensorReady) return;
    
    unsigned long now = millis();
    if (now - lastReadTime >= READ_INTERVAL) {
        int analogValue = analogRead(ANALOG_PIN);
        lastValue = analogValue;
        lastPPM = analogToPPM(analogValue);  // Convertir a PPM
        lastReadTime = now;
    }
}

float LitterboxMQ2Sensor::analogToPPM(int analogValue) {
    // Conversión aproximada de MQ2 (ajustar según calibración)
    // Voltage = analogValue * (5.0 / 1023.0)
    float voltage = analogValue * (5.0 / 1023.0);
    
    // Fórmula aproximada para MQ2 (se debe calibrar con gas conocido)
    // Esta es una aproximación básica
    float rs = (5.0 - voltage) / voltage;  // Resistencia del sensor
    float ratio = rs / 10.0;  // Ratio con resistencia de carga (10kΩ típico)
    
    // Conversión aproximada a PPM (calibrar según tu sensor específico)
    float ppm = 20.0 * pow(ratio, -2.2);  // Fórmula aproximada para LPG/gas
    
    // Limitar valores razonables
    if (ppm < 0) ppm = 0;
    if (ppm > 10000) ppm = 10000;
    
    return ppm;
}

float LitterboxMQ2Sensor::getValue() {
    return lastValue;  // Valor analógico crudo
}

float LitterboxMQ2Sensor::getPPM() {
    return lastPPM;    // Valor convertido a PPM
}

bool LitterboxMQ2Sensor::isReady() {
    return sensorReady;
}

String LitterboxMQ2Sensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    return "READY";
}