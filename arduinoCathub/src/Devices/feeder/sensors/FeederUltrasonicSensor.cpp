#include "FeederUltrasonicSensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// ---------- Helpers ----------
static long sendPulseAndMeasure(int trigPin, int echoPin, unsigned long timeoutUs) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    return pulseIn(echoPin, HIGH, timeoutUs); // 0 = timeout
}

static float durationToCm(long duration) {
    if (duration <= 0) return -1.0;
    return (duration * 0.034) / 2.0;
}

static float medianOf3(float a, float b, float c) {
    float v[3] = {a, b, c};
    for (int i = 0; i < 2; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (v[j] < v[i]) { float t = v[i]; v[i] = v[j]; v[j] = t; }
        }
    }
    // devolver el valor central no-negativo si existe
    if (v[1] >= 0) return v[1];
    if (v[0] >= 0) return v[0];
    if (v[2] >= 0) return v[2];
    return -1.0;
}

// ===== IMPLEMENTACIÓN DE FeederUltrasonicSensor1 =====
FeederUltrasonicSensor1::FeederUltrasonicSensor1(const char* id, const char* deviceId) 
    : sensorId(id), deviceId(deviceId), lastDistance(-1.0), lastReadTime(0), sensorReady(false) {}

bool FeederUltrasonicSensor1::initialize() {
    Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"INITIALIZING\",\"trig_pin\":" + String(TRIG_PIN) + ",\"echo_pin\":" + String(ECHO_PIN) + "}");
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    delay(50); // estabilizar sensor

    // Intentar varias veces cortas para obtener una primera lectura, pero no fallar si no hay eco
    const int ATTEMPTS = 3;
    long duration = 0;
    for (int i = 0; i < ATTEMPTS; ++i) {
        duration = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
        Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"INIT_TRY\",\"attempt\":" + String(i) + ",\"duration\":" + String(duration) + "}");
        if (duration > 0) break;
        delay(30);
    }

    // configurar sensor como listo para intentar lecturas en runtime
    sensorReady = true;
    if (duration > 0) {
        lastDistance = durationToCm(duration);
        Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"INIT_SUCCESS\",\"distance\":" + String(lastDistance) + "}");
    } else {
        lastDistance = -1.0;
        Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"INIT_WARNING\",\"reason\":\"NO_ECHO_YET\",\"note\":\"will attempt readings at runtime\"}");
    }
    return true;
}

void FeederUltrasonicSensor1::update() {
    if (!sensorReady) {
        Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"UPDATE_SKIPPED\",\"reason\":\"NOT_READY\"}");
        return;
    }
    unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) return;

    // 3 mediciones con pausas cortas para evitar cross-talk y usar mediana
    long d1 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
    delay(20);
    long d2 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
    delay(20);
    long d3 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);

    float cm = medianOf3(durationToCm(d1), durationToCm(d2), durationToCm(d3));
    Serial.println("{\"sensor\":\"FeederUltrasonic1\",\"action\":\"PULSE_RESULTS\",\"d1\":" + String(d1) + ",\"d2\":" + String(d2) + ",\"d3\":" + String(d3) + ",\"cm_med\":" + String(cm) + "}");

    if (cm >= 0) {
        lastDistance = cm;
    } // si cm < 0 mantiene la última lectura válida
    lastReadTime = now;
}

float FeederUltrasonicSensor1::getDistance() { return lastDistance; }
bool FeederUltrasonicSensor1::isReady() { return sensorReady; }
String FeederUltrasonicSensor1::getStatus() { return sensorReady ? "READY" : "NOT_INITIALIZED"; }
String FeederUltrasonicSensor1::getFoodStatus() {
    if (lastDistance <= 0) return "UNKNOWN";
    if (hasFood()) return "FULL";
    if (isEmpty()) return "EMPTY";
    return "PARTIAL";
}
const char* FeederUltrasonicSensor1::getSensorId() { return sensorId ? sensorId : "UNCONFIGURED"; }
const char* FeederUltrasonicSensor1::getDeviceId() { return deviceId ? deviceId : "UNCONFIGURED"; }

// ===== IMPLEMENTACIÓN DE FeederUltrasonicSensor2 =====
FeederUltrasonicSensor2::FeederUltrasonicSensor2(const char* id, const char* deviceId) 
    : sensorId(id), deviceId(deviceId), lastDistance(-1.0), lastReadTime(0), sensorReady(false) {}

bool FeederUltrasonicSensor2::initialize() {
    Serial.println("{\"sensor\":\"FeederUltrasonic2\",\"action\":\"INITIALIZING\",\"trig_pin\":" + String(TRIG_PIN) + ",\"echo_pin\":" + String(ECHO_PIN) + "}");
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    delay(50);

    const int ATTEMPTS = 3;
    long duration = 0;
    for (int i = 0; i < ATTEMPTS; ++i) {
        duration = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
        Serial.println("{\"sensor\":\"FeederUltrasonic2\",\"action\":\"INIT_TRY\",\"attempt\":" + String(i) + ",\"duration\":" + String(duration) + "}");
        if (duration > 0) break;
        delay(30);
    }

    sensorReady = true;
    if (duration > 0) {
        lastDistance = durationToCm(duration);
        Serial.println("{\"sensor\":\"FeederUltrasonic2\",\"action\":\"INIT_SUCCESS\",\"distance\":" + String(lastDistance) + "}");
    } else {
        lastDistance = -1.0;
        Serial.println("{\"sensor\":\"FeederUltrasonic2\",\"action\":\"INIT_WARNING\",\"reason\":\"NO_ECHO_YET\",\"note\":\"will attempt readings at runtime\"}");
    }
    return true;
}

void FeederUltrasonicSensor2::update() {
    if (!sensorReady) return;
    unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) return;

    long d1 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
    delay(25);
    long d2 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);
    delay(25);
    long d3 = sendPulseAndMeasure(TRIG_PIN, ECHO_PIN, TIMEOUT_US);

    float cm = medianOf3(durationToCm(d1), durationToCm(d2), durationToCm(d3));
    Serial.println("{\"sensor\":\"FeederUltrasonic2\",\"action\":\"PULSE_RESULTS\",\"d1\":" + String(d1) + ",\"d2\":" + String(d2) + ",\"d3\":" + String(d3) + ",\"cm_med\":" + String(cm) + "}");

    if (cm >= 0) lastDistance = cm;
    lastReadTime = now;
}

float FeederUltrasonicSensor2::getDistance() { return lastDistance; }
bool FeederUltrasonicSensor2::isReady() { return sensorReady; }
String FeederUltrasonicSensor2::getStatus() { return sensorReady ? "READY" : "NOT_INITIALIZED"; }
String FeederUltrasonicSensor2::getPlateStatus() {
    if (lastDistance <= 0) return "UNKNOWN";
    if (isFull()) return "FULL";
    if (isEmpty()) return "EMPTY";
    return "PARTIAL";
}
const char* FeederUltrasonicSensor2::getSensorId() { return sensorId ? sensorId : "UNCONFIGURED"; }
const char* FeederUltrasonicSensor2::getDeviceId() { return deviceId ? deviceId : "UNCONFIGURED"; }
