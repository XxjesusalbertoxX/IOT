#ifndef LITTERBOX_MQ2_SENSOR_H
#define LITTERBOX_MQ2_SENSOR_H

#include <Arduino.h>

class LitterboxMQ2Sensor {
private:
    static const int ANALOG_PIN = A0;
    static const unsigned long READ_INTERVAL = 500;
    
    float lastValue;        // Valor analógico crudo
    float lastPPM;          // Valor convertido a PPM
    unsigned long lastReadTime;
    bool sensorReady;
    
    // Función para convertir analógico a PPM
    float analogToPPM(int analogValue);
    
public:
    LitterboxMQ2Sensor();
    bool initialize();
    void update();
    float getValue();       // Valor analógico crudo (0-1023)
    float getPPM();         // Valor convertido a PPM
    bool isReady();
    String getStatus();
};

#endif