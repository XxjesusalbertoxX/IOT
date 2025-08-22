#include "LitterboxMQ2Sensor.h"
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"
#include <math.h>

LitterboxMQ2Sensor::LitterboxMQ2Sensor(const char* id, const char* deviceId,
                                       float vcc_, float rLoad_, float emaAlpha_) :
    sensorId(id),
    deviceId(deviceId),
    lastValue(0.0f),
    lastPPM(-1.0f),
    lastRs(0.0f),
    Ro(0.0f),
    lastReadTime(0),
    sensorReady(false),
    vcc(vcc_),
    rLoad(rLoad_),
    emaAlpha(emaAlpha_) {
}

bool LitterboxMQ2Sensor::initialize(bool autoCalibrate, int calSamples, unsigned long calDelayMs) {
    int v = analogRead(ANALOG_PIN);
    lastValue = (float)v;
    float voltage = lastValue * (vcc / 1023.0f);

    if (voltage > 0.001f) {
        lastRs = (rLoad * (vcc - voltage) / voltage);
    } else {
        lastRs = rLoad;
    }

    lastReadTime = millis();
    sensorReady = true;
    lastPPM = -1.0f; // no calibrado aún

    // Serial.println("{\"mq2\":\"INITIALIZED\",\"analog\":" + String((int)lastValue) + ",\"rs\":" + String(lastRs,3) + "}");

    if (autoCalibrate) {
        calibrateRo(calSamples, calDelayMs);
        // Serial.println("{\"mq2\":\"CALIBRATION_COMPLETE\",\"Ro\":" + String(Ro,3) + "}");
    }
    return sensorReady;
}

void LitterboxMQ2Sensor::update() {
    if (!sensorReady) return;
    unsigned long now = millis();
    if (now - lastReadTime < READ_INTERVAL) return;

    const int SAMPLES = 5;
    long sum = 0;
    for (int i = 0; i < SAMPLES; ++i) {
        sum += analogRead(ANALOG_PIN);
        delay(2);
    }
    float avg = (float)sum / (float)SAMPLES;

    // EMA smoothing
    lastValue = (emaAlpha * avg) + ((1.0f - emaAlpha) * lastValue);

    float voltage = lastValue * (vcc / 1023.0f);
    if (voltage > 0.001f) {
        lastRs = (rLoad * (vcc - voltage) / voltage);
    } else {
        lastRs = rLoad;
    }

    if (Ro > 0.0f) {
        float ratio = lastRs / Ro;
        lastPPM = analogToPPM_internal(ratio);
    } else {
        lastPPM = -1.0f; // no calibrado
    }

    // Serial.println("{\"mq2\":\"READ\",\"analog\":" + String((int)round(lastValue)) + ",\"rs\":" + String(lastRs,3) + ",\"ppm\":" + String(lastPPM,2) + "}");
    lastReadTime = now;
}

float LitterboxMQ2Sensor::getAnalog() {
    return lastValue;
}

float LitterboxMQ2Sensor::getPPM() {
    return lastPPM;
}

bool LitterboxMQ2Sensor::isReady() {
    return sensorReady;
}

String LitterboxMQ2Sensor::getStatus() {
    if (!sensorReady) return "NOT_INITIALIZED";
    String s = "READY";
    if (Ro <= 0.0f) s += "_UNCALIBRATED";
    return s;
}

const char* LitterboxMQ2Sensor::getSensorId() { return sensorId; }
const char* LitterboxMQ2Sensor::getDeviceId() { return deviceId; }

void LitterboxMQ2Sensor::calibrateRo(int samples, unsigned long delayMs) {
    double sumRs = 0.0;
    for (int i = 0; i < samples; ++i) {
        int v = analogRead(ANALOG_PIN);
        float voltage = v * (vcc / 1023.0f);
        float rsTmp = (voltage > 0.001f) ? (rLoad * (vcc - voltage) / voltage) : rLoad;
        sumRs += rsTmp;
        delay(delayMs);
    }
    float avgRs = (float)(sumRs / samples);
    Ro = avgRs / CLEAN_AIR_FACTOR;
    // Serial.println("{\"mq2\":\"Ro_calibrated\",\"avgRs\":" + String(avgRs,3) + ",\"Ro\":" + String(Ro,3) + "}");
}

float LitterboxMQ2Sensor::getRo() const { return Ro; }
float LitterboxMQ2Sensor::getRs() const { return lastRs; }
float LitterboxMQ2Sensor::getRatioRSRo() const {
    if (Ro <= 0.0f) return -1.0f;
    return lastRs / Ro;
}
bool LitterboxMQ2Sensor::isGasHigh(float ppmThreshold) {
    if (lastPPM < 0) return false;
    return lastPPM >= ppmThreshold;
}
float LitterboxMQ2Sensor::analogToPPM_internal(float ratio_rs_ro) {
    if (ratio_rs_ro <= 0.0f) return -1.0f;
    const float C = 20.0f;
    const float EXP = -2.2f;
    float ppm = C * pow(ratio_rs_ro, EXP);
    if (ppm < 0.0f) ppm = 0.0f;
    if (ppm > 10000.0f) ppm = 10000.0f;
    return ppm;
}
