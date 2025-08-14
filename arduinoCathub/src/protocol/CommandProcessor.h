#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "../sensors/litterbox/LitterboxStepperMotor.h"
#include "OptimizedProtocol.h"

class CommandProcessor {
private:
  LitterboxStepperMotor* litterboxMotor;
  OptimizedProtocol protocol;
  
  // Procesar comandos específicos por dispositivo
  void processLitterboxCommand(const String& jsonCommand);
  void processFeederCommand(const String& jsonCommand);
  void processEmergencyCommand(const String& jsonCommand);
  
public:
  CommandProcessor(LitterboxStepperMotor* lbMotor);
  
  void initialize();
  void processCommand(const String& jsonCommand);
  void update(); // Llamar en loop()
};

#endif