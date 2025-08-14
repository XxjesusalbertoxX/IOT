#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

class ConfigStore {
private:
    float calibrationFactor_;
    static ConfigStore* instance_;

public:
    ConfigStore() : calibrationFactor_(-7050.0) {}
    
    static ConfigStore& getInstance() {
        static ConfigStore instance;
        return instance;
    }
    
    bool initialize() { 
        return true; 
    }
    
    float getCalibrationFactor() const { 
        return calibrationFactor_; 
    }
    
    void setCalibrationFactor(float factor) { 
        calibrationFactor_ = factor; 
    }
    
    void save() {
        // Implementación básica - no hace nada por ahora
    }
    
    void load() {
        // Implementación básica - no hace nada por ahora
    }
};

#endif