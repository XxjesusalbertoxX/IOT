#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../state/ConfigStore.h"
#include "../sensors/SensorManager.h"

class CommandProcessor {
private:
    ConfigStore& configStore;
    SensorManager& sensorManager;
    
    void processCommand(String command);
    void sendWeightData();
    void tareWeightSensor();
    void calibrateWeightSensor(float knownWeight);
    void showHelp();
    
public:
    CommandProcessor(ConfigStore& config, SensorManager& sensors);
    void begin();
    void poll();
};

#endif