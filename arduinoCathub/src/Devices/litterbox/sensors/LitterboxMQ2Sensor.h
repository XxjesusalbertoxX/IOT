#ifndef LITTERBOX_MQ2_SENSOR_H
#define LITTERBOX_MQ2_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

class LitterboxMQ2Sensor {
private:
    static const int ANALOG_PIN = A0;
    static const unsigned long READ_INTERVAL = 500; // ms

    const char* sensorId;
    const char* deviceId;

    float lastValue;        // Promedio crudo (0..1023)
    float lastPPM;          // Valor convertido a PPM (aprox)
    float lastRs;           // Resistencia calculada (kΩ)
    float Ro;               // Resistencia en aire limpio (kΩ) -> calibrar!
    unsigned long lastReadTime;
    bool sensorReady;

    // Parámetros hardware
    float vcc;              // Voltaje de referencia (5.0 o 3.3)
    float rLoad;            // resistencia de carga en kOhm (ej: 10k -> 10.0)

    // Suavizado (EMA)
    float emaAlpha;

    // Factor de aire limpio (Rs/Ro en aire limpio) — valor orientativo
    static constexpr float CLEAN_AIR_FACTOR = 9.83f;

    // Conversión analógica -> PPM (aproximada). Ver .cpp para notas.
    float analogToPPM_internal(float ratio_rs_ro);

public:
    LitterboxMQ2Sensor(const char* id = SENSOR_ID_LITTER_MQ2,
                       const char* deviceId = DEVICE_ID_LITTERBOX,
                       float vcc = 5.0, float rLoad = 10.0, float emaAlpha = 0.2f);
    bool initialize(bool autoCalibrate = false, int calSamples = 50, unsigned long calDelayMs = 50);
    void update();
    float getAnalog();       // 0..1023 (promediado)
    float getPPM();          // PPM aproximado
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();

    // Métodos utilitarios:
    void calibrateRo(int samples = 50, unsigned long delayMs = 50); // calibrar Ro en aire limpio
    float getRo() const;
    float getRs() const;     // Rs actual (kΩ)
    float getRatioRSRo() const; // Rs / Ro
    bool isGasHigh(float ppmThreshold); // helper
};

#endif
