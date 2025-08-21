#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "../config/SensorIDs.h"
#include "../../config/DeviceIDs.h"

// Sensor para detectar presencia del gato / nivel de comida
class FeederUltrasonicSensor1 {
private:
    static const int TRIG_PIN = 4;   // Pin trigger para sensor 1
    static const int ECHO_PIN = 5;   // Pin echo para sensor 1
    static const unsigned long READ_INTERVAL = 100; // ms
    static const unsigned long TIMEOUT_US = 6000;   // µs, ~1 m roundtrip suficiente para comederos
    
    const char* sensorId;
    const char* deviceId;
    
    float lastDistance;       // -1.0 = sin lectura válida
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor1(const char* id = SENSOR_ID_FEEDER_SONIC1, const char* deviceId = DEVICE_ID_FEEDER);
    bool initialize();
    // NOTA: ajustar rangos según montaje físico; aquí valores recomendados
    bool hasFood() { return (lastDistance > 0 && lastDistance <= 4.0); }  // Depósito lleno (<=4cm)
    bool isEmpty() { return (lastDistance > 0 && lastDistance >= 6.0); }   // Depósito vacío (>=6cm)
    String getFoodStatus();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

// Sensor para medir nivel de comida en platito (si lo usas)
class FeederUltrasonicSensor2 {
private:
    static const int TRIG_PIN = 6;
    static const int ECHO_PIN = 7;
    static const unsigned long READ_INTERVAL = 120; // desfasado respecto al otro
    static const unsigned long TIMEOUT_US = 6000;
    
    const char* sensorId;
    const char* deviceId;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;

public:
    FeederUltrasonicSensor2(const char* id = SENSOR_ID_FEEDER_SONIC2, const char* deviceId = DEVICE_ID_FEEDER);
    bool initialize();
    bool isFull() { return (lastDistance > 0 && lastDistance <= 4.0); }   // Platito lleno
    bool isEmpty() { return (lastDistance > 0 && lastDistance >= 12.0); }  // Platito vacío
    String getPlateStatus();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    const char* getSensorId();
    const char* getDeviceId();
};

#endif
