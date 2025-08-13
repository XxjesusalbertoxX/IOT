#ifndef WEIGHT_COMMANDS_H
#define WEIGHT_COMMANDS_H

#include <Arduino.h>
#include "../sensors/WeightSensor.h"

class WeightCommands {
private:
    WeightSensor* weightSensor;
    
public:
    WeightCommands(WeightSensor* sensor);
    void processCommand(String command);
    void showHelp();
};

#endif