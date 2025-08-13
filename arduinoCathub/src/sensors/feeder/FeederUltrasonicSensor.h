#ifndef FEEDER_ULTRASONIC_SENSOR_H
#define FEEDER_ULTRASONIC_SENSOR_H

#include <Arduino.h>

class FeederUltrasonicSensor {
private:
    // Pines fijos internos (no por parámetro)
    int trigPin;
    int echoPin;
    String sensorName;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor(String name); // Solo recibe el nombre
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
    String getName();
};

// Clase específica para cada ultrasónico con pines fijos
class FeederUltrasonicSensor1 {
private:
    static const int TRIG_PIN = 4;  // Pines fijos del ultrasónico 1
    static const int ECHO_PIN = 5;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor1();
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
};

class FeederUltrasonicSensor2 {
private:
    static const int TRIG_PIN = 6;  // Pines fijos del ultrasónico 2
    static const int ECHO_PIN = 7;
    static const unsigned long READ_INTERVAL = 100;
    static const long TIMEOUT_US = 30000;
    
    float lastDistance;
    unsigned long lastReadTime;
    bool sensorReady;
    
public:
    FeederUltrasonicSensor2();
    bool initialize();
    void update();
    float getDistance();
    bool isReady();
    String getStatus();
};

#endif